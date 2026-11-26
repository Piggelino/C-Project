#pragma once

class DistanceEstimator {
public:
    double estimateProximity(double markerArea) const;
    const char* getCategory(double proximity) const;
};