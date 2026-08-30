#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>

class point{
public:
    point(const std::vector<double>& coords){
        coords_ = coords;
    }

    double distanceTo(const point& o) const {
        double sum = 0;
        for(int i = 0 ;i<coords_.size();i++){
            sum += std::pow(coords_[i] - o.coords_[i] , 2);
        }
        return std::sqrt(sum);
    }


private:
    std::vector<double>coords_;



};