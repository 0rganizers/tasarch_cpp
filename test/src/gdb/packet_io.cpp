#include <array>
#include <ostream>
#include <stdexcept>
#include "gdb/common.h"
#include "gdb/packet_io.h"
#include "tcp_server_client_test.h"
#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>
#include <ut/ut.hpp>
#include "config/config.h"
#include "async_test.h"
#include <asio/basic_waitable_timer.hpp>
#include "gdb/buffer.h"

namespace ut = boost::ut;
namespace gdb = tasarch::test::gdb;

ut::suite packet_io_tests = []{
    using namespace ut;
    using asio::ip::tcp;
    using namespace tasarch::gdb;

    auto config_val = tasarch::config::parse_toml("logging.test.gdb.level = 'trace'\nlogging.gdb.level = 'trace'");
    tasarch::config::conf()->load_from(config_val);

    std::string mean_data = R"(This text file is a test input for GDB's file transfer commands.  It
contains some characters which need to be escaped in remote protocol
packets, like "*" and "}" and "$" and "#".  Actually, it contains
a good sampling of printable characters:

!"#$%&'()*+,-./0123456789:;<=>?@
ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`
abcdefghijklmnopqrstuvwxyz{|}~

!"#$%&'()*+,-./0123456789:;<=>?@
ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`
abcdefghijklmnopqrstuvwxyz{|}~
)";

    std::string file_mean_data = "F1b9;" + mean_data;
    buffer mean_buffer(gdb_packet_buffer_size);

    auto rst_buf = [&]{
        mean_buffer.reset();
        mean_buffer.append_buf(file_mean_data);
    };

    std::string ack = "+";
    std::string nack = "-";

    std::array<u8, tasarch::gdb::gdb_packet_buffer_size> packet_buf{};

    // stolen from actual gdb output :)
    std::string encoded_mean_data = "\x24\x46\x31\x62\x39\x3b\x54\x68\x69\x73\x20\x74\x65\x78\x74\x20\x66\x69\x6c\x65\x20\x69\x73\x20\x61\x20\x74\x65\x73\x74\x20\x69\x6e\x70\x75\x74\x20\x66\x6f\x72\x20\x47\x44\x42\x27\x73\x20\x66\x69\x6c\x65\x20\x74\x72\x61\x6e\x73\x66\x65\x72\x20\x63\x6f\x6d\x6d\x61\x6e\x64\x73\x2e\x20\x20\x49\x74\x0a\x63\x6f\x6e\x74\x61\x69\x6e\x73\x20\x73\x6f\x6d\x65\x20\x63\x68\x61\x72\x61\x63\x74\x65\x72\x73\x20\x77\x68\x69\x63\x68\x20\x6e\x65\x65\x64\x20\x74\x6f\x20\x62\x65\x20\x65\x73\x63\x61\x70\x65\x64\x20\x69\x6e\x20\x72\x65\x6d\x6f\x74\x65\x20\x70\x72\x6f\x74\x6f\x63\x6f\x6c\x0a\x70\x61\x63\x6b\x65\x74\x73\x2c\x20\x6c\x69\x6b\x65\x20\x22\x7d\x0a\x22\x20\x61\x6e\x64\x20\x22\x7d\x5d\x22\x20\x61\x6e\x64\x20\x22\x7d\x04\x22\x20\x61\x6e\x64\x20\x22\x7d\x03\x22\x2e\x20\x20\x41\x63\x74\x75\x61\x6c\x6c\x79\x2c\x20\x69\x74\x20\x63\x6f\x6e\x74\x61\x69\x6e\x73\x0a\x61\x20\x67\x6f\x6f\x64\x20\x73\x61\x6d\x70\x6c\x69\x6e\x67\x20\x6f\x66\x20\x70\x72\x69\x6e\x74\x61\x62\x6c\x65\x20\x63\x68\x61\x72\x61\x63\x74\x65\x72\x73\x3a\x0a\x0a\x21\x22\x7d\x03\x7d\x04\x25\x26\x27\x28\x29\x7d\x0a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f\x40\x0a\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x0a\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x5d\x7e\x0a\x0a\x21\x22\x7d\x03\x7d\x04\x25\x26\x27\x28\x29\x7d\x0a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f\x40\x0a\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x0a\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x5d\x7e\x0a\x23\x35\x30";//"$" + mean_data + "#" + tasarch::gdb::encode_hex(checksum >> 4) + tasarch::gdb::encode_hex(checksum);
    // the output for a request is slightly different, since * is not encoded!
    std::string encoded_mean_data_req = "\x24\x46\x31\x62\x39\x3b\x54\x68\x69\x73\x20\x74\x65\x78\x74\x20\x66\x69\x6c\x65\x20\x69\x73\x20\x61\x20\x74\x65\x73\x74\x20\x69\x6e\x70\x75\x74\x20\x66\x6f\x72\x20\x47\x44\x42\x27\x73\x20\x66\x69\x6c\x65\x20\x74\x72\x61\x6e\x73\x66\x65\x72\x20\x63\x6f\x6d\x6d\x61\x6e\x64\x73\x2e\x20\x20\x49\x74\x0a\x63\x6f\x6e\x74\x61\x69\x6e\x73\x20\x73\x6f\x6d\x65\x20\x63\x68\x61\x72\x61\x63\x74\x65\x72\x73\x20\x77\x68\x69\x63\x68\x20\x6e\x65\x65\x64\x20\x74\x6f\x20\x62\x65\x20\x65\x73\x63\x61\x70\x65\x64\x20\x69\x6e\x20\x72\x65\x6d\x6f\x74\x65\x20\x70\x72\x6f\x74\x6f\x63\x6f\x6c\x0a\x70\x61\x63\x6b\x65\x74\x73\x2c\x20\x6c\x69\x6b\x65\x20\x22\x2a\x22\x20\x61\x6e\x64\x20\x22\x7d\x5d\x22\x20\x61\x6e\x64\x20\x22\x7d\x04\x22\x20\x61\x6e\x64\x20\x22\x7d\x03\x22\x2e\x20\x20\x41\x63\x74\x75\x61\x6c\x6c\x79\x2c\x20\x69\x74\x20\x63\x6f\x6e\x74\x61\x69\x6e\x73\x0a\x61\x20\x67\x6f\x6f\x64\x20\x73\x61\x6d\x70\x6c\x69\x6e\x67\x20\x6f\x66\x20\x70\x72\x69\x6e\x74\x61\x62\x6c\x65\x20\x63\x68\x61\x72\x61\x63\x74\x65\x72\x73\x3a\x0a\x0a\x21\x22\x7d\x03\x7d\x04\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f\x40\x0a\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x0a\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x5d\x7e\x0a\x0a\x21\x22\x7d\x03\x7d\x04\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f\x40\x0a\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x0a\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x5d\x7e\x0a\x23\x35\x30";

    auto recv_string = [](tcp::socket& sock) -> asio::awaitable<std::string>{
        std::string inp;
        inp.resize(tasarch::gdb::gdb_packet_buffer_size);
        size_t size = co_await sock.async_receive(asio::buffer(inp), asio::use_awaitable);
        inp.resize(size);
        co_return inp;
    };

    "simple packets test"_test = gdb::create_socket_test([&](tcp::socket remote, tcp::socket local) -> asio::awaitable<void>{
        tasarch::gdb::PacketIO io(remote);
        // for simple tests, no ack for now
        io.set_no_ack();
        rst_buf();
        co_await io.send_packet(mean_buffer);
        std::string recvd_str = co_await recv_string(local);
        expect(recvd_str == encoded_mean_data);

        co_await local.async_send(asio::buffer(encoded_mean_data), asio::use_awaitable);
        buffer recv_buf(gdb_packet_buffer_size);
        co_await io.receive_packet(recv_buf);
        std::string recvd = recv_buf.get_str();
        expect(recvd == file_mean_data);
        std::ofstream recvd_file;
        recvd_file.open("recvd.txt", std::ios::binary | std::ios::out | std::ios::trunc);
        recvd_file << recvd;
        recvd_file.close();

        std::ofstream file_mean_f;
        file_mean_f.open("file_mean_f.txt", std::ios::binary | std::ios::out | std::ios::trunc);
        file_mean_f << file_mean_data;
        file_mean_f.close();
    });

    "simple send with ack test"_test = gdb::create_dual_socket_test([&](tcp::socket remote) -> asio::awaitable<void>{
        tasarch::gdb::PacketIO io(remote);
        rst_buf();
        bool did_break = co_await io.send_packet(mean_buffer);
        expect(!did_break) << "did not expect a break!";
    }, [&](tcp::socket local) -> asio::awaitable<void>{
        // we should have received already
        std::string recvd_str = co_await recv_string(local);
        expect(recvd_str == encoded_mean_data);
        // respond with ack
        co_await local.async_send(asio::buffer(ack), asio::use_awaitable);
    });

    "simple recv with ack test"_test = gdb::create_dual_socket_test([&](tcp::socket remote) -> asio::awaitable<void>{
        tasarch::gdb::PacketIO io(remote);

        buffer recv_buf(gdb_packet_buffer_size);
        bool did_break = co_await io.receive_packet(recv_buf);
        expect(!did_break) << "did not expect a break!";
        
        expect(recv_buf.get_str() == file_mean_data);

    }, [&](tcp::socket local) -> asio::awaitable<void>{
        co_await local.async_send(asio::buffer(encoded_mean_data), asio::use_awaitable);
        // check for ack
        std::string recvd_str = co_await recv_string(local);
        expect(recvd_str == ack);
    });

    "simple send with no ack test"_test = gdb::create_dual_socket_test([&](tcp::socket remote) -> asio::awaitable<void>{
        tasarch::gdb::PacketIO io(remote);
        rst_buf();
        co_await io.send_packet(mean_buffer);
    }, [&](tcp::socket local) -> asio::awaitable<void>{
        // we should have received already
        std::string recvd_str = co_await recv_string(local);
        expect(recvd_str == encoded_mean_data);
        // respond with ack
        co_await local.async_send(asio::buffer(nack), asio::use_awaitable);
        // we should receive again received already
        std::string recvd_str2 = co_await recv_string(local);
        expect(recvd_str2 == encoded_mean_data);
        // respond with ack
        co_await local.async_send(asio::buffer(ack), asio::use_awaitable);
    });

    "simple recv with no ack test"_test = gdb::create_dual_socket_test([&](tcp::socket remote) -> asio::awaitable<void>{
        tasarch::gdb::PacketIO io(remote);

        buffer recv_buf(gdb_packet_buffer_size);
        co_await io.receive_packet(recv_buf);
        std::string rcvd = recv_buf.get_str();
        expect(rcvd == file_mean_data);
    }, [&](tcp::socket local) -> asio::awaitable<void>{
        std::string wrong_cksum = encoded_mean_data.substr(0, encoded_mean_data.size() - 1) + "f";
        co_await local.async_send(asio::buffer(wrong_cksum), asio::use_awaitable);
        // check for ack
        std::string recvd_str = co_await recv_string(local);
        expect(recvd_str == nack);

        // now send correct one
        co_await local.async_send(asio::buffer(encoded_mean_data), asio::use_awaitable);
        // check for ack
        std::string recvd_str2 = co_await recv_string(local);
        expect(recvd_str2 == ack);
    });

    "check error on too large packet"_test = gdb::create_dual_socket_test([&](tcp::socket remote) -> asio::awaitable<void>{
        tasarch::gdb::PacketIO io(remote);
        // this buf should be way too small!
        buffer recv_buf(10);
        throws_async_ex(io.receive_packet(recv_buf), tasarch::gdb::buffer_too_small);
    }, [&](tcp::socket local) -> asio::awaitable<void>{
        co_await local.async_send(asio::buffer(encoded_mean_data), asio::use_awaitable);
    });

    "break character on read"_test = gdb::create_dual_socket_test([&](tcp::socket remote) -> asio::awaitable<void>{
        tasarch::gdb::PacketIO io(remote);
        buffer recv_buf(gdb_packet_buffer_size);
        bool did_break = co_await io.receive_packet(recv_buf);
        expect(did_break) << "expected a break character!";
    }, [&](tcp::socket local) -> asio::awaitable<void>{
        co_await local.async_send(asio::buffer("\x03"), asio::use_awaitable);
    });

    "break character on write"_test = gdb::create_dual_socket_test([&](tcp::socket remote) -> asio::awaitable<void>{
        tasarch::gdb::PacketIO io(remote);
        rst_buf();
        bool did_break = co_await io.send_packet(mean_buffer);
        expect(did_break) << "expected a break character!";
    }, [&](tcp::socket local) -> asio::awaitable<void>{
        // we should have received already
        std::string recvd_str = co_await recv_string(local);
        expect(recvd_str == encoded_mean_data);
        // respond with break, then ack
        co_await local.async_send(asio::buffer("\x03"+ack), asio::use_awaitable);
    });

    "timeout on local close"_test = gdb::create_dual_socket_test([&](tcp::socket remote) -> asio::awaitable<void>{
        tasarch::gdb::PacketIO io(remote);
        // shorter timeout, so testing doesnt take as long!
        io.timeout = 500ms;
        buffer recv_buf(gdb_packet_buffer_size);
        throws_async_ex(io.receive_packet(recv_buf), tasarch::gdb::timed_out);
    }, [&](tcp::socket local) -> asio::awaitable<void>{
        local.close();
        co_return;
    });
};