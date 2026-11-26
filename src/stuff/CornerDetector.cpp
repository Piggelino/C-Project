#include "CornerDetector.hpp"

#include <algorithm>
#include <cmath>
#include <opencv2/imgproc.hpp>

#if __has_include(<opencv2/geometry.hpp>)
#include <opencv2/geometry.hpp>
#endif

bool orderCorners(
    const std::vector<cv::Point>& points,
    std::vector<cv::Point2f>& ordered
) {
    ordered.clear();

    if (points.size() != 4) {
        return false;
    }

    std::vector<cv::Point> hull;
    cv::convexHull(points, hull);

    if (hull.size() != 4) {
        return false;
    }

    cv::Point2f center(0, 0);

    for (const auto& p : hull) {
        ordered.emplace_back(p);
        center += ordered.back();
    }

    center *= 0.25f;

    // Sort corners clockwise.
    std::sort(
        ordered.begin(),
        ordered.end(),
        [center](auto a, auto b) {
            return std::atan2(
                a.y - center.y,
                a.x - center.x
            ) < std::atan2(
                b.y - center.y,
                b.x - center.x
            );
        }
    );

    // Ensure clockwise winding in image coordinates.
    if (cv::contourArea(ordered, true) < 0.0) {
        std::reverse(
            ordered.begin(),
            ordered.end()
        );
    }

    // Select a temporary geometric starting corner.
    const auto first = std::min_element(
        ordered.begin(),
        ordered.end(),
        [](auto a, auto b) {
            float sa = a.x + a.y;
            float sb = b.x + b.y;

            return sa == sb
                ? a.y < b.y
                : sa < sb;
        }
    );

    std::rotate(
        ordered.begin(),
        first,
        ordered.end()
    );

    return true;
}

std::vector<CornerCandidate> CornerDetector::detectCorners(
    const cv::Mat& gray,
    const cv::Mat& thresh
) const {
    std::vector<CornerCandidate> candidates;

    if (gray.empty() ||
        thresh.empty() ||
        gray.type() != CV_8UC1 ||
        thresh.type() != CV_8UC1 ||
        gray.size() != thresh.size()) {

        return candidates;
    }

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    cv::findContours(
        thresh,
        contours,
        hierarchy,
        cv::RETR_TREE,
        cv::CHAIN_APPROX_SIMPLE
    );

    for (std::size_t i = 0; i < contours.size(); ++i) {

        // In the inverted binary image,
        // odd-depth contours represent holes.
        int depth = 0;

        for (int p = hierarchy[i][3];
             p >= 0;
             p = hierarchy[p][3]) {

            ++depth;
        }

        if (depth % 2 != 0) {
            continue;
        }

        const auto& contour = contours[i];

        if (cv::contourArea(contour) < 500) {
            continue;
        }

        std::vector<cv::Point> polygon;

        cv::approxPolyDP(
            contour,
            polygon,
            0.02 * cv::arcLength(contour, true),
            true
        );

        if (polygon.size() != 4 ||
            !cv::isContourConvex(polygon)) {

            continue;
        }

        std::vector<cv::Point2f> corners;

        if (!orderCorners(polygon, corners)) {
            continue;
        }

        bool valid = true;

        // Reject corners too close to the image border.
        for (const auto& p : corners) {
            if (p.x < 6 ||
                p.y < 6 ||
                p.x >= gray.cols - 6 ||
                p.y >= gray.rows - 6) {

                valid = false;
            }
        }

        if (!valid) {
            continue;
        }

        // Refine the corner positions.
        cv::cornerSubPix(
            gray,
            corners,
            cv::Size(5, 5),
            cv::Size(-1, -1),
            cv::TermCriteria(
                cv::TermCriteria::EPS |
                cv::TermCriteria::MAX_ITER,
                40,
                0.01
            )
        );

        // Validate the refined corners.
        for (const auto& p : corners) {
            if (!std::isfinite(p.x) ||
                !std::isfinite(p.y) ||
                p.x < 0 ||
                p.y < 0 ||
                p.x >= gray.cols ||
                p.y >= gray.rows) {

                valid = false;
            }
        }

        if (!valid ||
            !cv::isContourConvex(corners) ||
            cv::contourArea(corners, true) <= 1.0) {

            continue;
        }

        candidates.push_back({
            polygon,
            corners
        });
    }

    return candidates;
}