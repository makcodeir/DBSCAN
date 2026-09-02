#include "imgui_viewer.hpp"

// Dear ImGui core and the two backends used here (GLFW platform + OpenGL3
// renderer), following the official GLFW + OpenGL3 integration example.
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <GL/gl.h>  // glViewport / glClearColor / glClear

#include <stdexcept>

imgui_viewer::imgui_viewer(const char* windowTitle, int width, int height)
    : window_(nullptr) {
    if (!glfwInit()) {
        throw std::runtime_error("imgui_viewer: glfwInit() failed");
    }

    // Ask for OpenGL 3.0 (GLSL 130), matching the ImGui OpenGL3 backend.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window_ = glfwCreateWindow(width, height, windowTitle, nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("imgui_viewer: could not create window");
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);  // vsync: keep the frame rate at the monitor's rate

    // Setup Dear ImGui context and style.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // keyboard controls
    ImGui::StyleColorsDark();

    // Setup platform/renderer backends. install_callback = true lets the
    // GLFW backend chain its callbacks to any existing ones.
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

imgui_viewer::~imgui_viewer() {
    if (!window_) {
        return;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window_);
    glfwTerminate();
}

bool imgui_viewer::is_running() const {
    return window_ && !glfwWindowShouldClose(window_);
}

void imgui_viewer::begin_frame() {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void imgui_viewer::end_frame() {
    // Rendering: clear the framebuffer, draw the UI, present it.
    ImGui::Render();
    int displayWidth, displayHeight;
    glfwGetFramebufferSize(window_, &displayWidth, &displayHeight);
    glViewport(0, 0, displayWidth, displayHeight);
    glClearColor(0.08f, 0.08f, 0.10f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_);
}
