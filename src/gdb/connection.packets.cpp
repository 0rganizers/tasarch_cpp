#include <stdexcept>
#include <utility>
#include <fmt/core.h>
#include "connection.h"
#include "easter_eggs.h"
#include "gdb/gdb_err.h"

namespace tasarch::gdb {
    void connection::handle_read_mem(size_t address, size_t len)
    {
        logger->info("reading memory from 0x{:x}", address);
        if (internal_mem::has_addr(address)) {
            auto data = internal_mem::read_data(address, len);
            std::string s(reinterpret_cast<char*>(data.data()), data.size());
            this->append_hex(s);
        } else {
            this->append_hex("a");
        }
    }

    void connection::handle_write_mem(size_t address, size_t len, std::vector<u8> data)
    {
        logger->info("writing memory to 0x{:x}", address);
        if (internal_mem::has_addr(address)) {
            internal_mem::write_data(address, len, data);
        } else {
        }
        this->append_ok();
    }

    void connection::handle_query(query_type type)
    {
        std::string name;
        auto it = query_handlers.end();
        bool did_find = false;
        u8 curr = 0;
        while (packet_buf.read_size() > 0) {
            if (it != query_handlers.end() && it->second.separator == '\0') {
                did_find = true;
                break;
            }

            curr = packet_buf.get_byte();
            
            if (it != query_handlers.end() && it->second.separator == curr) {
                did_find = true;
                break;
            }

            name += curr;
            it = query_handlers.find(name);
        }

        if (did_find) {
            if (type == get_val) {
                if (it->second.get_handler.has_value()) {
                    it->second.get_handler.value()();
                    return;
                }
                throw std::runtime_error(fmt::format("query {} is not gettable!", name));
            }
            if (it->second.set_handler.has_value()) {
                it->second.set_handler.value()();
                return;
            }
            throw std::runtime_error(fmt::format("query {} is not settable", name));
        }
        throw unknown_request(fmt::format("query {} not recognized!", name));
    }

    void connection::handle_supported(std::vector<feature> features)
    {
        std::string feats;
        for (const auto& feat : features) {
            feats += feat.name;
            feats += "; ";
        }
        this->remote_features = features;
        logger->info("Supported features: {}", feats);

        std::vector<feature> response = this->our_features;

        for (auto& pair : query_handlers) {
            if (pair.second.advertise) {
                std::string feat_name;
                if (pair.second.type() == get_val) {
                    feat_name += "q";
                } else {
                    feat_name += "Q";
                }
                feat_name += pair.second.name;
                feat_name += "+";
            }
        }

        ArrayCoder<FeatureCoder>::encode_to(response, this->resp_buf);
    }

    void connection::handle_file_reply(int64_t retcode, std::optional<size_t> errorno, std::optional<std::string> ctrlc, std::optional<std::string> attachement)
    {
        if (!this->io_resp.empty()) {
            auto resume = std::move(this->io_resp.front());
            this->io_resp.pop();
            bool did_break = false;
            if (ctrlc.has_value() && ctrlc.value() == "C") {
                did_break = true;
            }
            resume(remote_io_reply{retcode, errorno, did_break, std::move(attachement)});
            this->wakeup_request();
        } else {
            throw std::runtime_error("Got file reply, but no one waiting on it! What??");
        }
    }
} // namespace tasarch::gdb