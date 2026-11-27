#pragma once

#include <array>
#include <opencv2/core.hpp>
#include <string>
#include <vector>

struct MarkerMatchResult {
    bool matched = false;
    int rotationIndex = -1;
    double similarity = -1.0;

    std::string id;

    double margin = 0.0;

    std::array<double, 4> scores = {
        -1.0, -1.0, -1.0, -1.0
    };

    cv::Mat normalizedCandidate;

    std::string reason = "Not evaluated";
};

class MarkerTemplateMatcher {
private:
    static constexpr int SIZE = 160;

    std::array<cv::Mat, 4> referencePatterns;

    static cv::Mat normalize(const cv::Mat& image);
    static cv::Mat loadPattern(const std::string& path);
    static cv::Mat extractInterior(const cv::Mat& image);
    static bool hasBlackBorder(const cv::Mat& image);
    static double compare(const cv::Mat& candidate, const cv::Mat& reference);

public:
    bool loadTemplates(const std::string& directory);
    bool loadTemplate(const std::string& path);
    static cv::Mat rectify(const cv::Mat& image, const std::vector<cv::Point2f>& corners, int size);
    MarkerMatchResult recognize(const cv::Mat& gray, const std::vector<cv::Point2f>& corners) const;
    bool matches(const cv::Mat& gray, const std::vector<cv::Point2f>& corners) const;
};