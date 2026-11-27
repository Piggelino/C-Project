#include "CameraHandler.hpp"

#include <opencv2/opencv.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#if __has_include(<opencv2/geometry.hpp>)
#include <opencv2/geometry.hpp>
#endif

namespace {

struct BestCandidate {
    int index = -1;
    MarkerMatchResult match;
};

struct Detection {
    bool found = false;
    double proximity = 0.0;
    double rotation = 0.0;
    cv::Mat warpedImage;
    cv::Mat isolatedPixels;
};

BestCandidate findBestCandidate(
    const cv::Mat& gray,
    const std::vector<CornerCandidate>& candidates,
    cv::Mat& display,
    cv::Mat& candidateView,
    const MarkerTemplateMatcher& matcher
) {
    BestCandidate best;
    best.match.reason = "No geometric candidate";

    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const auto& candidate = candidates[i];
        auto match = matcher.recognize(gray, candidate.corners);

        cv::polylines(
            display,
            candidate.contour,
            true,
            match.matched ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255),
            2
        );

        const bool betterMatch =
            best.index < 0 ||
            (match.matched && !best.match.matched) ||
            (match.matched == best.match.matched &&
             match.similarity > best.match.similarity);

        if (betterMatch) {
            best.index = static_cast<int>(i);
            best.match = std::move(match);
        }
    }

    if (!best.match.normalizedCandidate.empty()) {
        candidateView = best.match.normalizedCandidate;
    }

    return best;
}

void drawDetection(
    const CornerCandidate& candidate,
    const MarkerMatchResult& match,
    const cv::Mat& frame,
    const cv::Mat& thresh,
    cv::Mat& display,
    RotationEstimator& rotationEstimator,
    DistanceEstimator& distanceEstimator,
    Detection& detection
) {
    auto corners = candidate.corners;
    if (!rotationEstimator.correctCornerOrder(corners, match.rotationIndex)) {
        return;
    }

    detection.rotation = rotationEstimator.estimateRotation(corners);
    detection.warpedImage = MarkerTemplateMatcher::rectify(frame, corners, 400);
    if (!std::isfinite(detection.rotation) || detection.warpedImage.empty()) {
        return;
    }

    detection.found = true;
    detection.proximity = distanceEstimator.estimateProximity(
        cv::contourArea(candidate.contour)
    );

    std::vector<cv::Point> points;
    points.reserve(corners.size());
    for (const auto& corner : corners) {
        points.emplace_back(cvRound(corner.x), cvRound(corner.y));
    }

    cv::polylines(display, points, true, cv::Scalar(0, 255, 0), 3);
    cv::arrowedLine(display, points[0], points[1], cv::Scalar(255, 0, 0), 3);

    for (int i = 0; i < 4; ++i) {
        cv::circle(display, points[i], 5, cv::Scalar(0, 0, 255), cv::FILLED);
        cv::putText(
            display,
            std::string(1, static_cast<char>('A' + i)),
            points[i] + cv::Point(8, -8),
            cv::FONT_HERSHEY_SIMPLEX,
            0.7,
            cv::Scalar(0, 255, 255),
            2
        );
    }

    const std::string label =
        match.id + " | " +
        distanceEstimator.getCategory(detection.proximity) + " | " +
        std::to_string(static_cast<int>(detection.proximity * 100)) + "% | " +
        std::to_string(static_cast<int>(detection.rotation)) + " deg";
    const auto box = cv::boundingRect(candidate.contour);

    cv::putText(
        display,
        label,
        {box.x, std::max(25, box.y - 10)},
        cv::FONT_HERSHEY_SIMPLEX,
        0.6,
        cv::Scalar(0, 255, 0),
        2
    );

    cv::Mat mask = cv::Mat::zeros(thresh.size(), CV_8UC1);
    cv::drawContours(
        mask,
        std::vector<std::vector<cv::Point>>{points},
        0,
        cv::Scalar(255),
        cv::FILLED
    );
    cv::bitwise_and(thresh, mask, detection.isolatedPixels);
}

void printDiagnostics(
    unsigned long frameNumber,
    std::size_t candidateCount,
    const MarkerMatchResult& match,
    const Detection& detection
) {
    std::cout << "Frame " << frameNumber
              << " | candidates: " << candidateCount
              << " | " << match.reason << '\n';

    for (int i = 0; i < 4; ++i) {
        std::cout << i * 90 << " deg: " << match.scores[i] << '\n';
    }

    std::cout << "Best: " << match.similarity
              << " | margin: " << match.margin
              << " | detected: " << std::boolalpha << detection.found;
    if (detection.found) {
        std::cout << " | angle: " << detection.rotation;
    }
    std::cout << '\n';
}

void saveEvidence(
    unsigned long frameNumber,
    const cv::Mat& frame,
    const cv::Mat& thresh,
    const cv::Mat& candidateView,
    const cv::Mat& display
) {
    const std::string prefix = "evidence_" + std::to_string(frameNumber);
    const std::vector<std::pair<std::string, const cv::Mat*>> images = {
        {"_original.png", &frame},
        {"_threshold.png", &thresh},
        {"_candidate.png", &candidateView},
        {"_overlay.png", &display}
    };

    bool saved = true;
    for (const auto& image : images) {
        saved = cv::imwrite(prefix + image.first, *image.second) && saved;
    }

    std::cout << (saved ? "Evidence saved: " : "Evidence save failed: ")
              << prefix << '\n';
}

} // namespace

CameraHandler::CameraHandler()
    : soundSources{
        AudioSource("resources/pipNoise1.mp3"),
        AudioSource("resources/pipNoise2.mp3"),
        AudioSource("resources/pipNoise3.mp3"),
        AudioSource("resources/pipNoise4.mp3")
    } {}

void CameraHandler::run() {
    if (!markerMatcher.loadTemplates("resources")) {
        std::cerr << "ERROR: Could not load Hiro templates!\n";
        return;
    }

    cv::VideoCapture camera(0);
    if (!camera.isOpened()) {
        std::cerr << "ERROR: Could not open camera!\n";
        return;
    }

    std::cout << "Press q to quit, s to save evidence.\n";

    cv::Mat frame, gray, thresh;
    unsigned long frameNumber = 0;

    while (camera.read(frame) && !frame.empty()) {
        ++frameNumber;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(gray, thresh, 80, 255, cv::THRESH_BINARY_INV);

        cv::Mat display = frame.clone();
        cv::Mat candidateView = cv::Mat::zeros(160, 160, CV_8UC1);
        Detection detection;
        detection.warpedImage = cv::Mat::zeros(400, 400, frame.type());
        detection.isolatedPixels = cv::Mat::zeros(thresh.size(), CV_8UC1);

        const auto candidates = cornerDetector.detectCorners(gray, thresh);
        const auto best = findBestCandidate(
            gray, candidates, display, candidateView, markerMatcher
        );

        if (best.index >= 0 && best.match.matched) {
            drawDetection(
                candidates[best.index],
                best.match,
                frame,
                thresh,
                display,
                rotationEstimator,
                distanceEstimator,
                detection
            );
        }

        if (detection.found &&
            best.match.rotationIndex >= 0 &&
            best.match.rotationIndex < static_cast<int>(soundSources.size())) {
            mySpeaker.updateFrequency(
                soundSources[best.match.rotationIndex],
                detection.proximity
            );
        } else {
            mySpeaker.resetDetection();
        }

        if (!detection.found) {
            cv::putText(
                display,
                best.match.matched
                    ? "Invalid rotation/rectification"
                    : best.match.reason,
                {10, 25},
                cv::FONT_HERSHEY_SIMPLEX,
                0.6,
                cv::Scalar(0, 0, 255),
                2
            );
        }

        cv::imshow("Camera Feed", display);
        cv::imshow("Recognition Candidate", candidateView);
        cv::imshow("Warped Image", detection.warpedImage);
        cv::imshow("Isolated Mask", detection.isolatedPixels);

        const int key = cv::waitKey(1) & 0xFF;
        if (key == 'q' || key == 'Q') {
            break;
        }

        if (frameNumber % 30 == 0 || key == 's' || key == 'S') {
            printDiagnostics(frameNumber, candidates.size(), best.match, detection);
        }
        if (key == 's' || key == 'S') {
            saveEvidence(frameNumber, frame, thresh, candidateView, display);
        }
    }

    mySpeaker.resetDetection();
    camera.release();
    cv::destroyAllWindows();
}
