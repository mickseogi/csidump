# 검증 계획

## 이미 자동 검증한 항목

호스트 단위시험과 Python sample parser에서 확인:

1. magic/version/header size
2. 명시적 little-endian serialization
3. sample count 64
4. int16 payload 256 bytes
5. 전체 packet 368 bytes
6. bin 0..63 순서 보존
7. 각 bin real→imag 순서
8. payload CRC32
9. int16 값을 double로 정확히 확장한 경우 component와 amplitude 오차 0

실행:

```sh
cmake -S . -B build -DBUILD_ROUTER_BINARY=OFF
cmake --build build
ctest --test-dir build --output-on-failure
python3 python/generate_sample_packet.py
```

## AX3000T에서 반드시 추가 검증할 항목

1. AP와 STA 링크를 20 MHz로 고정
2. 실제 vendor event의 I count=64, Q count=64인지 확인
3. packet length=368, payload=256인지 확인
4. 10분 이상 `udp_sequence` gap/duplicate/reorder 통계
5. 각 vendor `rx_index`의 실제 관측 범위
6. `iw phy`의 RX mask와 physical antenna mapping 실험
7. static channel에서 bin 0..63 값 분포를 수집해 DC/null 위치 검증
8. legacy double build와 동일 원본 frame을 병렬로 얻을 수 있는 계측 경로를 만든 뒤 amplitude 비교
9. CLOCK_REALTIME의 라우터 간 offset/drift 측정
10. driver patch 이후 여러 수신기 tx_sequence 일치 확인

## 실패 안전 정책

- I/Q 개수가 다르면 전송하지 않음
- sample_count가 64가 아니면 전송하지 않음
- strict mode인데 BW가 20 MHz가 아니면 종료/거부
- unknown metadata를 0으로 꾸며 valid 처리하지 않음
- tx_sequence가 없으면 0/invalid로 명시
