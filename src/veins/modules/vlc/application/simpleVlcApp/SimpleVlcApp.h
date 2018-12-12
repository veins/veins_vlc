#pragma once

#include <omnetpp.h>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "veins/modules/mobility/traci/TraCIScenarioManager.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/mobility/traci/TraCICommandInterface.h"
#include "veins/modules/utility/TimerManager.h"
#include "veins/modules/world/annotations/AnnotationManager.h"
#include "veins/modules/vlc/utility/Utils.h"

#include "veins/modules/messages/BaseFrame1609_4_m.h"
#include "veins/modules/vlc/messages/VlcMessage_m.h"

namespace Veins {

class SimpleVlcApp : public cSimpleModule {
protected:
    bool debug;

    int toLower;
    int fromLower;
    int byteLength;
    double transmissionPeriod;

    std::string sumoId;

    Veins::TimerManager timerManager{this};
    mutable TraCIScenarioManager* traciManager;
    TraCIMobility* mobility;
    AnnotationManager* annotations;
    TraCICommandInterface* traci;

public:
    SimpleVlcApp();
    ~SimpleVlcApp();
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage* msg);
    virtual int numInitStages() const
    {
        return 4;
    }

protected:
    VlcMessage* generateVlcMessage(int accessTechnology, int sendingModule);
};

} // namespace Veins
