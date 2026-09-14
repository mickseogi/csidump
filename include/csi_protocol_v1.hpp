#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace csi_v1 {

constexpr std::array<std::uint8_t, 4> kMagic{{'C', 'S', 'I', '1'}};
constexpr std::uint16_t kVersion = 1;
constexpr std::uint16_t kHeaderSize = 112;
constexpr std::uint16_t kStrictFftSize = 64;
constexpr std::uint8_t kSampleFormatInt16Iq = 1;
constexpr std::int8_t kUnknownS8 = static_cast<std::int8_t>(-128);
constexpr std::uint8_t kUnknownU8 = 0xff;
constexpr std::uint16_t kUnknownU16 = 0;
constexpr std::int16_t kUnknownDcIndex = -1;

// Bit flags carried in HeaderV1::flags.
enum Flags : std::uint32_t {
    kFlagNativeFftOrder            = 1u << 0,
    kFlagFftShiftApplied           = 1u << 1,
    kFlagTxSequenceValid           = 1u << 2,
    kFlagFirmwareTimestampValid    = 1u << 3,
    kFlagRssiValid                 = 1u << 4,
    kFlagNoiseFloorValid           = 1u << 5,
    kFlagMcsValid                  = 1u << 6,
    kFlagNssValid                  = 1u << 7,
    kFlagGuardIntervalValid        = 1u << 8,
    kFlagPhyModeRawValid           = 1u << 9,
    kFlagRxChainMaskValid          = 1u << 10,
    kFlagCenterFrequencyValid      = 1u << 11,
    kFlagPrimaryChannelValid       = 1u << 12,
    kFlagPayloadCrc32Valid         = 1u << 13,
    kFlagRawIntegerNoScaling       = 1u << 14,
    kFlagStrictFft64               = 1u << 15,
    kFlagBandwidthValid            = 1u << 16,
    kFlagDriverDataCountValid      = 1u << 17,
    kFlagMetadataCliOverride       = 1u << 18,
    kFlagIqCountMismatch           = 1u << 19,
    kFlagDcIndexKnown              = 1u << 20,
    kFlagTxAddressValid            = 1u << 21,
    kFlagVendorSnrValid            = 1u << 22,
};

enum class TimestampSource : std::uint8_t {
    kUnknown = 0,
    kClockRealtime = 1,
    kClockMonotonic = 2,
    kWifiTsf = 3,
    kKernel = 4,
    kFirmware = 5,
};

enum class TxSequenceSource : std::uint8_t {
    kUnavailable = 0,
    kPayloadInjected = 1,
    kIeee80211SequenceControl = 2,
    kWifiTsf = 3,
    kFirmwareSequence = 4,
};

enum class MetadataSource : std::uint8_t {
    kUnknown = 0,
    kVendorEvent = 1,
    kCommandLine = 2,
    kMixed = 3,
};

struct SampleInt16 {
    std::int16_t real = 0;
    std::int16_t imag = 0;
};

// Logical representation only. It is never sent with sizeof()/memcpy().
// serialize_packet_v1() writes every field explicitly in little-endian order.
struct HeaderV1 {
    std::uint32_t flags = 0;
    std::uint32_t receiver_id = 0;

    std::uint64_t udp_sequence = 0;
    std::uint64_t tx_sequence = 0;
    std::uint64_t timestamp_ns = 0;
    std::uint64_t firmware_timestamp_raw = 0;

    std::uint16_t center_frequency_mhz = 0;
    std::uint16_t bandwidth_mhz = 0;
    std::uint16_t fft_size = kStrictFftSize;
    std::uint16_t sample_count = kStrictFftSize;

    std::uint8_t rx_chain_index = 0;
    std::uint8_t rx_chain_count = 0;
    std::uint8_t rx_chain_mask = 0;
    std::uint8_t sample_format = kSampleFormatInt16Iq;

    std::int8_t rssi_dbm = kUnknownS8;
    std::int8_t noise_floor_dbm = kUnknownS8;
    std::uint8_t mcs = kUnknownU8;
    std::uint8_t nss = kUnknownU8;

    std::uint8_t guard_interval = kUnknownU8;
    std::uint8_t phy_mode_raw = kUnknownU8;
    TimestampSource timestamp_source = TimestampSource::kUnknown;
    TxSequenceSource tx_sequence_source = TxSequenceSource::kUnavailable;

    std::int32_t scale_numerator = 1;
    std::int32_t scale_denominator = 1;

    std::uint32_t payload_size = 0;
    std::uint32_t payload_crc32 = 0;

    std::uint16_t primary_channel = 0;
    std::uint16_t driver_data_count = 0;
    std::int8_t shift_bits = 0;
    MetadataSource metadata_source = MetadataSource::kUnknown;
    std::int16_t dc_index = kUnknownDcIndex;

    std::array<std::uint8_t, 6> transmitter_address{{0, 0, 0, 0, 0, 0}};
    std::uint8_t vendor_data_bw = kUnknownU8;
    std::uint8_t vendor_primary_channel_index = kUnknownU8;
    std::uint8_t vendor_snr = kUnknownU8;
    std::uint8_t vendor_rx_mode = kUnknownU8;
    std::uint16_t vendor_tx_index = 0;
    std::uint32_t vendor_h_index = 0;
    std::uint32_t reserved = 0;
};

std::uint32_t crc32_ieee(const std::uint8_t* data, std::size_t size);

bool serialize_packet_v1(
    HeaderV1 header,
    const std::vector<SampleInt16>& samples,
    std::vector<std::uint8_t>* output,
    std::string* error);

}  // namespace csi_v1
