#include "CameraHandler.hpp"

#include <opencv2/opencv.hpp>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <opencv2/imgproc.hpp>

#if __has_include(<opencv2/geometry.hpp>)
#include <opencv2/geometry.hpp>
#endif

#include <iostream>
#include <string>
#include <vector>

CameraHandler::CameraHandler()
    : soundSources{
        AudioSource("resources/pipNoise.mp3"),
        AudioSource("resources/pipNoise2.mp3"),
        AudioSource("resources/pipNoise3.mp3"),
        AudioSource("resources/pipNoise4.mp3")
    } {
}

void CameraHandler::run() {

    if (!markerMatcher.loadTemplates("resources")) {

        std::cerr
            << "ERROR: Could not load Hiro templates!\n";

        return;
    }

    cv::VideoCapture cap(0);

    if (!cap.isOpened()) {

        std::cerr
            << "ERROR: Could not open camera!\n";

        return;
    }

    std::cout
        << "Press q to quit, s to save evidence.\n";

    cv::Mat frame, gray, blurred, thresh;

    const cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(3, 3)
    );

    // Keep the last valid detection briefly if recognition drops out.
    using Clock = std::chrono::steady_clock;
    constexpr auto holdTime = std::chrono::milliseconds(200);
    bool hasPreviousDetection = false;
    Clock::time_point lastDetection;
    double lastProximity = 0.0;
    double lastRotation = 0.0;
    std::vector<cv::Point> lastCorners;

    unsigned long frameNumber = 0;
    unsigned long acceptedFrames = 0;

    while (cap.read(frame) && !frame.empty()) {

        ++frameNumber;

        cv::cvtColor(
            frame,
            gray,
            cv::COLOR_BGR2GRAY
        );

        // Reduce noise and select the threshold from the current frame.
        cv::GaussianBlur(gray, blurred, cv::Size(3, 3), 0);

        cv::threshold(
            blurred,
            thresh,
            0,
            255,
            cv::THRESH_BINARY_INV | cv::THRESH_OTSU
        );

        // Close small gaps in the inverted black marker border.
        cv::morphologyEx(thresh, thresh, cv::MORPH_CLOSE, kernel);

        // Preserve the original frame.
        cv::Mat display = frame.clone();

        cv::Mat candidateView = cv::Mat::zeros(160, 160, CV_8UC1);

        cv::Mat warpedImage = cv::Mat::zeros(
            400,
            400,
            frame.type()
        );

        cv::Mat isolatedMarkerPixels =
            cv::Mat::zeros(
                thresh.size(),
                CV_8UC1
            );

        // Detect possible markers.
        const auto candidates =
            cornerDetector.detectCorners(
                gray,
                thresh
            );

        int selected = -1;

        MarkerMatchResult best;

        best.reason = "No geometric candidate";

        // Evaluate every candidate.
        for (std::size_t i = 0;
             i < candidates.size();
             ++i) {

            const auto& candidate =
                candidates[i];

            MarkerMatchResult match =
                markerMatcher.recognize(
                    gray,
                    candidate.corners
                );

            // Green: recognized marker.
            // Red: rejected geometric candidate.
            cv::Scalar contourColor;

            if (match.matched) {
                contourColor = cv::Scalar(0, 255, 0);
            } else {
                contourColor = cv::Scalar(0, 0, 255);
            }

            cv::polylines(
                display,
                candidate.contour,
                true,
                contourColor,
                2
            );

            // Prefer a recognized marker.
            // Otherwise retain the best rejected one.
            if (selected < 0 ||
                (match.matched && !best.matched) ||
                (match.matched == best.matched &&
                 match.similarity > best.similarity)) {

                selected = static_cast<int>(i);
                best = match;
            }
        }

        if (!best.normalizedCandidate.empty()) {

            candidateView =
                best.normalizedCandidate;
        }

        bool markerDetected = false;

        double proximity = 0.0;
        double rotation = 0.0;

        if (selected >= 0 && best.matched) {

            const auto& candidate =
                candidates[selected];

            auto corners = candidate.corners;

            // Restore physical corner correspondence.
            if (rotationEstimator.correctCornerOrder(
                    corners,
                    best.rotationIndex
                )) {

                rotation =
                    rotationEstimator.estimateRotation(
                        corners
                    );

                // Create an upright marker image.
                cv::Mat upright =
                    markerMatcher.rectify(
                        frame,
                        corners,
                        400
                    );

                if (std::isfinite(rotation) &&
                    !upright.empty()) {

                    markerDetected = true;

                    warpedImage = upright;

                    double area = cv::contourArea(
                        candidate.contour
                    );

                    proximity = distanceEstimator.estimateProximity(area);

                    std::vector<cv::Point> points;

                    for (const auto& p : corners) {

                        points.emplace_back(
                            cvRound(p.x),
                            cvRound(p.y)
                        );
                    }

                    lastCorners = points;

                    // Draw recognized marker.
                    cv::polylines(
                        display,
                        points,
                        true,
                        cv::Scalar(0, 255, 0),
                        3
                    );

                    // Draw the physical A-B edge.
                    cv::arrowedLine(
                        display,
                        points[0],
                        points[1],
                        cv::Scalar(255, 0, 0),
                        3
                    );

                    // Label the physical corners.
                    for (int i = 0; i < 4; ++i) {

                        cv::circle(
                            display,
                            points[i],
                            5,
                            cv::Scalar(0, 0, 255),
                            cv::FILLED
                        );

                        cv::putText(
                            display,
                            std::string(
                                1,
                                static_cast<char>('A' + i)
                            ),
                            points[i] + cv::Point(8, -8),
                            cv::FONT_HERSHEY_SIMPLEX,
                            0.7,
                            cv::Scalar(0, 255, 255),
                            2
                        );
                    }

                    std::string label =
                        best.id + " | " +
                        distanceEstimator.getCategory(
                            proximity
                        ) + " | " +
                        std::to_string(
                            static_cast<int>(
                                proximity * 100
                            )
                        ) + "% | " +
                        std::to_string(
                            static_cast<int>(rotation)
                        ) + " deg";

                    cv::Rect box = cv::boundingRect(
                        candidate.contour
                    );

                    cv::putText(
                        display,
                        label,
                        {
                            box.x,
                            std::max(25, box.y - 10)
                        },
                        cv::FONT_HERSHEY_SIMPLEX,
                        0.6,
                        cv::Scalar(0, 255, 0),
                        2
                    );

                    // Isolate the accepted marker.
                    cv::Mat mask = cv::Mat::zeros(
                        thresh.size(),
                        CV_8UC1
                    );

                    cv::drawContours(
                        mask,
                        std::vector<
                            std::vector<cv::Point>
                        >{points},
                        0,
                        cv::Scalar(255),
                        cv::FILLED
                    );

                    cv::bitwise_and(
                        thresh,
                        mask,
                        isolatedMarkerPixels
                    );
                }
            }
        }

        // Only a new valid recognition refreshes the saved detection.
        const auto now = Clock::now();

        std::size_t soundIndex = 0;
        // Update sound only for valid markers.
        if (markerDetected &&
            best.rotationIndex >= 0 &&
            best.rotationIndex <
                static_cast<int>(soundSources.size())) {

            soundIndex =
                static_cast<std::size_t>(
                    best.rotationIndex
                );
        }

        if (markerDetected) {
            ++acceptedFrames;
            hasPreviousDetection = true;
            lastDetection = now;
            lastProximity = proximity;
            lastRotation = rotation;
        }

        // Tolerate a short dropout, but never refresh the timeout on a miss.
        const bool holdingDetection = !markerDetected &&
            hasPreviousDetection && now - lastDetection <= holdTime;

        if (markerDetected || holdingDetection) {
            mySpeaker.updateFrequency(
                soundSources[soundIndex],
                proximity
            );
        } else {
            hasPreviousDetection = false;
            lastCorners.clear();
            mySpeaker.resetDetection();
        }

        // Yellow means last known position, not a fresh recognition.
        if (holdingDetection) {
            cv::polylines(
                display, lastCorners, true, cv::Scalar(0, 255, 255), 3
            );
        }

        // Explain why recognition failed.
        if (!markerDetected) {

            std::string message;

            if (best.matched) {
                message = "Invalid rotation/rectification";
            } else if (holdingDetection) {
                message = "Holding: " + best.reason;
            } else {
                message = best.reason;
            }

            cv::putText(
                display,
                message,
                {10, 25},
                cv::FONT_HERSHEY_SIMPLEX,
                0.6,
                cv::Scalar(0, 0, 255),
                2
            );
        }

        // Show the processing stages.
        cv::imshow("Camera Feed", display);
        cv::imshow("Grayscale", gray);
        cv::imshow("Thresholded Image", thresh);
        cv::imshow("Recognition Candidate", candidateView);
        cv::imshow("Warped Image", warpedImage);
        cv::imshow("Isolated Mask", isolatedMarkerPixels);

        int key = cv::waitKey(1) & 0xFF;

        if (key == 'q' || key == 'Q') {
            break;
        }

        // Print diagnostic information.
        if (frameNumber % 30 == 0 ||
            key == 's' ||
            key == 'S') {

            std::cout
                << "Frame " << frameNumber
                << " | candidates: "
                << candidates.size()
                << " | " << best.reason
                << '\n';

            for (int i = 0; i < 4; ++i) {

                std::cout
                    << i * 90 << " deg: "
                    << best.scores[i]
                    << '\n';
            }

            std::cout
                << "Best: " << best.similarity
                << " | margin: " << best.margin
                << " | detected: "
                << std::boolalpha
                << markerDetected
                 << " | holding: " << holdingDetection
                 << " | accepted frames: " << acceptedFrames
                 << "/" << frameNumber;

            if (markerDetected) {

                std::cout
                    << " | angle: "
                    << rotation;
            }

            std::cout << '\n';
        }

        // Save repeatable test evidence.
        if (key == 's' || key == 'S') {

            std::string prefix =
                "evidence_" +
                std::to_string(frameNumber);

            bool saved = cv::imwrite(
                prefix + "_original.png",
                frame
            );

            saved = cv::imwrite(
                prefix + "_threshold.png",
                thresh
            ) && saved;

            saved = cv::imwrite(
                prefix + "_candidate.png",
                candidateView
            ) && saved;

            saved = cv::imwrite(
                prefix + "_overlay.png",
                display
            ) && saved;

            if (saved) {
                std::cout << "Evidence saved: " << prefix << '\n';
            } else {
                std::cout << "Evidence save failed: " << prefix << '\n';
            }
        }
    }

    mySpeaker.resetDetection();

    cap.release();

    cv::destroyAllWindows();
}
