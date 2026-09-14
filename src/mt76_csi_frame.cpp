#include "mt76_csi_frame.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>

#include <net/if.h>
#include <linux/nl80211.h>

extern "C" {
#include <netlink/attr.h>
#include <netlink/msg.h>
#include <unl.h>
}

namespace {

constexpr std::uint32_t kMtkVendorId = 0x0ce7;
constexpr int kDumpPerRequest = 3;

enum MtkNl80211VendorSubcmd : std::uint32_t {
    kVendorSubcmdCsiCtrl = 0xc2,
};

enum MtkVendorAttrCsiCtrl {
    kCsiCtrlUnspec,
    kCsiCtrlCfg,
    kCsiCtrlCfgMode,
    kCsiCtrlCfgType,
    kCsiCtrlCfgVal1,
    kCsiCtrlCfgVal2,
    kCsiCtrlMacAddr,
    kCsiCtrlInterval,
    kCsiCtrlDumpNum,
    kCsiCtrlData,
    kNumCsiCtrlAttrs,
    kCsiCtrlMax = kNumCsiCtrlAttrs - 1,
};

enum MtkVendorAttrCsiData {
    kCsiDataUnspec,
    kCsiDataPad,
    kCsiDataVer,
    kCsiDataTs,
    kCsiDataRssi,
    kCsiDataSnr,
    kCsiDataBw,
    kCsiDataChIdx,
    kCsiDataTa,
    kCsiDataI,
    kCsiDataQ,
    kCsiDataInfo,
    kCsiDataReserved1,
    kCsiDataReserved2,
    kCsiDataReserved3,
    kCsiDataReserved4,
    kCsiDataTxAnt,
    kCsiDataRxAnt,
    kCsiDataMode,
    kCsiDataHIdx,
    kNumCsiDataAttrs,
    kCsiDataMax = kNumCsiDataAttrs - 1,
};

std::int16_t bit_preserving_s16(std::uint16_t raw) {
    std::int16_t signed_value = 0;
    static_assert(sizeof(signed_value) == sizeof(raw), "unexpected int16 size");
    std::memcpy(&signed_value, &raw, sizeof(raw));
    return signed_value;
}

std::int8_t bit_preserving_s8(std::uint8_t raw) {
    std::int8_t signed_value = 0;
    static_assert(sizeof(signed_value) == sizeof(raw), "unexpected int8 size");
    std::memcpy(&signed_value, &raw, sizeof(raw));
    return signed_value;
}

}  // namespace

class Mt76CsiApi::Impl {
public:
    Impl() {
        std::memset(csi_ctrl_policy_, 0, sizeof(csi_ctrl_policy_));
        std::memset(csi_data_policy_, 0, sizeof(csi_data_policy_));

        csi_ctrl_policy_[kCsiCtrlCfg].type = NLA_NESTED;
        csi_ctrl_policy_[kCsiCtrlCfgMode].type = NLA_U8;
        csi_ctrl_policy_[kCsiCtrlCfgType].type = NLA_U8;
        csi_ctrl_policy_[kCsiCtrlCfgVal1].type = NLA_U8;
        csi_ctrl_policy_[kCsiCtrlCfgVal2].type = NLA_U32;
        csi_ctrl_policy_[kCsiCtrlMacAddr].type = NLA_NESTED;
        csi_ctrl_policy_[kCsiCtrlDumpNum].type = NLA_U16;
        csi_ctrl_policy_[kCsiCtrlData].type = NLA_NESTED;

        csi_data_policy_[kCsiDataVer].type = NLA_U8;
        csi_data_policy_[kCsiDataTs].type = NLA_U32;
        csi_data_policy_[kCsiDataRssi].type = NLA_U8;
        csi_data_policy_[kCsiDataSnr].type = NLA_U8;
        csi_data_policy_[kCsiDataBw].type = NLA_U8;
        csi_data_policy_[kCsiDataChIdx].type = NLA_U8;
        csi_data_policy_[kCsiDataTa].type = NLA_NESTED;
        csi_data_policy_[kCsiDataI].type = NLA_NESTED;
        csi_data_policy_[kCsiDataQ].type = NLA_NESTED;
        csi_data_policy_[kCsiDataInfo].type = NLA_U32;
        csi_data_policy_[kCsiDataTxAnt].type = NLA_U8;
        csi_data_policy_[kCsiDataRxAnt].type = NLA_U8;
        csi_data_policy_[kCsiDataMode].type = NLA_U8;
        csi_data_policy_[kCsiDataHIdx].type = NLA_U32;
    }

    static int dump_callback(struct nl_msg* msg, void* arg) {
        auto* self = static_cast<Impl*>(arg);
        if (self == nullptr || self->active_output_ == nullptr) return NL_SKIP;

        struct nlattr* ctrl[kNumCsiCtrlAttrs]{};
        struct nlattr* data[kNumCsiDataAttrs]{};
        struct nlattr* vendor_data =
            unl_find_attr(&self->unl_, msg, NL80211_ATTR_VENDOR_DATA);
        if (vendor_data == nullptr) {
            std::fprintf(stderr, "CSI vendor data attribute not found\n");
            return NL_SKIP;
        }

        nla_parse_nested(ctrl, kCsiCtrlMax, vendor_data, self->csi_ctrl_policy_);
        if (ctrl[kCsiCtrlData] == nullptr) return NL_SKIP;
        nla_parse_nested(data, kCsiDataMax, ctrl[kCsiCtrlData], self->csi_data_policy_);

        const int required[] = {
            kCsiDataTs, kCsiDataRssi, kCsiDataSnr, kCsiDataBw,
            kCsiDataChIdx, kCsiDataTa, kCsiDataI, kCsiDataQ,
            kCsiDataInfo, kCsiDataRxAnt, kCsiDataMode, kCsiDataHIdx,
        };
        for (int attr : required) {
            if (data[attr] == nullptr) {
                std::fprintf(stderr, "CSI event missing required attribute %d\n", attr);
                return NL_SKIP;
            }
        }

        Mt76CsiFrame frame{};  // Zero-initialize all arrays and metadata.
        frame.firmware_timestamp_raw = nla_get_u32(data[kCsiDataTs]);
        frame.rssi_dbm = bit_preserving_s8(nla_get_u8(data[kCsiDataRssi]));
        frame.snr_raw = nla_get_u8(data[kCsiDataSnr]);
        frame.data_bw_raw = nla_get_u8(data[kCsiDataBw]);
        frame.primary_channel_index_raw = nla_get_u8(data[kCsiDataChIdx]);
        frame.ext_info = nla_get_u32(data[kCsiDataInfo]);
        frame.rx_mode_raw = nla_get_u8(data[kCsiDataMode]);
        frame.h_index = nla_get_u32(data[kCsiDataHIdx]);

        if (data[kCsiDataTxAnt] != nullptr) {
            frame.tx_index = nla_get_u8(data[kCsiDataTxAnt]);
        }
        if (data[kCsiDataRxAnt] != nullptr) {
            frame.rx_index = nla_get_u8(data[kCsiDataRxAnt]);
        }

        std::size_t ta_count = 0;
        std::size_t real_count = 0;
        std::size_t imag_count = 0;
        struct nlattr* cur = nullptr;
        int rem = 0;

        nla_for_each_nested(cur, data[kCsiDataTa], rem) {
            if (ta_count < frame.transmitter_address.size()) {
                frame.transmitter_address[ta_count++] = nla_get_u8(cur);
            }
        }
        nla_for_each_nested(cur, data[kCsiDataI], rem) {
            if (real_count < frame.real.size()) {
                frame.real[real_count] = bit_preserving_s16(nla_get_u16(cur));
            }
            ++real_count;
        }
        nla_for_each_nested(cur, data[kCsiDataQ], rem) {
            if (imag_count < frame.imag.size()) {
                frame.imag[imag_count] = bit_preserving_s16(nla_get_u16(cur));
            }
            ++imag_count;
        }

        if (real_count > frame.real.size() || imag_count > frame.imag.size()) {
            std::fprintf(stderr, "CSI event exceeded maximum supported bin count\n");
            return NL_SKIP;
        }

        frame.iq_count_match = (real_count == imag_count);
        frame.sample_count = static_cast<std::uint16_t>(
            std::min<std::size_t>(std::min(real_count, imag_count), 0xffffu));
        if (frame.sample_count == 0) {
            std::fprintf(stderr, "CSI event contained no complete I/Q pairs\n");
            return NL_SKIP;
        }

        self->active_output_->push_back(frame);
        return NL_SKIP;
    }

    bool dump(const char* interface_name, int requested_frames,
              std::vector<Mt76CsiFrame>* frames) {
        if (interface_name == nullptr || frames == nullptr || requested_frames <= 0) {
            return false;
        }
        const int ifindex = if_nametoindex(interface_name);
        if (ifindex == 0) {
            std::fprintf(stderr, "if_nametoindex(%s): %s\n",
                         interface_name, std::strerror(errno));
            return false;
        }

        frames->clear();
        active_output_ = frames;
        const int request_count = (requested_frames + kDumpPerRequest - 1) / kDumpPerRequest;

        for (int i = 0; i < request_count; ++i) {
            if (unl_genl_init(&unl_, "nl80211") < 0) {
                std::fprintf(stderr, "Failed to connect to nl80211\n");
                active_output_ = nullptr;
                return false;
            }

            struct nl_msg* msg = unl_genl_msg(&unl_, NL80211_CMD_VENDOR, true);
            if (msg == nullptr ||
                nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifindex) ||
                nla_put_u32(msg, NL80211_ATTR_VENDOR_ID, kMtkVendorId) ||
                nla_put_u32(msg, NL80211_ATTR_VENDOR_SUBCMD, kVendorSubcmdCsiCtrl)) {
                unl_free(&unl_);
                active_output_ = nullptr;
                return false;
            }

            struct nlattr* vendor = nla_nest_start(
                msg, NL80211_ATTR_VENDOR_DATA | NLA_F_NESTED);
            if (vendor == nullptr ||
                nla_put_u16(msg, kCsiCtrlDumpNum, kDumpPerRequest)) {
                unl_free(&unl_);
                active_output_ = nullptr;
                return false;
            }
            nla_nest_end(msg, vendor);

            const int ret = unl_genl_request(&unl_, msg, dump_callback, this);
            if (ret != 0) {
                std::fprintf(stderr, "nl80211 CSI dump failed: %s\n", std::strerror(-ret));
            }
            unl_free(&unl_);
        }

        active_output_ = nullptr;
        return !frames->empty();
    }

    int set(const char* interface_name, std::uint8_t mode, std::uint8_t type,
            std::uint8_t value1, std::uint32_t value2) {
        const int ifindex = if_nametoindex(interface_name);
        if (ifindex == 0) {
            std::fprintf(stderr, "if_nametoindex(%s): %s\n",
                         interface_name, std::strerror(errno));
            return 2;
        }
        if (unl_genl_init(&unl_, "nl80211") < 0) {
            std::fprintf(stderr, "Failed to connect to nl80211\n");
            return 2;
        }

        struct nl_msg* msg = unl_genl_msg(&unl_, NL80211_CMD_VENDOR, false);
        if (msg == nullptr ||
            nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifindex) ||
            nla_put_u32(msg, NL80211_ATTR_VENDOR_ID, kMtkVendorId) ||
            nla_put_u32(msg, NL80211_ATTR_VENDOR_SUBCMD, kVendorSubcmdCsiCtrl)) {
            unl_free(&unl_);
            return -ENOMEM;
        }

        struct nlattr* vendor = nla_nest_start(
            msg, NL80211_ATTR_VENDOR_DATA | NLA_F_NESTED);
        struct nlattr* cfg = nla_nest_start(msg, kCsiCtrlCfg | NLA_F_NESTED);
        if (vendor == nullptr || cfg == nullptr ||
            nla_put_u8(msg, kCsiCtrlCfgMode, mode) ||
            nla_put_u8(msg, kCsiCtrlCfgType, type) ||
            nla_put_u8(msg, kCsiCtrlCfgVal1, value1) ||
            nla_put_u32(msg, kCsiCtrlCfgVal2, value2)) {
            unl_free(&unl_);
            return -ENOMEM;
        }
        nla_nest_end(msg, cfg);
        nla_nest_end(msg, vendor);

        const int ret = unl_genl_request(&unl_, msg, nullptr, nullptr);
        if (ret != 0) {
            std::fprintf(stderr, "nl80211 CSI control failed: %s\n", std::strerror(-ret));
        }
        unl_free(&unl_);
        return ret;
    }

    int start(const char* interface_name) {
        int ret = set(interface_name, 2, 3, 0, 34);  // QoS data filter.
        if (ret != 0) return ret;
        ret = set(interface_name, 2, 9, 1, 0);       // Output through vendor event.
        if (ret != 0) return ret;
        return set(interface_name, 1, 0, 0, 0);      // Start CSI.
    }

    int stop(const char* interface_name) {
        return set(interface_name, 0, 0, 0, 0);
    }

private:
    struct unl unl_{};
    struct nla_policy csi_ctrl_policy_[kNumCsiCtrlAttrs]{};
    struct nla_policy csi_data_policy_[kNumCsiDataAttrs]{};
    std::vector<Mt76CsiFrame>* active_output_ = nullptr;
};

Mt76CsiApi::Mt76CsiApi() : impl_(new Impl()) {}
Mt76CsiApi::~Mt76CsiApi() { delete impl_; }
int Mt76CsiApi::start(const char* interface_name) { return impl_->start(interface_name); }
int Mt76CsiApi::stop(const char* interface_name) { return impl_->stop(interface_name); }
bool Mt76CsiApi::dump(const char* interface_name, int requested_frames,
                      std::vector<Mt76CsiFrame>* frames) {
    return impl_->dump(interface_name, requested_frames, frames);
}
