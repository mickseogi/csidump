#!/usr/bin/env python3
"""Reference transmitter payload for a future driver-level tx_sequence extractor.

This does NOT make current CSIdump expose tx_sequence. The CSI driver/firmware must
copy this uint64 value into the CSI vendor event for exact cross-receiver matching.
Use broadcast/multicast so all receivers observe one over-the-air frame; separate
unicast packets are separate 802.11 frames.
"""

from __future__ import annotations

import argparse
import socket
import struct
import time

PAYLOAD_MAGIC = b"TXSQ"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--destination", default="255.255.255.255")
    parser.add_argument("--port", type=int, default=6000)
    parser.add_argument("--rate-hz", type=float, default=10.0)
    parser.add_argument("--start", type=int, default=1)
    args = parser.parse_args()

    if args.rate_hz <= 0:
        raise SystemExit("rate must be positive")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    period = 1.0 / args.rate_hz
    sequence = args.start
    next_send = time.monotonic()
    try:
        while True:
            # Wire payload: 4-byte magic + uint64 LE sequence + uint64 LE sender time_ns.
            payload = PAYLOAD_MAGIC + struct.pack("<QQ", sequence, time.time_ns())
            sock.sendto(payload, (args.destination, args.port))
            print(f"tx_sequence={sequence}")
            sequence += 1
            next_send += period
            time.sleep(max(0.0, next_send - time.monotonic()))
    except KeyboardInterrupt:
        return 0
    finally:
        sock.close()


if __name__ == "__main__":
    raise SystemExit(main())
