# 클라이언트 요구사항 충족표

상태 정의: **충족**, **부분 충족**, **드라이버/펌웨어 작업 필요**, **실기 검증 필요**.

| 요구사항 | 상태 | 구현/판단 |
|---|---|---|
| FFT bin 0~63 전부 전송 | 충족(소스·단위시험) | 정확히 64개일 때만 index 0→63을 그대로 직렬화. 삭제/재배열/fftshift 없음 |
| `sample_count=64`, payload 256 bytes | 충족(소스·단위시험) | strict 모드가 다른 개수를 거부. 테스트 통과 |
| DC index 문서화 | **실기/드라이버 검증 필요** | 공개 소스에 정확한 DC 매핑이 없음. `dc_index=-1`, valid flag 해제. 추측값을 기록하지 않음 |
| 기존 0,1,63 제외 의미 | 문서화 완료 | 원본 CSIdump의 위치 기반 휴리스틱일 뿐이며 DC/guard/null이라는 근거 없음 |
| 대역폭별 FFT size | 문서화 완료 | upstream 상수: 20/40/80/160/320 MHz → 64/128/256/512/1024. 본 V1 strict는 20 MHz만 허용 |
| 원본 integer 직접 전송 | 충족(소스) | vendor nested u16의 bit pattern을 `int16_t`로 직접 보존. double 경로 없음 |
| real/imag bit width·signed·범위 | 충족(사용자 공간 기준) | signed 16-bit, -32768~32767 |
| firmware 내부 scaling 여부 | **펌웨어 문서/분석 필요** | 사용자 공간에서는 scaling/shift/normalization 없음. 펌웨어 내부 생성 방식은 공개 소스로 확정 불가 |
| scale/shift metadata | 충족 | 1/1, shift 0, raw integer flag |
| 동일 송신 프레임 `tx_sequence` | **드라이버/펌웨어 작업 필요** | 현 vendor event에 payload, 802.11 Sequence Control, TSF 또는 명시적 frame sequence가 없음. 현재 0/invalid로 전송 |
| 수신기별 `udp_sequence` | 충족(소스·단위시험) | CSI frame당 한 번 증가. 같은 frame을 여러 UDP client에 보낼 때 동일 값 사용 |
| nanosecond timestamp | 부분 충족 | `clock_gettime(CLOCK_REALTIME)` ns. 실제 clock resolution 로그 출력. 수신기 간 동기화는 NTP/PTP 필요 |
| receiver_id 실행 인자 | 충족 | `--receiver-id` 필수, 0 거부 |
| rx_chain_index | 부분 충족 | vendor `RX_ANT` 값을 그대로 제공. 물리 안테나 매핑은 미확정 |
| rx_chain_count/mask | 부분 충족 | 검증 후 CLI로 지정. 하드코딩/추측하지 않음 |
| center frequency/channel/BW/FFT/count | 충족 또는 CLI 기반 | center/channel/BW는 CLI+valid flag, FFT/count는 strict 64 |
| RSSI | 부분 충족 | vendor RSSI 직접 전달; 의미는 dBm으로 취급하되 실기 확인 필요 |
| noise/MCS/NSS/GI | 미제공(정직한 sentinel) | 현재 vendor event가 제공하지 않아 unknown과 invalid flag 사용 |
| PHY mode | 부분 충족 | vendor raw MODE를 보존하지만 표준 enum 의미는 미확정 |
| magic/version/header/payload/flags | 충족 | 112-byte V1 명시 직렬화 |
| padding 비의존, little-endian | 충족 | 모든 필드를 함수로 직접 serialize |
| CRC32 | 충족(소스·단위시험) | IEEE CRC32, payload 대상 |
| Python 파서·샘플·hex | 충족 | `python/`, `samples/` |
| ARM64 실행 파일 | **아직 미제공** | OpenWrt SDK 및 실제 AX3000T에서 빌드/실행 검증 전. 기존 바이너리 패치로 구현하기에는 변경 범위가 너무 큼 |

## 이전 `CSIdump-int16-raw` 판단

이전 시험 파일은 64-bin strict V1 헤더, receiver ID, sequence, CRC, 명시 직렬화, metadata validity, tx-sequence 정책을 갖고 있지 않다. 따라서 이번 클라이언트 납품 기준으로는 **사용 보류**가 맞다.
