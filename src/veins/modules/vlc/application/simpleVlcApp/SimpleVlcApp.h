/*
 * SimplestApp.h
 *
 *  Created on: Nov 25, 2014
 *      Author: Agon Memedi
 */

#ifndef VLCSIMPLEAPP_H_
#define VLCSIMPLEAPP_H_

#include <omnetpp.h>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "veins/modules/application/ieee80211p/BaseWaveApplLayer.h"
#include "veins/modules/mobility/traci/TraCIScenarioManager.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/mobility/traci/TraCICommandInterface.h"
#include "veins/modules/utility/timeoutManager/TimeoutObserver.h"
#include "veins/modules/utility/timeoutManager/TimeoutManager.h"
#include "veins/modules/world/annotations/AnnotationManager.h"
#include "veins/modules/vlc/utility/Utils.h"

#include "veins/modules/messages/WaveShortMessage_m.h"
#include "veins/modules/vlc/messages/VlcMessage_m.h"

using Veins::TraCIScenarioManager;
using Veins::TraCIScenarioManagerAccess;
using Veins::TraCIMobility;
using Veins::TraCICommandInterface;
using Veins::AnnotationManager;
using Veins::AnnotationManagerAccess;

// Types of timeouts
#define SEND_DSRC 1
#define SEND_VLC_HEAD 2
#define SEND_VLC_TAIL 3
#define SEND_VLC_BOTH 4

#define VOUT(smth) if(debug) {std::cout << "Module: [" << getFullPath() << "\t@" << simTime() << "]: " << smth << std::endl;}

class SimpleVlcApp: public cSimpleModule, public TimeoutObserver
{
protected:
    bool debug;

    int toLower;
    int fromLower;
    int byteLength;
    double transmissionPeriod;

    std::string sumoId;

    TimeoutManager* timeoutManager;
    mutable TraCIScenarioManager *traciManager;
    TraCIMobility* mobility;
    AnnotationManager* annotations;
    TraCICommandInterface* traci;

public:
    SimpleVlcApp();
    ~SimpleVlcApp();
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    virtual int numInitStages() const {return 4;}

    virtual void onTimeout(std::string name, short int kind);

protected:
    VlcMessage* generateVlcMessage(int accessTechnology, int sendingModule);
};

#endif /* SimplestApp_H_ */
