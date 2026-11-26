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
    if (points.size() != 4) return false;

    std::vector<cv::Point> hull;
    cv::convexHull(points, hull);
    if (hull.size() != 4) return false;

    cv::Point2f center(0, 0);
    for (const auto& point : hull) {
        ordered.emplace_back(point);
        center += ordered.back();
    }
    center *= 0.25f;

    std::sort(ordered.begin(), ordered.end(), [center](auto a, auto b) {
        return std::atan2(a.y - center.y, a.x - center.x) <
               std::atan2(b.y - center.y, b.x - center.x);
    });

    if (cv::contourArea(ordered, true) < 0.0) {
        std::reverse(ordered.begin(), ordered.end());
    }

    const auto first = std::min_element(ordered.begin(), ordered.end(), [](auto a, auto b) {
        const float sumA = a.x + a.y;
        const float sumB = b.x + b.y;
        return sumA == sumB ? a.y < b.y : sumA < sumB;
    });
    std::rotate(ordered.begin(), first, ordered.end());
    return true;
}

std::vector<CornerCandidate> CornerDetector::detectCorners(
    const cv::Mat& gray,
    const cv::Mat& thresh
) const {
    std::vector<CornerCandidate> candidates;
    if (gray.empty() || thresh.empty() ||
        gray.type() != CV_8UC1 || thresh.type() != CV_8UC1 ||
        gray.size() != thresh.size()) {
        return candidates;
    }

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(thresh, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    for (std::size_t i = 0; i < contours.size(); ++i) {
        // Odd-depth contours are holes, not marker boundaries.
        int depth = 0;
        for (int parent = hierarchy[i][3]; parent >= 0; parent = hierarchy[parent][3]) {
            ++depth;
        }
        if (depth % 2 != 0) continue;

        const auto& contour = contours[i];
        if (cv::contourArea(contour) < 500) continue;

        std::vector<cv::Point> polygon;
        const auto approximateSquare = [&polygon](const std::vector<cv::Point>& points) {
            const double perimeter = cv::arcLength(points, true);
            for (double epsilon : {0.02, 0.015, 0.03, 0.04}) {
                cv::approxPolyDP(points, polygon, epsilon * perimeter, true);
                if (polygon.size() == 4 && cv::isContourConvex(polygon)) return true;
            }
            return false;
        };

        if (!approximateSquare(contour)) {
            std::vector<cv::Point> hull;
            cv::convexHull(contour, hull);
            if (!approximateSquare(hull)) continue;
        }

        std::vector<cv::Point2f> corners;
        if (!orderCorners(polygon, corners)) continue;

        bool valid = true;
        for (const auto& point : corners) {
            if (point.x < 6 || point.y < 6 ||
                point.x >= gray.cols - 6 || point.y >= gray.rows - 6) {
                valid = false;
            }
        }
        if (!valid) continue;

        // Keep the geometric corners if subpixel refinement is unreliable.
        auto refined = corners;
        cv::cornerSubPix(
            gray,
            refined,
            cv::Size(5, 5),
            cv::Size(-1, -1),
            cv::TermCriteria(
                cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER,
                40,
                0.01
            )
        );

        double shortestSide = cv::norm(corners[1] - corners[0]);
        for (int j = 0; j < 4; ++j) {
            shortestSide = std::min(
                shortestSide,
                cv::norm(corners[(j + 1) % 4] - corners[j])
            );
        }
        const double maxShift = std::min(5.0, shortestSide * 0.10);

        for (int j = 0; j < 4; ++j) {
            const auto& point = refined[j];
            if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
                point.x < 0 || point.y < 0 ||
                point.x >= gray.cols || point.y >= gray.rows ||
                cv::norm(point - corners[j]) > maxShift) {
                valid = false;
                break;
            }
        }

        if (valid && cv::isContourConvex(refined) &&
            cv::contourArea(refined, true) > 1.0) {
            corners = refined;
        }

        candidates.push_back({polygon, corners});
    }

    return candidates;
}
