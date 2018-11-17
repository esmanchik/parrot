// In CLion open Run -> Edit Configurations -> sumosh -> Environment variables and set
// LD_LIBRARY_PATH to /home/sasha/Desktop/robot/parrot/sdk/out/arsdk-native/staging/lib:/home/sasha/Desktop/robot/parrot/sdk/out/arsdk-native/staging/usr/lib:

#include <iostream>
#include <unistd.h>
#include <atomic>

#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/highgui.hpp>

#include "JumpingSumo.h"

#define TAG "sumocv"

const int middle = 320;

int main() {
    using namespace cv;
    using namespace std;

    JumpingSumo sumo{};
    auto createTracker = []() {
        return TrackerMedianFlow::create();
        //return TrackerKCF::create();
    };
    Ptr<Tracker> tracker = createTracker();
    atomic_bool running {true};
    atomic_int frames {0};
    unique_ptr<Rect2d> nextFrame {nullptr};
    atomic_bool hasFrame;
    int failures = 0;

    JumpingSumo::FrameLambda onFrame = [createTracker, &tracker, &running, &frames, &hasFrame, &nextFrame, &failures](const JumpingSumo::Bytes &bytes) {
        Mat jpeg(1, bytes.size(), CV_8UC1, (void*)bytes.data());
        Mat frame = imdecode(jpeg, CV_LOAD_IMAGE_COLOR);

        // Define initial bounding box
        Rect2d bbox(300, 100, 200, 200);

        if (frames == 0) {
            // Uncomment the line below to select a different bounding box
            bbox = selectROI(frame, false);
            // Display bounding box.
            rectangle(frame, bbox, Scalar( 255, 0, 0 ), 2, 1 );

            imshow("Tracking", frame);
            tracker->init(frame, bbox);
        } else {
            // Update the tracking result
            bool ok = tracker->update(frame, bbox);
            if (ok)
            {
                nextFrame.reset(new Rect2d(bbox));
                hasFrame.store(true, memory_order_release);
                // Tracking success : Draw the tracked object
                rectangle(frame, bbox, Scalar( 255, 0, 0 ), 2, 1 );
            }
            else
            {
                // Tracking failure detected.
                putText(frame, "Tracking failure detected", Point(100,80), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,255),2);
                if (failures < 60) failures++;
                else {
                    failures = 0;
                    // Re-select ROI
                    bbox = selectROI(frame, false);
                    tracker = createTracker();
                    tracker->init(frame, bbox);
                }
            }
            line(frame, Point(middle, 100), Point(middle, 150), Scalar( 255, 0, 0 ), 2, 1);

            // Display tracker type on frame
            putText(frame, "Tracker", Point(100,20), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50,170,50),2);

            // Display FPS on frame
            stringstream ss{}; ss << frames;
            putText(frame, "Frames : " + ss.str(), Point(100,50), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50,170,50), 2);
        }

        frames++;

        // Display frame.
        imshow("Tracking", frame);

        // Exit if ESC pressed.
        int k = waitKey(1);
        if(k == 27)
        {
            ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "ESC");
            running = false;
        }
    };

    JumpingSumo::LoopLambda loop = [&running, &nextFrame, &hasFrame, &frames](ARCONTROLLER_Device_t *deviceController) {
        eARCONTROLLER_ERROR error;
        auto isError = [&error]() {
            if (error != ARCONTROLLER_OK) {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "\n- command error :%s", ARCONTROLLER_Error_ToString(error));
                return true;
            }
            return false;
        };
        auto move = [deviceController, &error](int force){
            error = deviceController->jumpingSumo->setPilotingPCMDSpeed(deviceController->jumpingSumo, force);
        };
        auto turn = [deviceController, &error](int force){
            error = deviceController->jumpingSumo->setPilotingPCMDTurn(deviceController->jumpingSumo, force);
        };
        int force = 0, udelay = 100;
        while (running) {
            if (!hasFrame.load(memory_order_acquire)) {
                usleep(udelay * 1000);
                continue;
            }
            auto box = nextFrame.release();
            hasFrame.store(false, memory_order_release);
            force = 0;
            bool rotate = false;
            auto boxMid = box->x + box->width / 2;
            if (boxMid < middle - 10) {
                force = -20;
                rotate = true;
            }
            if (boxMid > middle + 10) {
                force = 20;
                rotate = true;
            }
            if (!rotate) {
                if (box->width > 300) force = -90;
                if (box->width < 200) force = 90;
            }
            if (force == 0 || frames % 10 != 0) continue;
            error = deviceController->jumpingSumo->setPilotingPCMDFlag(deviceController->jumpingSumo, 1);
            if (isError()) break;
            if (rotate) turn(force);
            else move(force);
            if (isError()) break;
            usleep(udelay * 1000);
            error = deviceController->jumpingSumo->setPilotingPCMDFlag(deviceController->jumpingSumo, 0);
            if (isError()) break;
            error = deviceController->jumpingSumo->setPilotingPCMDSpeed(deviceController->jumpingSumo, 0);
            if (isError()) break;
            error = deviceController->jumpingSumo->setPilotingPCMDTurn(deviceController->jumpingSumo, 0);
            if (isError()) break;
        }
    };

    sumo.onFrame(onFrame).doLoop(loop).start();

    return 0;
}