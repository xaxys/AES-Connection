#include <iostream>
#include <sys/time.h>
#include <ctime>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include "aes.h"

constexpr size_t MICROSECONDS = 1000000;
constexpr size_t MEGABYTE = 1024 * 1024 * sizeof(uint8_t);
constexpr uint8_t key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                           0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                           0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

static size_t megabytesCount = 100;
static size_t plainLength = megabytesCount * MEGABYTE;

time_t
get_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return MICROSECONDS * tv.tv_sec + tv.tv_usec;
}

int
main(int argc, char* argv[]) {
    if (argc > 1) {
        try {
            megabytesCount = std::stoi(argv[1]);
            plainLength = megabytesCount * MEGABYTE;
        } catch (...) {
            std::cout << "invalid argument " << argv[1] << std::endl;
            std::cout << "usage:   " << argv[0] << " [data_scale]" << std::endl;
            std::cout << "example: " << argv[0] << " 100          // Test Scale 100MB" << std::endl;
            return 1;
        }
    }

    AES<AESKeyLength::AES_256> aes;
    srand(std::time(nullptr));

    std::cout << "[AES benchmark] test scale: " << plainLength / MEGABYTE << "MB" << std::endl;

    auto get_random_plain = [](size_t length) {
        auto plain = std::make_unique<uint8_t[]>(length);
        for (size_t i = 0; i < length; i++) {
            plain.get()[i] = rand() % 26 + 'a';
        }
        return plain;
    };

    {
        std::cout << ">>> start raw encrypt speedtest" << std::endl;
        auto plain = get_random_plain(plainLength);

        auto start = get_microseconds();
        auto out = aes.encrypt_cbc(plain.get(), plainLength, key);
        auto delta = get_microseconds() - start;

        auto speed = (double)megabytesCount / delta * MICROSECONDS;
        printf("raw encrypt speed %.2f MB/s\n", speed);

        std::cout << ">>> start raw decrypt speedtest" << std::endl;
        start = get_microseconds();
        auto decrypt_text = aes.decrypt_cbc(out.get(), plainLength, key);
        delta = get_microseconds() - start;

        speed = (double)megabytesCount / delta * MICROSECONDS;
        printf("raw decrypt speed %.2f MB/s\n", speed);
    }

    {
        std::cout << ">>> start tcp aes connection speedtest" << std::endl;
        using namespace boost::asio::ip;
        auto start = get_microseconds();
        boost::asio::io_service ioc;

        // start server
        boost::asio::spawn(ioc, [&ioc, &aes, &start](boost::asio::yield_context y) {
            try {
                // tcp acceptor
                tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 11037));
                auto tcp_socket = std::make_shared<tcp::socket>(ioc);
                acceptor.async_accept(*tcp_socket, y);
                boost::asio::spawn(y, [&aes, tcp_socket, &start](boost::asio::yield_context y) mutable {
                    boost::asio::streambuf buf;
                    // read plain text
                    try {
                        boost::asio::async_read(*tcp_socket, buf, boost::asio::transfer_all(), y);
                    } catch (boost::system::system_error& e) {
                        if (e.code() != boost::asio::error::eof) {
                            std::cout << e.what() << std::endl;
                        } else {
                            std::cout << "connection closed" << std::endl;
                        }
                    }
                    auto n = buf.size();
                    auto data = aes.decrypt_cbc(buffer_cast<const uint8_t*>(buf.data()), n, key);
                    std::cout << "received " << n << " bytes" << std::endl;
                    auto delta = get_microseconds() - start; // end timing here
                    auto speed = (double)megabytesCount / delta * MICROSECONDS;
                    printf("tcp average speed %.2f MB/s (transmission time included)\n", speed);
                });
            } catch (std::exception& e) {
                std::cerr << "server exception: " << e.what() << std::endl;
            }
        });
        boost::asio::spawn(ioc, [&ioc, &start, &aes, get_random_plain](boost::asio::yield_context y) {
            tcp::socket tcp_socket(ioc);
            try {
                boost::asio::async_connect(
                    tcp_socket, tcp::resolver(ioc).async_resolve(tcp::resolver::query("localhost", "11037"), y), y);
                // generate plain text
                auto plain = get_random_plain(plainLength);
                start = get_microseconds(); // start timing here
                auto out = aes.encrypt_cbc(plain.get(), plainLength, key);
                // send encrypted data
                boost::asio::async_write(tcp_socket, boost::asio::buffer(out.get(), plainLength), y);
                std::cout << "sent " << plainLength << " bytes" << std::endl;
            } catch (std::exception& e) {
                std::cerr << "client exception: " << e.what() << std::endl;
            }
        });
        ioc.run();
    }

    return 0;
}