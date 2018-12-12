//
// Copyright (C) 2006-2017 Agon Memedi <memedi@ccs-labs.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

/*
 * Based on PhyLayer80211p.cc from David Eckhoff
 */

#include "veins/modules/vlc/PhyLayerVlc.h"

#include "veins/modules/vlc/DeciderVlc.h"
#include "veins/modules/analogueModel/SimpleObstacleShadowing.h"
#include "veins/modules/vlc/analogueModel/VehicleObstacleShadowingForVlc.h"
#include "veins/base/connectionManager/BaseConnectionManager.h"
#include "veins/modules/messages/AirFrame11p_m.h"
#include "veins/modules/vlc/messages/AirFrameVlc_m.h"
#include "veins/base/phyLayer/MacToPhyControlInfo.h"

#include "veins/base/modules/BaseMacLayer.h" // Needed for createSingleFrequencyMapping()

using namespace Veins;

Define_Module(Veins::PhyLayerVlc);

/* Used for the LsvLightModel */
bool PhyLayerVlc::mapsInitialized = false;
std::map<std::string, PhotoDiode> PhyLayerVlc::photoDiodeMap = std::map<std::string, PhotoDiode>();
std::map<std::string, RadiationPattern> PhyLayerVlc::radiationPatternMap = std::map<std::string, RadiationPattern>();

void PhyLayerVlc::initialize(int stage)
{
    if (stage == 0) {
        txPower = par("txPower").doubleValue();
        if (txPower != FIXED_REFERENCE_POWER_MW)
            error("You have set the wrong reference power (txPower) in omnetpp.ini. Should be fixed to: FIXED_REFERENCE_POWER");
        bitrate = par("bitrate").doubleValue();
        direction = getParentModule()->par("direction").stringValue();
        collectCollisionStatistics = par("collectCollisionStatistics").boolValue();
    }
    BasePhyLayer::initialize(stage);
    if (stage == 0) {
        // TODO: maybe check the lengths of the VLC packet fields
    }
}

AnalogueModel* PhyLayerVlc::getAnalogueModelFromName(std::string name, ParameterMap& params)
{

    if (name == "EmpiricalLightModel") {
        return initializeEmpiricalLightModel(params);
    }
    else if (name == "LsvLightModel") {
        return initializeLsvLightModel(params);
    }
    else if (name == "VehicleObstacleShadowingForVlc") {
        return initializeVehicleObstacleShadowingForVlc(params);
    }
    return BasePhyLayer::getAnalogueModelFromName(name, params);
}

AnalogueModel* PhyLayerVlc::initializeEmpiricalLightModel(ParameterMap& params)
{

    bool debug;
    double headlightMaxTxRange = 0.0, taillightMaxTxRange = 0.0, headlightMaxTxAngle = 0.0, taillightMaxTxAngle = 0.0;
    ParameterMap::iterator it;

    it = params.find("headlightMaxTxRange");
    if (it != params.end()) {
        headlightMaxTxRange = it->second.doubleValue();
    }
    else {
        error("`headlightMaxTxRange` has not been specified in config-vlc.xml");
    }

    it = params.find("taillightMaxTxRange");
    if (it != params.end()) {
        taillightMaxTxRange = it->second.doubleValue();
    }
    else {
        error("`taillightMaxTxRange` has not been specified in config-vlc.xml");
    }

    it = params.find("headlightMaxTxAngle");
    if (it != params.end()) {
        headlightMaxTxAngle = it->second.doubleValue();
    }
    else {
        error("`headlightMaxTxAngle` has not been specified in config-vlc.xml");
    }

    it = params.find("taillightMaxTxAngle");
    if (it != params.end()) {
        taillightMaxTxAngle = it->second.doubleValue();
    }
    else {
        error("`taillightMaxTxAngle` has not been specified in config-vlc.xml");
    }

    // BasePhyLayer converts sensitivity from config.xml to mW @ line 72. Convert it back to dBm
    return new EmpiricalLightModel(FWMath::mW2dBm(sensitivity), headlightMaxTxRange, taillightMaxTxRange, headlightMaxTxAngle, taillightMaxTxAngle);
}

//  version using line-by-line parsing
AnalogueModel* PhyLayerVlc::initializeLsvLightModel(ParameterMap& params)
{
    if (mapsInitialized == false) {

        ParameterMap::iterator it;
        std::string radiationPatternFile;
        std::string photoDiodeFile;

        it = params.find("radiationPatternFile");
        if (it != params.end()) {
            radiationPatternFile = it->second.stringValue();
        }
        else {
            error("`radiationPatternFile` has not been specified in config-vlc-lsv.xml");
        }

        it = params.find("photoDiodeFile");
        if (it != params.end()) {
            photoDiodeFile = it->second.stringValue();
        }
        else {
            error("`photoDiodeFile` has not been specified in config-vlc-lsv.xml");
        }

        // For file parsing
        std::ifstream inputFile(radiationPatternFile);
        std::string line;
        int lineCounter;
        double value;

        // For radiation pattern
        std::string Id;
        std::vector<double> patternL, patternR, anglesL, anglesR, spectralEmission;

        lineCounter = 0;
        while (std::getline(inputFile, line)) {
            std::istringstream iss(line);
            switch (lineCounter) {
            case 0:
                iss >> Id;
                ++lineCounter;
                break;
            case 1:
                while (iss >> value) patternL.push_back(value);
                ++lineCounter;
                break;
            case 2:
                while (iss >> value) patternR.push_back(value);
                ++lineCounter;
                break;
            case 3:
                while (iss >> value) anglesL.push_back(value);
                ++lineCounter;
                break;
            case 4:
                while (iss >> value) anglesR.push_back(value);
                ++lineCounter;
                break;
            case 5:
                while (iss >> value) spectralEmission.push_back(value);
                lineCounter = 0;
                radiationPatternMap.insert(std::pair<std::string, RadiationPattern>(Id, RadiationPattern(Id, patternL, patternR, anglesL, anglesR, spectralEmission)));
                // Clear all vectors for next pattern
                patternL.clear();
                patternR.clear();
                anglesL.clear();
                anglesR.clear();
                spectralEmission.clear();
                break;
            }
            iss.str("");
        }

        // For file parsing
        std::ifstream inputFile2(photoDiodeFile);

        // For photodiode
        double area, gain;
        std::vector<double> spectralResponse;

        lineCounter = 0;
        while (std::getline(inputFile2, line)) {
            std::istringstream iss(line);
            switch (lineCounter) {
            case 0:
                iss >> Id;
                ++lineCounter;
                break;
            case 1:
                iss >> area;
                ++lineCounter;
                break;
            case 2:
                iss >> gain;
                ++lineCounter;
                break;
            case 3:
                while (iss >> value) spectralResponse.push_back(value);
                lineCounter = 0;
                photoDiodeMap.insert(std::pair<std::string, PhotoDiode>(Id, PhotoDiode(Id, area, gain, spectralResponse)));
                spectralResponse.clear();
                break;
            }
            iss.str("");
        }
        mapsInitialized = true;
    }
    return new LsvLightModel(&radiationPatternMap, &photoDiodeMap, FWMath::mW2dBm(sensitivity));
}

Decider* PhyLayerVlc::getDeciderFromName(std::string name, ParameterMap& params)
{
    if (name == "DeciderVlc") {
        return initializeDeciderVlc(params);
    }
    return BasePhyLayer::getDeciderFromName(name, params);
}

AnalogueModel* PhyLayerVlc::initializeVehicleObstacleShadowingForVlc(ParameterMap& params)
{

    // init with default value
    double carrierFrequency = 666e12;
    bool useTorus = world->useTorus();
    const Coord& playgroundSize = *(world->getPgs());

    ParameterMap::iterator it;

    // get carrierFrequency from config-vlc
    it = params.find("carrierFrequency");

    if (it != params.end()) { // parameter carrierFrequency has been specified in config-vlc.xml
        // set carrierFrequency
        carrierFrequency = it->second.doubleValue();
        EV_TRACE << "initializeVehicleObstacleShadowingForVlc(): carrierFrequency set from config-vlc.xml to " << carrierFrequency << endl;

        // check whether carrierFrequency is not smaller than specified in ConnectionManager
        if (cc->hasPar("carrierFrequency") && carrierFrequency < cc->par("carrierFrequency").doubleValue()) {
            // throw error
            throw cRuntimeError("initializeVehicleObstacleShadowingForVlc(): carrierFrequency can't be smaller than specified in ConnectionManager. Please adjust your config-vlc.xml file accordingly");
        }
    }
    else // carrierFrequency has not been specified in config-vlc.xml
    {
        if (cc->hasPar("carrierFrequency")) { // parameter carrierFrequency has been specified in ConnectionManager
            // set carrierFrequency according to ConnectionManager
            carrierFrequency = cc->par("carrierFrequency").doubleValue();
            EV_TRACE << "createPathLossModel(): carrierFrequency set from ConnectionManager to " << carrierFrequency << endl;
        }
        else // carrierFrequency has not been specified in ConnectionManager
        {
            // keep carrierFrequency at default value
            EV_TRACE << "createPathLossModel(): carrierFrequency set from default value to " << carrierFrequency << endl;
        }
    }

    VehicleObstacleControl* vehicleObstacleControlP = VehicleObstacleControlAccess().getIfExists();
    if (!vehicleObstacleControlP) throw cRuntimeError("initializeVehicleObstacleShadowingForVlc(): cannot find VehicleObstacleControl module");
    return new VehicleObstacleShadowingForVlc(*vehicleObstacleControlP, carrierFrequency, useTorus, playgroundSize);
}

Decider* PhyLayerVlc::initializeDeciderVlc(ParameterMap& params)
{
    double centerFreq = params["centerFrequency"];
    DeciderVlc* dec = new DeciderVlc(this, sensitivity, centerFreq, bitrate, direction, findHost()->getIndex(), collectCollisionStatistics);
    // dec->setPath(getParentModule()->getFullPath());
    return dec;
}

void PhyLayerVlc::handleMessage(cMessage* msg)
{
    // self messages
    if (msg->isSelfMessage()) {
        handleSelfMessage(msg);

        // MacPkts <- MacToPhyControlInfo
    }
    else if (msg->getArrivalGateId() == upperLayerIn) {
        // FIXME: is this needed?
        setRadioState(Veins::Radio::TX);
        BasePhyLayer::handleUpperMessage(msg);

        // controlmessages
    }
    else if (msg->getArrivalGateId() == upperControlIn) {
        BasePhyLayer::handleUpperControlMessage(msg);

        // AirFrames
        // msg received over air from other NICs
    }
    else if (msg->getKind() == AIR_FRAME) {
        if (dynamic_cast<AirFrame11p*>(msg)) {
            EV_TRACE << "Received message of type AirFrame11p, this message will be discarded" << std::endl;
            bubble("received message of type AirFrame11p, this message will be discarded!");
            getParentModule()->bubble("received message of type AirFrame11p, this message will be discarded!");
            delete msg;
            return;
        }
        if (dynamic_cast<AirFrameVlc*>(msg)) {
            AirFrameVlc* VlcMsg = check_and_cast<AirFrameVlc*>(msg);
            std::string txNode = VlcMsg->getSenderModule()->getModuleByPath("^.^.")->getFullName();
            if (txNode == getModuleByPath("^.^.")->getFullName()) {
                EV_TRACE << "Discarding received AirFrameVlc within the same host: " << VlcMsg->getSenderModule()->getFullPath() << std::endl;
                delete msg;
                return;
            }
            else {
                EV_TRACE << "AirFrameVlc id: " << VlcMsg->getId() << " handed to VLC PHY from: " << VlcMsg->getSenderModule()->getFullPath() << std::endl;
                bubble("Handing AirFrameVlc to lower layers to decide if it can be received");
                BasePhyLayer::handleAirFrame(static_cast<AirFrame*>(msg));
            }
        }
    }
    else {
        EV << "Unknown message received." << endl;
        delete msg;
    }
}

void PhyLayerVlc::handleSelfMessage(cMessage* msg)
{

    switch (msg->getKind()) {
    // transmission overBasePhyLayer::
    case TX_OVER: {
        assert(msg == txOverTimer);
        // TODO: Need a mechanism to inform the upper layers that the txOverTimer arrived
        break;
    }
    // radio switch over
    case RADIO_SWITCHING_OVER:
        assert(msg == radioSwitchingOverTimer);
        BasePhyLayer::finishRadioSwitching();
        break;

    // AirFrame
    case AIR_FRAME:
        BasePhyLayer::handleAirFrame(static_cast<AirFrame*>(msg));
        break;

    default:
        break;
    }
}
AirFrame* PhyLayerVlc::encapsMsg(cPacket* macPkt)
{
    // Similar to createSignal() & attachSignal() of the Mac1609_4
    int headerLength = PHY_VLC_HEADER; // length of the phy header (preamble w/o signal), used later when constructing phy frame
    //    setParametersForBitrate(bitrate);
    // macPkt->addBitLength(PHY_VLC_MHR);    // Not modeling the MAC frame size; MHR is part of PSDU according to standard so we assume the whole macPkt contains it

    // Initialize spectrum for signal representation here
    Spectrum overallSpectrum = Spectrum({666e12});

    // From Mac1609_4::attachSignal()
    simtime_t startTime = simTime();
    simtime_t duration = getFrameDuration(macPkt->getBitLength()); // Everything from the upper layer is considered as the PSDU

    // From Mac1609_4::createSignal()
    simtime_t start = startTime;
    simtime_t length = duration;

    Signal* s = new Signal(overallSpectrum, start, length);

    (*s)[0] = txPower;

    s->setBitrate(bitrate);

    s->setDataStart(0);
    s->setDataEnd(0);

    s->setCenterFrequencyIndex(0);

    // From PhyLayer80211p::encapsMsg()
    AirFrameVlc* frame = new AirFrameVlc(macPkt->getName(), AIR_FRAME);
    frame->setHeadOrNot(setDirection(direction));

    // set the members
    assert(s->getDuration() > 0);
    frame->setDuration(s->getDuration());
    // copy the signal into the AirFrame
    frame->setSignal(*s);
    // set priority of AirFrames above the normal priority to ensure
    // channel consistency (before any thing else happens at a time
    // point t make sure that the channel has removed every AirFrame
    // ended at t and added every AirFrame started at t)
    frame->setSchedulingPriority(airFramePriority());
    frame->setProtocolId(myProtocolId());
    frame->setBitLength(headerLength);
    frame->setId(world->getUniqueAirFrameId());
    frame->setChannel(radio->getCurrentChannel());

    // pointer and Signal not needed anymore
    delete s;
    s = 0;

    // encapsulate the mac packet into the phy frame
    frame->encapsulate(macPkt);

    // --- from here on, the AirFrame is the owner of the MacPacket ---
    macPkt = 0;
    EV_TRACE << "AirFrame w/ id: " << frame->getId() << " encapsulated, bit length: " << frame->getBitLength() << "\n";

    return frame;
}

simtime_t PhyLayerVlc::setRadioState(int rs)
{
    if (rs == Radio::TX) decider->switchToTx();
    return BasePhyLayer::setRadioState(rs);
}

simtime_t PhyLayerVlc::getFrameDuration(int payloadLengthBits) const
{
    // Following assumptions apply:
    // i) The SHR and the HEADER are sent with the same bitrate as the payload
    // ii) Due to OOK, the number of bits per symbol (n_nbps) == 1, so the payload is not divided
    simtime_t duration = (PHY_VLC_SHR + PHY_VLC_HEADER) / bitrate + payloadLengthBits / bitrate;
    return duration;
}

int PhyLayerVlc::setDirection(const std::string& direction)
{

    if (direction == "head") {
        return HEAD;
    }
    else if (direction == "tail") {
        return TAIL;
    }
    else {
        error("Parameter `direction` is set wrongly in your config-vlc! It is neither `head` nor `tail`!");
        return 0;
    }
}
