// ////////////////////////////////////////////////////////////
// Created on 6/10/18.
// Copyright (c) 2018. All rights reserved.
// ////////////////////////////////////////////////////////////
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <string>
#include <iostream>
#include <cctype>
#include <unistd.h>

#define CHECK(expected, msg)                                                                                           \
    {                                                                                                                  \
        if (!(expected)) {                                                                                             \
            std::perror((msg));                                                                                        \
            return EXIT_FAILURE;                                                                                       \
        }                                                                                                              \
    }

int main()
{
    constexpr int READ = 0;
    constexpr int WRITE = 1;
    int python_cin[2], python_cout[2];

    CHECK(pipe(python_cin) != -1, "pipe cin");
    CHECK(pipe(python_cout) != -1, "pipe cout");

    auto pid = fork();
    if (pid == -1) {
        close(python_cin[WRITE]);
        close(python_cin[READ]);
        close(python_cout[WRITE]);
        close(python_cout[READ]);
        return EXIT_FAILURE;
    }

    if (pid == 0) { // CHILD PROCESS
        // reads from 'python_cin' and writes to 'python_cout'
        close(python_cin[WRITE]);
        close(python_cout[READ]);

        // set pipes for python executable
        CHECK(dup2(python_cin[READ], STDIN_FILENO) == STDIN_FILENO, "dup2 stdin");
        CHECK(dup2(python_cout[WRITE], STDOUT_FILENO) == STDOUT_FILENO, "dup2 stdout");
        CHECK(dup2(python_cout[WRITE], STDERR_FILENO) == STDERR_FILENO, "dup2 stderr");

        close(python_cin[READ]);
        close(python_cout[WRITE]);

        std::string program = "python3";
        std::string executable = "/usr/bin/" + program;

        execl(executable.c_str(), program.c_str(), "-i", "-u", nullptr);
        std::cerr << "Failed to execute " << program << std::endl;
        return -1;

    } else { // PARENT PROCESS
        // writes to 'python_cin' and reads from 'python_cout'
        close(python_cin[READ]);
        close(python_cout[WRITE]);

        std::string input;
        char buf;
        std::stringstream output;

        // Exit on EOF, "exit()", or "quit()"
        while (!std::cin.eof() && input.find("exit()") == std::string::npos
               && input.find("quit()") == std::string::npos) {

            input = "";
            bool accept_input = false;

            // Read python output character by character until ">>> " or "... " is reached
            while (read(python_cout[READ], &buf, 1) > 0) {
                output << buf;
                if (buf == '\n') {
                    std::cout << output.str() << std::flush;
                    output.str("");
                } else if (output.str() == ">>> " || output.str() == "... ") {
                    std::cout << output.str() << std::flush;
                    output.str("");
                    accept_input = true;
                    break;
                }
            }

            // Wait for input from the user then send it to the python process
            if (accept_input) {
                std::getline(std::cin, input);
                input += '\n';
                CHECK(write(python_cin[WRITE], input.c_str(), input.size()) >= 0, "write");
            }
        }

        // Exiting. Close pipes and wait for child to finish.
        close(python_cin[WRITE]);
        close(python_cout[READ]);
        wait(nullptr);
        std::cout << "Exiting parent " << std::endl;
    }
    return 0;
}
