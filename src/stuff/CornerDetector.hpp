#pragma once

#include <opencv2/core.hpp>
#include <vector>

// A possible marker with four corners.
struct CornerCandidate {
    std::vector<cv::Point> contour;
    std::vector<cv::Point2f> corners;
};

// Order corners geometrically.
bool orderCorners(const std::vector<cv::Point>& points, std::vector<cv::Point2f>& ordered);

class CornerDetector {
public:
    std::vector<CornerCandidate> detectCorners(const cv::Mat& gray, const cv::Mat& thresh) const;
};