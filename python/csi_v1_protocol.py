#!/usr/bin/env python3
"""CSI1 protocol V1 parser/serializer helpers.

Wire format:
- 112-byte explicitly serialized header
- little-endian multi-byte integers
- exact magic bytes b"CSI1"
- payload format 1: signed int16 real, signed int16 imag for each bin
"""

from __future__ import annotations

from dataclasses import dataclass
import binascii
import struct
from typing import List, Tuple

MAGIC = b"CSI1"
VERSION = 1
HEADER_SIZE = 112
SAMPLE_FORMAT_INT16_IQ = 1

FLAG_NATIVE_FFT_ORDER = 1 << 0
FLAG_FFTSHIFT_APPLIED = 1 << 1
FLAG_TX_SEQUENCE_VALID = 1 << 2
FLAG_PAYLOAD_CRC32_VALID = 1 << 13
FLAG_RAW_INTEGER_NO_SCALING = 1 << 14
FLAG_STRICT_FFT64 = 1 << 15
FLAG_DC_INDEX_KNOWN = 1 << 20


@dataclass(frozen=True)
class HeaderV1:
    flags: int
    receiver_id: int
    udp_sequence: int
    tx_sequence: int
    timestamp_ns: int
    firmware_timestamp_raw: int
    center_frequency_mhz: int
    bandwidth_mhz: int
    fft_size: int
    sample_count: int
    rx_chain_index: int
    rx_chain_count: int
    rx_chain_mask: int
    sample_format: int
    rssi_dbm: int
    noise_floor_dbm: int
    mcs: int
    nss: int
    guard_interval: int
    phy_mode_raw: int
    timestamp_source: int
    tx_sequence_source: int
    scale_numerator: int
    scale_denominator: int
    payload_size: int
    payload_crc32: int
    primary_channel: int
    driver_data_count: int
    shift_bits: int
    metadata_source: int
    dc_index: int
    transmitter_address: bytes
    vendor_data_bw: int
    vendor_primary_channel_index: int
    vendor_snr: int
    vendor_rx_mode: int
    vendor_tx_index: int
    vendor_h_index: int
    reserved: int

    @property
    def tx_sequence_valid(self) -> bool:
        return bool(self.flags & FLAG_TX_SEQUENCE_VALID)

    @property
    def native_fft_order(self) -> bool:
        return bool(self.flags & FLAG_NATIVE_FFT_ORDER)

    @property
    def fftshift_applied(self) -> bool:
        return bool(self.flags & FLAG_FFTSHIFT_APPLIED)


def crc32_ieee(data: bytes) -> int:
    return binascii.crc32(data) & 0xFFFFFFFF


def parse_packet(packet: bytes, *, require_strict64: bool = True) -> Tuple[HeaderV1, List[Tuple[int, int]]]:
    if len(packet) < HEADER_SIZE:
        raise ValueError(f"packet too short: {len(packet)} < {HEADER_SIZE}")
    if packet[0:4] != MAGIC:
        raise ValueError(f"bad magic: {packet[0:4]!r}")

    version, header_size = struct.unpack_from("<HH", packet, 4)
    if version != VERSION:
        raise ValueError(f"unsupported version: {version}")
    if header_size != HEADER_SIZE:
        raise ValueError(f"unexpected header size: {header_size}")

    flags, receiver_id = struct.unpack_from("<II", packet, 8)
    udp_sequence, tx_sequence, timestamp_ns, firmware_timestamp_raw = struct.unpack_from(
        "<QQQQ", packet, 16
    )
    center_frequency_mhz, bandwidth_mhz, fft_size, sample_count = struct.unpack_from(
        "<HHHH", packet, 48
    )
    rx_chain_index, rx_chain_count, rx_chain_mask, sample_format = struct.unpack_from(
        "<BBBB", packet, 56
    )
    rssi_dbm, noise_floor_dbm, mcs, nss = struct.unpack_from("<bbBB", packet, 60)
    guard_interval, phy_mode_raw, timestamp_source, tx_sequence_source = struct.unpack_from(
        "<BBBB", packet, 64
    )
    scale_numerator, scale_denominator = struct.unpack_from("<ii", packet, 68)
    payload_size, payload_crc32 = struct.unpack_from("<II", packet, 76)
    primary_channel, driver_data_count = struct.unpack_from("<HH", packet, 84)
    shift_bits, metadata_source, dc_index = struct.unpack_from("<bBh", packet, 88)
    transmitter_address = packet[92:98]
    vendor_data_bw, vendor_primary_channel_index, vendor_snr, vendor_rx_mode = struct.unpack_from(
        "<BBBB", packet, 98
    )
    vendor_tx_index = struct.unpack_from("<H", packet, 102)[0]
    vendor_h_index, reserved = struct.unpack_from("<II", packet, 104)

    header = HeaderV1(
        flags=flags,
        receiver_id=receiver_id,
        udp_sequence=udp_sequence,
        tx_sequence=tx_sequence,
        timestamp_ns=timestamp_ns,
        firmware_timestamp_raw=firmware_timestamp_raw,
        center_frequency_mhz=center_frequency_mhz,
        bandwidth_mhz=bandwidth_mhz,
        fft_size=fft_size,
        sample_count=sample_count,
        rx_chain_index=rx_chain_index,
        rx_chain_count=rx_chain_count,
        rx_chain_mask=rx_chain_mask,
        sample_format=sample_format,
        rssi_dbm=rssi_dbm,
        noise_floor_dbm=noise_floor_dbm,
        mcs=mcs,
        nss=nss,
        guard_interval=guard_interval,
        phy_mode_raw=phy_mode_raw,
        timestamp_source=timestamp_source,
        tx_sequence_source=tx_sequence_source,
        scale_numerator=scale_numerator,
        scale_denominator=scale_denominator,
        payload_size=payload_size,
        payload_crc32=payload_crc32,
        primary_channel=primary_channel,
        driver_data_count=driver_data_count,
        shift_bits=shift_bits,
        metadata_source=metadata_source,
        dc_index=dc_index,
        transmitter_address=transmitter_address,
        vendor_data_bw=vendor_data_bw,
        vendor_primary_channel_index=vendor_primary_channel_index,
        vendor_snr=vendor_snr,
        vendor_rx_mode=vendor_rx_mode,
        vendor_tx_index=vendor_tx_index,
        vendor_h_index=vendor_h_index,
        reserved=reserved,
    )

    payload = packet[header_size:]
    if len(payload) != payload_size:
        raise ValueError(f"payload size mismatch: wire={len(payload)} header={payload_size}")
    if flags & FLAG_PAYLOAD_CRC32_VALID:
        actual_crc = crc32_ieee(payload)
        if actual_crc != payload_crc32:
            raise ValueError(
                f"CRC32 mismatch: actual=0x{actual_crc:08x} expected=0x{payload_crc32:08x}"
            )
    if sample_format != SAMPLE_FORMAT_INT16_IQ:
        raise ValueError(f"unsupported sample format: {sample_format}")
    expected_payload_size = sample_count * 4
    if payload_size != expected_payload_size:
        raise ValueError(
            f"int16 payload must be sample_count*4: {payload_size} != {expected_payload_size}"
        )
    if fft_size != sample_count:
        raise ValueError(f"raw V1 requires fft_size == sample_count: {fft_size} != {sample_count}")
    if require_strict64 and (fft_size != 64 or sample_count != 64):
        raise ValueError(f"strict V1 requires 64 bins, got fft={fft_size}, samples={sample_count}")
    if not header.native_fft_order or header.fftshift_applied:
        raise ValueError("strict receiver requires native FFT order with no fftshift")
    if scale_denominator == 0:
        raise ValueError("scale denominator is zero")

    flat = struct.unpack_from(f"<{sample_count * 2}h", payload, 0)
    samples = [(flat[i], flat[i + 1]) for i in range(0, len(flat), 2)]
    return header, samples


def format_mac(value: bytes) -> str:
    return ":".join(f"{item:02x}" for item in value)
