#include "csi_protocol_v1.hpp"

#include <limits>

namespace csi_v1 {
namespace {

void append_u8(std::vector<std::uint8_t>* out, std::uint8_t value) {
    out->push_back(value);
}

void append_s8(std::vector<std::uint8_t>* out, std::int8_t value) {
    out->push_back(static_cast<std::uint8_t>(value));
}

void append_le16(std::vector<std::uint8_t>* out, std::uint16_t value) {
    out->push_back(static_cast<std::uint8_t>(value & 0xffu));
    out->push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void append_le_s16(std::vector<std::uint8_t>* out, std::int16_t value) {
    append_le16(out, static_cast<std::uint16_t>(value));
}

void append_le32(std::vector<std::uint8_t>* out, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        out->push_back(static_cast<std::uint8_t>((value >> shift) & 0xffu));
    }
}

void append_le_s32(std::vector<std::uint8_t>* out, std::int32_t value) {
    append_le32(out, static_cast<std::uint32_t>(value));
}

void append_le64(std::vector<std::uint8_t>* out, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64; shift += 8) {
        out->push_back(static_cast<std::uint8_t>((value >> shift) & 0xffu));
    }
}

}  // namespace

std::uint32_t crc32_ieee(const std::uint8_t* data, std::size_t size) {
    std::uint32_t crc = 0xffffffffu;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^ (0xedb88320u & mask);
        }
    }
    return ~crc;
}

bool serialize_packet_v1(
    HeaderV1 header,
    const std::vector<SampleInt16>& samples,
    std::vector<std::uint8_t>* output,
    std::string* error) {
    if (output == nullptr) {
        if (error) *error = "output is null";
        return false;
    }
    if (header.sample_format != kSampleFormatInt16Iq) {
        if (error) *error = "only sample_format=1 (signed int16 I/Q) is implemented";
        return false;
    }
    if (samples.size() > std::numeric_limits<std::uint16_t>::max()) {
        if (error) *error = "sample count exceeds uint16";
        return false;
    }
    if (header.sample_count != samples.size()) {
        if (error) *error = "header.sample_count does not match payload sample count";
        return false;
    }
    if (header.fft_size != header.sample_count) {
        if (error) *error = "V1 raw mode requires fft_size == sample_count";
        return false;
    }
    if ((header.flags & kFlagStrictFft64) != 0u &&
        (header.fft_size != kStrictFftSize || header.sample_count != kStrictFftSize)) {
        if (error) *error = "strict FFT64 flag requires exactly 64 samples";
        return false;
    }
    if (header.scale_denominator == 0) {
        if (error) *error = "scale_denominator must not be zero";
        return false;
    }

    std::vector<std::uint8_t> payload;
    payload.reserve(samples.size() * 4u);
    for (const SampleInt16& sample : samples) {
        append_le_s16(&payload, sample.real);
        append_le_s16(&payload, sample.imag);
    }

    header.payload_size = static_cast<std::uint32_t>(payload.size());
    header.payload_crc32 = crc32_ieee(payload.data(), payload.size());
    header.flags |= kFlagPayloadCrc32Valid;

    output->clear();
    output->reserve(kHeaderSize + payload.size());

    output->insert(output->end(), kMagic.begin(), kMagic.end());              // 0
    append_le16(output, kVersion);                                             // 4
    append_le16(output, kHeaderSize);                                          // 6
    append_le32(output, header.flags);                                         // 8
    append_le32(output, header.receiver_id);                                   // 12
    append_le64(output, header.udp_sequence);                                  // 16
    append_le64(output, header.tx_sequence);                                   // 24
    append_le64(output, header.timestamp_ns);                                  // 32
    append_le64(output, header.firmware_timestamp_raw);                        // 40
    append_le16(output, header.center_frequency_mhz);                          // 48
    append_le16(output, header.bandwidth_mhz);                                 // 50
    append_le16(output, header.fft_size);                                      // 52
    append_le16(output, header.sample_count);                                  // 54
    append_u8(output, header.rx_chain_index);                                  // 56
    append_u8(output, header.rx_chain_count);                                  // 57
    append_u8(output, header.rx_chain_mask);                                   // 58
    append_u8(output, header.sample_format);                                   // 59
    append_s8(output, header.rssi_dbm);                                        // 60
    append_s8(output, header.noise_floor_dbm);                                 // 61
    append_u8(output, header.mcs);                                             // 62
    append_u8(output, header.nss);                                             // 63
    append_u8(output, header.guard_interval);                                  // 64
    append_u8(output, header.phy_mode_raw);                                    // 65
    append_u8(output, static_cast<std::uint8_t>(header.timestamp_source));      // 66
    append_u8(output, static_cast<std::uint8_t>(header.tx_sequence_source));    // 67
    append_le_s32(output, header.scale_numerator);                             // 68
    append_le_s32(output, header.scale_denominator);                           // 72
    append_le32(output, header.payload_size);                                  // 76
    append_le32(output, header.payload_crc32);                                 // 80
    append_le16(output, header.primary_channel);                               // 84
    append_le16(output, header.driver_data_count);                             // 86
    append_s8(output, header.shift_bits);                                      // 88
    append_u8(output, static_cast<std::uint8_t>(header.metadata_source));       // 89
    append_le_s16(output, header.dc_index);                                    // 90
    output->insert(output->end(), header.transmitter_address.begin(),
                   header.transmitter_address.end());                          // 92
    append_u8(output, header.vendor_data_bw);                                  // 98
    append_u8(output, header.vendor_primary_channel_index);                    // 99
    append_u8(output, header.vendor_snr);                                      // 100
    append_u8(output, header.vendor_rx_mode);                                  // 101
    append_le16(output, header.vendor_tx_index);                               // 102
    append_le32(output, header.vendor_h_index);                                // 104
    append_le32(output, header.reserved);                                      // 108

    if (output->size() != kHeaderSize) {
        if (error) *error = "internal header size mismatch";
        output->clear();
        return false;
    }

    output->insert(output->end(), payload.begin(), payload.end());
    return true;
}

}  // namespace csi_v1
