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
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        for(size_t i = 0; i < contours.size(); i++) {
                cv::drawContours(frame, contours, (int)i, cv::Scalar(0, 255, 0), 2);
        }
        for (let i = 0; i < contours.size(); ++i) {
            let tmp = new cv.Mat();
            let cnt = contours.get(i);
            // You can try more different parameters
            cv.approxPolyDP(cnt, tmp, 3, true);
            poly.push_back(tmp);
            cnt.delete(); tmp.delete();
        }
        // Count black pixels
       // int blackPixels = cv::countNonZero(thresh);
        //std::cout << "Black pixels: " << blackPixels << std::endl;
        // Show the frames
        cv::imshow("Camera Feed", frame);
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