
#include <iostream>
#include <cmath>
#include "DistanceEstimator.hpp"

int main() {
    DistanceEstimator estimator;
    int passed = 0;
    int total = 5;

    auto test = [&](double area, double expected, const char* category) {
        double actual = estimator.estimateProximity(area);
        const char* actualCategory = estimator.getCategory(actual);

        bool success = std::abs(actual - expected) < 0.01 &&
                       std::string(actualCategory) == category;

        std::cout << (success ? "PASS" : "FAIL")
                  << " | Area: " << area
                  << " | Expected: " << expected
                  << " | Actual: " << actual
                  << " | Category: " << actualCategory << "\n";

        if (success) ++passed;
    };

    test(0, 0.0, "FAR");
    test(1000, 0.0, "FAR");
    test(5000, 0.28, "FAR");
    test(30000, 1.0, "CLOSE");
    test(50000, 1.0, "CLOSE");

    std::cout << "\nResults: " << passed << "/" << total
              << " tests passed.\n";

    return passed == total ? 0 : 1;
}
