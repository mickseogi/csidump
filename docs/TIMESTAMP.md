# timestamp 출처와 정렬 한계

## timestamp_ns

수정본은 패킷 생성 시점에:

```cpp
clock_gettime(CLOCK_REALTIME, ...)
```

을 호출해 Unix epoch 기준 nanosecond 단위 정수를 기록한다. 시작 시 `clock_getres(CLOCK_REALTIME)` 결과도 로그로 출력한다.

주의:

- ns 단위 표현과 실제 clock resolution은 다를 수 있다.
- 이 시각은 CSI가 RF에서 수신된 정확한 순간이 아니라 userspace가 vendor event를 처리하고 UDP 패킷을 만드는 시점이다.
- 여러 라우터의 clock이 자동으로 동기화되는 것은 아니다.

여러 수신기 시간 정렬에는 NTP/PTP 설정, 동기화 오차 측정, drift 보정이 필요하다. 가장 좋은 해결은 driver/firmware가 Wi-Fi TSF 또는 kernel RX timestamp를 CSI event와 함께 제공하는 것이다.

## firmware_timestamp_raw

vendor event의 `CSI_DATA_TS`는 U32로 존재하지만 공개 userspace 소스에는 단위와 기준이 없다. 따라서 수정본은 이를 ns라고 오해하지 않고 원본 정수 그대로 `firmware_timestamp_raw`에 담는다.
