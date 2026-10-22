
#include "DistanceEstimator.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

int main() {
    DistanceEstimator estimator;

    // Zero and negative areas should produce zero proximity.
    assert(estimator.estimateProximity(0.0) == 0.0);
    assert(estimator.estimateProximity(-100.0) == 0.0);

    // Minimum and maximum area boundaries.
    assert(estimator.estimateProximity(1000.0) == 0.0);
    assert(estimator.estimateProximity(30000.0) == 1.0);

    // Larger marker area should produce greater proximity.
    assert(estimator.estimateProximity(5000.0) <
           estimator.estimateProximity(15000.0));

    // Proximity categories.
    assert(std::string(estimator.getCategory(0.0)) == "FAR");
    assert(std::string(estimator.getCategory(0.5)) == "MEDIUM");
    assert(std::string(estimator.getCategory(1.0)) == "CLOSE");

    // Horizontal center calculation.
    assert(std::abs(estimator.getCenterX(100, 200, 200, 100) - 150.0) < 0.001);

    std::cout << "All DistanceEstimator tests passed!" << std::endl;
    return 0;
}
