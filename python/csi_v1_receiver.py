#!/usr/bin/env python3
from __future__ import annotations

import argparse
from collections import defaultdict
import math
import socket
import sys

from csi_v1_protocol import format_mac, parse_packet


def main() -> int:
    parser = argparse.ArgumentParser(description="Receive and validate CSI1 V1 UDP packets")
    parser.add_argument("--listen-port", type=int, default=5555)
    parser.add_argument(
        "--router",
        action="append",
        default=[],
        help="router IP to register with; repeat for multiple receivers",
    )
    parser.add_argument("--print-every", type=int, default=100)
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4 * 1024 * 1024)
    sock.bind(("0.0.0.0", args.listen_port))

    for router in args.router:
        sock.sendto(b"register", (router, args.listen_port))
        print(f"registered with {router}:{args.listen_port}")

    previous_sequence = {}
    counts = defaultdict(int)
    errors = 0

    print(f"listening on UDP {args.listen_port}")
    try:
        while True:
            packet, source = sock.recvfrom(65535)
            source_ip = source[0]
            try:
                header, samples = parse_packet(packet, require_strict64=True)
            except ValueError as exc:
                errors += 1
                print(f"INVALID from {source_ip}: {exc}", file=sys.stderr)
                continue

            key = (source_ip, header.receiver_id)
            counts[key] += 1
            previous = previous_sequence.get(key)
            sequence_status = "first"
            if previous is not None:
                if header.udp_sequence == previous + 1:
                    sequence_status = "ok"
                elif header.udp_sequence <= previous:
                    sequence_status = "duplicate-or-reordered"
                else:
                    sequence_status = f"gap:{header.udp_sequence - previous - 1}"
            previous_sequence[key] = header.udp_sequence

            if counts[key] == 1 or counts[key] % max(args.print_every, 1) == 0:
                real0, imag0 = samples[0]
                amplitude0 = math.hypot(real0, imag0) * (
                    header.scale_numerator / header.scale_denominator
                )
                tx_text = str(header.tx_sequence) if header.tx_sequence_valid else "INVALID"
                print(
                    f"src={source_ip} receiver={header.receiver_id} "
                    f"udp_seq={header.udp_sequence}({sequence_status}) tx_seq={tx_text} "
                    f"ts_ns={header.timestamp_ns} chain={header.rx_chain_index}/"
                    f"{header.rx_chain_count} mask=0x{header.rx_chain_mask:02x} "
                    f"freq={header.center_frequency_mhz} bw={header.bandwidth_mhz} "
                    f"fft={header.fft_size} samples={header.sample_count} "
                    f"rssi={header.rssi_dbm} ta={format_mac(header.transmitter_address)} "
                    f"bin0=({real0},{imag0}) amp0={amplitude0:.6f} "
                    f"bytes={len(packet)} errors={errors}"
                )
    except KeyboardInterrupt:
        print("stopped")
        return 0
    finally:
        sock.close()


if __name__ == "__main__":
    raise SystemExit(main())
