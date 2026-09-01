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
