#pragma once

#include <limits>
#include <vector>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

#include "HiroDetector.hpp"
#include "MarkerGeometry.hpp"
#include "PoseEstimator.hpp"

struct MarkerDetection {
    std::vector<cv::Point2f> corners;
    std::vector<cv::Point> displayCorners;
    cv::Rect boundingBox;
    cv::Mat warpedImage;
    double area = 0.0;
    int firstCorner = 0;
    cv::Mat homography;
    cv::Mat rotation;
    cv::Mat translation;
    cv::Vec3d rotationDegrees = cv::Vec3d(0.0, 0.0, 0.0);
    bool poseValid = false;
};

class MarkerDetector {
public:
    bool detect(
        const cv::Mat& frame,
        cv::Mat& gray,
        cv::Mat& thresholded,
        MarkerDetection& detection
    ) const {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(
            gray,
            thresholded,
            80,
            255,
            cv::THRESH_BINARY_INV
        );

        detection = MarkerDetection{};
        detection.warpedImage = cv::Mat::zeros(400, 400, frame.type());

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(
            thresholded,
            contours,
            cv::RETR_EXTERNAL,
            cv::CHAIN_APPROX_SIMPLE
        );

        for (const std::vector<cv::Point>& contour : contours) {
            if (cv::contourArea(contour) < 100) {
                continue;
            }

            std::vector<cv::Point> approximatedContour;
            cv::approxPolyDP(
                contour,
                approximatedContour,
                0.04 * cv::arcLength(contour, true),
                true
            );

            if (approximatedContour.size() != 4 ||
                !cv::isContourConvex(approximatedContour)) {
                continue;
            }

            std::vector<cv::Point2f> corners;
            if (!MarkerGeometry::orderCorners(approximatedContour, corners)) {
                continue;
            }

            cv::cornerSubPix(
                gray,
                corners,
                cv::Size(5, 5),
                cv::Size(-1, -1),
                cv::TermCriteria(
                    cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER,
                    40,
                    0.01
                )
            );

            const std::vector<cv::Point2f> warpedCorners = {
                cv::Point2f(0.0f, 0.0f),
                cv::Point2f(399.0f, 0.0f),
                cv::Point2f(399.0f, 399.0f),
                cv::Point2f(0.0f, 399.0f)
            };
            const cv::Mat transform =
                cv::getPerspectiveTransform(corners, warpedCorners);
            cv::Mat warped;
            cv::warpPerspective(
                frame,
                warped,
                transform,
                cv::Size(400, 400)
            );

            cv::Point2f textCenter;
            if (!HiroDetector{}.findTextCenter(warped, textCenter)) {
                continue;
            }

            const cv::Mat cameraMatrix =
                PoseEstimator::cameraMatrix(frame.size());
            cv::Mat homography;
            cv::Mat rotation;
            cv::Mat translation;
            const bool poseValid = PoseEstimator::estimate(
                corners,
                cameraMatrix,
                0.1,
                homography,
                rotation,
                translation
            );

            std::vector<cv::Point2f> warpedMarkerCorners;
            cv::perspectiveTransform(corners, warpedMarkerCorners, transform);

            int firstCorner = 0;
            double shortestDistance = std::numeric_limits<double>::max();
            for (std::size_t i = 0; i < warpedMarkerCorners.size(); ++i) {
                const double distance =
                    cv::norm(warpedMarkerCorners[i] - textCenter);
                if (distance < shortestDistance) {
                    shortestDistance = distance;
                    firstCorner = static_cast<int>(i);
                }
            }

            detection.corners = corners;
            detection.displayCorners.clear();
            for (const cv::Point2f& corner : corners) {
                detection.displayCorners.emplace_back(
                    cvRound(corner.x),
                    cvRound(corner.y)
                );
            }
            detection.boundingBox = cv::boundingRect(approximatedContour);
            detection.warpedImage = warped;
            detection.area = cv::contourArea(approximatedContour);
            detection.firstCorner = firstCorner;
            detection.homography = homography;
            detection.rotation = rotation;
            detection.translation = translation;
            detection.poseValid = poseValid;
            if (poseValid) {
                detection.rotationDegrees =
                    PoseEstimator::rotationDegrees(rotation);
            }
            return true;
        }

        return false;
    }
};
