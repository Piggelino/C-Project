#include "MarkerTemplateMatcher.hpp"

#include <cmath>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <algorithm>
#if __has_include(<opencv2/geometry.hpp>)
#include <opencv2/geometry.hpp>
#endif

cv::Mat MarkerTemplateMatcher::normalize(
    const cv::Mat& image
) {
    if (image.empty() || image.type() != CV_8UC1) {
        return {};
    }

    cv::Mat resized, binary;

    cv::resize(image, resized, cv::Size(SIZE, SIZE));

    cv::threshold(
        resized,
        binary,
        0,
        255,
        cv::THRESH_BINARY | cv::THRESH_OTSU
    );

    return binary;
}

cv::Mat MarkerTemplateMatcher::loadPattern(const std::string& path) {
    cv::Mat image = cv::imread( path, cv::IMREAD_GRAYSCALE);

    if (image.empty()) {
        return {};
    }

    cv::Mat mask;

    cv::threshold(
        image,
        mask,
        100,
        255,
        cv::THRESH_BINARY_INV
    );

    std::vector<cv::Point> blackPoints;

    cv::findNonZero(mask, blackPoints);

    if (blackPoints.empty()) {
        return {};
    }

    return normalize(
        image(cv::boundingRect(blackPoints))
    );
}

bool MarkerTemplateMatcher::loadTemplates(
    const std::string& directory
) {
    const std::array<std::string, 4> names = {
        "marker_0.png",
        "marker_90.png",
        "marker_180.png",
        "marker_270.png"
    };

    std::array<cv::Mat, 4> patterns;

    for (int i = 0; i < 4; ++i) {

        const std::string path =
            directory + "/" + names[i];

        patterns[i] = loadPattern(path);

        if (patterns[i].empty()) {

            std::cerr
                << "Invalid template: "
                << path << '\n';

            return false;
        }
    }

    referencePatterns = patterns;

    return true;
}

// Compatibility with the previous implementation.
bool MarkerTemplateMatcher::loadTemplate(
    const std::string& path
) {
    cv::Mat original = loadPattern(path);

    if (original.empty()) {
        return false;
    }

    referencePatterns[0] = original;

    cv::rotate(
        original,
        referencePatterns[1],
        cv::ROTATE_90_CLOCKWISE
    );

    cv::rotate(
        original,
        referencePatterns[2],
        cv::ROTATE_180
    );

    cv::rotate(
        original,
        referencePatterns[3],
        cv::ROTATE_90_COUNTERCLOCKWISE
    );

    return true;
}

// Transform the detected marker into a square.
cv::Mat MarkerTemplateMatcher::rectify(
    const cv::Mat& image,
    const std::vector<cv::Point2f>& corners,
    int size
) {
    if (image.empty() ||
        corners.size() != 4 ||
        size < 2) {

        return {};
    }

    // Validate the source coordinates.
    for (const auto& p : corners) {
        if (!std::isfinite(p.x) ||
            !std::isfinite(p.y) ||
            p.x < 0 ||
            p.y < 0 ||
            p.x >= image.cols ||
            p.y >= image.rows) {

            return {};
        }
    }

    if (!cv::isContourConvex(corners) ||
        cv::contourArea(corners, true) <= 1.0) {

        return {};
    }

    const float last =
        static_cast<float>(size - 1);

    // Destination coordinates.
    const std::vector<cv::Point2f> destination = {
        {0, 0},
        {last, 0},
        {last, last},
        {0, last}
    };

    // Calculate the perspective transformation.
    cv::Mat H = cv::getPerspectiveTransform(
        corners,
        destination
    );

    if (H.empty() || !cv::checkRange(H)) {
        return {};
    }

    // Verify that the matrix maps the
    // four source points correctly.
    std::vector<cv::Point2f> projected;

    cv::perspectiveTransform(
        corners,
        projected,
        H
    );

    for (int i = 0; i < 4; ++i) {

        double error = cv::norm(
            projected[i] - destination[i]
        );

        if (!std::isfinite(error) ||
            error > 0.5) {

            return {};
        }
    }

    cv::Mat warped;

    cv::warpPerspective(
        image,
        warped,
        H,
        cv::Size(size, size),
        cv::INTER_LINEAR,
        cv::BORDER_REPLICATE
    );

    return warped;
}

// Extract the central region of the marker.
cv::Mat MarkerTemplateMatcher::extractInterior(
    const cv::Mat& image
) {
    const int m = static_cast<int>(
        image.cols * 0.25
    );

    return image(
        cv::Rect(
            m,
            m,
            image.cols - 2 * m,
            image.rows - 2 * m
        )
    );
}

// Validate the black border.
bool MarkerTemplateMatcher::hasBlackBorder(
    const cv::Mat& image
) {
    const int n = image.cols;

    // Sample safely inside the thick border, away from the outer edge.
    const int offset = n / 20;
    const int w = n / 12;

    const int span = n - 2 * offset;

    const std::array<cv::Rect, 4> strips = {

        cv::Rect(offset, offset, span, w),

        cv::Rect(offset, n - offset - w, span, w),

        cv::Rect(offset, offset, w, span),

        cv::Rect( n - offset - w, offset, w, span )
    };

    for (const auto& strip : strips) {

        cv::Mat pixels = image(strip);

        double white =
            static_cast<double>(
                cv::countNonZero(pixels)
            ) / pixels.total();

        if (white > 0.20) {
            return false;
        }
    }

    return true;
}

// Search for the reference interior within a slightly larger region.
double MarkerTemplateMatcher::compare(
    const cv::Mat& candidate,
    const cv::Mat& reference
) {
    const cv::Mat pattern = extractInterior(reference);
    const int margin = static_cast<int>(candidate.cols * 0.20);

    const cv::Mat searchArea = candidate(cv::Rect(
        margin, margin,
        candidate.cols - 2 * margin,
        candidate.rows - 2 * margin
    ));

    // Normalized correlation needs a non-constant reference pattern.
    const int white = cv::countNonZero(pattern);
    if (white == 0 || static_cast<std::size_t>(white) == pattern.total()) {
        return -1.0;
    }

    cv::Mat scores;
    cv::matchTemplate(searchArea, pattern, scores, cv::TM_CCOEFF_NORMED);

    double bestScore;
    cv::minMaxLoc(scores, nullptr, &bestScore);
    if (std::isfinite(bestScore)) {
        return bestScore;
    } else {
        return -1.0;
    }
}

// Recognize Hiro and recover its orientation.
MarkerMatchResult MarkerTemplateMatcher::recognize(
    const cv::Mat& gray,
    const std::vector<cv::Point2f>& corners
) const {
    MarkerMatchResult result;

    for (const auto& pattern : referencePatterns) {

        if (pattern.empty()) {

            result.reason = "Templates not loaded";

            return result;
        }
    }

    cv::Mat warped = rectify(
        gray,
        corners,
        SIZE
    );

    cv::Mat candidate = normalize(warped);

    if (candidate.empty()) {
        result.reason = "Invalid rectification";
        return result;
    }
    result.normalizedCandidate = candidate;

    double best = -1.0;
    double second = -1.0;

    int bestIndex = -1;

    // Compare all four reference images.
    for (int i = 0; i < 4; ++i) {
        double score = compare(
            candidate,
            referencePatterns[i]
        );

        result.scores[i] = score;

        if (score > best) {
            second = best;
            best = score;
            bestIndex = i;

        } else if (score > second) {
            second = score;
        }
    }

    result.similarity = best;
    result.margin = best - second;

    // Validate image contrast.
    double low, high;

    cv::minMaxLoc(
        warped,
        &low,
        &high
    );

    cv::Mat interior =
        extractInterior(candidate);

    double white =
        static_cast<double>(
            cv::countNonZero(interior)
        ) / interior.total();

    // Reject invalid or ambiguous markers.
    if (high - low < 40 ||
        white < 0.05 ||
        white > 0.95) {

        result.reason = "Low contrast or missing pattern";

    } else if (!hasBlackBorder(candidate)) {
        result.reason = "Invalid black border";

    } else if (best < 0.75) {
        result.reason = "Similarity too low";

    } else if (result.margin < 0.04) {
        result.reason = "Ambiguous orientation";

    } else {

        result.matched = true;
        result.id = "HIRO";
        result.rotationIndex = bestIndex;
        result.reason = "Accepted";
    }

    return result;
}

bool MarkerTemplateMatcher::matches(
    const cv::Mat& gray,
    const std::vector<cv::Point2f>& corners
) const {
    return recognize(gray, corners).matched;
}
