
#include <iostream>
#include <vector>
#include <string>
#include "CornerDetector.hpp"

int main() {
    int passed = 0;
    int total = 0;

    auto test = [&](const std::string& name,
                    const std::vector<cv::Point>& points,
                    bool expected,
                    const std::vector<cv::Point2f>& expectedCorners = {}) {
        ++total;

        std::vector<cv::Point2f> actualCorners;
        bool actual = orderCorners(points, actualCorners);

        bool success = (actual == expected);

        if (expected && actual) {
            success = (actualCorners == expectedCorners);
        }

        std::cout << (success ? "PASS" : "FAIL")
                  << " | " << name << "\n";

        if (!success) {
            std::cout << "  Expected valid: " << expected
                      << " | Actual valid: " << actual << "\n";
        }

        if (success) ++passed;
    };

    // Four corners in the correct order
    test("Normal square",
         {{10,10}, {90,10}, {90,90}, {10,90}},
         true,
         {{10,10}, {90,10}, {90,90}, {10,90}});

    // Four corners supplied in a different order
    test("Shuffled corners",
         {{90,90}, {10,90}, {10,10}, {90,10}},
         true,
         {{10,10}, {90,10}, {90,90}, {10,90}});

    // A rectangular marker
    test("Rectangle",
         {{20,10}, {120,10}, {120,60}, {20,60}},
         true,
         {{20,10}, {120,10}, {120,60}, {20,60}});

    // Invalid number of corners
    test("Three corners",
         {{10,10}, {90,10}, {90,90}},
         false);

    test("Five corners",
         {{10,10}, {90,10}, {90,90}, {10,90}, {50,50}},
         false);

    // Four identical points should be rejected
    test("Duplicate corners",
         {{10,10}, {10,10}, {10,10}, {10,10}},
         false);

    std::cout << "\nResults: " << passed << "/" << total
              << " tests passed.\n";

    return passed == total ? 0 : 1;
}
