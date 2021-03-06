#ifndef SUMOSH_JUMPINGSUMO_H
#define SUMOSH_JUMPINGSUMO_H

#include <functional>

extern "C" {
#include <libARSAL/ARSAL.h>
#include <libARDiscovery/ARDiscovery.h>
#include <libARController/ARController.h>
}

class JumpingSumo {
public:
    using LoopFunc = void (*)(ARCONTROLLER_Device_t *);
    using LoopLambda = std::function<auto (ARCONTROLLER_Device_t *) -> void>;

    class Bytes {
        uint8_t *d;
        uint32_t s;
    public:
        Bytes(uint8_t *data, uint32_t size) : d(data), s(size) {}
        uint8_t *data() const { return d; }
        uint32_t size() const { return s; }
    };

    using FrameFunc = void (*)(const Bytes &);
    using FrameLambda = std::function<auto (const Bytes &) -> void>;

    JumpingSumo() {
        deviceController = nullptr;
        batteryPercent = 0;
        framer = [](const Bytes &frame) {};
        loop = [](ARCONTROLLER_Device_t * device) {};
    }

    void start();
    JumpingSumo & doLoop(LoopLambda);
    JumpingSumo & onFrame(FrameLambda);

    inline uint8_t getBatteryPercent() {
        return batteryPercent;
    }

private:
    ARCONTROLLER_Device_t* deviceController;

    ARSAL_Sem_t stateSem;

    LoopLambda loop;
    FrameLambda framer;

    volatile uint8_t batteryPercent;
    
    static void stateChangedProxy(eARCONTROLLER_DEVICE_STATE newState, eARCONTROLLER_ERROR error, void *customData);
    void stateChanged(eARCONTROLLER_DEVICE_STATE newState, eARCONTROLLER_ERROR error);

    static eARCONTROLLER_ERROR didReceiveFrameProxy(ARCONTROLLER_Frame_t *frame, void *customData);
    void didReceiveFrame(uint8_t *image, uint32_t size);

    static void commandReceivedProxy(eARCONTROLLER_DICTIONARY_KEY commandKey, ARCONTROLLER_DICTIONARY_ELEMENT_t *elementDictionary, void *customData);
};


#endif //SUMOSH_JUMPINGSUMO_H
