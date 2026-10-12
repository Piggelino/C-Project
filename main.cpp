#include <iostream>
#include <cstdlib> 
#include <string> 
#include <vector>

#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>
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
        }
    }
};

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
                markerDetected = true;
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
                );//Efter detta måste vi isolera svarta pixlarna i den gröna konturen och räkna antalet svarta pixlar i den gröna konturen.
                //För att isolera de svarta pixlarna behöver vi hitta kordinaterna för den gröna konturens hörn och sedan skapa en mask som täcker det området. 
                //Sedan kan vi använda den masken för att räkna antalet svarta pixlar i det området.
                //Om antalet svarta pixlar är större än ett visst tröskelvärde, ska vi spela upp ljudet omgående i en loop, och om antalet svarta pixlar blöir fler ska vi spela upp ljudet i en snabbare takt. 
                //Om antalet svarta pixlar minskar ska vi sakta ner ljudet tills det stannar helt.
            }
        }

        if (markerDetected) {//detta kan inte gälla till när vi har implementerat logiken för att räkna svarta pixlar i den gröna konturen.
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