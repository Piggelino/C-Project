#include "CornerDetector.hpp"

#if __has_include(<opencv2/geometry/2d.hpp>)
#include <opencv2/geometry/2d.hpp>
#endif

bool CornerDetector::orderCorners(
    const std::vector<cv::Point>& points,
    std::vector<cv::Point2f>& ordered
) {
    if (points.size() != 4) {
        return false;
    }

    ordered.clear();
    ordered.reserve(4);

    int topLeft = 0;
    int topRight = 0;
    int bottomRight = 0;
    int bottomLeft = 0;

    float smallestSum = 0.0f;
    float largestSum = 0.0f;
    float smallestDifference = 0.0f;
    float largestDifference = 0.0f;

    for (std::size_t i = 0; i < points.size(); ++i) {
        float sum = static_cast<float>(points[i].x + points[i].y);
        float difference = static_cast<float>(points[i].x - points[i].y);

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

std::vector<CornerCandidate> CornerDetector::detectCorners(
    const cv::Mat& gray,
    const cv::Mat& thresh
) {
    std::vector<CornerCandidate> candidates;

    // A contour is a line around a connected white area in the threshold image.
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(
        thresh,
        contours,
        cv::RETR_EXTERNAL,
        cv::CHAIN_APPROX_SIMPLE
    );

    for (const std::vector<cv::Point>& contour : contours) {
        if (cv::contourArea(contour) < 500) {
            continue;
        }

        std::vector<cv::Point> approximatedContour;
        double epsilon = 0.02 * cv::arcLength(contour, true);
        cv::approxPolyDP(
            contour,
            approximatedContour,
            epsilon,
            true
        );

        // Validate the marker candidate.
        bool isConvex = approximatedContour.size() == 4 &&
                        cv::isContourConvex(approximatedContour);

        if (!isConvex) {
            continue;
        }

        // Order and refine the marker corners.
        std::vector<cv::Point2f> corners;
        if (!orderCorners(approximatedContour, corners)) {
            continue;
        }

        cv::TermCriteria criteria(
            cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER,
            40,
            0.01
        );

        cv::cornerSubPix(
            gray,
            corners,
            cv::Size(5, 5),
            cv::Size(-1, -1),
            criteria
        );

        candidates.push_back({approximatedContour, corners});
    }

    return candidates;
}