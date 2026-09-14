#!/usr/bin/env python3
"""Verify losslessness when the legacy doubles are exact expansions of int16 values."""

from __future__ import annotations

import argparse
import math
import struct


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("legacy_payload", help="raw legacy payload containing interleaved little-endian doubles")
    parser.add_argument("--sample-count", type=int, default=64)
    args = parser.parse_args()

    payload = open(args.legacy_payload, "rb").read()
    expected = args.sample_count * 2 * 8
    if len(payload) != expected:
        raise SystemExit(f"expected {expected} bytes, got {len(payload)}")
    values = struct.unpack(f"<{args.sample_count * 2}d", payload)

    non_integer = 0
    out_of_range = 0
    max_component_error = 0.0
    max_amplitude_error = 0.0
    for index in range(0, len(values), 2):
        real_d, imag_d = values[index], values[index + 1]
        if not real_d.is_integer() or not imag_d.is_integer():
            non_integer += 1
            continue
        if not (-32768 <= real_d <= 32767 and -32768 <= imag_d <= 32767):
            out_of_range += 1
            continue
        real_i, imag_i = int(real_d), int(imag_d)
        max_component_error = max(
            max_component_error, abs(real_d - real_i), abs(imag_d - imag_i)
        )
        max_amplitude_error = max(
            max_amplitude_error,
            abs(math.hypot(real_d, imag_d) - math.hypot(real_i, imag_i)),
        )

    print(f"non_integer_pairs={non_integer}")
    print(f"out_of_int16_range_pairs={out_of_range}")
    print(f"max_component_error={max_component_error}")
    print(f"max_amplitude_error={max_amplitude_error}")
    return 0 if non_integer == 0 and out_of_range == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
