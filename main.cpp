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

    while (true) {
        // Grab a frame
        if (!cap.read(frame)) {
            std::cerr << "ERROR: Could not read frame from camera!" << std::endl;
            break;
        }

        // Show the frame
        cv::imshow("Camera Feed", frame);

        // Exit when user presses 'q'
        char key = (char)cv::waitKey(1);
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