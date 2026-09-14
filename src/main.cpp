#include "csi_server_v1.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

namespace {
std::atomic<bool> g_stop{false};
void signal_handler(int) { g_stop.store(true); }

std::uint64_t parse_u64(const std::string& text, const char* name) {
    std::size_t used = 0;
    const unsigned long long value = std::stoull(text, &used, 0);
    if (used != text.size()) throw std::invalid_argument(std::string(name) + " has trailing text");
    return static_cast<std::uint64_t>(value);
}

void usage(const char* program) {
    std::cerr
        << "Usage:\n  " << program
        << " <wifi_interface> <interval_ms> <udp_port> --receiver-id <id>"
           " --bandwidth-mhz 20 [options]\n\n"
           "Options:\n"
           "  --center-frequency-mhz <MHz>\n"
           "  --primary-channel <channel>\n"
           "  --rx-chain-count <count>\n"
           "  --rx-chain-mask <mask, e.g. 0x03>\n"
           "\nV1 strict mode sends exactly 64 native-order int16 I/Q bins and rejects other sizes.\n";
}
}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 8) {
            usage(argv[0]);
            return 2;
        }

        ServerConfig config;
        config.interface_name = argv[1];
        config.interval_ms = static_cast<unsigned>(parse_u64(argv[2], "interval_ms"));
        config.udp_port = static_cast<int>(parse_u64(argv[3], "udp_port"));

        for (int index = 4; index < argc; ++index) {
            const std::string option = argv[index];
            if (index + 1 >= argc) throw std::invalid_argument("missing value for " + option);
            const std::string value = argv[++index];
            if (option == "--receiver-id") {
                config.receiver_id = static_cast<std::uint32_t>(parse_u64(value, "receiver_id"));
            } else if (option == "--center-frequency-mhz") {
                config.center_frequency_mhz = static_cast<std::uint16_t>(parse_u64(value, "center_frequency_mhz"));
                config.center_frequency_from_cli = true;
            } else if (option == "--bandwidth-mhz") {
                config.bandwidth_mhz = static_cast<std::uint16_t>(parse_u64(value, "bandwidth_mhz"));
                config.bandwidth_from_cli = true;
            } else if (option == "--primary-channel") {
                config.primary_channel = static_cast<std::uint16_t>(parse_u64(value, "primary_channel"));
                config.primary_channel_from_cli = true;
            } else if (option == "--rx-chain-count") {
                config.rx_chain_count = static_cast<std::uint8_t>(parse_u64(value, "rx_chain_count"));
                config.rx_chain_from_cli = true;
            } else if (option == "--rx-chain-mask") {
                config.rx_chain_mask = static_cast<std::uint8_t>(parse_u64(value, "rx_chain_mask"));
                config.rx_chain_from_cli = true;
            } else {
                throw std::invalid_argument("unknown option: " + option);
            }
        }

        if (config.receiver_id == 0 || !config.bandwidth_from_cli || config.bandwidth_mhz != 20 ||
            !config.center_frequency_from_cli || !config.primary_channel_from_cli ||
            !config.rx_chain_from_cli) {
            throw std::invalid_argument(
                "V1 production mode requires non-zero --receiver-id, --bandwidth-mhz 20, "
                "--center-frequency-mhz, --primary-channel, --rx-chain-count and --rx-chain-mask");
        }
        if (config.rx_chain_count == 0 || config.rx_chain_mask == 0) {
            throw std::invalid_argument("rx chain count and mask must both be non-zero");
        }
        unsigned mask_bits = 0;
        for (std::uint8_t mask = config.rx_chain_mask; mask != 0; mask >>= 1u) {
            mask_bits += mask & 1u;
        }
        if (mask_bits != config.rx_chain_count) {
            throw std::invalid_argument("rx-chain-count must equal the number of set bits in rx-chain-mask");
        }

        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        CsiServerV1 server(config);
        const int result = server.start();
        if (result != 0) return result;
        while (!g_stop.load() && server.running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        server.stop();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        usage(argv[0]);
        return 2;
    }
}
