#pragma once

class DistanceEstimator {
public:
    // Estimate proximity from the detected marker's area.
    // 0.0 = far away, 1.0 = very close.
    double estimateProximity(double markerArea) const;

    // Convert the proximity value into a category.
    const char* getCategory(double proximity) const;
};