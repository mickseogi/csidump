#pragma once

#include "csi_protocol_v1.hpp"
#include "mt76_csi_frame.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct ServerConfig {
    std::string interface_name;
    unsigned interval_ms = 100;
    int udp_port = 5555;
    std::uint32_t receiver_id = 0;

    std::uint16_t center_frequency_mhz = 0;
    std::uint16_t bandwidth_mhz = 20;
    std::uint16_t primary_channel = 0;
    std::uint8_t rx_chain_count = 0;
    std::uint8_t rx_chain_mask = 0;

    bool center_frequency_from_cli = false;
    bool bandwidth_from_cli = false;
    bool primary_channel_from_cli = false;
    bool rx_chain_from_cli = false;
    bool strict_fft64 = true;
};

class CsiServerV1 {
public:
    explicit CsiServerV1(ServerConfig config);
    ~CsiServerV1();

    CsiServerV1(const CsiServerV1&) = delete;
    CsiServerV1& operator=(const CsiServerV1&) = delete;

    int start();
    void stop();
    bool running() const { return running_.load(); }

private:
    void monitor_loop();
    void registration_loop();
    void send_frame(const Mt76CsiFrame& frame);
    std::uint64_t realtime_ns() const;
    std::uint16_t bandwidth_from_vendor(std::uint8_t raw) const;

    ServerConfig config_;
    Mt76CsiApi api_;
    std::atomic<bool> running_{false};
    std::thread monitor_thread_;
    std::thread registration_thread_;
    int udp_socket_ = -1;
    std::mutex clients_mutex_;
    std::vector<std::pair<std::string, int>> clients_;
    std::uint64_t udp_sequence_ = 0;
};
