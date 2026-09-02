#ifndef IMGUI_VIEWER_HPP
#define IMGUI_VIEWER_HPP

// ============================================================================
// imgui_viewer - minimal wrapper around the Dear ImGui application lifecycle
// ============================================================================
// Encapsulates the GLFW + OpenGL3 boilerplate from the official integration
// guide (https://github.com/ocornut/imgui/wiki/Getting-Started) so that a
// program only has to deal with the actual drawing:
//
//     imgui_viewer viewer("my window", 1024, 720);   // init GLFW + ImGui
//
//     while (viewer.is_running()) {
//         viewer.begin_frame();                      // start a new frame
//         ... ImGui drawing code goes here ...
//         viewer.end_frame();                        // render + present
//     }
//     // the destructor shuts everything down
//
// Throws std::runtime_error if the window or ImGui cannot be initialized
// (for example when no display server is available).

struct GLFWwindow;  // forward declaration: GLFW types stay out of this header

class imgui_viewer {
public:
    imgui_viewer(const char* windowTitle, int width, int height);
    ~imgui_viewer();

    // A viewer owns OS-level resources and must not be copied.
    imgui_viewer(const imgui_viewer&) = delete;
    imgui_viewer& operator=(const imgui_viewer&) = delete;

    bool is_running() const;  // false once the user closes the window
    void begin_frame();       // polls events and starts a new ImGui frame
    void end_frame();         // renders the frame and swaps the buffers

private:
    GLFWwindow* window_;
};

#endif
