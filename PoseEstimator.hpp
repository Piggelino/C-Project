#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>

class PoseEstimator {
public:
    static cv::Mat cameraMatrix(const cv::Size& imageSize) {
        const double focalLength =
            static_cast<double>(std::max(imageSize.width, imageSize.height));

        return (cv::Mat_<double>(3, 3) <<
            focalLength, 0, imageSize.width / 2.0,
            0, focalLength, imageSize.height / 2.0,
            0, 0, 1);
    }

    static bool estimate(
        const std::vector<cv::Point2f>& imageCorners,
        const cv::Mat& cameraMatrix,
        double markerSize,
        cv::Mat& homography,
        cv::Mat& rotation,
        cv::Mat& translation
    ) {
        if (imageCorners.size() != 4) {
            return false;
        }

        const float halfSize = static_cast<float>(markerSize / 2.0);
        const std::vector<cv::Point3f> objectCorners = {
            {-halfSize, -halfSize, 0},
            { halfSize, -halfSize, 0},
            { halfSize,  halfSize, 0},
            {-halfSize,  halfSize, 0}
        };
        const std::vector<cv::Point2f> markerCorners = {
            {-halfSize, -halfSize},
            { halfSize, -halfSize},
            { halfSize,  halfSize},
            {-halfSize,  halfSize}
        };
        const cv::Mat distortion = cv::Mat::zeros(5, 1, CV_64F);

        homography = cv::findHomography(
            markerCorners,
            imageCorners,
            cv::RANSAC,
            3.0
        );
        if (homography.empty() ||
            !cv::solvePnP(
                objectCorners,
                imageCorners,
                cameraMatrix,
                distortion,
                rotation,
                translation,
                false,
                cv::SOLVEPNP_ITERATIVE)) {
            return false;
        }

        cv::solvePnPRefineLM(
            objectCorners,
            imageCorners,
            cameraMatrix,
            distortion,
            rotation,
            translation
        );
        return true;
    }

    static cv::Vec3d rotationDegrees(const cv::Mat& rotation) {
        cv::Mat matrix;
        cv::Rodrigues(rotation, matrix);

        const double yScale = std::hypot(
            matrix.at<double>(0, 0),
            matrix.at<double>(1, 0)
        );
        const bool gimbalLock = yScale < 1e-6;
        const double x = std::atan2(
            gimbalLock ? -matrix.at<double>(1, 2) : matrix.at<double>(2, 1),
            gimbalLock ? matrix.at<double>(1, 1) : matrix.at<double>(2, 2)
        );
        const double y = std::atan2(-matrix.at<double>(2, 0), yScale);
        const double z = gimbalLock
            ? 0.0
            : std::atan2(matrix.at<double>(1, 0), matrix.at<double>(0, 0));

        constexpr double degreesPerRadian = 180.0 / CV_PI;
        return {
            x * degreesPerRadian,
            y * degreesPerRadian,
            z * degreesPerRadian
        };
    }
};
