
#pragma once
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <vector>

class MarkerTemplateMatcher {
private:
    cv::Mat referencePattern;
    static constexpr int SIZE = 160;

    // Normalize a marker image for comparison.
    static cv::Mat normalize(const cv::Mat& image) {
        cv::Mat gray, binary;

        if (image.channels() == 3) {
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = image.clone();
        }

        cv::threshold(gray, binary, 0, 255,
                      cv::THRESH_BINARY | cv::THRESH_OTSU);

        cv::resize(binary, binary, cv::Size(SIZE, SIZE));
        return binary;
    }

public:
    // Load the reference Hiro marker.
    bool loadTemplate(const std::string& path) {
        cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);

        if (image.empty()) {
            return false;
        }

        // Remove the white margins around the Hiro marker.
cv::Mat blackMask;
cv::threshold(image, blackMask, 100, 255, cv::THRESH_BINARY_INV);

std::vector<cv::Point> blackPoints;
cv::findNonZero(blackMask, blackPoints);

if (blackPoints.empty()) {
    return false;
}

cv::Rect markerBounds = cv::boundingRect(blackPoints);

// Compare only the actual marker, without white margins.
referencePattern = normalize(image(markerBounds));
return true;
    }

    // Check whether a four-corner candidate matches Hiro.
    bool matches(const cv::Mat& gray,
                 const std::vector<cv::Point2f>& corners) const {
        if (referencePattern.empty() || corners.size() != 4) {
            return false;
        }

        std::vector<cv::Point2f> destination = {
            {0, 0},
            {SIZE - 1.0f, 0},
            {SIZE - 1.0f, SIZE - 1.0f},
            {0, SIZE - 1.0f}
        };

        cv::Mat transform =
            cv::getPerspectiveTransform(corners, destination);

        cv::Mat warped;
        cv::warpPerspective(gray, warped, transform,
                            cv::Size(SIZE, SIZE));

        cv::Mat candidate = normalize(warped);

        // Check all four rotations.
        for (int rotation = 0; rotation < 4; ++rotation) {
            cv::Mat difference;
            cv::absdiff(candidate, referencePattern, difference);

            double similarity =
                1.0 - static_cast<double>(cv::countNonZero(difference))
                      / (SIZE * SIZE);

            // DEBUG: Print the similarity score for each candidate.
std::cout << "Hiro similarity: " << similarity << std::endl;

if (similarity >= 0.9) {
    return true;
}

            cv::rotate(candidate, candidate, cv::ROTATE_90_CLOCKWISE);
        }

        return false;
    }
};
