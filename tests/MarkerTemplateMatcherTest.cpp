
#include "MarkerTemplateMatcher.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

int main() {
    int passed = 0;
    int total = 0;

    auto test = [&](const std::string& name, bool actual, bool expected) {
        ++total;
        if (actual == expected) {
            ++passed;
            std::cout << "PASS | " << name << '\n';
        } else {
            std::cout << "FAIL | " << name
                      << " | Expected: " << expected
                      << " | Actual: " << actual << '\n';
        }
    };

    const std::string templatePath = "resources/marker2.png.webp";

    MarkerTemplateMatcher matcher;

    // Test 1: Missing template.
    test("Nonexistent template",
         matcher.loadTemplate("resources/does_not_exist.webp"), false);

    // Test 2: Valid template.
    bool loaded = matcher.loadTemplate(templatePath);
    test("Load Hiro template", loaded, true);

    if (!loaded) {
        std::cout << "Cannot continue: Hiro template failed to load.\n";
        return 1;
    }

    cv::Mat hiro = cv::imread(templatePath, cv::IMREAD_GRAYSCALE);

    // Find the marker bounds using the same threshold as loadTemplate.
    cv::Mat blackMask;
    cv::threshold(hiro, blackMask, 100, 255, cv::THRESH_BINARY_INV);

    std::vector<cv::Point> blackPoints;
    cv::findNonZero(blackMask, blackPoints);

    if (blackPoints.empty()) {
        std::cout << "Cannot continue: no marker pixels found.\n";
        return 1;
    }

    // Calculate the bounding rectangle of the detected marker pixels.
    int minX = hiro.cols;
    int minY = hiro.rows;
    int maxX = -1;
    int maxY = -1;

    for (const cv::Point& point : blackPoints) {
        minX = std::min(minX, point.x);
        minY = std::min(minY, point.y);
        maxX = std::max(maxX, point.x);
        maxY = std::max(maxY, point.y);
    }

    cv::Rect bounds(minX, minY, maxX - minX + 1, maxY - minY + 1);
    cv::Mat marker = hiro(bounds).clone();

    // Corners cover the complete cropped marker.
    auto cornersFor = [](const cv::Mat& image) {
        return std::vector<cv::Point2f>{
            {0.0f, 0.0f},
            {static_cast<float>(image.cols - 1), 0.0f},
            {static_cast<float>(image.cols - 1),
             static_cast<float>(image.rows - 1)},
            {0.0f, static_cast<float>(image.rows - 1)}
        };
    };

    // Test 3: Genuine Hiro marker.
    test("Match genuine Hiro",
         matcher.matches(marker, cornersFor(marker)), true);

    // Test 4: Unrelated image.
    cv::Mat unrelated(marker.size(), CV_8UC1, cv::Scalar(255));
    cv::rectangle(unrelated,
                  cv::Point(20, 20),
                  cv::Point(unrelated.cols - 20, unrelated.rows - 20),
                  cv::Scalar(0), -1);

    test("Reject unrelated image",
         matcher.matches(unrelated, cornersFor(unrelated)), false);

    // Test 5: Hiro rotated 90 degrees.
    cv::Mat rotated90;
    cv::rotate(marker, rotated90, cv::ROTATE_90_CLOCKWISE);

    test("Hiro rotated 90 degrees",
         matcher.matches(rotated90, cornersFor(rotated90)), true);

    // Test 6: Hiro rotated 180 degrees.
    cv::Mat rotated180;
    cv::rotate(marker, rotated180, cv::ROTATE_180);

    test("Hiro rotated 180 degrees",
         matcher.matches(rotated180, cornersFor(rotated180)), true);

    // Test 7: Incorrect number of corners.
    test("Reject three corners",
         matcher.matches(marker, {{0, 0}, {100, 0}, {100, 100}}),
         false);

    // Test 8: Matching without loading a reference.
    MarkerTemplateMatcher emptyMatcher;
    test("Reject when template not loaded",
         emptyMatcher.matches(marker, cornersFor(marker)), false);

    std::cout << "\nResults: " << passed << "/" << total
              << " tests passed.\n";

    return passed == total ? 0 : 1;
}
