#include <iostream>
#include <vector>
#include <boost/process.hpp>
#include <boost/asio.hpp>

namespace bp = boost::process;

class Python
{
public:
    Python() : pipe_from_python_(ios_), pipe_to_python_(ios_)
    {
        python_ = bp::child(bp::search_path("python3"), bp::std_out > pipe_from_python_, bp::std_in < pipe_to_python_);

        input_to_python_ = "";
        boost::asio::async_read(pipe_from_python_,
                                boost::asio::buffer(output_from_python_),
                                [this](const boost::system::error_code &ec, std::size_t size) {
                                    this->handle_read(ec, size);
                                });
    }

    void run()
    {
        ios_.run();
        python_.wait();
        std::cout << "python3 exited with error code " << python_.exit_code() << std::endl;
    }

    void handle_read(const boost::system::error_code &ec, std::size_t size)
    {
        if (ec) {
            std::cerr << ec.message() << std::endl;
        } else {
            std::cout << output_from_python_.size() << " : " << size << std::endl;
            assert(output_from_python_.size() == size);

            std::cout << output_from_python_.data() << std::endl;

            //            std::cin >> input_to_python_;
            //            boost::asio::async_write(pipe_to_python_,
            //                                     boost::asio::buffer(input_to_python_),
            //                                     [this](const boost::system::error_code &ec, std::size_t size) {
            //                                         this->handle_write(ec, size);
            //                                     });
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

    bp::async_pipe pipe_from_python_;
    bp::async_pipe pipe_to_python_;

    bp::child python_;

    std::vector<char> output_from_python_;
    std::string input_to_python_;
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
//        std::cout << buf.size() << " : " << size << std::endl;
        //        assert(buf.size() == size);

        std::string result;

        if (size != 0) {
            std::istream is(&buf);
            std::getline(is, result, ' ');
            result += ' ';
            stream << result << std::flush;
        }

        if (ready_for_input(result)) {
            std::cout << "do input here" << std::endl;
        } else {
            //            boost::asio::async_read(pipe,
            //                                    buf,
            //                                    [&](const boost::system::error_code &ec, std::size_t size) {
            //                                        handle_read(ec, size, pipe, buf);
            //                                    });
            boost::asio::async_read_until(pipe, buf, ' ', [&](const boost::system::error_code &ec, std::size_t size) {
                handle_read(ec, size, pipe, buf, stream);
            });
        }
    }
};

int main()
{
#if 0
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