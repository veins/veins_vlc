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

#include "veins/modules/vlc/DeciderVlc.h"
#include "veins/modules/vlc/PhyLayerVlc.h"
#include "veins/modules/analogueModel/SimplePathlossModel.h"
#include "veins/modules/analogueModel/PERModel.h"
#include "veins/base/modules/BaseMacLayer.h" // Needed for createSingleFrequencyMapping()
#include "veins/base/phyLayer/MacToPhyControlInfo.h"

#include "veins/modules/vlc/messages/AirFrameVlc_m.h"

#include "veins/modules/analogueModel/SimpleObstacleShadowing.h"
#include "veins/modules/analogueModel/VehicleObstacleShadowing.h"
#include "veins/base/connectionManager/BaseConnectionManager.h"
#include "veins/modules/utility/Consts80211p.h"
#include "veins/modules/messages/AirFrame11p_m.h"

using Veins::ObstacleControlAccess;

Define_Module(PhyLayerVlc);

void PhyLayerVlc::initialize(int stage) {
	if (stage == 0) {
		debug = par("debug").boolValue();

		power = par("txPower").doubleValue();
        if(power != FIXED_REFERENCE_POWER_MW)
            error("You have set the wrong reference power (txPower) in omnetpp.ini. Should be fixed to: FIXED_REFERENCE_POWER");
        bitrate = par("bitrate").doubleValue();
        bandwidth = par("bandwidth").doubleValue();
        frequency = par("centerFrequency").doubleValue();
        direction = getParentModule()->par("direction").stringValue();
        collectCollisionStatistics = par("collectCollisionStatistics").boolValue();

        VLCDEBUG("[READING CONFIG XML PARAMETERS]"
                << "\nPower: " << power
                << "\nBitrate: " << bitrate
                << "\nBandwidth: " << bandwidth
                << "\nFrequency: " << frequency
                << "\nDirection: " << direction
                << "\nCollectCollisionStatistics: " << collectCollisionStatistics
                )
	}
	BasePhyLayer::initialize(stage);
	if (stage == 0) {
	    // TODO: maybe check the lengths of the VLC packet fields
		//erase the RadioStateAnalogueModel
		analogueModels.erase(analogueModels.begin());
	}
}

AnalogueModel* PhyLayerVlc::getAnalogueModelFromName(std::string name, ParameterMap& params) {

	if (name == "EmpiricalLightModel") {
        return initializeEmpiricalLightModel(params);
    }
	else if (name == "VehicleObstacleShadowing") {
        return initializeVehicleObstacleShadowing(params);
    }
    else if (name == "SimpleObstacleShadowing") {
        return initializeSimpleObstacleShadowing(params);
    }
	return BasePhyLayer::getAnalogueModelFromName(name, params);
}

AnalogueModel* PhyLayerVlc::initializeEmpiricalLightModel(ParameterMap& params) {
    double headlightMaxTxRange = 0.0, taillightMaxTxRange = 0.0, headlightMaxTxAngle = 0.0, taillightMaxTxAngle = 0.0;
    ParameterMap::iterator it;

    it = params.find("headlightMaxTxRange");
    // check if parameter specified in config.xml
    if ( it != params.end() ){
        headlightMaxTxRange = it->second.doubleValue();
    }
    else{
        error("headlightMaxTxRange has not been specified in config-vlc.xml");
    }

    it = params.find("taillightMaxTxRange");
    if ( it != params.end() ){
        taillightMaxTxRange = it->second.doubleValue();
    }
    else{
        error("taillightMaxTxRange has not been specified in config-vlc.xml");
    }

    it = params.find("headlightMaxTxAngle");
    if ( it != params.end() ){
        headlightMaxTxAngle = it->second.doubleValue();
    }
    else{
        error("headlightMaxTxAngle has not been specified in config-vlc.xml");
    }

    it = params.find("taillightMaxTxAngle");
    if ( it != params.end() ){
        taillightMaxTxAngle = it->second.doubleValue();
    }
    else{
        error("taillightMaxTxAngle has not been specified in config-vlc.xml");
    }

    VLCDEBUG("[READING ANALOG MODEL PARAMETERS]"
            << "headlightMaxTxRange: " << headlightMaxTxRange
            << "taillightMaxTxRange: " << taillightMaxTxRange
            << "headlightMaxTxAngle: " << headlightMaxTxAngle
            << "taillightMaxTxAngle: " << taillightMaxTxAngle
            )

    // BasePhyLayer converts sensitivity from config.xml to mW @ line 72. Convert it back to dBm
    return new EmpiricalLightModel(FWMath::mW2dBm(sensitivity), headlightMaxTxRange, taillightMaxTxRange, headlightMaxTxAngle, taillightMaxTxAngle);
}

Decider* PhyLayerVlc::getDeciderFromName(std::string name, ParameterMap& params) {
  if(name == "DeciderVlc") {
      return initializeDeciderVlc(params);
  }
  return BasePhyLayer::getDeciderFromName(name, params);
}

AnalogueModel* PhyLayerVlc::initializeSimpleObstacleShadowing(ParameterMap& params){

    // init with default value
    double carrierFrequency = 5.890e+9;
    bool useTorus = world->useTorus();
    const Coord& playgroundSize = *(world->getPgs());

    ParameterMap::iterator it;

    // get carrierFrequency from config
    it = params.find("carrierFrequency");

    if ( it != params.end() ) // parameter carrierFrequency has been specified in config.xml
    {
        // set carrierFrequency
        carrierFrequency = it->second.doubleValue();
        coreEV << "initializeSimpleObstacleShadowing(): carrierFrequency set from config.xml to " << carrierFrequency << endl;

        // check whether carrierFrequency is not smaller than specified in ConnectionManager
        if(cc->hasPar("carrierFrequency") && carrierFrequency < cc->par("carrierFrequency").doubleValue())
        {
            // throw error
            throw cRuntimeError("initializeSimpleObstacleShadowing(): carrierFrequency can't be smaller than specified in ConnectionManager. Please adjust your config.xml file accordingly");
        }
    }
    else // carrierFrequency has not been specified in config.xml
    {
        if (cc->hasPar("carrierFrequency")) // parameter carrierFrequency has been specified in ConnectionManager
        {
            // set carrierFrequency according to ConnectionManager
            carrierFrequency = cc->par("carrierFrequency").doubleValue();
            coreEV << "createPathLossModel(): carrierFrequency set from ConnectionManager to " << carrierFrequency << endl;
        }
        else // carrierFrequency has not been specified in ConnectionManager
        {
            // keep carrierFrequency at default value
            coreEV << "createPathLossModel(): carrierFrequency set from default value to " << carrierFrequency << endl;
        }
    }

    ObstacleControl* obstacleControlP = ObstacleControlAccess().getIfExists();
    if (!obstacleControlP) throw cRuntimeError("initializeSimpleObstacleShadowing(): cannot find ObstacleControl module");
    return new SimpleObstacleShadowing(*obstacleControlP, carrierFrequency, useTorus, playgroundSize, coreDebug);
}

AnalogueModel* PhyLayerVlc::initializeVehicleObstacleShadowing(ParameterMap& params){

    // init with default value
    double carrierFrequency = 2.412e+9;
    bool useTorus = world->useTorus();
    const Coord& playgroundSize = *(world->getPgs());
    bool enableVlc = true;

    ParameterMap::iterator it;

    // get carrierFrequency from config
    it = params.find("carrierFrequency");

    if ( it != params.end() ) // parameter carrierFrequency has been specified in config.xml
    {
        // set carrierFrequency
        carrierFrequency = it->second.doubleValue();
        coreEV << "initializeVehicleObstacleShadowing(): `carrierFrequency` set from config-vlc.xml: " << carrierFrequency << endl;

        // check whether carrierFrequency is not smaller than specified in ConnectionManager
        if(cc->hasPar("carrierFrequency") && carrierFrequency < cc->par("carrierFrequency").doubleValue())
        {
            // throw error
            throw cRuntimeError("initializeSimpleObstacleShadowing(): carrierFrequency can't be smaller than specified in ConnectionManager. Please adjust your config.xml file accordingly");
        }
    }
    else // carrierFrequency has not been specified in config.xml
    {
        if (cc->hasPar("carrierFrequency")) // parameter carrierFrequency has been specified in ConnectionManager
        {
            // set carrierFrequency according to ConnectionManager
            carrierFrequency = cc->par("carrierFrequency").doubleValue();
            coreEV << "createPathLossModel(): carrierFrequency set from ConnectionManager to " << carrierFrequency << endl;
        }
        else // carrierFrequency has not been specified in ConnectionManager
        {
            // keep carrierFrequency at default value
            coreEV << "createPathLossModel(): carrierFrequency set from default value to " << carrierFrequency << endl;
        }
    }

    it = params.find("enableVlc");
    if ( it != params.end() ){
        enableVlc = it->second.boolValue();
    }
    else{
        error("`enableVlc` has not been specified in config-vlc.xml");
    }

    ObstacleControl* obstacleControlP = ObstacleControlAccess().getIfExists();
    if (!obstacleControlP) throw cRuntimeError("initializeVehicleObstacleShadowing(): cannot find ObstacleControl module");
    return new VehicleObstacleShadowing(*obstacleControlP, carrierFrequency, useTorus, playgroundSize, coreDebug, enableVlc);
}

Decider* PhyLayerVlc::initializeDeciderVlc(ParameterMap& params) {
  double centerFreq = params["centerFrequency"];
  ASSERT2(centerFreq == frequency, "`frequency` in omnetpp.ini is not the same with `centerFrequency` from `config.xml`");
  DeciderVlc* dec = new DeciderVlc(this, sensitivity, centerFreq, bandwidth, bitrate, direction, findHost()->getIndex(), collectCollisionStatistics, coreDebug);
  dec->setPath(getParentModule()->getFullPath());
  return dec;
}

void PhyLayerVlc::handleMessage(cMessage* msg) {
    //self messages
    if(msg->isSelfMessage()) {
        handleSelfMessage(msg);

    //MacPkts <- MacToPhyControlInfo
    // msg received from our upper layer, i.e., msg we should send to the channel
    } else if(msg->getArrivalGateId() == upperLayerIn) {
        setRadioState(Veins::Radio::TX);
        BasePhyLayer::handleUpperMessage(msg);

    //Control messages
    } else if(msg->getArrivalGateId() == upperControlIn) {
        BasePhyLayer::handleUpperControlMessage(msg);

    //AirFrames
    // msg received over air from other NICs
    } else if(msg->getKind() == AIR_FRAME){
        if(dynamic_cast<AirFrame11p *>(msg)){
            VLCDEBUG("Received message of type AirFrame11p, this message will be discarded")
            bubble("received message of type AirFrame11p, this message will be discarded!");
            getParentModule()->bubble("received message of type AirFrame11p, this message will be discarded!");
            delete msg;
            return;
        }
        if(dynamic_cast<AirFrameVlc *>(msg) ){
            AirFrameVlc* VlcMsg = check_and_cast<AirFrameVlc *>(msg);
            std::string txNode = VlcMsg->getSenderModule()->getModuleByPath("^.^.")->getFullName();
            if(txNode == getModuleByPath("^.^.")->getFullName() ){
                VLCDEBUG("Discarding received AirFrameVlc within the same host: " << VlcMsg->getSenderModule()->getFullPath())
                delete msg;
                return;
            }
            else{
                VLCDEBUG("AirFrameVlc id: " << VlcMsg->getId() <<" handed to VLC PHY from: " << VlcMsg->getSenderModule()->getFullPath())
                bubble("Handing AirFrameVlc to lower layers to decide if it can be received");
                BasePhyLayer::handleAirFrame(static_cast<AirFrame*>(msg));
            }
        }
    //unknown message
    } else {
        EV << "Unknown message received." << endl;
        delete msg;
    }
}

simtime_t PhyLayerVlc::setRadioState(int rs) {
    radio->switchTo(Radio::TX, simTime());
    radio->endSwitch(simTime());
    return SimTime(0);
}

AirFrame *PhyLayerVlc::encapsMsg(cPacket *macPkt)
{
    // Similar to createSignal() & attachSignal() of the Mac1609_4
    int headerLength = PHY_VLC_HEADER;      // length of the phy header (preamble w/o signal), used later when constructing phy frame
    setParametersForBitrate(bitrate);
    //macPkt->addBitLength(PHY_VLC_MHR);    // Not modeling the MAC frame size; MHR is part of PSDU according to standard so we assume the whole macPkt contains it

    // From Mac1609_4::attachSignal()
    simtime_t startTime = simTime();
    simtime_t duration = getFrameDuration(macPkt->getBitLength());  // Everything from the upper layer is considered as the PSDU

    // From Mac1609_4::createSignal()
    simtime_t start = startTime;
    simtime_t length = duration;

    simtime_t end = start + length;
    //create signal with start at current simtime and passed length
    Signal* s = new Signal(start, length);

    //create and set tx power mapping
    ConstMapping* txPowerMapping = BaseMacLayer::createSingleFrequencyMapping(start, end, frequency, bandwidth/2, FIXED_REFERENCE_POWER_MW);
    s->setTransmissionPower(txPowerMapping);

    Mapping* bitrateMapping = MappingUtils::createMapping(DimensionSet::timeDomain(), Mapping::STEPS);

    Argument pos(start);                            // Initialize a position with time dimension which corresponds to the start
    bitrateMapping->setValue(pos, bitrate);         // For current pos set the bitrate (this is the header bitrate indicator)

    pos.setTime(start + (headerLength / bitrate));  // Modify the time-point of pos to point to a time-point after the header
    bitrateMapping->setValue(pos, bitrate);         // Set new bitrate for the new time point (payload bitrate indicator)
    s->setBitrate(bitrateMapping);


    // From PhyLayer80211p::encapsMsg()
    AirFrameVlc* frame = new AirFrameVlc(macPkt->getName(), AIR_FRAME);
    frame->setHeadOrNot(setDirection(direction));

    // set the members
    VLCDEBUG("s->getDuration(): " << s->getDuration())
    assert(s->getDuration() > 0);
    frame->setDuration(s->getDuration());
    // copy the signal into the AirFrame
    frame->setSignal(*s);
    //set priority of AirFrames above the normal priority to ensure
    //channel consistency (before any thing else happens at a time
    //point t make sure that the channel has removed every AirFrame
    //ended at t and added every AirFrame started at t)
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
    VLCDEBUG("AirFrame w/ id: " << frame->getId() << " encapsulated, bit length: " << frame->getBitLength())

    return frame;
}

simtime_t PhyLayerVlc::getFrameDuration(int payloadLengthBits, enum PHY_MCS mcs) const {
    //FIXME: This needs to be fixed: the header part is from VLC, the rest from IEEE 802.11!
    simtime_t duration;
    if (mcs == MCS_DEFAULT) {
        // calculate frame duration according to Equation (17-29) of the IEEE 802.11-2007 standard
        duration = (PHY_VLC_SHR + PHY_VLC_HEADER)/bitrate + T_SYM_80211P * ceil( (16 + payloadLengthBits + 6)/(n_dbps) );
    }
    else {
        uint32_t ndbps = getNDBPS(mcs);
        duration = (PHY_VLC_SHR + PHY_VLC_HEADER)/bitrate + T_SYM_80211P * ceil( (16 + payloadLengthBits + 6)/(ndbps) );
    }

    return duration;
}

void PhyLayerVlc::setParametersForBitrate(uint64_t bitrate) {
    for (unsigned int i = 0; i < NUM_BITRATES_80211P; i++) {
        if (bitrate == BITRATES_80211P[i]) {
            n_dbps = N_DBPS_80211P[i];
            return;
        }
    }
    error("Chosen Bitrate is not valid; Valid rates are: 3Mbps, 4.5Mbps, 6Mbps, 9Mbps, 12Mbps, 18Mbps, 24Mbps and 27Mbps. Please adjust your omnetpp.ini file accordingly");
}

int PhyLayerVlc::setDirection(const std::string& direction) {

    if (direction == "head") {
        return HEAD;
    }
    else if (direction == "tail"){
        return TAIL;
    }
    else{
        error("Parameter `direction` is set wrongly in your config! It is neither `head` nor `tail`!");
        return 0;
    }
}

void PhyLayerVlc::handleSelfMessage(cMessage* msg) {

	switch(msg->getKind()) {
	//transmission overBasePhyLayer::
	case TX_OVER: {
		assert(msg == txOverTimer);
		//TODO: Need a mechanism to inform the upper layers that the txOverTimer arrived
		break;
	}
	//radio switch over
	case RADIO_SWITCHING_OVER:
		assert(msg == radioSwitchingOverTimer);
		BasePhyLayer::finishRadioSwitching();
		break;

	//AirFrame
	case AIR_FRAME:
		BasePhyLayer::handleAirFrame(static_cast<AirFrame*>(msg));
		break;

	//ChannelSenseRequest
	case CHANNEL_SENSE_REQUEST:
		BasePhyLayer::handleChannelSenseRequest(msg);
		break;

	default:
		break;
	}
}
