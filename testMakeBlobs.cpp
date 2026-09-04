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
    // ImGui visualization & interactive control
    // ========================================================================
    // Opens a window that plots every generated point (small dots) together
    // with the cluster centers (big dots). An interactive control panel allows
    // dynamically tuning parameters (standard deviation, noise, point count)
    // with immediate real-time visual feedback.
    try {
        imgui_viewer viewer("testMakeBlobs: generated blobs", 1024, 720);

        int centerCount = static_cast<int>(blobs.centers().size());
        int neighborCount = static_cast<int>(blobs.points().size() / blobs.centers().size());
        float clusterStd = static_cast<float>(blobs.clusterStd());
        float noiseMean = static_cast<float>(blobs.noiseMean());
        float pointRadius = 3.0f;

        // Viewport camera parameters
        float zoom = 1.0f;
        ImVec2 panOffset(0.0f, 0.0f);
        const float sidebarWidth = 340.0f;

        while(viewer.is_running()){
            viewer.begin_frame();

            const ImGuiIO& io = ImGui::GetIO();

            // Canvas layout: right side of the window, dedicated non-overlapping area
            const ImVec2 canvasOrigin(sidebarWidth, 0.0f);
            const ImVec2 canvasSize(io.DisplaySize.x > sidebarWidth ? io.DisplaySize.x - sidebarWidth : 0.0f,
                                    io.DisplaySize.y);

            // Handle interactive pan & zoom when hovering canvas (and not over ImGui windows)
            const ImVec2 mousePos = io.MousePos;
            const bool mouseInCanvas = (mousePos.x >= canvasOrigin.x && mousePos.x <= canvasOrigin.x + canvasSize.x &&
                                        mousePos.y >= canvasOrigin.y && mousePos.y <= canvasOrigin.y + canvasSize.y);

            if (mouseInCanvas && !io.WantCaptureMouse) {
                // Mouse wheel zooming centered at pan
                if (io.MouseWheel != 0.0f) {
                    const float zoomFactor = io.MouseWheel > 0.0f ? 1.15f : (1.0f / 1.15f);
                    zoom *= zoomFactor;
                    if (zoom < 0.05f) zoom = 0.05f;
                    if (zoom > 50.0f) zoom = 50.0f;
                }

                // Drag to pan (left click or middle/right click)
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
                    ImGui::IsMouseDragging(ImGuiMouseButton_Right) ||
                    ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                    panOffset.x += io.MouseDelta.x;
                    panOffset.y += io.MouseDelta.y;
                }
            }

            // Draw blob visualization inside the dedicated canvas viewport
            blob_draw(blobs, ImGui::GetBackgroundDrawList(),
                      canvasOrigin, canvasSize, pointRadius, zoom, panOffset);

            // Interactive controls panel docked on the left
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(sidebarWidth, io.DisplaySize.y), ImGuiCond_Always);
            ImGui::Begin("Blob Controls", nullptr,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

            ImGui::Text("%zu clusters, %zu points",
                        blobs.centers().size(), blobs.points().size());
            ImGui::Separator();

            bool pointsChanged = false;
            bool centersChanged = false;

            // Cluster standard deviation (density / focal concentration control)
            ImGui::TextDisabled("Controls point spread around centers:");
            if (ImGui::SliderFloat("Cluster Spread (Std)", &clusterStd, 0.001f, 0.5f, "%.4f", ImGuiSliderFlags_Logarithmic)) {
                pointsChanged = true;
            }

            // Noise mean offset
            if (ImGui::SliderFloat("Noise Offset", &noiseMean, -0.5f, 0.5f, "%.3f")) {
                pointsChanged = true;
            }

            // Points per cluster
            if (ImGui::SliderInt("Points / Cluster", &neighborCount, 5, 500)) {
                pointsChanged = true;
            }

            // Number of cluster centers
            if (ImGui::SliderInt("Cluster Centers", &centerCount, 1, 10)) {
                centersChanged = true;
            }

            // Point render radius
            ImGui::SliderFloat("Point Radius", &pointRadius, 1.0f, 10.0f, "%.1f px");

            ImGui::Separator();

            if (ImGui::Button("Regenerate Points")) {
                pointsChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("New Random Centers")) {
                centersChanged = true;
            }

            // Camera / View controls
            ImGui::Separator();
            ImGui::Text("View Controls:");
            if (ImGui::Button("Reset View / Auto-Fit")) {
                zoom = 1.0f;
                panOffset = ImVec2(0.0f, 0.0f);
            }
            ImGui::SameLine();
            ImGui::Text("Zoom: %.2fx", zoom);
            ImGui::TextDisabled("Canvas interaction: drag to pan, scroll to zoom");

            // Apply updates
            if (centersChanged) {
                blobs.regenerateAll(centerCount, 2, neighborCount, clusterStd, noiseMean);
            } else if (pointsChanged) {
                blobs.regenerate(neighborCount, clusterStd, noiseMean);
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Legend:");
            ImGui::BulletText("Small dots: generated points");
            ImGui::BulletText("Large dots: cluster centers");

            ImGui::End();

            viewer.end_frame();
        }
    } catch(const std::exception& e) {
        std::cerr << "Could not open the visualization window: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
