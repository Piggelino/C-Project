#include <algorithm>
#include <iostream>
#include <string>

#include "AudioOutput.hpp"
#include "DistanceEstimator.hpp"

#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#include "MarkerDetector.hpp"
#define OPENCV_AVAILABLE 1
#else
#define OPENCV_AVAILABLE 0
#endif

int main() {
#if !OPENCV_AVAILABLE
    std::cerr << "ERROR: OpenCV headers were not found. "
                 "Install OpenCV and configure the include path.\n";
    return -1;
#else
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "ERROR: Could not open camera!" << std::endl;
        return -1;
    }

    std::cout << "Camera opened successfully. Press 'q' to quit."
              << std::endl;

    cv::Mat frame;
    cv::Mat gray;
    cv::Mat thresholded;
    MarkerDetection detection;
    MarkerDetector markerDetector;
    AudioSource source("pipNoise.mp3");
    AudioOutput speaker;
    DistanceEstimator distanceEstimator;

    while (true) {
        if (!cap.read(frame)) {
            std::cerr << "ERROR: Could not read frame from camera!"
                      << std::endl;
            break;
        }

        const bool markerDetected = markerDetector.detect(
            frame,
            gray,
            thresholded,
            detection
        );

        if (markerDetected) {
            const double proximity =
                distanceEstimator.estimateProximity(detection.area);
            const char* category =
                distanceEstimator.getCategory(proximity);

            cv::polylines(
                frame,
                detection.displayCorners,
                true,
                cv::Scalar(0, 255, 0),
                3
            );

            for (std::size_t i = 0; i < detection.displayCorners.size(); ++i) {
                const cv::Point& corner = detection.displayCorners[i];
                const int cornerNumber =
                    (static_cast<int>(i) - detection.firstCorner + 4) % 4 + 1;
                const std::string number = std::to_string(cornerNumber);
                const cv::Size textSize = cv::getTextSize(
                    number,
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.45,
                    1,
                    nullptr
                );
                cv::putText(
                    frame,
                    number,
                    cv::Point(
                        corner.x - textSize.width / 2,
                        corner.y + textSize.height / 2
                    ),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.45,
                    cv::Scalar(255, 255, 255),
                    1,
                    cv::LINE_AA
                );
            }

            const std::string label =
                std::string(category) + " | " +
                std::to_string(static_cast<int>(proximity * 100)) +
                "% | Area: " +
                std::to_string(static_cast<int>(detection.area)) + " px2";
            cv::putText(
                frame,
                label,
                cv::Point(
                    detection.boundingBox.x,
                    std::max(25, detection.boundingBox.y - 10)
                ),
                cv::FONT_HERSHEY_SIMPLEX,
                0.7,
                cv::Scalar(0, 255, 0),
                2
            );

            if (detection.poseValid) {
                const cv::Vec3d& angles = detection.rotationDegrees;
                const cv::Mat& translation = detection.translation;
                const std::string poseLabel =
                    "R: " +
                    std::to_string(static_cast<int>(angles[0])) + "," +
                    std::to_string(static_cast<int>(angles[1])) + "," +
                    std::to_string(static_cast<int>(angles[2])) +
                    " deg T: " +
                    std::to_string(translation.at<double>(0)) + "," +
                    std::to_string(translation.at<double>(1)) + "," +
                    std::to_string(translation.at<double>(2)) + " m";
                cv::putText(
                    frame,
                    poseLabel,
                    cv::Point(
                        detection.boundingBox.x,
                        std::min(
                            frame.rows - 10,
                            detection.boundingBox.y +
                                detection.boundingBox.height + 25
                        )
                    ),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.45,
                    cv::Scalar(255, 255, 0),
                    1,
                    cv::LINE_AA
                );
            }

            speaker.updateFrequency(source, proximity);
        } else {
            speaker.resetDetection();
        }

        cv::imshow("Camera Feed", frame);
        cv::imshow("Grayscale", gray);
        cv::imshow("Thresholded Image", thresholded);
        cv::imshow("Warped Image", detection.warpedImage);

        const int key = cv::waitKey(1) & 0xFF;
        if (key == 'q' || key == 'Q') {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    std::cout << "Camera closed. Program finished." << std::endl;
    return 0;
#endif
}
