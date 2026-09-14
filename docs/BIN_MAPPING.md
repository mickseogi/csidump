# 64개 FFT bin 대응표

- 전송 순서: **vendor/firmware가 제공한 native 배열 순서 그대로**
- `fftshift`: 적용하지 않음
- 제거: 없음
- payload 시작 기준으로 bin `k`의 real offset은 `4*k`, imag offset은 `4*k+2`
- `dc_index`: 현재 공개된 드라이버/CSIdump 코드만으로 확정할 수 없어 `-1`(unknown)로 전송

| sample index | 원본 대응 | payload offset | 순서 |
|---:|---|---:|---|
| 0 | vendor I/Q attribute position 0 | payload byte 0 | real then imag |
| 1 | vendor I/Q attribute position 1 | payload byte 4 | real then imag |
| 2 | vendor I/Q attribute position 2 | payload byte 8 | real then imag |
| 3 | vendor I/Q attribute position 3 | payload byte 12 | real then imag |
| 4 | vendor I/Q attribute position 4 | payload byte 16 | real then imag |
| 5 | vendor I/Q attribute position 5 | payload byte 20 | real then imag |
| 6 | vendor I/Q attribute position 6 | payload byte 24 | real then imag |
| 7 | vendor I/Q attribute position 7 | payload byte 28 | real then imag |
| 8 | vendor I/Q attribute position 8 | payload byte 32 | real then imag |
| 9 | vendor I/Q attribute position 9 | payload byte 36 | real then imag |
| 10 | vendor I/Q attribute position 10 | payload byte 40 | real then imag |
| 11 | vendor I/Q attribute position 11 | payload byte 44 | real then imag |
| 12 | vendor I/Q attribute position 12 | payload byte 48 | real then imag |
| 13 | vendor I/Q attribute position 13 | payload byte 52 | real then imag |
| 14 | vendor I/Q attribute position 14 | payload byte 56 | real then imag |
| 15 | vendor I/Q attribute position 15 | payload byte 60 | real then imag |
| 16 | vendor I/Q attribute position 16 | payload byte 64 | real then imag |
| 17 | vendor I/Q attribute position 17 | payload byte 68 | real then imag |
| 18 | vendor I/Q attribute position 18 | payload byte 72 | real then imag |
| 19 | vendor I/Q attribute position 19 | payload byte 76 | real then imag |
| 20 | vendor I/Q attribute position 20 | payload byte 80 | real then imag |
| 21 | vendor I/Q attribute position 21 | payload byte 84 | real then imag |
| 22 | vendor I/Q attribute position 22 | payload byte 88 | real then imag |
| 23 | vendor I/Q attribute position 23 | payload byte 92 | real then imag |
| 24 | vendor I/Q attribute position 24 | payload byte 96 | real then imag |
| 25 | vendor I/Q attribute position 25 | payload byte 100 | real then imag |
| 26 | vendor I/Q attribute position 26 | payload byte 104 | real then imag |
| 27 | vendor I/Q attribute position 27 | payload byte 108 | real then imag |
| 28 | vendor I/Q attribute position 28 | payload byte 112 | real then imag |
| 29 | vendor I/Q attribute position 29 | payload byte 116 | real then imag |
| 30 | vendor I/Q attribute position 30 | payload byte 120 | real then imag |
| 31 | vendor I/Q attribute position 31 | payload byte 124 | real then imag |
| 32 | vendor I/Q attribute position 32 | payload byte 128 | real then imag |
| 33 | vendor I/Q attribute position 33 | payload byte 132 | real then imag |
| 34 | vendor I/Q attribute position 34 | payload byte 136 | real then imag |
| 35 | vendor I/Q attribute position 35 | payload byte 140 | real then imag |
| 36 | vendor I/Q attribute position 36 | payload byte 144 | real then imag |
| 37 | vendor I/Q attribute position 37 | payload byte 148 | real then imag |
| 38 | vendor I/Q attribute position 38 | payload byte 152 | real then imag |
| 39 | vendor I/Q attribute position 39 | payload byte 156 | real then imag |
| 40 | vendor I/Q attribute position 40 | payload byte 160 | real then imag |
| 41 | vendor I/Q attribute position 41 | payload byte 164 | real then imag |
| 42 | vendor I/Q attribute position 42 | payload byte 168 | real then imag |
| 43 | vendor I/Q attribute position 43 | payload byte 172 | real then imag |
| 44 | vendor I/Q attribute position 44 | payload byte 176 | real then imag |
| 45 | vendor I/Q attribute position 45 | payload byte 180 | real then imag |
| 46 | vendor I/Q attribute position 46 | payload byte 184 | real then imag |
| 47 | vendor I/Q attribute position 47 | payload byte 188 | real then imag |
| 48 | vendor I/Q attribute position 48 | payload byte 192 | real then imag |
| 49 | vendor I/Q attribute position 49 | payload byte 196 | real then imag |
| 50 | vendor I/Q attribute position 50 | payload byte 200 | real then imag |
| 51 | vendor I/Q attribute position 51 | payload byte 204 | real then imag |
| 52 | vendor I/Q attribute position 52 | payload byte 208 | real then imag |
| 53 | vendor I/Q attribute position 53 | payload byte 212 | real then imag |
| 54 | vendor I/Q attribute position 54 | payload byte 216 | real then imag |
| 55 | vendor I/Q attribute position 55 | payload byte 220 | real then imag |
| 56 | vendor I/Q attribute position 56 | payload byte 224 | real then imag |
| 57 | vendor I/Q attribute position 57 | payload byte 228 | real then imag |
| 58 | vendor I/Q attribute position 58 | payload byte 232 | real then imag |
| 59 | vendor I/Q attribute position 59 | payload byte 236 | real then imag |
| 60 | vendor I/Q attribute position 60 | payload byte 240 | real then imag |
| 61 | vendor I/Q attribute position 61 | payload byte 244 | real then imag |
| 62 | vendor I/Q attribute position 62 | payload byte 248 | real then imag |
| 63 | vendor I/Q attribute position 63 | payload byte 252 | real then imag |
