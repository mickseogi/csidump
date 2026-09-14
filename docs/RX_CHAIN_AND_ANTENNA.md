# RX chain / antenna_idx 해석

## 확인된 사실

- 원본 vendor attribute 이름은 `CSI_DATA_RX_ANT`다.
- 원본 CSIdump는 이 값을 `rx_idx`에 저장한다.
- 원본 parser의 `ANTENNA_NUM 3`은 userspace 반복 범위를 0,1,2로 제한한 하드코딩이다.

## 확정할 수 없는 부분

공개 저장소만으로 `rx_idx`가 다음 중 정확히 무엇인지 확정할 수 없다.

- 제품 외부 봉 안테나 번호
- RF receive chain 번호
- baseband chain 번호
- spatial stream 번호

명칭과 동작상 **firmware가 보고한 RX antenna/chain index**로 취급하는 것이 가장 안전하지만, 물리 봉과의 매핑은 실기 검증이 필요하다. `ANTENNA_NUM=3`은 실제 하드웨어에 세 chain만 있다는 증거가 아니다.

## 수정본 정책

- 0..2 반복을 제거하고 vendor event가 준 `rx_index`를 그대로 전송한다.
- `rx_chain_count`와 `rx_chain_mask`는 추측하지 않는다.
- 라우터에서 다음 결과와 실제 수신 로그를 확인한 뒤 실행 인자로 넣는다.

```sh
iw phy phy1 info | grep -i -E 'Available Antennas|Configured Antennas'
```

예를 들어 검증 결과가 RX mask `0x03`이고 두 chain이라면:

```sh
--rx-chain-count 2 --rx-chain-mask 0x03
```

을 사용한다. 확인 전에는 count=0, mask=0, valid flag 해제가 맞다.
