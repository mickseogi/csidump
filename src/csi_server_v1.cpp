#include "csi_server_v1.hpp"

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

namespace {

bool address_nonzero(const std::array<std::uint8_t, 6>& address) {
    return std::any_of(address.begin(), address.end(),
                       [](std::uint8_t value) { return value != 0; });
}

}  // namespace

CsiServerV1::CsiServerV1(ServerConfig config) : config_(std::move(config)) {}
CsiServerV1::~CsiServerV1() { stop(); }

int CsiServerV1::start() {
    if (running_.load()) return 0;
    if (config_.receiver_id == 0) {
        std::cerr << "receiver_id must be a non-zero stable ID\n";
        return -1;
    }

    udp_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket_ < 0) {
        std::cerr << "socket: " << std::strerror(errno) << "\n";
        return -1;
    }
    int reuse = 1;
    setsockopt(udp_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(static_cast<std::uint16_t>(config_.udp_port));
    if (bind(udp_socket_, reinterpret_cast<sockaddr*>(&server), sizeof(server)) < 0) {
        std::cerr << "bind: " << std::strerror(errno) << "\n";
        close(udp_socket_);
        udp_socket_ = -1;
        return -1;
    }

    const int api_result = api_.start(config_.interface_name.c_str());
    if (api_result != 0) {
        std::cerr << "failed to enable CSI, code=" << api_result << "\n";
        close(udp_socket_);
        udp_socket_ = -1;
        return api_result;
    }

    timespec resolution{};
    if (clock_getres(CLOCK_REALTIME, &resolution) == 0) {
        std::cout << "timestamp source=CLOCK_REALTIME, clock_getres="
                  << resolution.tv_sec << "s " << resolution.tv_nsec << "ns\n";
    }
    std::cout << "Important: CLOCK_REALTIME resolution is not the same as cross-receiver sync. "
                 "Use NTP/PTP and measure residual clock error.\n";
    std::cout << "Strict raw mode: exactly 64 native-order bins; no fftshift; no DC/guard removal.\n";
    std::cout << "tx_sequence is INVALID until the driver/firmware exports a same-frame identifier.\n";

    running_.store(true);
    registration_thread_ = std::thread(&CsiServerV1::registration_loop, this);
    monitor_thread_ = std::thread(&CsiServerV1::monitor_loop, this);
    return 0;
}

void CsiServerV1::stop() {
    if (!running_.exchange(false)) return;
    if (udp_socket_ >= 0) shutdown(udp_socket_, SHUT_RDWR);
    if (registration_thread_.joinable()) registration_thread_.join();
    if (monitor_thread_.joinable()) monitor_thread_.join();
    api_.stop(config_.interface_name.c_str());
    if (udp_socket_ >= 0) {
        close(udp_socket_);
        udp_socket_ = -1;
    }
}

void CsiServerV1::registration_loop() {
    char buffer[256]{};
    while (running_.load()) {
        sockaddr_in client{};
        socklen_t client_size = sizeof(client);
        const ssize_t received = recvfrom(
            udp_socket_, buffer, sizeof(buffer) - 1, 0,
            reinterpret_cast<sockaddr*>(&client), &client_size);
        if (received <= 0) continue;
        buffer[received] = '\0';
        if (std::strcmp(buffer, "register") != 0) continue;

        char ip[INET_ADDRSTRLEN]{};
        if (inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip)) == nullptr) continue;
        const int port = ntohs(client.sin_port);
        const std::pair<std::string, int> endpoint{ip, port};

        std::lock_guard<std::mutex> lock(clients_mutex_);
        if (std::find(clients_.begin(), clients_.end(), endpoint) == clients_.end()) {
            clients_.push_back(endpoint);
            std::cout << "registered UDP client " << ip << ':' << port << "\n";
        }
    }
}

void CsiServerV1::monitor_loop() {
    std::uint64_t rejected_count = 0;
    while (running_.load()) {
        std::vector<Mt76CsiFrame> frames;
        if (api_.dump(config_.interface_name.c_str(), 100, &frames)) {
            for (const Mt76CsiFrame& frame : frames) {
                if (!frame.iq_count_match || frame.sample_count != csi_v1::kStrictFftSize) {
                    ++rejected_count;
                    if (rejected_count <= 10 || rejected_count % 1000 == 0) {
                        std::cerr << "drop CSI event: strict mode requires matching I/Q counts "
                                     "and exactly 64 bins; got count="
                                  << frame.sample_count
                                  << " iq_match=" << frame.iq_count_match << "\n";
                    }
                    continue;
                }
                const std::uint16_t vendor_bw = bandwidth_from_vendor(frame.data_bw_raw);
                if (config_.bandwidth_from_cli && vendor_bw != 0 &&
                    vendor_bw != config_.bandwidth_mhz) {
                    ++rejected_count;
                    std::cerr << "drop CSI event: vendor bandwidth=" << vendor_bw
                              << " MHz differs from configured bandwidth="
                              << config_.bandwidth_mhz << " MHz\n";
                    continue;
                }
                if (config_.strict_fft64 && config_.bandwidth_mhz != 20) {
                    std::cerr << "strict 64-bin V1 requires 20 MHz; configured bandwidth is "
                              << config_.bandwidth_mhz << " MHz\n";
                    running_.store(false);
                    break;
                }
                send_frame(frame);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.interval_ms));
    }
}

void CsiServerV1::send_frame(const Mt76CsiFrame& frame) {
    std::vector<std::pair<std::string, int>> clients;
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients = clients_;
    }
    if (clients.empty()) return;

    std::vector<csi_v1::SampleInt16> samples;
    samples.reserve(csi_v1::kStrictFftSize);
    for (std::size_t index = 0; index < csi_v1::kStrictFftSize; ++index) {
        samples.push_back({frame.real[index], frame.imag[index]});
    }

    csi_v1::HeaderV1 header;
    header.flags = csi_v1::kFlagNativeFftOrder |
                   csi_v1::kFlagRawIntegerNoScaling |
                   csi_v1::kFlagStrictFft64 |
                   csi_v1::kFlagDriverDataCountValid |
                   csi_v1::kFlagRssiValid |
                   csi_v1::kFlagFirmwareTimestampValid |
                   csi_v1::kFlagPhyModeRawValid |
                   csi_v1::kFlagVendorSnrValid;
    header.receiver_id = config_.receiver_id;
    header.udp_sequence = ++udp_sequence_;

    // Deliberately invalid rather than inventing a value from h_idx or host time.
    header.tx_sequence = 0;
    header.tx_sequence_source = csi_v1::TxSequenceSource::kUnavailable;

    header.timestamp_ns = realtime_ns();
    header.timestamp_source = csi_v1::TimestampSource::kClockRealtime;
    header.firmware_timestamp_raw = frame.firmware_timestamp_raw;

    const std::uint16_t vendor_bw = bandwidth_from_vendor(frame.data_bw_raw);
    header.bandwidth_mhz = config_.bandwidth_from_cli
        ? config_.bandwidth_mhz : vendor_bw;
    if (header.bandwidth_mhz != 0) header.flags |= csi_v1::kFlagBandwidthValid;

    header.center_frequency_mhz = config_.center_frequency_mhz;
    if (config_.center_frequency_from_cli) {
        header.flags |= csi_v1::kFlagCenterFrequencyValid |
                        csi_v1::kFlagMetadataCliOverride;
    }
    header.primary_channel = config_.primary_channel;
    if (config_.primary_channel_from_cli) {
        header.flags |= csi_v1::kFlagPrimaryChannelValid |
                        csi_v1::kFlagMetadataCliOverride;
    }

    header.fft_size = csi_v1::kStrictFftSize;
    header.sample_count = csi_v1::kStrictFftSize;
    header.driver_data_count = frame.sample_count;
    header.sample_format = csi_v1::kSampleFormatInt16Iq;
    header.scale_numerator = 1;
    header.scale_denominator = 1;
    header.shift_bits = 0;
    header.dc_index = csi_v1::kUnknownDcIndex;

    header.rx_chain_index = static_cast<std::uint8_t>(frame.rx_index & 0xffu);
    header.rx_chain_count = config_.rx_chain_count;
    header.rx_chain_mask = config_.rx_chain_mask;
    if (config_.rx_chain_from_cli) {
        header.flags |= csi_v1::kFlagRxChainMaskValid |
                        csi_v1::kFlagMetadataCliOverride;
    }

    header.rssi_dbm = frame.rssi_dbm;
    header.noise_floor_dbm = csi_v1::kUnknownS8;
    header.mcs = csi_v1::kUnknownU8;
    header.nss = csi_v1::kUnknownU8;
    header.guard_interval = csi_v1::kUnknownU8;
    header.phy_mode_raw = frame.rx_mode_raw;

    header.metadata_source = config_.center_frequency_from_cli ||
                             config_.bandwidth_from_cli ||
                             config_.primary_channel_from_cli ||
                             config_.rx_chain_from_cli
        ? csi_v1::MetadataSource::kMixed
        : csi_v1::MetadataSource::kVendorEvent;

    header.transmitter_address = frame.transmitter_address;
    if (address_nonzero(frame.transmitter_address)) {
        header.flags |= csi_v1::kFlagTxAddressValid;
    }
    header.vendor_data_bw = frame.data_bw_raw;
    header.vendor_primary_channel_index = frame.primary_channel_index_raw;
    header.vendor_snr = frame.snr_raw;
    header.vendor_rx_mode = frame.rx_mode_raw;
    header.vendor_tx_index = frame.tx_index;
    header.vendor_h_index = frame.h_index;

    std::vector<std::uint8_t> packet;
    std::string error;
    if (!csi_v1::serialize_packet_v1(header, samples, &packet, &error)) {
        std::cerr << "serialization failed: " << error << "\n";
        return;
    }

    for (const auto& client : clients) {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<std::uint16_t>(client.second));
        if (inet_pton(AF_INET, client.first.c_str(), &address.sin_addr) != 1) continue;
        const ssize_t sent = sendto(
            udp_socket_, packet.data(), packet.size(), 0,
            reinterpret_cast<sockaddr*>(&address), sizeof(address));
        if (sent != static_cast<ssize_t>(packet.size())) {
            std::cerr << "sendto " << client.first << ':' << client.second
                      << " failed/short: " << std::strerror(errno) << "\n";
        }
    }
}

std::uint64_t CsiServerV1::realtime_ns() const {
    timespec now{};
    if (clock_gettime(CLOCK_REALTIME, &now) != 0) return 0;
    return static_cast<std::uint64_t>(now.tv_sec) * 1000000000ull +
           static_cast<std::uint64_t>(now.tv_nsec);
}

std::uint16_t CsiServerV1::bandwidth_from_vendor(std::uint8_t raw) const {
    switch (raw) {
        case 0: return 20;
        case 1: return 40;
        case 2: return 80;
        case 3: return 160;
        case 4: return 320;
        default: return 0;
    }
}
