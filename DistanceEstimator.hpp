
#pragma once

#include <algorithm>
#include <cmath>

class DistanceEstimator {
public:
    // Estimate proximity from the detected marker's area.
    // 0.0 = far away, 1.0 = very close.
    double estimateProximity(double markerArea) const {
        const double minArea = 1000.0;
        const double maxArea = 30000.0;

        if (markerArea <= 0.0) {
            return 0.0;
        }

        double proximity =
            (std::sqrt(markerArea) - std::sqrt(minArea)) /
            (std::sqrt(maxArea) - std::sqrt(minArea));

        return std::clamp(proximity, 0.0, 1.0);
    }

    // Convert the proximity value into a category.
    const char* getCategory(double proximity) const {
        if (proximity < 0.33) {
            return "FAR";
        }

        if (proximity < 0.66) {
            return "MEDIUM";
        }

        return "CLOSE";
    }
};
