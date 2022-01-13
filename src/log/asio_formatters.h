#ifndef __ASIO_FORMATTERS_H
#define __ASIO_FORMATTERS_H

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h" // must be included

#include "clangd_fix.h"
#include <asio.hpp>

template<typename OStream>
OStream &operator<<(OStream &os, const asio::ip::tcp::endpoint& to_log)
{
    fmt::format_to(std::ostream_iterator<char>(os), "{}:{}", to_log.address().to_string(), to_log.port());
    return os;
}

#endif /* __ASIO_FORMATTERS_H */
