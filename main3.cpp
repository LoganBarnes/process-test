// ////////////////////////////////////////////////////////////
// Created on 6/7/18.
// Copyright (c) 2018. All rights reserved.
// ////////////////////////////////////////////////////////////
#include <iostream>
#include <vector>
#include <boost/process.hpp>
#include <boost/asio.hpp>

namespace bp = boost::process;

class Python
{
public:
    Python()
        : cout_pipe_(ios_)
        , cerr_pipe_(ios_)
        , cin_pipe_(ios_)
        , python_(bp::search_path("python3"),
                  "-i",
                  bp::std_out > cout_pipe_,
                  bp::std_err > cerr_pipe_,
                  bp::std_in < cin_pipe_)
    {
        boost::asio::async_read_until(cout_pipe_,
                                      boost::asio::dynamic_buffer(cout_buffer_),
                                      ">>> ",
                                      [this](const boost::system::error_code &ec, std::size_t size) {
                                          this->handle_read(ec, size, true);
                                      });

        boost::asio::async_read_until(cerr_pipe_,
                                      boost::asio::dynamic_buffer(cerr_buffer_),
                                      ">>> ",
                                      [this](const boost::system::error_code &ec, std::size_t size) {
                                          this->handle_read(ec, size, false);
                                      });
    }

    void run()
    {
        ios_.run();
        python_.wait();
        std::cout << "python3 exited with error code " << python_.exit_code() << std::endl;
    }

    void handle_read(const boost::system::error_code &ec, std::size_t bytes_transferred, bool is_cout)
    {
        if (ec) {
            std::cerr << ec.message() << std::endl;
        } else {
            assert(bytes_transferred != 0);

            auto &buffer = (is_cout ? cout_buffer_ : cerr_buffer_);
            auto &stream = (is_cout ? std::cout : std::cerr);

            std::string result;
            stream << "successfully read " << bytes_transferred << " bytes" << std::endl;

            result = buffer.substr(0, bytes_transferred);
            buffer.erase(0, bytes_transferred);
            stream << result << std::flush;

            std::getline(std::cin, cin_buffer_);

            if (!std::cin) {
                cin_buffer_ = "quit()\n";
            } else {
                cin_buffer_ += '\n';
            }

            boost::asio::async_write(cin_pipe_,
                                     boost::asio::buffer(cin_buffer_.data(), cin_buffer_.size()),
                                     [this, is_cout](const boost::system::error_code &ec, std::size_t size) {
                                         this->handle_write(ec, size, is_cout);
                                     });
        }
    }

    void handle_write(const boost::system::error_code &ec, std::size_t bytes_transferred, bool is_cout)
    {
        if (ec) {
            std::cerr << ec.message() << std::endl;
        } else {
            auto &pipe = (is_cout ? cout_pipe_ : cerr_pipe_);
            auto &buffer = (is_cout ? cout_buffer_ : cerr_buffer_);
            auto &stream = (is_cout ? std::cout : std::cerr);

            stream << "successfully wrote " << cin_buffer_ << std::flush;

            boost::asio::async_read_until(pipe,
                                          boost::asio::dynamic_buffer(buffer),
                                          ">>> ",
                                          [this, is_cout](const boost::system::error_code &ec, std::size_t size) {
                                              this->handle_read(ec, size, is_cout);
                                          });
        }
    }

private:
    boost::asio::io_service ios_;

    bp::async_pipe cout_pipe_;
    bp::async_pipe cerr_pipe_;
    bp::async_pipe cin_pipe_;

    bp::child python_;

    std::string cout_buffer_;
    std::string cerr_buffer_;
    std::string cin_buffer_;
};

int main()
{
    Python python;
    python.run();

    return 0;
}