#include "JumpingSumo.h"

#include <string>
#include <unistd.h>

#define TAG "JumpingSumo"
#define JS_IP_ADDRESS "192.168.2.1"
#define JS_DISCOVERY_PORT 44444

void batteryStateChanged (uint8_t percent)
{
    // callback of changing of battery level
    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "%d%% of battery left", (unsigned)percent);
}

// called when a command has been received from the drone
void commandReceived (eARCONTROLLER_DICTIONARY_KEY commandKey, ARCONTROLLER_DICTIONARY_ELEMENT_t *elementDictionary, void *customData)
{
    ARCONTROLLER_Device_t *deviceController = (ARCONTROLLER_Device_t *)customData;

    if (deviceController == NULL)
        return;
    // if the command received is a battery state changed
    if (commandKey == ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_BATTERYSTATECHANGED)
    {
        ARCONTROLLER_DICTIONARY_ARG_t *arg = NULL;
        ARCONTROLLER_DICTIONARY_ELEMENT_t *singleElement = NULL;

        if (elementDictionary != NULL)
        {
            // get the command received in the device controller
            HASH_FIND_STR (elementDictionary, ARCONTROLLER_DICTIONARY_SINGLE_KEY, singleElement);

            if (singleElement != NULL)
            {
                // get the value
                HASH_FIND_STR (singleElement->arguments, ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_BATTERYSTATECHANGED_PERCENT, arg);

                if (arg != NULL)
                {
                    // update UI
                    batteryStateChanged (arg->value.U8);
                }
                else
                {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "arg is NULL");
                }
            }
            else
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "singleElement is NULL");
            }
        }
        else
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "elements is NULL");
        }
    }
}

eARCONTROLLER_ERROR decoderConfigCallback (ARCONTROLLER_Stream_Codec_t codec, void *customData)
{
    ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "decoderConfigCallback codec.type :%d", codec.type);

    return ARCONTROLLER_OK;
}

void JumpingSumo::start() {
    using namespace std;
    ARSAL_Sem_Init (&(stateSem), 0, 0);

    eARDISCOVERY_ERROR errorDiscovery = ARDISCOVERY_OK;
    ARDISCOVERY_Device_t *device = ARDISCOVERY_Device_New(&errorDiscovery);

    if (errorDiscovery == ARDISCOVERY_OK) {
        errorDiscovery = ARDISCOVERY_Device_InitWifi(device, ARDISCOVERY_PRODUCT_JS, "JS", JS_IP_ADDRESS, JS_DISCOVERY_PORT);
        if (errorDiscovery == ARDISCOVERY_OK) {
            eARCONTROLLER_ERROR error = ARCONTROLLER_OK;
            ARCONTROLLER_Device_t *deviceController = ARCONTROLLER_Device_New(device, &error);
            if (error == ARCONTROLLER_OK) {
                error = ARCONTROLLER_Device_AddStateChangedCallback(deviceController, stateChangedProxy, this);
                if (error != ARCONTROLLER_OK)
                {
                    ARSAL_PRINT (ARSAL_PRINT_ERROR, TAG, "add State callback failed.");
                }
                error = ARCONTROLLER_Device_AddCommandReceivedCallback (deviceController, commandReceived, deviceController);
                if (error != ARCONTROLLER_OK)
                {
                    ARSAL_PRINT (ARSAL_PRINT_ERROR, TAG, "add callback failed.");
                }
                error = ARCONTROLLER_Device_SetVideoStreamCallbacks(deviceController, decoderConfigCallback, didReceiveFrameProxy, NULL , this);
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
                    loop(deviceController);
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
}

void JumpingSumo::stateChangedProxy(eARCONTROLLER_DEVICE_STATE newState, eARCONTROLLER_ERROR error, void *customData) {
    auto sumo = static_cast<JumpingSumo *>(customData);
    sumo->stateChanged(newState, error);
}


void JumpingSumo::stateChanged(eARCONTROLLER_DEVICE_STATE newState, eARCONTROLLER_ERROR error) {
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

JumpingSumo &JumpingSumo::doLoop(LoopLambda fn) {
    loop = fn;
    return *this;
}

void JumpingSumo::didReceiveFrame(uint8_t *image, uint32_t size) {
    this->framer(Bytes(image, size));
}

eARCONTROLLER_ERROR JumpingSumo::didReceiveFrameProxy(ARCONTROLLER_Frame_t *frame, void *customData)
{
    if (frame == NULL) return ARCONTROLLER_OK;
    auto sumo = static_cast<JumpingSumo *>(customData);
    sumo->didReceiveFrame(frame->data , frame->used);
    return ARCONTROLLER_OK;
}

JumpingSumo & JumpingSumo::onFrame(FrameLambda fn) {
    framer = fn;
    return *this;
}
