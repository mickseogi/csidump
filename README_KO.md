# CSIdump CSI1 V1 수정 설계

이 묶음은 클라이언트의 신규 요구사항을 기준으로 기존 `MtkCSIdump`를 재설계한 **소스·프로토콜·검증 패키지**다.

## 가장 중요한 결론

기존에 만든 `CSIdump-int16-raw` 시험 바이너리는 이번 납품 규격으로 사용하면 안 된다. 새 규격은 단순한 자료형 패치가 아니라 다음을 함께 요구한다.

- 64-bin strict raw mode
- 원본 signed int16 직접 전송
- versioned 112-byte header
- 명시적 little-endian serialization
- receiver ID
- UDP sequence
- ns timestamp와 source
- metadata validity flags
- payload CRC32
- RX chain metadata 정책
- tx sequence source 정책

이 변경 범위는 기존 ARM64 바이너리 몇 바이트를 패치해서 안전하게 구현할 수준이 아니다. 따라서 이 묶음은 **소스 재빌드 방식**으로 작성했다.

현재 제공 상태:

- 프로토콜 serializer: 작성 및 호스트 C++ 단위시험 통과
- Python receiver/parser: 작성 및 sample packet 검증 통과
- sample binary/hex: 생성 완료
- router integration source: 작성 완료, OpenWrt SDK/AX3000T 실기 빌드·실행 검증 필요
- 최종 ARM64 실행 파일: 아직 미제공
- exact cross-receiver tx_sequence: 현 driver/firmware event 한계로 미구현

## 전송 포맷

```text
112-byte CSI1 V1 header
+ 64 × { int16 real; int16 imag; }
= 112 + 256
= 368 bytes
```

Payload 순서:

```text
bin 0 real, bin 0 imag,
bin 1 real, bin 1 imag,
...
bin 63 real, bin 63 imag
```

- native vendor array order
- fftshift 없음
- index 제거 없음
- userspace scale 없음
- `scale_numerator=1`
- `scale_denominator=1`
- `shift_bits=0`

## 20 MHz 고정이 필요함

클라이언트가 `fft_size=64`, `sample_count=64`를 필수로 요구했으므로 V1 strict 모드는 **20 MHz만 허용**한다.

upstream의 raw count 정의:

| bandwidth | raw FFT slots |
|---:|---:|
| 20 MHz | 64 |
| 40 MHz | 128 |
| 80 MHz | 256 |
| 160 MHz | 512 |
| 320 MHz | 1024 |

현재 실험 링크가 80 MHz로 설정돼 있다면 먼저 AP/STA를 20 MHz로 변경해야 한다. 프로그램이 80 MHz 데이터를 64개로 임의 자르거나 padding하지는 않는다. strict mode는 실제 I/Q pair가 64가 아니면 해당 event를 폐기한다.

## 원본 정수 보존

upstream userspace 구조는 I와 Q를 `s16` 배열로 가진다. 새 코드의 callback은 netlink `u16` bit pattern을 `int16_t`로 그대로 복사한다.

```text
firmware/vendor event u16 bit pattern
→ bit-preserving int16_t
→ direct little-endian int16 payload
```

`double → int16` cast 경로는 없다.

이는 **userspace에 전달된 정수 CSI를 무손실 보존**한다는 뜻이다. 펌웨어 내부에서 CSI를 생성할 때 적용하는 고정소수점 scale은 공개 userspace 소스만으로 확인되지 않았다.

## 기존 61개 처리 제거

원본 parser는 64개 중 index 0, 1, 63을 위치만 보고 제거했다. 공개 코드에는 세 index가 실제로 DC/guard/null이라는 근거가 없다. 새 코드는 0..63을 그대로 전송한다.

정확한 DC 위치는 아직 확정하지 않았다. 패킷에는:

```text
dc_index = -1
DC_INDEX_KNOWN flag = 0
```

을 기록한다. 잘못된 DC index를 문서화하는 것보다 이후 실기/driver 검증으로 확정하는 편이 안전하다.

## tx_sequence 상태

현재 MediaTek CSI vendor event에는 다음이 공개돼 있다.

```text
TS, RSSI, SNR, BW, CH_IDX, TA, I, Q, INFO,
TX_ANT, RX_ANT, MODE, H_IDX
```

그러나 payload sequence, 802.11 Sequence Control, 명시적 TSF, firmware frame sequence는 없다. 따라서 userspace CSIdump만으로 서로 다른 수신기의 CSI가 동일 송신 frame에서 왔는지 보장할 수 없다.

현재 V1은:

```text
tx_sequence = 0
tx_sequence_source = unavailable
TX_SEQUENCE_VALID = 0
```

으로 정직하게 표시한다. 자세한 driver 연동 계약은 `docs/TX_SEQUENCE_DRIVER_INTEGRATION.md`에 있다.

## 실행 인자 설계

예정 실행 형식:

```sh
CSIdumpV1 phy1-sta0 100 5555 \
  --receiver-id 1 \
  --bandwidth-mhz 20 \
  --center-frequency-mhz 5180 \
  --primary-channel 36 \
  --rx-chain-count 2 \
  --rx-chain-mask 0x03
```

chain count/mask는 실제 `iw phy` 결과와 CSI 로그를 확인한 뒤 넣어야 한다. 확인 전에는 추측값을 쓰지 않는다.

## Python 수신기

```sh
cd python
python3 csi_v1_receiver.py \
  --listen-port 5555 \
  --router 192.168.2.1 \
  --print-every 100
```

여러 수신기 등록:

```sh
python3 csi_v1_receiver.py \
  --listen-port 5555 \
  --router 192.168.2.1 \
  --router 192.168.2.2 \
  --router 192.168.2.3
```

수신기는 magic/version/header size/sample size/CRC/FFT order를 검증하고 receiver별 UDP sequence gap, duplicate, reorder를 표시한다.

## 호스트 프로토콜 시험

```sh
cmake -S . -B build -DBUILD_ROUTER_BINARY=OFF
cmake --build build
ctest --test-dir build --output-on-failure
python3 python/generate_sample_packet.py
```

현재 시험 결과:

```text
64 bins: PASS
payload 256 bytes: PASS
packet 368 bytes: PASS
real/imag order: PASS
bin 0..63 order: PASS
CRC32: PASS
exact int16→double amplitude equivalence: PASS
```

이 결과는 protocol/serializer의 합성 데이터 시험이다. 실제 AX3000T vendor event 검증은 별도로 수행해야 한다.

## 파일 안내

- `include/csi_protocol_v1.hpp`: C++ 논리 헤더와 enum
- `include/csi_protocol_v1_wire.h`: C offset 정의와 문서용 구조
- `src/csi_protocol_v1.cpp`: 명시적 LE serialization + CRC32
- `src/mt76_csi_frame.cpp`: 원본 int16 직접 추출, 실제 I/Q count 기록
- `src/csi_server_v1.cpp`: strict 64 검사, sequence, timestamp, UDP 전송
- `src/main.cpp`: 실행 인자 처리
- `python/csi_v1_receiver.py`: 수신/검증 예제
- `samples/csi_v1_sample.bin`: sample packet
- `samples/csi_v1_sample.hex`: hex dump
- `samples/bin_mapping.csv`: bin mapping
- `docs/COMPLIANCE_MATRIX.md`: 요구사항 충족/미충족표
- `docs/HEADER_OFFSETS.md`: 필드별 offset 표
- `docs/TX_SEQUENCE_DRIVER_INTEGRATION.md`: driver 작업 계약
- `docs/RX_CHAIN_AND_ANTENNA.md`: antenna/rx chain 판단
- `docs/TEST_PLAN.md`: 실기 검증 계획

## 납품 전 남은 필수 단계

1. AP/STA 20 MHz 고정
2. OpenWrt SDK로 ARM64 빌드
3. AX3000T에서 vendor I/Q count가 실제 64인지 확인
4. 10분/장시간 UDP sequence·CRC 시험
5. DC index 실기 또는 driver 문서로 확정
6. RX chain 물리 매핑 확인
7. driver/firmware에 tx_sequence export 추가
8. 여러 수신기에서 동일 broadcast frame의 tx_sequence 일치 검증
