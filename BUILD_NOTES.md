# 빌드 상태와 주의사항

## 검증 완료

현재 환경에서 다음을 실제 수행했다.

```text
C++ protocol library compile: PASS
C++ unit test: PASS
Python syntax compile: PASS
sample packet generation/parse: PASS
```

## 아직 검증하지 못한 것

OpenWrt용 router executable은 이 환경에 MT7981/filogic OpenWrt SDK와 target용 `libnl-tiny` 개발 파일이 준비돼 있지 않아 실제 ARM64 link까지 수행하지 못했다.

따라서 이 묶음을 “실행 검증된 최종 CSIdump 바이너리”라고 부르면 안 된다. `src/mt76_csi_frame.cpp`는 upstream vendor API를 보수적으로 재작성한 integration source이며 SDK 빌드 중 include/library 차이가 발견될 수 있다.

## 권장 빌드 방식

OpenWrt 24.10.1 mediatek/filogic SDK에서 package로 빌드하는 방식을 권장한다. `openwrt/Makefile`은 시작점이며, 실제 package tree에 복사할 때 source 경로를 SDK package 구조에 맞게 조정해야 한다.

대상 라우터에서 기존 파일을 덮어쓰지 말고 `/tmp/CSIdumpV1`로 시험한다.

## 바이너리 패치를 사용하지 않은 이유

신규 요구사항은 CLI parser, 새 header, CRC, sequence state, timestamp source, metadata validity, direct netlink int16 extraction까지 바꾼다. 기존 7 MB ARM64 ELF를 명령 단위로 패치하는 방식은 control-flow와 object layout 변경 위험이 지나치게 크다. 이번 버전은 source rebuild가 올바른 접근이다.
