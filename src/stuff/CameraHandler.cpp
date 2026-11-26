#include "CameraHandler.hpp"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#if __has_include(<opencv2/geometry/2d.hpp>)
#include <opencv2/geometry/2d.hpp>
#endif

CameraHandler::CameraHandler()
    : mySource("resources/pipNoise.mp3") {
}

void CameraHandler::run() {

    cv::VideoCapture cap(0);

    if (!cap.isOpened()) {
        std::cerr << "ERROR: Could not open camera!" << std::endl;
        return;
    }

    std::cout << "Camera opened successfully. Press 'q' to quit." << std::endl;

    cv::Mat frame;
    cv::Mat gray, thresh;
    cv::Mat warpedImage;

    if (!markerMatcher.loadTemplate("resources/marker2.png.webp")) {
        std::cerr << "ERROR: Could not load Hiro template!" << std::endl;
        return;
    }

    while (true) {

        if (!cap.read(frame)) {
            std::cerr << "ERROR: Could not read frame from camera!" << std::endl;
            break;
        }

        // Convert to grayscale
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // Apply Thresholding
        cv::threshold(gray, thresh, 80, 255, cv::THRESH_BINARY_INV);

        bool markerDetected = false;
        double proximity = 0.0;
        warpedImage = cv::Mat::zeros(400, 400, frame.type());

        // Detect four-corner candidates using the dedicated detector.
        const std::vector<CornerCandidate> candidates =
            cornerDetector.detectCorners(gray, thresh);

        for (const CornerCandidate& candidate : candidates) {

            const auto& corners = candidate.corners;
            const auto& approximatedContour = candidate.contour;

            if (!markerMatcher.matches(gray, corners)) {
                continue;
            }

            const std::vector<cv::Point2f> warpedCorners = {
                cv::Point2f(0.0f, 0.0f),
                cv::Point2f(399.0f, 0.0f),
                cv::Point2f(399.0f, 399.0f),
                cv::Point2f(0.0f, 399.0f)
            };

            const cv::Mat perspectiveTransform =
                cv::getPerspectiveTransform(corners, warpedCorners);

            cv::warpPerspective(
                frame,
                warpedImage,
                perspectiveTransform,
                cv::Size(400, 400)
            );

            // Measure the marker's area in square pixels.
            double markerArea = cv::contourArea(approximatedContour);

            // Estimate proximity using the distance estimator.
            proximity = distanceEstimator.estimateProximity(markerArea);

            const char* category =
                distanceEstimator.getCategory(proximity);

            markerDetected = true;

            // Draw the refined marker outline.
            std::vector<cv::Point> displayCorners;

            for (const cv::Point2f& corner : corners) {
                displayCorners.emplace_back(
                    cvRound(corner.x),
                    cvRound(corner.y)
                );
            }

            cv::polylines(
                frame,
                displayCorners,
                true,
                cv::Scalar(0, 255, 0),
                3
            );

            // Display proximity and area on the camera feed.
            std::string label =
                std::string(category) + " | " +
                std::to_string(static_cast<int>(proximity * 100)) +
                "% | Area: " +
                std::to_string(static_cast<int>(markerArea)) + " px2";

            cv::Rect box = cv::boundingRect(approximatedContour);

            cv::Point textPosition(
                box.x,
                std::max(25, box.y - 10)
            );

            cv::putText(
                frame,
                label,
                textPosition,
                cv::FONT_HERSHEY_SIMPLEX,
                0.7,
                cv::Scalar(0, 255, 0),
                2
            );

            cv::Mat mask = cv::Mat::zeros(thresh.size(), CV_8UC1);

            std::vector<std::vector<cv::Point>> maskContour = {
                displayCorners
            };

            cv::drawContours(
                mask,
                maskContour,
                0,
                cv::Scalar(255),
                cv::FILLED
            );

            cv::Mat isolatedMarkerPixels;

            cv::bitwise_and(thresh, mask, isolatedMarkerPixels);

            cv::imshow("Isolated Mask", isolatedMarkerPixels);

            break;
        }

        if (markerDetected) {
            mySpeaker.updateFrequency(mySource, proximity);
        } else {
            mySpeaker.resetDetection();
        }

        // Display camera images.
        cv::imshow("Camera Feed", frame);
        cv::imshow("Grayscale", gray);
        cv::imshow("Thresholded Image", thresh);
        cv::imshow("Warped Image", warpedImage);

        // Exit when user presses 'q'.
        int key = (int)cv::waitKey(1) & 0xFF;

        if (key == 'q' || key == 'Q') {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();

    std::cout << "Camera closed. Program finished." << std::endl;
}
