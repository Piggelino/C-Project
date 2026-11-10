#include "CameraHandler.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#if __has_include(<opencv2/geometry/2d.hpp>)
#include <opencv2/geometry/2d.hpp>
#endif

#include "CornerDetector.hpp"

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

        // A contour is a line around a connected white area in the threshold image.
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(
            thresh,
            contours,
            cv::RETR_EXTERNAL,
            cv::CHAIN_APPROX_SIMPLE
        );

        bool markerDetected = false;
        double proximity = 0.0;
        warpedImage = cv::Mat::zeros(400, 400, frame.type());

        // Find the first valid four-corner contour.
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

            std::cout << "Detected corners:\n";
            std::cout << "  Top-left:     (" << corners[0].x << ", "
                      << corners[0].y << ")\n";
            std::cout << "  Top-right:    (" << corners[1].x << ", "
                      << corners[1].y << ")\n";
            std::cout << "  Bottom-right: (" << corners[2].x << ", "
                      << corners[2].y << ")\n";
            std::cout << "  Bottom-left:  (" << corners[3].x << ", "
                      << corners[3].y << ")\n";

            // Measure the marker's area in square pixels.
            double markerArea = cv::contourArea(approximatedContour);

            // Estimate proximity using the distance estimator.
            proximity = distanceEstimator.estimateProximity(markerArea);
            const char* category = distanceEstimator.getCategory(proximity);

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
                std::to_string(static_cast<int>(proximity * 100)) + "% | Area: " +
                std::to_string(static_cast<int>(markerArea)) + " px2";

            cv::Rect box = cv::boundingRect(approximatedContour);
            cv::Point textPosition(box.x, std::max(25, box.y - 10));

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
            std::vector<std::vector<cv::Point>> maskContour = {displayCorners};
            cv::drawContours(mask, maskContour, 0, cv::Scalar(255), cv::FILLED);

            cv::Mat isolatedMarkerPixels;
            
            cv::bitwise_and(thresh, mask, isolatedMarkerPixels);
            cv::imshow("Isolated Mask", isolatedMarkerPixels);// prof of concept, can be removed later

            break; 
        }

        if (markerDetected) {
            
            mySpeaker.updateFrequency(mySource, proximity);
        } else {
            mySpeaker.resetDetection();
        }

        // the original camera image, grayscale image, and threshold image.
        cv::imshow("Camera Feed", frame);
        cv::imshow("Grayscale", gray);
        cv::imshow("Thresholded Image", thresh);
        cv::imshow("Warped Image", warpedImage);

        // Exit when user presses 'q'
        int key = (int)cv::waitKey(1) & 0xFF;
        if (key == 'q' || key == 'Q') {
            break;
        }

    }

    cap.release();
    cv::destroyAllWindows();

    std::cout << "Camera closed. Program finished." << std::endl;
}