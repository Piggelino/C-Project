#include <iostream>
#include <cstdlib> 
#include <fstream>
#include <string> 
#include <vector>

#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
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
    bool isPlaying;
public:
    AudioOutput() {
        isPlaying = false;
    }
    void play(AudioSource& source) {
        if (!isPlaying) {
            std::string command = "afplay " + source.getFilePath() + " &";
            system(command.c_str());
            isPlaying = true;
            std::cout << "Spelar ljud: " << source.getFilePath() << std::endl;
        }
    }
    void resetDetection() {
        isPlaying = false;
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

bool loadMarkerContour(
    const std::vector<std::string>& paths,
    std::vector<cv::Point>& markerContour
) {
    for (const std::string& path : paths) {
        std::ifstream markerFile(path);
        if (!markerFile.good()) {
            continue;
        }

        cv::Mat marker = cv::imread(path, cv::IMREAD_GRAYSCALE);
        if (marker.empty()) {
            continue;
        }

        cv::Mat markerThreshold;
        cv::threshold(
            marker,
            markerThreshold,
            80,
            255,
            cv::THRESH_BINARY_INV
        );

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(
            markerThreshold,
            contours,
            cv::RETR_EXTERNAL,
            cv::CHAIN_APPROX_SIMPLE
        );

        double largestArea = 0.0;
        for (const std::vector<cv::Point>& contour : contours) {
            double area = cv::contourArea(contour);
            if (area > largestArea) {
                largestArea = area;
                markerContour = contour;
            }
        }

        if (!markerContour.empty()) {
            return true;
        }
    }

    return false;
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

    std::vector<cv::Point> markerContour;
    if (!loadMarkerContour(
            {"marker.png", "../marker.png", "../../marker.png"},
            markerContour
        )) {
        std::cerr << "ERROR: Could not load marker.png." << std::endl;
        return -1;
    }

    cv::Mat frame;
    cv::Mat gray, thresh;

    AudioSource mySource("pipNoise.mp3");
    AudioOutput mySpeaker;

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

            bool isConvex = approximatedContour.size() == 4 &&
                            cv::isContourConvex(approximatedContour);
            if (!isConvex) {
                continue;
            }

            double shapeDifference = cv::matchShapes(
                approximatedContour,
                markerContour,
                cv::CONTOURS_MATCH_I1,
                0.0
            );
            if (shapeDifference > 0.05) {
                continue;
            }

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

            std::cout << "Detected corners:\n";
            std::cout << "  Top-left:     (" << corners[0].x << ", "
                 << corners[0].y << ")\n";
            std::cout << "  Top-right:    (" << corners[1].x << ", "
                << corners[1].y << ")\n";
            std::cout << "  Bottom-right: (" << corners[2].x << ", "
                << corners[2].y << ")\n";
            std::cout << "  Bottom-left:  (" << corners[3].x << ", "
                << corners[3].y << ")\n";

            markerDetected = true;

            std::vector<cv::Point> displayCorners;
            for (const cv::Point2f& corner : corners) {
                displayCorners.emplace_back(
                    cv::Point(cvRound(corner.x), cvRound(corner.y))
                );
            }
            cv::polylines(
                frame,
                displayCorners,
                true,
                cv::Scalar(0, 255, 0),
                3
            );
            break;
        }

        if (markerDetected) {
            mySpeaker.play(mySource);
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