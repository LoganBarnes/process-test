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
        , python_(bp::search_path("python3"),
                  "-i",
                  bp::std_out > cout_pipe_,
                  bp::std_err > cerr_pipe_,
                  bp::std_in < cin_to_python_)
    {
        //        std::string s = "\n";
        //        boost::system::error_code e;
        //
        //        for (int i = 0 ;i < 5; ++i) {
        //            auto sent = boost::asio::write(cin_pipe_, boost::asio::buffer(s), e);
        //            std::cout << "Sent: " << sent << std::endl;
        //
        //            if (e) {
        //                std::cerr << "ERROR: " << e.message() << std::endl;
        //            }
        //        }
        //        cin_to_python_ << bp::opstream::eofbit;
        //        python_.
        //        python_.get_stdin();

        boost::asio::async_read_until(cout_pipe_,
                                      cout_buffer_,
                                      ' ',
                                      [this](const boost::system::error_code &ec, std::size_t size) {
                                          this->handle_read(ec, size, true);
                                      });

        //        boost::asio::async_read_until(cerr_pipe_,
        //                                      cerr_buffer_,
        //                                      ' ',
        //                                      [this](const boost::system::error_code &ec, std::size_t size) {
        //                                          this->handle_read(ec, size, false);
        //                                      });
    }

    void run()
    {
        ios_.run();
        python_.wait();
        std::cout << "python3 exited with error code " << python_.exit_code() << std::endl;
    }

    void handle_read(const boost::system::error_code &ec, std::size_t size, bool is_cout)
    {
        auto &pipe = (is_cout ? cout_pipe_ : cerr_pipe_);
        auto &buffer = (is_cout ? cout_buffer_ : cerr_buffer_);
        auto &stream = (is_cout ? std::cout : std::cerr);

        if (ec) {
            std::cerr << ec.message() << std::endl;
        } else {
            std::string result;

            if (size != 0) {
                std::istream is(&buffer);
                std::getline(is, result, ' ');
                result += ' ';
                stream << result << std::flush;
            }

            if ((result.find(">>>") != std::string::npos || result.find("...") != std::string::npos)) {
                std::string input_to_python;
                std::getline(std::cin, input_to_python);

                cin_to_python_ << input_to_python << '\n' << std::flush;
            }
            boost::asio::async_read_until(pipe,
                                          buffer,
                                          ' ',
                                          [this, is_cout](const boost::system::error_code &ec, std::size_t size) {
                                              this->handle_read(ec, size, is_cout);
                                          });
        }
    }

    //    void handle_write(const boost::system::error_code &ec, std::size_t size)
    //    {
    //        if (ec) {
    //            std::cerr << ec.message() << std::endl;
    //        } else {
    //            std::cout << input_to_python_.size() << " : " << size << std::endl;
    //            assert(input_to_python_.size() == size);
    //            input_to_python_ = "";
    //
    //            boost::asio::async_read(pipe_from_python_,
    //                                    boost::asio::buffer(output_from_python_),
    //                                    [this](const boost::system::error_code &ec, std::size_t size) {
    //                                        this->handle_read(ec, size);
    //                                    });
    //        }
    //    }

private:
    boost::asio::io_service ios_;

    bp::async_pipe cout_pipe_;
    bp::async_pipe cerr_pipe_;
    bp::opstream cin_to_python_;

    bp::child python_;

    boost::asio::streambuf cout_buffer_;
    boost::asio::streambuf cerr_buffer_;
};

// expecting ">>>" or "..."
bool ready_for_input(const std::string &buf)
{
    return buf.size() >= 3 && buf[0] == buf[1] && buf[1] == buf[2] && (buf[0] == '>' || buf[0] == '.');
}

template <typename Stream>
void handle_read(const boost::system::error_code &ec,
                 std::size_t size,
                 bp::async_pipe &pipe,
                 boost::asio::streambuf &buf,
                 Stream &stream)
{
    if (ec) {
        std::cerr << ec.message() << std::endl;
    } else {
        std::string result;

        if (size != 0) {
            std::istream is(&buf);
            std::getline(is, result, ' ');
            result += ' ';
            stream << result << std::flush;
        }

        if (result.find_first_of(">>>") != std::string::npos) {
            std::string input_to_python;
            std::getline(std::cin, input_to_python);
            input_to_python += '\n';
            //            boost::asio::async_write(pipe_to_python_,
            //                                     boost::asio::buffer(input_to_python_),
            //                                     [this](const boost::system::error_code &ec, std::size_t size) {
            //                                         this->handle_write(ec, size);
            //                                     });
        } else {
            boost::asio::async_read_until(pipe, buf, ' ', [&](const boost::system::error_code &ec, std::size_t size) {
                handle_read(ec, size, pipe, buf, stream);
            });
        }
    }
};

//void handle_write(const boost::system::error_code &ec, std::size_t size)
//{
//    if (ec) {
//        std::cerr << ec.message() << std::endl;
//    } else {
//        input_to_python_ = "";
//
//        boost::asio::async_read(pipe_from_python_,
//                                boost::asio::buffer(output_from_python_),
//                                [this](const boost::system::error_code &ec, std::size_t size) {
//                                    this->handle_read(ec, size);
//                                });
//    }
//}

int main()
{
#if 1
    Python python;
    python.run();
#else
    boost::asio::io_service ios;

    bp::async_pipe stdout_from_python(ios);
    bp::async_pipe stderr_from_python(ios);
    boost::asio::streambuf cout_from_python;
    boost::asio::streambuf cerr_from_python;

    bp::child python(bp::search_path("python3"), bp::std_out > stdout_from_python, bp::std_err > stderr_from_python);

    boost::asio::async_read_until(stdout_from_python,
                                  cout_from_python,
                                  ' ',
                                  [&](const boost::system::error_code &ec, std::size_t size) {
                                      handle_read(ec, size, stdout_from_python, cout_from_python, std::cout);
                                  });

    boost::asio::async_read_until(stderr_from_python,
                                  cerr_from_python,
                                  ' ',
                                  [&](const boost::system::error_code &ec, std::size_t size) {
                                      handle_read(ec, size, stderr_from_python, cerr_from_python, std::cerr);
                                  });

    ios.run();
    python.wait();
    std::cout << "python3 exited with error code " << python.exit_code() << std::endl;

#endif
    return 0;
}