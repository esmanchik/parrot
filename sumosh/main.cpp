// In CLion open Run -> Edit Configurations -> sumosh -> Environment variables and set
// LD_LIBRARY_PATH to /home/sasha/Desktop/robot/parrot/sdk/out/arsdk-native/staging/lib:/home/sasha/Desktop/robot/parrot/sdk/out/arsdk-native/staging/usr/lib:

#include <iostream>
#include <unistd.h>

extern "C" {
#include <libARSAL/ARSAL.h>
#include <libARDiscovery/ARDiscovery.h>
#include <libARController/ARController.h>
}

#define TAG "sumosh"
#define JS_IP_ADDRESS "192.168.2.1"
#define JS_DISCOVERY_PORT 44444

ARSAL_Sem_t stateSem;

void stateChanged (eARCONTROLLER_DEVICE_STATE newState, eARCONTROLLER_ERROR error, void *customData) {
    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "    - stateChanged newState: %d .....", newState);

    switch (newState)
    {
        case ARCONTROLLER_DEVICE_STATE_STOPPED:
        case ARCONTROLLER_DEVICE_STATE_RUNNING:
            ARSAL_Sem_Post (&(stateSem));
            break;

        default:
            break;
    }

}

struct CallbackData {
    int frameNb = 0;
};

eARCONTROLLER_ERROR decoderConfigCallback (ARCONTROLLER_Stream_Codec_t codec, void *customData)
{
    ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "decoderConfigCallback codec.type :%d", codec.type);

    return ARCONTROLLER_OK;
}

eARCONTROLLER_ERROR didReceiveFrameCallback (ARCONTROLLER_Frame_t *frame, void *customData)
{
    int tmp = 0;
    CallbackData *pcbd = (CallbackData *)customData;
    int &frameNb = pcbd == NULL ? tmp : pcbd->frameNb;
    if (frame != NULL) {
        char filename[20];
        snprintf(filename, sizeof(filename), "frames/frame%03d.jpg", frameNb % 25);
        frameNb++;
        FILE *img = fopen(filename, "w");
        if (img == NULL) {
            ARSAL_PRINT(ARSAL_PRINT_WARNING, TAG, "failed to open file %s", filename);
            return ARCONTROLLER_ERROR;
        } else {
            fwrite(frame->data , frame->used, 1, img);
            fclose(img);
        }
    } else {
        ARSAL_PRINT(ARSAL_PRINT_WARNING, TAG, "frame is NULL.");
    }

    return ARCONTROLLER_OK;
}

int main() {
    using namespace std;
    ARSAL_Sem_Init (&(stateSem), 0, 0);
    CallbackData cbd;

    eARDISCOVERY_ERROR errorDiscovery = ARDISCOVERY_OK;
    ARDISCOVERY_Device_t *device = ARDISCOVERY_Device_New(&errorDiscovery);

    if (errorDiscovery == ARDISCOVERY_OK) {
        errorDiscovery = ARDISCOVERY_Device_InitWifi(device, ARDISCOVERY_PRODUCT_JS, "JS", JS_IP_ADDRESS, JS_DISCOVERY_PORT);
        if (errorDiscovery == ARDISCOVERY_OK) {
            eARCONTROLLER_ERROR error = ARCONTROLLER_OK;
            ARCONTROLLER_Device_t *deviceController = ARCONTROLLER_Device_New(device, &error);
            if (error == ARCONTROLLER_OK) {
                error = ARCONTROLLER_Device_AddStateChangedCallback(deviceController, stateChanged, &cbd);
                if (error != ARCONTROLLER_OK)
                {
                    ARSAL_PRINT (ARSAL_PRINT_ERROR, TAG, "add State callback failed.");
                }
                error = ARCONTROLLER_Device_SetVideoStreamCallbacks(deviceController, decoderConfigCallback, didReceiveFrameCallback, NULL , &cbd);
                if (error != ARCONTROLLER_OK)
                {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- error :%s", ARCONTROLLER_Error_ToString(error));
                }
                error = ARCONTROLLER_Device_Start (deviceController);
                if (error == ARCONTROLLER_OK) {
                    // wait state update update
                    ARSAL_Sem_Wait (&(stateSem));
                    eARCONTROLLER_DEVICE_STATE deviceState = ARCONTROLLER_Device_GetState (deviceController, &error);
                    if ((error != ARCONTROLLER_OK) || (deviceState != ARCONTROLLER_DEVICE_STATE_RUNNING))
                    {
                        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- deviceState :%d", deviceState);
                        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- error :%s", ARCONTROLLER_Error_ToString(error));
                    }
                    error = deviceController->jumpingSumo->sendMediaStreamingVideoEnable (deviceController->jumpingSumo, 1);
                    if (error != ARCONTROLLER_OK)
                    {
                        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- error :%s", ARCONTROLLER_Error_ToString(error));
                    }
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
                            if (error != ARCONTROLLER_OK)
                            {
                                ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "\n- command error :%s", ARCONTROLLER_Error_ToString(error));
                                break;
                            }
                        } else {
                            cin >> force >> udelay;
                            cout << "Doing " << command << " with " << force << " ..." << endl;
                            error = deviceController->jumpingSumo->setPilotingPCMDFlag(deviceController->jumpingSumo, 1);
                            if (error != ARCONTROLLER_OK)
                            {
                                ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "\n- command error :%s", ARCONTROLLER_Error_ToString(error));
                                break;
                            }
                            if (command.substr(0, 1) == "t")
                                error = deviceController->jumpingSumo->setPilotingPCMDTurn(deviceController->jumpingSumo, force);
                            else
                                error = deviceController->jumpingSumo->setPilotingPCMDSpeed(deviceController->jumpingSumo, force);
                            if (error != ARCONTROLLER_OK)
                            {
                                ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "\n- command error :%s", ARCONTROLLER_Error_ToString(error));
                                break;
                            }
                            usleep(udelay * 1000);
                            error = deviceController->jumpingSumo->setPilotingPCMDFlag (deviceController->jumpingSumo, 0);
                            if (error != ARCONTROLLER_OK)
                            {
                                ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- command error :%s", ARCONTROLLER_Error_ToString(error));
                            }
                            error = deviceController->jumpingSumo->setPilotingPCMDSpeed (deviceController->jumpingSumo, 0);
                            if (error != ARCONTROLLER_OK)
                            {
                                ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- command error :%s", ARCONTROLLER_Error_ToString(error));
                            }
                            error = deviceController->jumpingSumo->setPilotingPCMDTurn (deviceController->jumpingSumo, 0);
                            if (error != ARCONTROLLER_OK)
                            {
                                ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- command error :%s", ARCONTROLLER_Error_ToString(error));
                            }
                            cout << ".....done" << endl;
                        }
                        command = "";
                        cin >> command;
                    }
                    deviceState = ARCONTROLLER_Device_GetState (deviceController, &error);
                    if ((error == ARCONTROLLER_OK) && (deviceState != ARCONTROLLER_DEVICE_STATE_STOPPED))
                    {
                        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "Disconnecting ...");

                        error = ARCONTROLLER_Device_Stop (deviceController);

                        if (error == ARCONTROLLER_OK)
                        {
                            // wait state update update
                            ARSAL_Sem_Wait (&(stateSem));
                        }
                    }
                } else {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- error :%s", ARCONTROLLER_Error_ToString(error));
                }
                ARCONTROLLER_Device_Delete (&deviceController);
            } else {
                ARSAL_PRINT (ARSAL_PRINT_ERROR, TAG, "Creation of deviceController failed.");
            }
        } else {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "Discovery error :%s", ARDISCOVERY_Error_ToString(errorDiscovery));
        }
        ARDISCOVERY_Device_Delete(&device);
    } else {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "Create device error :%s", ARDISCOVERY_Error_ToString(errorDiscovery));
    }

    return 0;
}