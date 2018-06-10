// ////////////////////////////////////////////////////////////
// Created on 6/10/18.
// Copyright (c) 2018. All rights reserved.
// ////////////////////////////////////////////////////////////
#pragma once

#include <imgui.h>
#include <memory>
#include <string>

struct GLFWwindow;

namespace impy {

class PythonConsole
{
public:
    PythonConsole(int python_input_pipe, int python_output_pipe, int width = 640, int height = 480);
    ~PythonConsole();

    void run();

    // move good
    PythonConsole(PythonConsole &&) noexcept = default;
    PythonConsole &operator=(PythonConsole &&) noexcept = default;

    // copy bad
    PythonConsole(const PythonConsole &) = delete;
    PythonConsole &operator=(const PythonConsole &) = delete;

private:
    int python_input_pipe_;
    int python_output_pipe_;

    std::shared_ptr<int> glfw_;
    std::shared_ptr<GLFWwindow> window_;
    std::shared_ptr<bool> imgui_;

    bool writing_;

    std::string history_;
    std::string input_;
    char buf_;
    std::string output_;

    void read_python();
    void write_python();
    void configure_gui(int w, int h);
};

} // namespace impy
