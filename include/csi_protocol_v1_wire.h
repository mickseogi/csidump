#ifndef CSI_PROTOCOL_V1_WIRE_H
#define CSI_PROTOCOL_V1_WIRE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CSI_V1_VERSION 1u
#define CSI_V1_HEADER_SIZE 112u
#define CSI_V1_SAMPLE_FORMAT_INT16_IQ 1u
#define CSI_V1_STRICT_SAMPLE_COUNT 64u
#define CSI_V1_INT16_PAYLOAD_SIZE 256u

/* Exact wire offsets. Multi-byte fields are little-endian. */
enum csi_v1_offset {
    CSI_V1_OFF_MAGIC = 0,
    CSI_V1_OFF_VERSION = 4,
    CSI_V1_OFF_HEADER_SIZE = 6,
    CSI_V1_OFF_FLAGS = 8,
    CSI_V1_OFF_RECEIVER_ID = 12,
    CSI_V1_OFF_UDP_SEQUENCE = 16,
    CSI_V1_OFF_TX_SEQUENCE = 24,
    CSI_V1_OFF_TIMESTAMP_NS = 32,
    CSI_V1_OFF_FIRMWARE_TIMESTAMP_RAW = 40,
    CSI_V1_OFF_CENTER_FREQUENCY_MHZ = 48,
    CSI_V1_OFF_BANDWIDTH_MHZ = 50,
    CSI_V1_OFF_FFT_SIZE = 52,
    CSI_V1_OFF_SAMPLE_COUNT = 54,
    CSI_V1_OFF_RX_CHAIN_INDEX = 56,
    CSI_V1_OFF_RX_CHAIN_COUNT = 57,
    CSI_V1_OFF_RX_CHAIN_MASK = 58,
    CSI_V1_OFF_SAMPLE_FORMAT = 59,
    CSI_V1_OFF_RSSI_DBM = 60,
    CSI_V1_OFF_NOISE_FLOOR_DBM = 61,
    CSI_V1_OFF_MCS = 62,
    CSI_V1_OFF_NSS = 63,
    CSI_V1_OFF_GUARD_INTERVAL = 64,
    CSI_V1_OFF_PHY_MODE_RAW = 65,
    CSI_V1_OFF_TIMESTAMP_SOURCE = 66,
    CSI_V1_OFF_TX_SEQUENCE_SOURCE = 67,
    CSI_V1_OFF_SCALE_NUMERATOR = 68,
    CSI_V1_OFF_SCALE_DENOMINATOR = 72,
    CSI_V1_OFF_PAYLOAD_SIZE = 76,
    CSI_V1_OFF_PAYLOAD_CRC32 = 80,
    CSI_V1_OFF_PRIMARY_CHANNEL = 84,
    CSI_V1_OFF_DRIVER_DATA_COUNT = 86,
    CSI_V1_OFF_SHIFT_BITS = 88,
    CSI_V1_OFF_METADATA_SOURCE = 89,
    CSI_V1_OFF_DC_INDEX = 90,
    CSI_V1_OFF_TRANSMITTER_ADDRESS = 92,
    CSI_V1_OFF_VENDOR_DATA_BW = 98,
    CSI_V1_OFF_VENDOR_PRIMARY_CHANNEL_INDEX = 99,
    CSI_V1_OFF_VENDOR_SNR = 100,
    CSI_V1_OFF_VENDOR_RX_MODE = 101,
    CSI_V1_OFF_VENDOR_TX_INDEX = 102,
    CSI_V1_OFF_VENDOR_H_INDEX = 104,
    CSI_V1_OFF_RESERVED = 108,
};

/* Documentation model only. Do not send this struct with sendto(). */
struct CsiUdpHeaderV1Logical {
    uint8_t magic[4];
    uint16_t version;
    uint16_t header_size;
    uint32_t flags;
    uint32_t receiver_id;
    uint64_t udp_sequence;
    uint64_t tx_sequence;
    uint64_t timestamp_ns;
    uint64_t firmware_timestamp_raw;
    uint16_t center_frequency_mhz;
    uint16_t bandwidth_mhz;
    uint16_t fft_size;
    uint16_t sample_count;
    uint8_t rx_chain_index;
    uint8_t rx_chain_count;
    uint8_t rx_chain_mask;
    uint8_t sample_format;
    int8_t rssi_dbm;
    int8_t noise_floor_dbm;
    uint8_t mcs;
    uint8_t nss;
    uint8_t guard_interval;
    uint8_t phy_mode_raw;
    uint8_t timestamp_source;
    uint8_t tx_sequence_source;
    int32_t scale_numerator;
    int32_t scale_denominator;
    uint32_t payload_size;
    uint32_t payload_crc32;
    uint16_t primary_channel;
    uint16_t driver_data_count;
    int8_t shift_bits;
    uint8_t metadata_source;
    int16_t dc_index;
    uint8_t transmitter_address[6];
    uint8_t vendor_data_bw;
    uint8_t vendor_primary_channel_index;
    uint8_t vendor_snr;
    uint8_t vendor_rx_mode;
    uint16_t vendor_tx_index;
    uint32_t vendor_h_index;
    uint32_t reserved;
};

struct CsiSampleInt16Logical {
    int16_t real;
    int16_t imag;
};

#ifdef __cplusplus
}
#endif

#endif
