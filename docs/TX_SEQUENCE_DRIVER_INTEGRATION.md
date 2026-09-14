# 동일 송신 프레임 tx_sequence 구현 계약

## 현재 결론

CSIdump userspace 프로그램만 수정해서는 여러 수신기의 CSI를 **동일한 송신 Wi-Fi frame 단위로 확정 매칭할 수 없다.** 현 vendor event에는 그 식별자가 없다.

V1 패킷에는 `tx_sequence`, `tx_sequence_source`, `TX_SEQUENCE_VALID` 공간을 마련했지만 현재 빌드에서는:

```text
tx_sequence = 0
tx_sequence_source = 0 (unavailable)
TX_SEQUENCE_VALID = 0
```

으로 보낸다. 거짓 식별자를 만드는 것보다 안전하다.

## 권장 구현

1. 송신기는 하나의 broadcast 또는 multicast payload에 다음을 넣는다.

```text
byte[4]  = "TXSQ"
uint64LE = tx_sequence
uint64LE = sender_timestamp_ns (선택)
```

2. 각 수신기의 mt76/MediaTek CSI 생성 경로가 해당 수신 frame의 payload에서 `tx_sequence`를 읽는다.
3. CSI vendor event에 새 U64 attribute를 추가한다.
4. CSIdump가 그 값을 읽어 `tx_sequence`에 넣고 valid flag를 설정한다.

## 중요한 무선 조건

각 STA에 별도 unicast를 보내면 송신 payload sequence가 같더라도 **서로 다른 802.11 frame**이다. 동일 over-the-air frame을 여러 수신기가 관측해야 한다면 broadcast/multicast 또는 별도의 raw 802.11 injection 설계가 필요하다.

## 대안 우선순위

1. payload-injected uint64 sequence + driver export
2. 802.11 Sequence Control + transmitter address/TID와 함께 export
3. Wi-Fi TSF + frame metadata
4. firmware frame sequence

802.11 Sequence Control은 재전송, TID, fragment 및 wrap-around를 함께 고려해야 하므로 단독 uint16 값보다 payload uint64가 단순하다.

## 포함된 도구

`python/tx_sequence_broadcaster.py`는 향후 driver integration용 payload 형식을 생성한다. 현재 CSIdump가 이 payload를 읽는 기능은 없으므로 이 스크립트만 실행한다고 tx_sequence가 채워지지는 않는다.
