// ////////////////////////////////////////////////////////////
// Created on 6/10/18.
// Copyright (c) 2018. All rights reserved.
// ////////////////////////////////////////////////////////////
#include "PythonConsole.hpp"
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include <string>
#include <sstream>
#include <iostream>
#include <unistd.h>

namespace impy {

PythonConsole::PythonConsole(int python_input_pipe, int python_output_pipe, int width, int height)
    : python_input_pipe_(python_input_pipe), python_output_pipe_(python_output_pipe), writing_(false)
{
    std::string title = "Python Console";

    // Set the error callback before any other GLFW calls so we get proper error reporting
    glfwSetErrorCallback([](int error, const char *description) {
        std::cerr << "ERROR: (" << error << ") " << description << std::endl;
    });

    glfw_ = std::shared_ptr<int>(new int(glfwInit()), [](auto p) {
        glfwTerminate();
        delete p;
    });

    if (*glfw_ == 0) {
        throw std::runtime_error("GLFW init failed");
    }

    if (width == 0 || height == 0) {
        const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        width = mode->width;
        height = mode->height;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5); // highest on mac :(
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RESIZABLE, true);

    window_ = std::shared_ptr<GLFWwindow>( //
        glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr),
        [](auto p) {
            if (p) {
                glfwDestroyWindow(p);
            }
        });

    if (window_ == nullptr) {
        throw std::runtime_error("GLFW window creation failed");
    }

    glfwMakeContextCurrent(window_.get());
    glfwSwapInterval(1);

    gl3wInit();

    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    imgui_ = std::shared_ptr<bool>( //
        new bool(ImGui_ImplGlfwGL3_Init(window_.get(), true)),
        [](auto p) {
            ImGui_ImplGlfwGL3_Shutdown();
            delete p;
        });

    ImGui::StyleColorsDark();

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glFrontFace(GL_CCW);
    glClearColor(0.f, 0.f, 0.f, 1.f);
}

PythonConsole::~PythonConsole()
{
    if (close(python_input_pipe_) == -1) {
        std::perror("close input");
    }
    if (close(python_output_pipe_) == -1) {
        std::perror("close output");
    }
};

void PythonConsole::run()
{
    do {
        read_python();

        int w, h;
        glfwGetFramebufferSize(window_.get(), &w, &h);

        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplGlfwGL3_NewFrame();
        configure_gui(w, h);
        ImGui::Render();

        ImGui_ImplGlfwGL3_NewFrame();
        configure_gui(w, h);
        ImGui::Render();

        ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_.get());
        glfwWaitEvents();
    } while (!glfwWindowShouldClose(window_.get()));
}

void PythonConsole::read_python()
{
    if (!writing_) {
        // Read python output character by character until ">>> " or "... " is reached
        while (read(python_output_pipe_, &buf_, 1) > 0) {
            output_ += buf_; // TODO: do this more efficiently
            if (output_.size() >= 4) {
                std::string end = output_.substr(output_.size() - 4);
                if (end == ">>> " || end == "... ") {
                    std::cout << output_ << std::flush;
                    history_ += output_.substr(0, output_.size() - 4);
                    output_ = end;
                    writing_ = true;
                    break;
                }
            }
        }
    }
}

void PythonConsole::write_python()
{
    if (write(python_input_pipe_, input_.c_str(), input_.size()) == -1) {
        std::perror("write");
    }

    std::cout << input_ << std::flush;
    history_ += output_ + input_;
    output_ = "";

    // Exit on "exit()" or "quit()"
    if (input_.find("quit()") != std::string::npos || input_.find("exit()") != std::string::npos) {
        glfwSetWindowShouldClose(window_.get(), true);
    }

    writing_ = false;
}

void PythonConsole::configure_gui(int w, int h)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));

    ImGui::Begin("Python Console", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    ImGui::Text("%s", history_.c_str());
    ImGui::Text("%s", output_.c_str());
    ImGui::SameLine();

    char buf[1024];
    ImGui::SetKeyboardFocusHere();
    if (ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
        input_ = std::string(buf) + "\n";
        write_python();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace impy
