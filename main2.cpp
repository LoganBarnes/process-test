// ////////////////////////////////////////////////////////////
// Created on 6/8/18.
// Copyright (c) 2018. All rights reserved.
// ////////////////////////////////////////////////////////////
#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/process.hpp>
#include <boost/process/async.hpp>
#include <iostream>
#include <fstream>

namespace bp = boost::process;
using boost::system::error_code;
using namespace std::chrono_literals;

using Loop = boost::function<void()>;
using Buffer = std::array<char, 500>;

int main()
{
    boost::asio::io_service svc;
    auto on_exit
        = [](int code, std::error_code ec) { std::cout << "Exited " << code << " (" << ec.message() << ")\n"; };

    auto CompressCmd = bp::search_path("python3");

    bp::async_pipe in{svc}, out{svc}, err{svc};
    bp::child process(CompressCmd, "-i", bp::std_in<in, bp::std_out> out, bp::std_err > err, svc, bp::on_exit(on_exit));

    Loop read_loop, write_loop, err_loop;

    Buffer recv_buffer;
    std::size_t total_received = 0;
    read_loop = [&read_loop, &out, &recv_buffer, &total_received] {
        out.async_read_some(boost::asio::buffer(recv_buffer), [&](error_code ec, size_t transferred) {
            std::cout << "ReadLoop: " << ec.message() << " got " << transferred << " bytes\n";
            std::string msg(recv_buffer.begin(), recv_buffer.begin() + transferred);
            std::cerr << msg << std::flush;
            total_received += transferred;
            if (!ec) {
                read_loop(); // continue reading
            }
        });
    };

    Buffer err_buffer;
    std::size_t err_received = 0;
    err_loop = [&err_loop, &err, &err_buffer, &err_received] {
        err.async_read_some(boost::asio::buffer(err_buffer), [&](error_code ec, size_t transferred) {
            std::cerr << "ErrLoop: " << ec.message() << " got " << transferred << " bytes\n";
            std::string msg(err_buffer.begin(), err_buffer.begin() + transferred);
            std::cerr << msg << std::flush;
            err_received += transferred;
            if (!ec)
                err_loop(); // continue reading
        });
    };

    //    std::ifstream ifs("../main2.cpp");
    std::string str;
    std::size_t total_written = 0;
    Buffer send_buffer;
    boost::asio::high_resolution_timer send_delay(svc);
    write_loop = [&write_loop, &in, &str, &send_buffer, &total_written, &send_delay] {
        //        if (!ifs.good())
        //        {
        //            error_code ec;
        //            in.close(ec);
        //            std::cout << "WriteLoop: closed stdin (" << ec.message() << ")\n";
        //            return;
        //        }
        std::getline(std::cin, str);
        str += '\n';
        std::strncpy(send_buffer.data(), str.c_str(), sizeof(send_buffer));

        std::cout << "str: " << str << std::endl;

        boost::asio::async_write(in,
                                 boost::asio::buffer(send_buffer.data(), str.size()),
                                 [&](error_code ec, size_t transferred) {
                                     std::cout << "WriteLoop: " << ec.message() << " sent " << transferred
                                               << " bytes\n";
                                     std::string msg(send_buffer.begin(), send_buffer.begin() + transferred);
                                     std::cerr << msg << std::flush;
                                     total_written += transferred;
                                     if (!ec) {
                                         send_delay.expires_from_now(1s);
                                         send_delay.async_wait([&write_loop](error_code ec) {
                                             std::cout << "WriteLoop: send delay " << ec.message() << "\n";
                                             if (!ec)
                                                 write_loop(); // continue writing
                                         });
                                     }
                                 });
        str = "";
    };

    read_loop(); // async
    err_loop(); // async
    write_loop(); // async

    svc.run(); // Await all async operations

    std::cout << "Process exitcode " << process.exit_code() << ", total_received=" << total_received << "\n";
}
