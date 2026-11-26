#pragma once

#include <vector>
#include <opencv2/opencv.hpp>

struct CornerCandidate {
    std::vector<cv::Point> contour;
    std::vector<cv::Point2f> corners;
};

class CornerDetector {
private:
    bool orderCorners(
        const std::vector<cv::Point>& points,
        std::vector<cv::Point2f>& ordered
    );

public:
    std::vector<CornerCandidate> detectCorners(
        const cv::Mat& gray,
        const cv::Mat& thresh
    );
};
