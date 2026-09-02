#include "makeBlobs.cpp"
#include "imgui_viewer.hpp"  // GLFW + Dear ImGui window lifecycle
#include "imgui.h"           // ImGui widgets used in the example below
#include <iostream>

int main(){
    blob blobs;

    std::cout << "Generated " << blobs.centers().size()
              << " clusters, each with " << blobs.points().size() / blobs.centers().size()
              << " points\n";

    std::cout << "\nCluster centers:\n";
    for(const auto& center : blobs.centers()){
        for(double coord : center){
            std::cout << coord << ' ';
        }
        std::cout << '\n';
    }

    std::cout << "\nFirst 5 points of cluster 0:\n";
    for(std::size_t i = 0; i < 5; ++i){
        const auto& p = blobs.points()[i];
        std::cout << '(' << p[0] << ", " << p[1] << ")\n";
    }

    // ========================================================================
    // ImGui visualization example
    // ========================================================================
    // Opens a window that plots every generated point (small dots) together
    // with the cluster centers (big dots). The window stays open until it is
    // closed, and closing it ends the program.
    try {
        imgui_viewer viewer("testMakeBlobs: generated blobs", 1024, 720);

        while(viewer.is_running()){
            viewer.begin_frame();

            // Use the whole window as one big drawing canvas.
            const ImGuiIO& io = ImGui::GetIO();
            blob_draw(blobs, ImGui::GetBackgroundDrawList(),
                      ImVec2(0.0f, 0.0f), io.DisplaySize);

            // Small semi-transparent info box in the top-left corner.
            ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f));
            ImGui::Begin("blob info", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoFocusOnAppearing);
            ImGui::Text("%zu clusters, %zu points",
                        blobs.centers().size(), blobs.points().size());
            ImGui::Text("small dots = points, big dots = cluster centers");
            ImGui::Text("close the window to quit");
            ImGui::End();

            viewer.end_frame();
        }
    } catch(const std::exception& e) {
        std::cerr << "Could not open the visualization window: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
