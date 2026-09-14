# CSIdump for Xiaomi AX3000T

Xiaomi AX3000T에서 **Wi-Fi CSI(Channel State Information)** 데이터를 수집하기 위한 OpenWrt 기반 프로젝트입니다.

본 프로젝트는 MediaTek Wi-Fi 장치의 CSI 데이터를 수집하는 오픈소스 프로젝트인  
[MtkWifiRev/MtkCSIdump](https://github.com/MtkWifiRev/MtkCSIdump)를 기반으로 합니다.

Xiaomi AX3000T에서 CSI 데이터를 수집하기 위해 OpenWrt 환경을 사용하며,  
OpenWrt 구성에는 다음 저장소를 참고했습니다.

- [MtkWifiRev/MtkCSIdump](https://github.com/MtkWifiRev/MtkCSIdump)
- [openwrt-xiaomi/openwrt](https://github.com/openwrt-xiaomi/openwrt)

---

## 1. 프로젝트 개요

CSI(Channel State Information)는 Wi-Fi 신호가 송신기에서 수신기까지 전달되는 과정에서 발생하는 채널의 상태를 나타내는 정보입니다.

일반적인 Wi-Fi 통신에서는 CSI 정보를 사용자가 직접 확인할 수 없지만,  
지원되는 Wi-Fi 칩셋과 드라이버를 이용하면 각 OFDM subcarrier에 대한 복소수 형태의 채널 정보를 얻을 수 있습니다.

본 프로젝트에서는 Xiaomi AX3000T의 MediaTek Wi-Fi 칩셋과 OpenWrt의 `mt76` 드라이버를 이용하여 CSI 데이터를 수집합니다.

전체적인 데이터 흐름은 다음과 같습니다.

```text
Wi-Fi Frame
     │
     ▼
MediaTek Wi-Fi Chip
     │
     ▼
mt76 Driver
     │
     ▼
CSI Vendor Event
     │
     ▼
CSIdump
     │
     ▼
CSI I/Q Data
     │
     ▼
UDP
     │
     ▼
PC / Server
     │
     ▼
Data Processing / Visualization / Storage
```

라우터에서 수집된 CSI 데이터는 UDP를 통해 외부 PC 또는 서버로 전송할 수 있으며,  
전송된 데이터는 저장, 분석, 시각화 또는 머신러닝 데이터셋으로 활용할 수 있습니다.

---

## 2. 사용 환경

본 프로젝트에서 사용한 주요 환경은 다음과 같습니다.

| 항목 | 사용 환경 |
|---|---|
| Router | Xiaomi AX3000T |
| OS | OpenWrt |
| Platform | MediaTek Filogic |
| Wireless Driver | mt76 |
| CSI Collector | MtkCSIdump 기반 |
| Programming Language | C++ |
| C++ Standard | C++17 |
| Build System | CMake |
| Data Transport | UDP |
| Receiver / Analysis | Python |
| Communication | Netlink / UDP |

---

## 3. 기반 프로젝트

### MtkCSIdump

CSI 데이터 수집 기능은 다음 프로젝트를 기반으로 합니다.

https://github.com/MtkWifiRev/MtkCSIdump

MtkCSIdump는 MediaTek Wi-Fi 장치에서 CSI 정보를 추출하기 위한 프로젝트입니다.

주요 기능은 다음과 같습니다.

- MediaTek Wi-Fi CSI 수집
- mt76 드라이버와 연동
- Netlink를 이용한 CSI Event 수신
- CSI I/Q 데이터 처리
- UDP 기반 CSI 데이터 전송
- 실시간 CSI 데이터 활용

본 프로젝트에서는 MtkCSIdump의 **MediaTek CSI 수집 구조와 mt76 연동 방식을 기반으로 Xiaomi AX3000T CSI 수집 환경을 구성**했습니다.

---

### OpenWrt

라우터 운영체제는 OpenWrt를 사용합니다.

참고한 저장소는 다음과 같습니다.

https://github.com/openwrt-xiaomi/openwrt

OpenWrt는 임베디드 네트워크 장비를 위한 Linux 기반 운영체제입니다.

본 프로젝트에서는 OpenWrt를 통해 다음 기능을 사용합니다.

- Xiaomi AX3000T Linux 환경 구성
- MediaTek Wi-Fi 드라이버 사용
- mt76 기반 CSI 기능 접근
- CSI 수집 프로그램 실행
- 네트워크 인터페이스 관리
- UDP CSI 데이터 전송

즉, OpenWrt 자체가 CSI 분석 프로그램은 아니며,  
**CSIdump가 실행될 수 있는 라우터 운영체제 및 드라이버 환경을 제공하는 역할**을 합니다.

---

## 4. 주요 기술

### OpenWrt

Xiaomi AX3000T에서 동작하는 Linux 기반 라우터 운영체제입니다.

일반적인 공유기 펌웨어보다 자유롭게 Linux 프로그램과 네트워크 도구를 사용할 수 있기 때문에 CSI 데이터 수집 환경을 구성하는 데 사용했습니다.

---

### mt76

`mt76`은 Linux에서 MediaTek Wi-Fi 칩셋을 지원하는 무선 드라이버입니다.

본 프로젝트에서 CSI 데이터는 대략 다음 경로를 통해 전달됩니다.

```text
MediaTek Wi-Fi Hardware
        │
        ▼
Wi-Fi Firmware
        │
        ▼
mt76 Driver
        │
        ▼
Vendor Event
        │
        ▼
Netlink
        │
        ▼
CSIdump
```

즉, CSIdump가 Wi-Fi 칩을 직접 제어하는 것이 아니라  
**Linux의 mt76 드라이버가 전달해 주는 CSI 정보를 CSIdump가 받아 처리하는 구조**입니다.

---

### Netlink

Netlink는 Linux Kernel과 Userspace 프로그램 사이의 통신에 사용되는 인터페이스입니다.

CSI 데이터는 드라이버 영역에서 발생하기 때문에 Userspace 프로그램인 CSIdump가 이를 전달받기 위한 통신 수단이 필요합니다.

본 프로젝트에서는 이 과정에서 Netlink가 사용됩니다.

```text
Kernel
   │
   │ Netlink
   ▼
CSIdump
```

---

### C++17

라우터에서 동작하는 CSI 수집 프로그램은 C++ 기반입니다.

주요 역할은 다음과 같습니다.

- CSI 이벤트 수신
- CSI 데이터 파싱
- I/Q 데이터 처리
- CSI Metadata 처리
- UDP Packet 생성
- CSI 데이터 전송

---

### CMake

C++ 프로젝트의 빌드를 관리하기 위해 CMake를 사용합니다.

CMake를 통해 소스 파일과 라이브러리 의존성을 정의하고 실행 파일을 생성할 수 있습니다.

OpenWrt용 프로그램의 경우 일반 PC에서 바로 컴파일하는 것이 아니라  
OpenWrt SDK 또는 OpenWrt Build System의 Cross Compiler를 이용해야 합니다.

```text
x86_64 Linux PC
       │
       ▼
OpenWrt SDK
       │
       ▼
Cross Compile
       │
       ▼
ARM64 Binary
       │
       ▼
Xiaomi AX3000T
```

---

### UDP

CSI 데이터는 실시간으로 지속적으로 발생하기 때문에 외부 PC 또는 서버로 데이터를 전송할 때 UDP를 사용할 수 있습니다.

```text
Xiaomi AX3000T
      │
      │ CSI Data
      │ UDP
      ▼
PC / Server
```

UDP는 연결 과정이 간단하고 전송 오버헤드가 작기 때문에 실시간 CSI 데이터 전송에 적합합니다.

---

### Python

Python은 주로 라우터가 아닌 외부 PC 또는 서버에서 사용합니다.

주요 사용 목적은 다음과 같습니다.

- UDP Packet 수신
- CSI 데이터 파싱
- 데이터 저장
- CSI 값 확인
- 데이터 시각화
- 실험 데이터 검증
- 머신러닝용 데이터 전처리

---

## 5. CSI 데이터

CSI 데이터는 각 Wi-Fi OFDM Subcarrier의 채널 상태를 복소수 값으로 표현합니다.

일반적으로 하나의 CSI 값은 다음과 같이 표현할 수 있습니다.

```text
CSI = I + jQ
```

여기서

```text
I = Real Component
Q = Imaginary Component
```

입니다.

예를 들어 다음과 같은 CSI 값이 있다고 하면

```text
I = 120
Q = -35
```

복소수 형태로는

```text
120 - 35j
```

와 같이 표현할 수 있습니다.

이를 이용하여 CSI의 Amplitude와 Phase를 계산할 수도 있습니다.

```text
Amplitude = sqrt(I² + Q²)

Phase = atan2(Q, I)
```

이러한 CSI 변화를 시간에 따라 관찰하면 주변 환경 변화에 따른 Wi-Fi Channel 변화를 확인할 수 있습니다.

---

## 6. 동작 구조

전체적인 구조는 다음과 같습니다.

```text
               Wi-Fi Signal
                    │
                    ▼
          Xiaomi AX3000T
    ┌──────────────────────────┐
    │        OpenWrt           │
    │                          │
    │  MediaTek Wi-Fi Chip     │
    │           │              │
    │           ▼              │
    │        mt76 Driver       │
    │           │              │
    │           ▼              │
    │         Netlink          │
    │           │              │
    │           ▼              │
    │         CSIdump          │
    └────────────┬─────────────┘
                 │
                 │ UDP
                 ▼
           PC / Server
    ┌──────────────────────────┐
    │      UDP Receiver        │
    │                          │
    │      CSI Parser          │
    │                          │
    │      Data Storage        │
    │                          │
    │      Visualization       │
    │                          │
    │      Data Analysis       │
    └──────────────────────────┘
```

---

## 7. 프로젝트에서 수행한 작업

본 프로젝트는 MtkCSIdump와 OpenWrt를 처음부터 새로 개발한 프로젝트가 아닙니다.

기존 오픈소스 프로젝트를 기반으로 **Xiaomi AX3000T에서 실제 CSI 데이터를 수집할 수 있는 환경을 구축하는 것**을 목적으로 진행했습니다.

주요 작업은 다음과 같습니다.

1. Xiaomi AX3000T에 OpenWrt 환경 구성
2. MediaTek / mt76 기반 CSI 수집 환경 확인
3. MtkCSIdump 소스 분석
4. OpenWrt 환경에서 CSIdump 빌드
5. Cross Compilation 환경 구성
6. AX3000T에서 CSIdump 실행
7. Wi-Fi 인터페이스에서 CSI 데이터 수집
8. UDP를 통한 외부 PC 데이터 전송
9. 외부 PC에서 CSI Packet 수신
10. CSI I/Q 데이터 검증 및 분석

즉 프로젝트의 핵심은

```text
MtkCSIdump
        +
OpenWrt
        +
Xiaomi AX3000T
        ↓
실제 CSI 수집 환경 구축
```

입니다.

---

## 8. Build 구조

일반적인 Linux 프로그램과 달리 AX3000T에서 실행되는 프로그램은 ARM64 환경에서 동작해야 합니다.

따라서 일반 PC의 GCC로 직접 빌드한 프로그램을 AX3000T에서 실행할 수 없습니다.

OpenWrt의 Cross Compilation 환경을 이용해야 합니다.

```text
Build PC
   │
   │ Source Code
   ▼
OpenWrt SDK / Buildroot
   │
   │ Cross Compile
   ▼
ARM64 Executable
   │
   │ SCP / SFTP
   ▼
Xiaomi AX3000T
   │
   ▼
Execute CSIdump
```

---

## 9. 실행 예시

빌드된 CSIdump 실행 파일을 AX3000T로 전송합니다.

예를 들어 다음과 같이 실행 권한을 부여할 수 있습니다.

```bash
chmod +x CSIdump
```

이후 CSI 수집 프로그램을 실행합니다.

MtkCSIdump의 실행 방식은 사용하는 버전과 빌드 설정에 따라 다를 수 있습니다.

예:

```bash
./CSIdump <interface> <rate> <port>
```

예를 들어:

```bash
./CSIdump phy1-sta0 100 5555
```

와 같은 형태로 CSI를 수집하고 UDP Port를 통해 데이터를 전달할 수 있습니다.

> 실제 인터페이스 이름 및 실행 옵션은 사용 중인 OpenWrt 및 CSIdump 버전에 따라 달라질 수 있습니다.

---

## 10. 활용 목적

수집된 CSI 데이터는 다양한 Wi-Fi Sensing 연구에 활용할 수 있습니다.

예를 들면 다음과 같습니다.

- Wi-Fi Channel 분석
- 사람 움직임 감지
- Motion Detection
- 환경 변화 감지
- 비접촉 센싱
- 시계열 데이터 분석
- 머신러닝 데이터셋 생성
- 딥러닝 기반 환경 상태 추정

CSI는 단순 RSSI보다 훨씬 세밀한 채널 정보를 제공하므로 주변 환경 변화 분석에 활용할 수 있습니다.

---

## 11. 프로젝트 구성

프로젝트는 크게 다음과 같은 요소로 구성됩니다.

```text
CSIdump Project
│
├── OpenWrt
│   ├── Xiaomi AX3000T
│   ├── MediaTek Filogic
│   └── mt76
│
├── CSI Collector
│   ├── MtkCSIdump
│   ├── Netlink
│   └── C++17
│
├── Network
│   └── UDP
│
└── Receiver
    ├── Python
    ├── CSI Parsing
    ├── Data Storage
    └── Visualization
```

---

## 12. Technology Stack

### Router / Embedded

- Xiaomi AX3000T
- OpenWrt
- Linux
- MediaTek Filogic
- mt76

### CSI Collection

- MtkCSIdump
- C++
- C++17
- Netlink

### Build

- CMake
- GCC
- OpenWrt SDK
- Cross Compilation

### Network

- UDP
- IPv4

### Data Processing

- Python
- CSI Parser
- CSI Visualization

---

## 13. Upstream Projects

본 프로젝트는 다음 오픈소스 프로젝트를 기반으로 합니다.

### MtkCSIdump

Repository:

https://github.com/MtkWifiRev/MtkCSIdump

MediaTek 기반 CSI 데이터 수집 및 mt76 연동 구조를 참고했습니다.

---

### OpenWrt for Xiaomi

Repository:

https://github.com/openwrt-xiaomi/openwrt

Xiaomi AX3000T OpenWrt 환경 구축 및 펌웨어 빌드에 사용했습니다.

OpenWrt upstream:

https://github.com/openwrt/openwrt

---

## 14. 프로젝트 목적

이 프로젝트의 최종 목적은 Xiaomi AX3000T와 MediaTek Wi-Fi 칩셋을 이용하여  
**Wi-Fi CSI 데이터를 안정적으로 수집할 수 있는 실험 환경을 구축하는 것**입니다.

최종적으로는 다음과 같은 데이터 파이프라인을 구축하는 것을 목표로 합니다.

```text
Wi-Fi
  │
  ▼
CSI Collection
  │
  ▼
UDP Transmission
  │
  ▼
Data Storage
  │
  ▼
Data Processing
  │
  ▼
Visualization
  │
  ▼
Machine Learning / Wi-Fi Sensing
```

---

## 15. 참고

본 저장소는 MtkCSIdump 및 OpenWrt 프로젝트를 기반으로 한 실험 및 연구용 프로젝트입니다.

각 upstream 프로젝트의 원본 코드와 라이선스는 해당 저장소를 참고하십시오.

- MtkCSIdump: https://github.com/MtkWifiRev/MtkCSIdump
- OpenWrt Xiaomi: https://github.com/openwrt-xiaomi/openwrt
- OpenWrt: https://github.com/openwrt/openwrt
