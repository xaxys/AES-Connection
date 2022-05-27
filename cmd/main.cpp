#include <iostream>
#include <regex>
#include <memory>
#include <span>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include "aes.h"

// This REPL not ready yet
// if you want to see speedtest, please go to ../speedtest
using namespace boost::asio::ip;

int
main() {
    AES<AESKeyLength::AES_256> aes;
    unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                           0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                           0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    boost::asio::io_service ioc;
    auto tcp_socket = std::make_shared<tcp::socket>(ioc);
    boost::asio::spawn(ioc, [&key, &aes, tcp_socket](boost::asio::yield_context y) mutable {
        while (true) {
            boost::asio::streambuf buf;
            try {
                boost::asio::async_read(*tcp_socket, buf, boost::asio::transfer_all(), y);
            } catch (boost::system::system_error &e) {
                if (e.code() != boost::asio::error::eof) {
                    std::cout << e.what() << std::endl;
                } else {
                    std::cout << "connection closed" << std::endl;
                }
            }
            auto data = aes.decrypt_cbc(buffer_cast<const uint8_t*>(buf.data()), buf.size(), key);
            std::cout << "[ --> ] " << data.get() << std::endl;
        }
        std::cout << "listening end" << std::endl;
    });

    std::cout << "AES CBC Security TCP connection REPL" << std::endl;
    boost::asio::spawn(ioc, [&ioc, &key, &aes, tcp_socket](boost::asio::yield_context y) {
        while (true) {
            std::cout << "\r> ";
            std::string command;
            std::getline(std::cin, command);
            if (command == "exit") {
                break;
            } else if (std::smatch result; std::regex_match(command, result, std::regex(R"(\s*listen\s+(\d+)\s*)", std::regex_constants::icase))) {
                auto port = std::stoi(result[1]);
                std::cout << "listening on port " << port << std::endl;
                boost::asio::spawn(y, [&ioc, &key, &aes, tcp_socket, port](boost::asio::yield_context y) {
                    try {
                        tcp::acceptor acceptor_server(ioc, tcp::endpoint(tcp::v4(), port));
                        acceptor_server.async_accept(*tcp_socket, y);
                        std::cout << "server done\n";
                    }
                    catch (std::exception &e) {
                        std::cerr << "server exception: " << e.what() << std::endl;
                    }
                });
            } else if (std::smatch result; std::regex_match(command, result, std::regex(R"(\s*connect\s+((\b25[0-5]|\b2[0-4][0-9]|\b[01]?[0-9][0-9]?)(\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)){3}|localhost)\s+(\d+)\s*)", std::regex_constants::icase))) {
                auto ip = result[1];
                auto port = result[5];
                std::cout << "connecting to " << ip << ":" << port << std::endl;
                boost::asio::spawn(y, [&ioc, tcp_socket, ip, port](boost::asio::yield_context y) {
                    try {
                        boost::asio::async_connect(*tcp_socket, tcp::resolver(ioc).async_resolve(tcp::resolver::query(ip, port), y), y);
                        boost::asio::async_write(*tcp_socket, boost::asio::buffer("connect from client"), y);
                    } catch (std::exception &e) {
                        std::cerr << "client exception: " << e.what() << std::endl;
                    }
                });
            } else {
                std::cout << "[ <-- ] " << command << std::endl;
                if (command.size() % aes.block_size != 0) {
                    command.resize(command.size() + aes.block_size - command.size() % aes.block_size);
                }
                auto len = command.size();
                auto data = aes.encrypt_cbc(reinterpret_cast<uint8_t*>(command.data()), len, key);
                std::cout << "[ DBUG ] " << data.get() << std::endl;
                boost::asio::spawn(y, [len, tcp_socket, command, data = std::move(data)](boost::asio::yield_context y) {
                    try {
                        std::cout << "[ DBUG ] Write TCP Start" << std::endl;
                        boost::asio::async_write(*tcp_socket, boost::asio::buffer(data.get(), len), y);
                        std::cout << "[ DBUG ] Write TCP Done" << std::endl;
                    } catch (std::exception &e) {
                        std::cerr << "client exception: " << e.what() << std::endl;
                    }
                });
            }
        }
    });
    ioc.run();
    return 0;
}
