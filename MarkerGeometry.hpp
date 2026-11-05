#pragma once

#include <vector>

#include <opencv2/opencv.hpp>

class MarkerGeometry {
public:
    static bool orderCorners(
        const std::vector<cv::Point>& points,
        std::vector<cv::Point2f>& ordered
    ) {
        if (points.size() != 4) {
            return false;
        }

        int topLeft = 0;
        int topRight = 0;
        int bottomRight = 0;
        int bottomLeft = 0;
        float smallestSum = 0.0f;
        float largestSum = 0.0f;
        float smallestDifference = 0.0f;
        float largestDifference = 0.0f;

        for (std::size_t i = 0; i < points.size(); ++i) {
            const float sum = static_cast<float>(
                points[i].x + points[i].y
            );
            const float difference = static_cast<float>(
                points[i].x - points[i].y
            );

            if (i == 0 || sum < smallestSum) {
                smallestSum = sum;
                topLeft = static_cast<int>(i);
            }
            if (i == 0 || sum > largestSum) {
                largestSum = sum;
                bottomRight = static_cast<int>(i);
            }
            if (i == 0 || difference > largestDifference) {
                largestDifference = difference;
                topRight = static_cast<int>(i);
            }
            if (i == 0 || difference < smallestDifference) {
                smallestDifference = difference;
                bottomLeft = static_cast<int>(i);
            }
        }

        if (topLeft == topRight || topLeft == bottomRight ||
            topLeft == bottomLeft || topRight == bottomRight ||
            topRight == bottomLeft || bottomRight == bottomLeft) {
            return false;
        }

        ordered = {
            cv::Point2f(points[topLeft]),
            cv::Point2f(points[topRight]),
            cv::Point2f(points[bottomRight]),
            cv::Point2f(points[bottomLeft])
        };
        return true;
    }
};
