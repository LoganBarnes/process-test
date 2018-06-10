// ////////////////////////////////////////////////////////////
// Created on 6/8/18.
// Copyright (c) 2018. All rights reserved.
// ////////////////////////////////////////////////////////////
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <algorithm>
#include <cctype>
#include <locale>
#include <thread>

//#define THREAD

// trim from start (in place)
static inline void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s)
{
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s)
{
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s)
{
    trim(s);
    return s;
}

using namespace std;

bool valid_input(const std::string &input)
{
    bool valid = !std::cin.eof();
    valid &= trim_copy(input).find("exit()") == std::string::npos;
    valid &= trim_copy(input).find("quit()") == std::string::npos;
    return valid;
}

int main()
{
    constexpr int READ = 0;
    constexpr int WRITE = 1;
    int python_cin[2], python_cout[2], python_cerr[2]; //, bytes_read;

    if (pipe(python_cin) != 0) {
        throw std::runtime_error("Failed to pipe cin :(");
    }

    if (pipe(python_cout) != 0) {
        throw std::runtime_error("Failed to pipe cout :(");
    }

    if (pipe(python_cerr) != 0) {
        throw std::runtime_error("Failed to pipe cerr :(");
    }

    auto pid = fork();
    if (pid == -1) {
        close(python_cin[WRITE]);
        close(python_cin[READ]);
        close(python_cout[WRITE]);
        close(python_cout[READ]);
        close(python_cerr[WRITE]);
        close(python_cerr[READ]);
        throw std::runtime_error("Error on fork :(");
    }

    if (pid == 0) {
        std::cout << "In child process!" << std::endl;

        close(python_cin[WRITE]);
        close(python_cout[READ]);
        close(python_cerr[READ]);

        if (dup2(python_cin[READ], STDIN_FILENO) == -1) {
            throw std::runtime_error("Failed to duplicate cin pipe");
        }

        if (dup2(python_cout[WRITE], STDOUT_FILENO) == -1) {
            throw std::runtime_error("Failed to duplicate cout pipe");
        }

        if (dup2(python_cerr[WRITE], STDERR_FILENO) == -1) {
            throw std::runtime_error("Failed to duplicate cerr pipe");
        }

        close(python_cin[READ]);
        close(python_cout[WRITE]);
        close(python_cerr[WRITE]);

        execl("/usr/bin/python3", "python3", "-i", nullptr);
        std::cerr << "Failed to execute python3 " << std::endl;
        return -1;

    } else {
        std::cout << "In parent process!" << std::endl;

        // parent writes to 'python_cin' and reads from 'python_cout'
        close(python_cin[READ]);
        close(python_cout[WRITE]);
        close(python_cerr[WRITE]);

        if (fcntl(python_cout[READ], F_SETFL, fcntl(python_cout[READ], F_GETFL) | O_NONBLOCK) > 0) {
            std::perror("cout fcntl");
            return EXIT_FAILURE;
        }
        if (fcntl(python_cerr[READ], F_SETFL, fcntl(python_cerr[READ], F_GETFL) | O_NONBLOCK) > 0) {
            std::perror("cerr fcntl");
            return EXIT_FAILURE;
        }

        std::string input;
        char buf;
        std::stringstream ss;
        while (valid_input(input)) {

            input = "";
            bool accept_input = false;

            while (read(python_cout[READ], &buf, 1) > 0) {
                ss << buf;
                if (buf == '\n') {
                    std::cout << ss.str() << std::flush;
                    ss.str("");
                } else if (ss.str() == ">>> " || ss.str() == "... ") {
                    std::cout << ss.str() << std::flush;
                    ss.str("");
                    accept_input = true;
                    break;
                }
            }

            while (read(python_cerr[READ], &buf, 1) > 0) {
                ss << buf;
                if (buf == '\n') {
                    std::cerr << ss.str() << std::flush;
                    ss.str("");
                } else if (ss.str() == ">>> " || ss.str() == "... ") {
                    std::cerr << ss.str() << std::flush;
                    ss.str("");
                    accept_input = true;
                    break;
                }
            }

            if (accept_input) {
#ifdef THREAD
                std::thread t([&] {
#endif
                    std::getline(std::cin, input);
                    input += '\n';
                    write(python_cin[WRITE], input.c_str(), input.size());
#ifdef THREAD
                });
                t.detach();
#endif
            }
        }

        close(python_cin[WRITE]);
        close(python_cout[READ]);
        close(python_cerr[READ]);
        wait(nullptr);
        std::cout << "Exiting parent " << std::endl;
    }

    //    int pipefd[2];
    //    pid_t cpid;
    //    char buf;
    //
    //    if (pipe(pipefd) == -1) {
    //        std::cerr << "pipe failed" << std::endl;
    //        return EXIT_FAILURE;
    //    }
    //
    //    cpid = fork();
    //    if (cpid == -1) {
    //        std::cerr << "fork failed" << std::endl;
    //        return EXIT_FAILURE;
    //    }
    //
    //    if (cpid == 0) { //Child process
    //        do {
    //            read(pipefd[0], &buf, 1);
    //            cout << buf << " command received.\n";
    //        } while (buf != 't');
    //        if (buf == 't') {
    //            close(pipefd[0]);
    //            close(pipefd[1]);
    //            _exit(EXIT_SUCCESS);
    //        }
    //    } else { //Parent process
    //        do {
    //            cout << "Enter a command (q, u, p, t): ";
    //            cin >> buf;
    //            write(pipefd[1], &buf, 1);
    //        } while (buf != 't');
    //        if (buf == 't') {
    //            cout << "\nExiting program...";
    //            close(pipefd[1]);
    //            close(pipefd[0]);
    //            exit(EXIT_SUCCESS);
    //        }
    //    }
}
