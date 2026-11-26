#include "CameraHandler.hpp"

#include <opencv2/opencv.hpp>
#include <algorithm>
#include <cmath>
#include <opencv2/imgproc.hpp>

#if __has_include(<opencv2/geometry.hpp>)
#include <opencv2/geometry.hpp>
#endif

#include <iostream>
#include <string>
#include <vector>

CameraHandler::CameraHandler()
    : mySource("resources/pipNoise.mp3") {
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

    cv::Mat frame, gray, thresh;

    unsigned long frameNumber = 0;

    while (cap.read(frame) && !frame.empty()) {

        ++frameNumber;

        cv::cvtColor(
            frame,
            gray,
            cv::COLOR_BGR2GRAY
        );

        cv::threshold(
            gray,
            thresh,
            80,
            255,
            cv::THRESH_BINARY_INV
        );

        // Preserve the original frame.
        cv::Mat display = frame.clone();

        cv::Mat candidateView = cv::Mat::zeros(
            160,
            160,
            CV_8UC1
        );

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
            cv::polylines(
                display,
                candidate.contour,
                true,
                match.matched
                    ? cv::Scalar(0, 255, 0)
                    : cv::Scalar(0, 0, 255),
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

                    proximity =
                        distanceEstimator.estimateProximity(
                            area
                        );

                    std::vector<cv::Point> points;

                    for (const auto& p : corners) {

                        points.emplace_back(
                            cvRound(p.x),
                            cvRound(p.y)
                        );
                    }

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

        // Update sound only for valid markers.
        if (markerDetected) {

            mySpeaker.updateFrequency(
                mySource,
                proximity,
                rotation
            );

        } else {

            mySpeaker.resetDetection();
        }

        // Explain why recognition failed.
        if (!markerDetected) {

            cv::putText(
                display,
                best.matched
                    ? "Invalid rotation/rectification"
                    : best.reason,
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
                << markerDetected;

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

            std::cout
                << (saved
                    ? "Evidence saved: "
                    : "Evidence save failed: ")
                << prefix << '\n';
        }
    }

    mySpeaker.resetDetection();

    cap.release();

    cv::destroyAllWindows();
}