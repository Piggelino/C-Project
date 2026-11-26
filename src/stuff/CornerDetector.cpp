#include "CornerDetector.hpp"
#include <algorithm>

#if __has_include(<opencv2/geometry/2d.hpp>)
#include <opencv2/geometry/2d.hpp>
#endif

bool CornerDetector::orderCorners(
    const std::vector<cv::Point>& points,
    std::vector<cv::Point2f>& ordered
) {
    ordered.clear();

    if (points.size() != 4) {
        return false;
    }

    // Arrange the points around the convex outline.
    std::vector<cv::Point> hull;

    cv::convexHull(points, hull, false, true);

    // Reject duplicate points and invalid quadrilaterals.
    if (hull.size() != 4) {
        return false;
    }

    // Find the starting corner.
    auto start = std::min_element(
        hull.begin(),
        hull.end(),
        [](const cv::Point& a, const cv::Point& b) {

            long long sumA = static_cast<long long>(a.x) + a.y;
            long long sumB = static_cast<long long>(b.x) + b.y;

            if (sumA == sumB) {
                return a.y < b.y;
            }

            return sumA < sumB;
        }
    );

    // Place the starting corner first.
    std::rotate(hull.begin(), start, hull.end());

    // Store the ordered corners.
    ordered.reserve(4);

    for (const cv::Point& point : hull) {
        ordered.emplace_back(
            static_cast<float>(point.x),
            static_cast<float>(point.y)
        );
    }

    return true;
}

std::vector<CornerCandidate> CornerDetector::detectCorners(
    const cv::Mat& gray,
    const cv::Mat& thresh
) {
    std::vector<CornerCandidate> candidates;

    // Find contours in the threshold image.
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

        double epsilon =
            0.02 * cv::arcLength(contour, true);

        cv::approxPolyDP(
            contour,
            approximatedContour,
            epsilon,
            true
        );

        // Validate the marker candidate.
        bool isConvex =
            approximatedContour.size() == 4 &&
            cv::isContourConvex(approximatedContour);

        if (!isConvex) {
            continue;
        }

        // Order the marker corners.
        std::vector<cv::Point2f> corners;

        if (!orderCorners(approximatedContour, corners)) {
            continue;
        }

        // Refine the corner positions.
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

        candidates.push_back({
            approximatedContour,
            corners
        });
    }

    return candidates;
}