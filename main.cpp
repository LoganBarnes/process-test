// ////////////////////////////////////////////////////////////
// Created on 6/7/18.
// Copyright (c) 2018. All rights reserved.
// ////////////////////////////////////////////////////////////
#include <iostream>
#include <vector>
#include <boost/process.hpp>
#include <boost/asio.hpp>
#include <boost/regex.hpp>

namespace bp = boost::process;

namespace {
const boost::regex matcher("\\n|^>>> $|^... $");
}

//#define NORMAL_MODE

class Python
{
public:
    Python()
        : cout_pipe_(ios_)
        , cerr_pipe_(ios_)
        , cin_pipe_(ios_)
        , python_(bp::search_path("python3"), "-i", "-u"
#ifdef NORMAL_MODE
          )
    {
#else
                  ,
                  bp::std_out > cout_pipe_,
                  bp::std_err > cerr_pipe_,
                  bp::std_in < cin_pipe_)
    {
        boost::asio::async_read_until(cout_pipe_,
                                      boost::asio::dynamic_buffer(cout_buffer_),
                                      matcher,
                                      [this](const boost::system::error_code &ec, std::size_t size) {
                                          this->handle_read(ec, size, true, true);
                                      });

        boost::asio::async_read_until(cerr_pipe_,
                                      boost::asio::dynamic_buffer(cerr_buffer_),
                                      matcher,
                                      [this](const boost::system::error_code &ec, std::size_t size) {
                                          this->handle_read(ec, size, false, true);
                                      });
#endif
    }

    void run()
    {
        ios_.run();
        python_.wait();
        std::cout << "python3 exited with error code " << python_.exit_code() << std::endl;
    }

    void handle_read(const boost::system::error_code &ec, std::size_t bytes_transferred, bool is_cout, bool first_time)
    {
        if (ec) {
            std::cerr << ec.message() << std::endl;
        } else {
            assert(bytes_transferred != 0);

            auto &pipe = (is_cout ? cout_pipe_ : cerr_pipe_);
            auto &buffer = (is_cout ? cout_buffer_ : cerr_buffer_);
            auto &stream = (is_cout ? std::cout : std::cerr);

            std::string result;

            result = buffer.substr(0, bytes_transferred);
            buffer.erase(0, bytes_transferred);

            if (result.rfind(">>> ") != std::string::npos || result.rfind("... ") != std::string::npos) {

//                if (first_time) {
//                    boost::system::error_code e;
//                    std::string new_line("\n");
//                    std::size_t size = boost::asio::write(cin_pipe_, boost::asio::buffer(new_line, new_line.size()), e);
//                    if (e) {
//                        std::cerr << e.message() << std::endl;
//                    } else {
//                        assert(new_line.size() == size);
//                    }
//                    first_time = false;
//                } else {

                    stream << result << std::flush;
                    std::thread t([this] {
                        std::getline(std::cin, cin_buffer_);

                        if (!std::cin) {
                            cin_buffer_ = "quit()\n";
                        } else {
                            cin_buffer_ += "\n";
                        }

                        boost::system::error_code e;
                        std::size_t size
                            = boost::asio::write(cin_pipe_,
                                                 boost::asio::buffer(cin_buffer_.data(), cin_buffer_.size()),
                                                 e);
                        if (e) {
                            std::cerr << e.message() << std::endl;
                        } else {
                            assert(size == cin_buffer_.size());
                        }
                    });
                    t.detach();
                    first_time = true;
//                }
            } else {
                stream << result << std::flush;
            }

            boost::asio::async_read_until(pipe,
                                          boost::asio::dynamic_buffer(buffer),
                                          matcher,
                                          [this, is_cout, first_time](const boost::system::error_code &ec,
                                                                      std::size_t size) {
                                              this->handle_read(ec, size, is_cout, first_time);
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

            boost::asio::async_read_until(pipe,
                                          boost::asio::dynamic_buffer(buffer),
                                          matcher,
                                          [this, is_cout](const boost::system::error_code &ec, std::size_t size) {
                                              this->handle_read(ec, size, is_cout, true);
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