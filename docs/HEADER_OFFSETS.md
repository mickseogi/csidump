# CSI1 V1 헤더 오프셋

- 헤더 크기: **112 bytes**
- magic: 바이트 문자열 `43 53 49 31` (`CSI1`)
- 모든 다중 바이트 정수: **little-endian**
- 구조체 메모리를 직접 전송하지 않음. `src/csi_protocol_v1.cpp`가 필드별로 직렬화함.
- payload: 64 × `(int16 real, int16 imag)` = **256 bytes**
- 전체 UDP application payload: **368 bytes**

> 클라이언트 예시의 `0x43534931`을 정수로 little-endian 직렬화하면 wire byte가 `31 49 53 43`이 되어 ASCII `CSI1`과 반대가 된다. 이 구현은 혼동을 없애기 위해 magic을 정수가 아니라 정확한 4바이트 문자열 `CSI1`로 정의한다.

| Offset | 필드 | 자료형 | 크기 | 단위/값 | 의미 |
|---:|---|---|---:|---|---|
| 0 | magic | byte[4] | 4 | `CSI1` | 프로토콜 식별자 |
| 4 | version | uint16 | 2 | 1 | 프로토콜 버전 |
| 6 | header_size | uint16 | 2 | 112 | 헤더 길이 |
| 8 | flags | uint32 | 4 | bit mask | 유효성·정렬·포맷 플래그 |
| 12 | receiver_id | uint32 | 4 | 사용자 설정 | 수신기 고정 ID |
| 16 | udp_sequence | uint64 | 8 | 1씩 증가 | 해당 CSIdump 프로세스의 UDP CSI 순번 |
| 24 | tx_sequence | uint64 | 8 | source별 | 동일 송신 프레임 식별자. 현재는 invalid/0 |
| 32 | timestamp_ns | uint64 | 8 | ns since Unix epoch | `CLOCK_REALTIME` 캡처 시각 |
| 40 | firmware_timestamp_raw | uint64 | 8 | 미확정 | vendor event의 원본 `TS` u32를 0 확장 |
| 48 | center_frequency_mhz | uint16 | 2 | MHz | 실행 인자로 지정, valid flag 필요 |
| 50 | bandwidth_mhz | uint16 | 2 | MHz | strict V1에서는 20 |
| 52 | fft_size | uint16 | 2 | bins | strict V1에서는 64 |
| 54 | sample_count | uint16 | 2 | I/Q pairs | strict V1에서는 64 |
| 56 | rx_chain_index | uint8 | 1 | vendor raw | `CSI_DATA_RX_ANT` 값 |
| 57 | rx_chain_count | uint8 | 1 | count | 실행 인자로 검증 후 지정 |
| 58 | rx_chain_mask | uint8 | 1 | bit mask | 실행 인자로 검증 후 지정 |
| 59 | sample_format | uint8 | 1 | 1 | signed int16 real/imag |
| 60 | rssi_dbm | int8 | 1 | dBm | vendor event RSSI bit pattern |
| 61 | noise_floor_dbm | int8 | 1 | dBm | 현재 unknown=-128 |
| 62 | mcs | uint8 | 1 | index | 현재 unknown=255 |
| 63 | nss | uint8 | 1 | streams | 현재 unknown=255 |
| 64 | guard_interval | uint8 | 1 | enum/raw | 현재 unknown=255 |
| 65 | phy_mode_raw | uint8 | 1 | vendor raw | `CSI_DATA_MODE`; 표준 PHY enum으로 해석하지 않음 |
| 66 | timestamp_source | uint8 | 1 | enum | 1=`CLOCK_REALTIME` |
| 67 | tx_sequence_source | uint8 | 1 | enum | 0=unavailable 등 |
| 68 | scale_numerator | int32 | 4 | ratio | 현재 1 |
| 72 | scale_denominator | int32 | 4 | ratio | 현재 1 |
| 76 | payload_size | uint32 | 4 | bytes | int16 strict V1에서는 256 |
| 80 | payload_crc32 | uint32 | 4 | IEEE CRC32 | payload만 계산 |
| 84 | primary_channel | uint16 | 2 | channel | 실행 인자로 지정, valid flag 필요 |
| 86 | driver_data_count | uint16 | 2 | pairs | 실제 I/Q 중 작은 쪽의 개수 |
| 88 | shift_bits | int8 | 1 | bits | 현재 0 |
| 89 | metadata_source | uint8 | 1 | enum | vendor/CLI/mixed |
| 90 | dc_index | int16 | 2 | index | 현재 unknown=-1 |
| 92 | transmitter_address | byte[6] | 6 | MAC | vendor event TA |
| 98 | vendor_data_bw | uint8 | 1 | raw | 원본 BW attribute |
| 99 | vendor_primary_channel_index | uint8 | 1 | raw | 원본 CH_IDX attribute |
| 100 | vendor_snr | uint8 | 1 | raw | 원본 SNR attribute |
| 101 | vendor_rx_mode | uint8 | 1 | raw | 원본 MODE attribute |
| 102 | vendor_tx_index | uint16 | 2 | raw | TX_ANT 값 |
| 104 | vendor_h_index | uint32 | 4 | raw | H_IDX 값; tx_sequence로 사용하지 않음 |
| 108 | reserved | uint32 | 4 | 0 | 향후 확장 |

## 주요 flags

| Bit | 이름 | 의미 |
|---:|---|---|
| 0 | native FFT order | vendor 배열 순서를 유지 |
| 1 | fftshift applied | 이 구현에서는 항상 0 |
| 2 | tx_sequence valid | 현재 드라이버에서는 0 |
| 10 | RX chain mask valid | count/mask가 검증되어 설정됨 |
| 13 | payload CRC32 valid | CRC 사용 |
| 14 | raw integer/no scaling | userspace scaling 없음 |
| 15 | strict FFT64 | 정확히 64개만 전송 |
| 20 | DC index known | 현재 0 |
