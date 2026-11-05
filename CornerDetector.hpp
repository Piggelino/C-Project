#pragma once

#include <vector>
#include <opencv2/opencv.hpp>

bool orderCorners(
    const std::vector<cv::Point>& points,
    std::vector<cv::Point2f>& ordered
);