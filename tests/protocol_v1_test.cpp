#include "csi_protocol_v1.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
std::uint16_t read_le16(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return static_cast<std::uint16_t>(data.at(offset)) |
           static_cast<std::uint16_t>(data.at(offset + 1)) << 8u;
}
std::uint32_t read_le32(const std::vector<std::uint8_t>& data, std::size_t offset) {
    std::uint32_t value = 0;
    for (unsigned index = 0; index < 4; ++index) {
        value |= static_cast<std::uint32_t>(data.at(offset + index)) << (index * 8u);
    }
    return value;
}
std::int16_t read_le_s16(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return static_cast<std::int16_t>(read_le16(data, offset));
}
}  // namespace

int main() {
    std::vector<csi_v1::SampleInt16> samples;
    for (int index = 0; index < 64; ++index) {
        samples.push_back({static_cast<std::int16_t>(index - 32),
                           static_cast<std::int16_t>(32 - index)});
    }

    csi_v1::HeaderV1 header;
    header.flags = csi_v1::kFlagNativeFftOrder |
                   csi_v1::kFlagRawIntegerNoScaling |
                   csi_v1::kFlagStrictFft64;
    header.receiver_id = 7;
    header.udp_sequence = 9;
    header.timestamp_ns = 123456789;
    header.timestamp_source = csi_v1::TimestampSource::kClockRealtime;
    header.fft_size = 64;
    header.sample_count = 64;
    header.driver_data_count = 64;

    std::vector<std::uint8_t> packet;
    std::string error;
    assert(csi_v1::serialize_packet_v1(header, samples, &packet, &error));
    assert(packet.size() == 112 + 256);
    assert(packet[0] == 'C' && packet[1] == 'S' && packet[2] == 'I' && packet[3] == '1');
    assert(read_le16(packet, 4) == 1);
    assert(read_le16(packet, 6) == 112);
    assert(read_le32(packet, 12) == 7);
    assert(read_le16(packet, 52) == 64);
    assert(read_le16(packet, 54) == 64);
    assert(read_le32(packet, 76) == 256);

    const std::uint32_t expected_crc = csi_v1::crc32_ieee(packet.data() + 112, 256);
    assert(read_le32(packet, 80) == expected_crc);
    for (int index = 0; index < 64; ++index) {
        const std::size_t offset = 112 + static_cast<std::size_t>(index) * 4;
        assert(read_le_s16(packet, offset) == index - 32);
        assert(read_le_s16(packet, offset + 2) == 32 - index);

        const double legacy_real = static_cast<double>(samples[index].real);
        const double legacy_imag = static_cast<double>(samples[index].imag);
        const double integer_amplitude = std::hypot(
            static_cast<double>(samples[index].real),
            static_cast<double>(samples[index].imag));
        const double legacy_amplitude = std::hypot(legacy_real, legacy_imag);
        assert(integer_amplitude == legacy_amplitude);
    }

    std::cout << "protocol_v1_test passed: 64 bins, 256-byte payload, CRC32, exact int16/double amplitude equivalence\n";
    return 0;
}
