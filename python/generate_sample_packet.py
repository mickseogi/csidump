#!/usr/bin/env python3
from __future__ import annotations

import binascii
from pathlib import Path
import struct

from csi_v1_protocol import (
    FLAG_NATIVE_FFT_ORDER,
    FLAG_PAYLOAD_CRC32_VALID,
    FLAG_RAW_INTEGER_NO_SCALING,
    FLAG_STRICT_FFT64,
    HEADER_SIZE,
    MAGIC,
    VERSION,
    crc32_ieee,
    parse_packet,
)

ROOT = Path(__file__).resolve().parents[1]
SAMPLE_BIN = ROOT / "samples" / "csi_v1_sample.bin"
SAMPLE_HEX = ROOT / "samples" / "csi_v1_sample.hex"


def build_sample() -> bytes:
    # Deterministic values: bin k -> real=k-32, imag=32-k.
    samples = [(index - 32, 32 - index) for index in range(64)]
    payload = b"".join(struct.pack("<hh", real, imag) for real, imag in samples)
    crc = crc32_ieee(payload)

    flags = (
        FLAG_NATIVE_FFT_ORDER
        | FLAG_PAYLOAD_CRC32_VALID
        | FLAG_RAW_INTEGER_NO_SCALING
        | FLAG_STRICT_FFT64
        | (1 << 3)   # firmware timestamp valid
        | (1 << 4)   # RSSI valid
        | (1 << 10)  # rx chain mask valid
        | (1 << 11)  # center frequency valid
        | (1 << 12)  # primary channel valid
        | (1 << 16)  # bandwidth valid
        | (1 << 17)  # driver count valid
        | (1 << 18)  # CLI metadata override
        | (1 << 21)  # transmitter address valid
        | (1 << 22)  # vendor SNR valid
    )

    header = bytearray()
    header += MAGIC
    header += struct.pack("<HH", VERSION, HEADER_SIZE)
    header += struct.pack("<II", flags, 1)
    header += struct.pack("<QQQQ", 42, 0, 1_750_000_000_123_456_789, 123456)
    header += struct.pack("<HHHH", 5180, 20, 64, 64)
    header += struct.pack("<BBBB", 0, 2, 0x03, 1)
    header += struct.pack("<bbBB", -42, -128, 0xFF, 0xFF)
    header += struct.pack("<BBBB", 0xFF, 3, 1, 0)
    header += struct.pack("<ii", 1, 1)
    header += struct.pack("<II", len(payload), crc)
    header += struct.pack("<HH", 36, 64)
    header += struct.pack("<bBh", 0, 3, -1)
    header += bytes.fromhex("ccd843b82447")
    header += struct.pack("<BBBB", 0, 0, 35, 3)
    header += struct.pack("<HII", 0, 987654, 0)
    assert len(header) == HEADER_SIZE, len(header)
    return bytes(header) + payload


def main() -> None:
    packet = build_sample()
    SAMPLE_BIN.write_bytes(packet)
    SAMPLE_HEX.write_text(binascii.hexlify(packet, sep=b" ").decode() + "\n", encoding="utf-8")
    header, samples = parse_packet(packet)
    assert header.sample_count == 64
    assert len(samples) == 64
    assert samples[0] == (-32, 32)
    assert samples[63] == (31, -31)
    print(f"wrote {SAMPLE_BIN} ({len(packet)} bytes)")
    print(f"CRC32=0x{header.payload_crc32:08x}")


if __name__ == "__main__":
    main()
