#pragma once

#include <algorithm>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

class HiroDetector {
public:
    bool findTextCenter(
        const cv::Mat& warpedImage,
        cv::Point2f& textCenter
    ) const {
        cv::Mat gray;
        cv::cvtColor(warpedImage, gray, cv::COLOR_BGR2GRAY);

        cv::Mat whitePixels;
        cv::threshold(gray, whitePixels, 150, 255, cv::THRESH_BINARY);

        cv::Mat labels;
        cv::Mat componentStats;
        cv::Mat componentCentroids;
        const int componentCount = cv::connectedComponentsWithStats(
            whitePixels,
            labels,
            componentStats,
            componentCentroids
        );

        int innerComponent = -1;
        int innerArea = 0;
        for (int component = 1; component < componentCount; ++component) {
            const int x = componentStats.at<int>(
                component,
                cv::CC_STAT_LEFT
            );
            const int y = componentStats.at<int>(
                component,
                cv::CC_STAT_TOP
            );
            const int width = componentStats.at<int>(
                component,
                cv::CC_STAT_WIDTH
            );
            const int height = componentStats.at<int>(
                component,
                cv::CC_STAT_HEIGHT
            );
            const int area = componentStats.at<int>(
                component,
                cv::CC_STAT_AREA
            );

            if (x > 0 && y > 0 && x + width < warpedImage.cols &&
                y + height < warpedImage.rows && area > innerArea &&
                width > warpedImage.cols / 5 &&
                height > warpedImage.rows / 5) {
                innerComponent = component;
                innerArea = area;
            }
        }

        if (innerComponent == -1) {
            return false;
        }

        const int x = componentStats.at<int>(
            innerComponent,
            cv::CC_STAT_LEFT
        );
        const int y = componentStats.at<int>(
            innerComponent,
            cv::CC_STAT_TOP
        );
        const int width = componentStats.at<int>(
            innerComponent,
            cv::CC_STAT_WIDTH
        );
        const int height = componentStats.at<int>(
            innerComponent,
            cv::CC_STAT_HEIGHT
        );
        const int insetX = std::max(1, width / 10);
        const int insetY = std::max(1, height / 10);
        const cv::Rect textRegion(
            x + insetX,
            y + insetY,
            width - 2 * insetX,
            height - 2 * insetY
        );

        cv::Mat darkText;
        cv::threshold(
            gray(textRegion),
            darkText,
            0,
            255,
            cv::THRESH_BINARY_INV | cv::THRESH_OTSU
        );

        if (cv::countNonZero(darkText) <
            static_cast<int>(darkText.total() * 0.002)) {
            return false;
        }

        const cv::Moments moments = cv::moments(darkText, true);
        if (moments.m00 <= 0.0) {
            return false;
        }

        textCenter = cv::Point2f(
            static_cast<float>(textRegion.x + moments.m10 / moments.m00),
            static_cast<float>(textRegion.y + moments.m01 / moments.m00)
        );
        return true;
    }
};
