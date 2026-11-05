#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <string>


class MarkerTemplateMatcher {
private:
    cv::Mat referencePattern;
    static constexpr int SIZE = 160;

    // Normalize a marker image for comparison.
    static cv::Mat normalize(const cv::Mat& image);

public:
    // Load the reference Hiro marker.
    bool loadTemplate(const std::string& path);

    // Check whether a four-corner candidate matches Hiro.
    bool matches(const cv::Mat& gray,
                 const std::vector<cv::Point2f>& corners) const;
};