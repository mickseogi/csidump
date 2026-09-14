#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

constexpr std::size_t kMaxCsiBins = 1024;

struct Mt76CsiFrame {
    std::array<std::int16_t, kMaxCsiBins> real{};
    std::array<std::int16_t, kMaxCsiBins> imag{};
    std::uint16_t sample_count = 0;
    bool iq_count_match = true;

    std::int8_t rssi_dbm = static_cast<std::int8_t>(-128);
    std::uint8_t snr_raw = 0xff;
    std::uint32_t firmware_timestamp_raw = 0;
    std::uint8_t data_bw_raw = 0xff;
    std::uint8_t primary_channel_index_raw = 0xff;
    std::array<std::uint8_t, 6> transmitter_address{};
    std::uint32_t ext_info = 0;
    std::uint8_t rx_mode_raw = 0xff;
    std::uint16_t tx_index = 0;
    std::uint16_t rx_index = 0;
    std::uint32_t h_index = 0;
};

class Mt76CsiApi {
public:
    Mt76CsiApi();
    ~Mt76CsiApi();
    Mt76CsiApi(const Mt76CsiApi&) = delete;
    Mt76CsiApi& operator=(const Mt76CsiApi&) = delete;

    int start(const char* interface_name);
    int stop(const char* interface_name);
    bool dump(const char* interface_name, int requested_frames,
              std::vector<Mt76CsiFrame>* frames);

private:
    class Impl;
    Impl* impl_;
};
