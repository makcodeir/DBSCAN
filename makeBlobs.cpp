#include <vector>
#include <random>

class blob{
public:
    blob()
        : noiseMean_(0.0) {
        generateCenters(4, 2);
        generatePoints(42);
    }

    blob(const std::vector<std::vector<double>>& centers, int neighborCount = 42, double noiseMean = 0.0)
        : centers_(centers), noiseMean_(noiseMean) {
        generatePoints(neighborCount);
    }

    blob(int centerCount, int dimensions = 2, int neighborCount = 42, double noiseMean = 0.0)
        : noiseMean_(noiseMean) {
        generateCenters(centerCount, dimensions);
        generatePoints(neighborCount);
    }

    const std::vector<std::vector<double>>& centers() const {
        return centers_;
    }

    const std::vector<std::vector<double>>& points() const {
        return points_;
    }

private:
    void generateCenters(int centerCount, int dimensions){
        std::uniform_real_distribution<> dist(0.0, 1.0);
        for(int i = 0; i < centerCount; ++i){
            std::vector<double> center(dimensions);
            for(int d = 0; d < dimensions; ++d){
                center[d] = dist(engine_);
            }
            centers_.push_back(center);
        }
    }

    void generatePoints(int neighborCount){
        std::normal_distribution<> dist(noiseMean_, 1.0);
        for(std::size_t c = 0; c < centers_.size(); ++c){
            for(int p = 0; p < neighborCount; ++p){
                std::vector<double> point(centers_[c].size());
                for(std::size_t d = 0; d < point.size(); ++d){
                    point[d] = centers_[c][d] + dist(engine_);
                }
                points_.push_back(point);
            }
        }
    }

    std::mt19937 engine_{std::random_device{}()};
    std::vector<std::vector<double>> centers_;
    std::vector<std::vector<double>> points_;
    double noiseMean_;
};

// ============================================================================
// ImGui visualization section
// ============================================================================
// Draws the generated blob data with Dear ImGui so clusters can be inspected
// visually. The drawing is intentionally kept independent of any windowing
// code: these functions only submit circles into an ImDrawList, while the
// window/ImGui setup lives in imgui_viewer (see testMakeBlobs.cpp for a
// complete usage example).
//
// Only the first two coordinates of a point are drawn, so 2D blobs plot as
// expected and higher-dimensional blobs show their first two dimensions.

#include "imgui.h"

// Computes the bounding box of all generated points.
// Leaves the outputs untouched when the blob has no points.
void blob_bounds(const blob& b, float& minX, float& minY, float& maxX, float& maxY){
    if(b.points().empty()){
        return;
    }
    minX = minY = 1e30f;
    maxX = maxY = -1e30f;
    for(const auto& p : b.points()){
        const float x = p.empty() ? 0.0f : static_cast<float>(p[0]);
        const float y = p.size() > 1 ? static_cast<float>(p[1]) : 0.0f;
        if(x < minX) minX = x;
        if(y < minY) minY = y;
        if(x > maxX) maxX = x;
        if(y > maxY) maxY = y;
    }
}

// Draws one blob (all points as small dots, cluster centers as big dots)
// inside the screen rectangle [origin, origin + size] of the given draw list.
void blob_draw(const blob& b, ImDrawList* drawList, const ImVec2& origin,
               const ImVec2& size, float pointRadius = 3.0f){
    if(!drawList || size.x <= 0.0f || size.y <= 0.0f){
        return;
    }

    float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;
    blob_bounds(b, minX, minY, maxX, maxY);

    // Keep a small margin so border points are not clipped, and guard against
    // a degenerate bounding box (all points in one spot).
    const float pad = pointRadius + 2.0f;
    const float spanX = maxX - minX > 1e-9f ? maxX - minX : 1.0f;
    const float spanY = maxY - minY > 1e-9f ? maxY - minY : 1.0f;
    const float scaleX = (size.x - 2.0f * pad) / spanX;
    const float scaleY = (size.y - 2.0f * pad) / spanY;
    const float scale = scaleX < scaleY ? scaleX : scaleY;  // uniform: no distortion

    // Map data coordinates to screen coordinates, centered on the canvas.
    const float dataMidX = 0.5f * (minX + maxX);
    const float dataMidY = 0.5f * (minY + minY);
    const ImVec2 canvasMid(origin.x + 0.5f * size.x, origin.y + 0.5f * size.y);
    auto toScreen = [&](const std::vector<double>& p){
        const float x = p.empty() ? 0.0f : static_cast<float>(p[0]);
        const float y = p.size() > 1 ? static_cast<float>(p[1]) : 0.0f;
        return ImVec2(canvasMid.x + (x - dataMidX) * scale,
                      canvasMid.y - (y - dataMidY) * scale);  // flip Y: +y up
    };

    // Theme colors keep the drawing consistent with the ImGui style.
    const ImU32 pointColor  = ImGui::GetColorU32(ImGuiCol_PlotLines);
    const ImU32 centerColor = ImGui::GetColorU32(ImGuiCol_PlotHistogram);

    for(const auto& p : b.points()){
        drawList->AddCircleFilled(toScreen(p), pointRadius, pointColor);
    }

    // Cluster centers are drawn last so they sit on top of the points.
    for(const auto& c : b.centers()){
        drawList->AddCircleFilled(toScreen(c), pointRadius + 2.0f, centerColor);
    }
}
