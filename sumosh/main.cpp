// In CLion open Run -> Edit Configurations -> sumosh -> Environment variables and set
// LD_LIBRARY_PATH to /home/sasha/Desktop/robot/parrot/sdk/out/arsdk-native/staging/lib:/home/sasha/Desktop/robot/parrot/sdk/out/arsdk-native/staging/usr/lib:

#include <iostream>
#include <unistd.h>
#include "JumpingSumo.h"

#define TAG "sumosh"

int main() {
    using namespace std;

    JumpingSumo sumo{};

    int frameNb = 0;
    JumpingSumo::FrameLambda onFrame = [&frameNb](const JumpingSumo::Bytes &bytes) {
        char filename[20];
        snprintf(filename, sizeof(filename), "frames/frame%03d.jpg", frameNb % 25);
        frameNb++;
        FILE *img = fopen(filename, "w");
        if (img == NULL) {
            ARSAL_PRINT(ARSAL_PRINT_WARNING, TAG, "failed to open file %s", filename);
            //return ARCONTROLLER_ERROR;
        } else {
            fwrite(bytes.data(), bytes.size(), 1, img);
            fclose(img);
        }
        //return ARCONTROLLER_OK;
    };

    JumpingSumo::LoopLambda loop = [](ARCONTROLLER_Device_t *deviceController) {
        eARCONTROLLER_ERROR error;
        int force = 0, udelay = 500;
        string command;
        cin >> command;
        while (true) {
            if (command.empty() || command == "\n" || command.substr(0, 1) == "q") break;
            if (command.substr(0, 1) == "p") {
                string posture;
                cin >> posture;
                auto type = ARCOMMANDS_JUMPINGSUMO_PILOTING_POSTURE_TYPE_JUMPER;
                if (posture.substr(0, 1) == "k") {
                    type = ARCOMMANDS_JUMPINGSUMO_PILOTING_POSTURE_TYPE_KICKER;
                }
                error = deviceController->jumpingSumo->sendPilotingPosture(deviceController->jumpingSumo, type);
                if (error != ARCONTROLLER_OK) {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "\n- command error :%s", ARCONTROLLER_Error_ToString(error));
                    break;
                }
            } else {
                cin >> force >> udelay;
                ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "Doing %s with %d ...", command.c_str(), force);
                error = deviceController->jumpingSumo->setPilotingPCMDFlag(deviceController->jumpingSumo, 1);
                if (error != ARCONTROLLER_OK) {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "\n- command error :%s", ARCONTROLLER_Error_ToString(error));
                    break;
                }
                if (command.substr(0, 1) == "t")
                    error = deviceController->jumpingSumo->setPilotingPCMDTurn(deviceController->jumpingSumo, force);
                else
                    error = deviceController->jumpingSumo->setPilotingPCMDSpeed(deviceController->jumpingSumo, force);
                if (error != ARCONTROLLER_OK) {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "\n- command error :%s", ARCONTROLLER_Error_ToString(error));
                    break;
                }
                usleep(udelay * 1000);
                error = deviceController->jumpingSumo->setPilotingPCMDFlag(deviceController->jumpingSumo, 0);
                if (error != ARCONTROLLER_OK) {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- command error :%s", ARCONTROLLER_Error_ToString(error));
                }
                error = deviceController->jumpingSumo->setPilotingPCMDSpeed(deviceController->jumpingSumo, 0);
                if (error != ARCONTROLLER_OK) {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- command error :%s", ARCONTROLLER_Error_ToString(error));
                }
                error = deviceController->jumpingSumo->setPilotingPCMDTurn(deviceController->jumpingSumo, 0);
                if (error != ARCONTROLLER_OK) {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- command error :%s", ARCONTROLLER_Error_ToString(error));
                }
                ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, ".....done");
            }
            command = "";
            cin >> command;
        }
    };

    sumo.onFrame(onFrame).doLoop(loop).start();

    return 0;
}