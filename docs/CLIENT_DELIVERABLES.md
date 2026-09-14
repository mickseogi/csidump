# 요청된 결과물 위치

1. 최종 C/C++ 헤더 구조 정의
   - `include/csi_protocol_v1.hpp`
   - `include/csi_protocol_v1_wire.h`
2. 각 필드 offset/자료형/크기/endian/단위/의미
   - `docs/HEADER_OFFSETS.md`
3. CSI payload 생성 코드
   - `src/csi_server_v1.cpp::send_frame`
4. UDP serialization 코드
   - `src/csi_protocol_v1.cpp::serialize_packet_v1`
5. Python 수신/파싱 예제
   - `python/csi_v1_receiver.py`
   - `python/csi_v1_protocol.py`
6. sample UDP binary / hex dump
   - `samples/csi_v1_sample.bin`
   - `samples/csi_v1_sample.hex`
7. 64개 bin index 대응표
   - `docs/BIN_MAPPING.md`
   - `samples/bin_mapping.csv`
8. integer 변환 전후 비교
   - `docs/INTEGER_EQUIVALENCE.md`
   - `python/compare_integer_double.py`
   - `tests/protocol_v1_test.cpp`
9. antenna_idx/rx_chain_index 의미
   - `docs/RX_CHAIN_AND_ANTENNA.md`
10. timestamp와 tx_sequence 출처
   - `docs/TIMESTAMP.md`
   - `docs/TX_SEQUENCE_DRIVER_INTEGRATION.md`

추가:

- 전체 요구사항 상태: `docs/COMPLIANCE_MATRIX.md`
- upstream 코드 감사: `docs/UPSTREAM_AUDIT.md`
- 실기 검증 계획: `docs/TEST_PLAN.md`
