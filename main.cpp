#include <iostream>

#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#define OPENCV_AVAILABLE 1
#else
#define OPENCV_AVAILABLE 0
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

    cv::Mat frame;
    cv::Mat gray, thresh;

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

        // Check every detected shape.
        for (const std::vector<cv::Point>& contour : contours) {
            // Ignore very small areas, which are usually camera noise.
            if (cv::contourArea(contour) < 500) {
                continue;
            }

            // This is the simplified version of the original contour.
            std::vector<cv::Point> approximatedContour;

            // The allowed error is 2% of the contour's perimeter.
            double epsilon = 0.02 * cv::arcLength(contour, true);
            cv::approxPolyDP(
                contour,
                approximatedContour,
                epsilon,
                true
            );

            // A convex contour has no inward dents. A square is convex.
            bool isConvex = approximatedContour.size() >= 3 &&
                            cv::isContourConvex(approximatedContour);

            if (isConvex) {
                // Draw the simplified convex shape in green.
                std::vector<std::vector<cv::Point>> shape{
                    approximatedContour
                };
                cv::drawContours(
                    frame,
                    shape,
                    0,
                    cv::Scalar(0, 255, 0),
                    3
                );
            }
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