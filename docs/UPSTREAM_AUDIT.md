# upstream MtkCSIdump 감사 결과

대상: `MtkWifiRev/MtkCSIdump` main 소스와 v0.1 실행 파일.

## 원본 정수 자료형

`wifi_drv_api/mt76_api.h`의 `csi_data`는 다음 형태다.

```cpp
s16 data_i[...];
s16 data_q[...];
```

따라서 userspace에 도착한 원본 real/imag 저장형은 signed 16-bit다. 범위는 `-32768..32767`이다.

원본 `mt76_api.cpp`는 nested netlink attribute를 `nla_get_u16()`으로 읽어 `s16` 배열에 저장한다. 수정본은 값 변환의 모호성을 피하려고 u16 bit pattern을 `memcpy`로 int16에 그대로 보존한다.

## userspace scaling 여부

원본 파서는 `s16 → double` 정적 변환만 수행하고, 전송 전에 normalization, division, shift 또는 scale을 하지 않는다. 수정본은 double 단계를 제거하고 int16을 직접 전송한다.

단, **MediaTek 펌웨어가 CSI 값을 만들기 전에 내부적으로 어떤 고정소수점 scaling을 적용하는지는 이 userspace 저장소만으로 확정할 수 없다.** 이 문서는 “펌웨어에서 userspace로 전달된 원본 정수”를 무손실 보존한다는 의미다.

## 61쌍이 나온 이유

원본 파서는 20 MHz 기본값 64개를 정한 뒤:

```cpp
start_idx = 2;
end_idx = num_subcarriers - 1;
for (i = start_idx; i < end_idx; ++i)
```

를 사용한다. 따라서 index 0, 1, 63을 버리고 2..62, 총 61쌍을 전송한다.

주석은 “DC offset issues”를 언급하지만, 공개 코드에는 각 index가 실제로 DC/guard/null이라는 매핑 근거가 없다. 수정본은 해당 휴리스틱을 제거한다.

## 개수/대역폭 처리 문제

원본 구조체에는 `data_num`과 `ch_bw`가 있으나 원본 callback은 두 필드를 설정하지 않는다. 실제 BW는 `data_bw`에 저장하고, I/Q를 읽은 실제 개수도 기록하지 않는다. 그런데 파서는 `ch_bw`로 64/128/256/512를 추정한다.

수정본은:

1. 객체를 0 초기화한다.
2. I와 Q의 실제 nested element 개수를 각각 센다.
3. 둘이 다르면 flag를 남기고 strict mode에서 폐기한다.
4. `min(I_count, Q_count)`를 driver count로 기록한다.
5. strict V1에서는 정확히 64일 때만 전송한다.

## tx_sequence 관련 upstream 한계

공개 vendor event attribute에는 TS, RSSI, SNR, BW, CH_IDX, TA, I, Q, INFO, TX_ANT, RX_ANT, MODE, H_IDX가 있다. 명시적인 payload sequence, 802.11 Sequence Control, Wi-Fi TSF 또는 firmware frame sequence attribute는 없다.

`h_idx`나 host timestamp를 tx_sequence로 재사용하면 여러 수신기에서 같은 송신 frame이라는 보장이 없으므로 수정본은 그렇게 하지 않는다.
