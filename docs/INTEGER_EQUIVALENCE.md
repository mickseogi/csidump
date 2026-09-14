# int16 변환 전후 정확도

## 소스 수준 결론

userspace 원본은 signed 16-bit I/Q다. 기존 코드는 이 정수를 `double`로 확장했다.

```text
int16 x → double(x)
```

IEEE-754 double은 모든 int16 정수를 정확히 표현하므로 기존 double 값이 단순 확장값이라면:

```text
int16 → double → 같은 int16
```

은 무손실이다.

새 구현은 더 나아가 double을 거치지 않고 int16 bit pattern을 직접 전송한다.

## amplitude

scale=1일 때:

```text
amplitude_double  = sqrt(real_double² + imag_double²)
amplitude_integer = sqrt(real_int16² + imag_int16²)
```

기존 double이 int16의 정확한 확장이라면 두 값은 수학적으로 같다.

## 수행한 합성 시험

64개 bin에 정수 I/Q를 넣어 legacy double 계산과 int16 계산을 비교했다.

```text
max component error: 0
max amplitude error: 0
```

이 시험은 serializer와 자료형 논리를 검증한다. 실제 펌웨어에서 받은 legacy packet에 비정수 double이 없는지는 `python/compare_integer_double.py`로 capture를 추가 검증해야 한다.
