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
#include "PythonConsole.hpp"

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

        {
            impy::PythonConsole console(python_cin[WRITE], python_cout[READ]);
            console.run();
            // console closes pipes when exiting scope
            // TODO make pipe class that handles creation and deletion
        }

        wait(nullptr);
        std::cout << "Exiting parent " << std::endl;
    }
    return 0;
}
