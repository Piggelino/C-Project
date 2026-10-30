#include <iostream>
#include <cstdlib> 
#include <cmath>
#include <string> 
#include <vector>
#include "DistanceEstimator.hpp"

#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#define OPENCV_AVAILABLE 1
#else
#define OPENCV_AVAILABLE 0
#endif

class AudioSource {
private:
    std::string filePath;
public:
    AudioSource(std::string path) {
        filePath = path;
    }
    std::string getFilePath() {
        return filePath;
    }
};

class AudioOutput {
private:
    int frameCounter;
public:
    AudioOutput() {
        frameCounter = 0;
    }
    void updateFrequency(AudioSource& source, int blackPixels) {
        frameCounter++;
        int framesToWait = -1;

        if (blackPixels > 2000){
            framesToWait = 2;
        } 
        else if (blackPixels > 1000){
            framesToWait = 10;
        }
        else if (blackPixels > 500){
            framesToWait = 30;
        }
        else if (blackPixels > 100){
            framesToWait = 60;
        }
        if (framesToWait != -1 && frameCounter >= framesToWait) {
            std::string command = "afplay " + source.getFilePath() + " &";
            system(command.c_str());
            frameCounter = 0;
            std::cout << "Spelar ljud med dynamisk frekvens: " << source.getFilePath() << std::endl;
        }
    }
    void resetDetection() {
        frameCounter = 0;
    }
};


#if OPENCV_AVAILABLE
bool orderCorners(
    const std::vector<cv::Point>& points,
    std::vector<cv::Point2f>& ordered
) {
    if (points.size() != 4) {
        return false;
    }

    ordered.clear();
    ordered.reserve(4);

    int topLeft = 0;
    int topRight = 0;
    int bottomRight = 0;
    int bottomLeft = 0;

    float smallestSum = 0.0f;
    float largestSum = 0.0f;
    float smallestDifference = 0.0f;
    float largestDifference = 0.0f;

    for (std::size_t i = 0; i < points.size(); ++i) {
        float sum = static_cast<float>(points[i].x + points[i].y);
        float difference = static_cast<float>(points[i].x - points[i].y);

        if (i == 0 || sum < smallestSum) {
            smallestSum = sum;
            topLeft = static_cast<int>(i);
        }
        if (i == 0 || sum > largestSum) {
            largestSum = sum;
            bottomRight = static_cast<int>(i);
        }
        if (i == 0 || difference > largestDifference) {
            largestDifference = difference;
            topRight = static_cast<int>(i);
        }
        if (i == 0 || difference < smallestDifference) {
            smallestDifference = difference;
            bottomLeft = static_cast<int>(i);
        }
    }

    if (topLeft == topRight || topLeft == bottomRight ||
        topLeft == bottomLeft || topRight == bottomRight ||
        topRight == bottomLeft || bottomRight == bottomLeft) {
        return false;
    }

    ordered = {
        cv::Point2f(points[topLeft]),
        cv::Point2f(points[topRight]),
        cv::Point2f(points[bottomRight]),
        cv::Point2f(points[bottomLeft])
    };

    return true;
}
#endif

int main() {
#if !OPENCV_AVAILABLE
    std::cerr << "ERROR: OpenCV headers were not found. Install OpenCV and configure the include path.\n";
    return -1;
#else
    // Try to open the default camera (index 0)
    cv::VideoCapture cap(0);

    if (!cap.isOpened()) {
        std::cerr << "ERROR: Could not open camera!" << std::endl;
        return -1;
    }

    std::cout << "Camera opened successfully. Press 'q' to quit." << std::endl;

    cv::Mat marker_template = cv::imread("hiro_template.png", cv::IMREAD_GRAYSCALE);
    if (marker_template.empty()) {
        std::cerr << "ERROR: Could not load hiro_template.png" << std::endl;
        return -1;
    }

    constexpr int warpedMarkerSize = 300;
    // Säkerställ att mallen är exakt 300x300 pixlar för att matcha warpedMarker
    cv::resize(marker_template, marker_template, cv::Size(warpedMarkerSize, warpedMarkerSize));

    cv::Mat frame;
    cv::Mat gray, thresh;

    AudioSource mySource("pipNoise.mp3");
    AudioOutput mySpeaker;
    DistanceEstimator distanceEstimator;

    while (true) {
        // Grab a frame
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
        int blackPixels = 0;

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

            // Map a canonical, square marker to the detected quadrilateral.
            // The top edge of the mapped square gives the marker's image-plane
            // rotation. This is not the full 3D pose rotation.

            const std::vector<cv::Point2f> referenceCorners = {
                cv::Point2f(0.0f, 0.0f),
                cv::Point2f(static_cast<float>(warpedMarkerSize - 1), 0.0f),
                cv::Point2f(
                    static_cast<float>(warpedMarkerSize - 1),
                    static_cast<float>(warpedMarkerSize - 1)
                ),
                cv::Point2f(0.0f, static_cast<float>(warpedMarkerSize - 1))
            };

            cv::Mat homography = cv::findHomography(
                referenceCorners,
                corners,
                cv::RANSAC
            );

            double rotationDegrees = 0.0;

            double bestMatchVAl = -1.0; 


            if (!homography.empty()) {
                cv::Mat warpedMarker;

                // H maps the reference square to the camera image. Using
                // WARP_INVERSE_MAP maps the camera image back to the square.
                cv::warpPerspective(
                    gray,
                    warpedMarker,
                    homography,
                    cv::Size(warpedMarkerSize, warpedMarkerSize),
                    cv::INTER_LINEAR | cv::WARP_INVERSE_MAP,
                    cv::BORDER_CONSTANT,
                    cv::Scalar(0, 0, 0)
                );

                cv::imshow("Warped Marker", warpedMarker);

                cv::Mat rotatedWarp;
                warpedMarker.copyTo(rotatedWarp);

                // Testa alla 4 möjliga rotationer av den upprätade markören
                for (int rot = 0; rot < 4; ++rot) {
                    cv::Mat match_result;
                    cv::matchTemplate(rotatedWarp, marker_template, match_result, cv::TM_CCOEFF_NORMED);
                    
                    double minVal, maxVal;
                    cv::minMaxLoc(match_result, &minVal, &maxVal);
                    
                    if (maxVal > bestMatchVal) {
                        bestMatchVal = maxVal;
                    }

                    // Rotera bilden 90 grader inför nästa iteration
                    cv::rotate(rotatedWarp, rotatedWarp, cv::ROTATE_90_CLOCKWISE);
                }
                if (bestMatchVal < 0.6) {
                    continue; // Detta var en fyrkant, men inte Hiro
                }

                std::vector<cv::Point2f> mappedTopEdge;
                cv::perspectiveTransform(
                    std::vector<cv::Point2f>{
                        referenceCorners[0],
                        referenceCorners[1]
                    },
                    mappedTopEdge,
                    homography
                );

                const cv::Point2f topEdge = mappedTopEdge[1] - mappedTopEdge[0];
                rotationDegrees = std::atan2(topEdge.y, topEdge.x) *
                                  180.0 / CV_PI;
            }else {
                continue; // Homography misslyckades, hoppa över denna kontur
            }

            std::cout << "Detected corners:\n";
            std::cout << "  Top-left:     (" << corners[0].x << ", "
                      << corners[0].y << ")\n";
            std::cout << "  Top-right:    (" << corners[1].x << ", "
                      << corners[1].y << ")\n";
            std::cout << "  Bottom-right: (" << corners[2].x << ", "
                      << corners[2].y << ")\n";
            std::cout << "  Bottom-left:  (" << corners[3].x << ", "
                      << corners[3].y << ")\n";
            std::cout << "  Rotation:     " << rotationDegrees
                      << " degrees\n";

            // Measure the marker's area in square pixels.
            double markerArea = cv::contourArea(approximatedContour);

            // Estimate proximity using the distance estimator.
            double proximity = distanceEstimator.estimateProximity(markerArea);
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
                std::to_string(static_cast<int>(bestMatchVal * 100)) + "% | Rot: " +
                std::to_string(static_cast<int>(proximity * 100)) + "% | Area: " +
                std::to_string(static_cast<int>(markerArea)) + " px2 | Rotation: " +
                std::to_string(static_cast<int>(rotationDegrees)) + " deg";

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
            blackPixels = cv::countNonZero(isolatedMarkerPixels);

            cv::imshow("Isolated Mask", isolatedMarkerPixels);// prof of concept, can be removed later

            break; 
        }

        if (markerDetected) {
            
            mySpeaker.updateFrequency(mySource, blackPixels);
        } else {
            mySpeaker.resetDetection();
        }

        // Show the original camera image, grayscale image, and threshold image.
        cv::imshow("Camera Feed", frame);
        cv::imshow("Grayscale", gray);
        cv::imshow("Thresholded Image", thresh);

        // Exit when user presses 'q'
        int key = (int)cv::waitKey(1) & 0xFF;
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