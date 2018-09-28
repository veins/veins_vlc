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
 * Based on Decider80211p.cc from David Eckhoff
 * and modifications by Bastian Bloessl, Stefan Joerer, Michele Segata
 */

#include "veins/modules/vlc/DeciderVlc.h"
#include "veins/modules/phy/DeciderResult80211.h"
#include "veins/base/phyLayer/Signal_.h"
#include "veins/modules/messages/AirFrame11p_m.h"
#include "veins/modules/vlc/messages/AirFrameVlc_m.h"
#include "veins/modules/utility/ConstsPhy.h"
#include "veins/modules/utility/Consts80211p.h"
#include "veins/base/utils/FWMath.h"
#include "veins/modules/vlc/utility/Utils.h"

using Veins::AirFrame;

simtime_t DeciderVlc::processNewSignal(AirFrame* msg) {
    DECIDER_VLC_DBG("processNewSignal()")
    AirFrameVlc *frame = check_and_cast<AirFrameVlc *>(msg);

    // get the receiving power of the Signal at start-time and center frequency
    Signal& signal = frame->getSignal();

    Argument start(DimensionSet::timeFreqDomain());
    start.setTime(signal.getReceptionStart());
    start.setArgValue(Dimension::frequency(), centerFrequency);

    signalStates[frame] = EXPECT_END;

    double recvPower = signal.getReceivingPower()->getValue(start);
    double recvPower_dbm = FWMath::mW2dBm(recvPower);

    DECIDER_VLC_DBG("Received power: " << recvPower
            << "\tReceived power (dbm): " << recvPower_dbm)

    if (recvPower < sensitivity) {
        DECIDER_VLC_DBG("Received power below sensitivity! Do not receive again.")
        frame->setUnderVlcSensitivity(true);
        return signal.getReceptionEnd();
    }
    else {
        setChannelIdleStatus(false);

        DECIDER_VLC_DBG("AirFrame: " << frame->getId() << " with [rxPower vs. sens]: (" << recvPower << " > " << sensitivity << ") -> Trying to receive AirFrame."
                << "\n\tNext handle time at the end of the signal @: "<< signal.getReceptionEnd() << " in: " << signal.getReceptionEnd() - simTime() )

        myBusyTime += signal.getDuration().dbl();

        if (!currentSignal.first) {
            //NIC is not yet synced to any frame, so sync to this one and try to decode it
            currentSignal.first = frame;
            DECIDER_VLC_DBG("Synced to AirFrame: " << frame->getId())
        }
        else {
            //NIC is currently trying to decode another frame. this frame will be simply treated as interference
            DECIDER_VLC_DBG("Already synced to another AirFrame. Treating AirFrame: "<< frame->getId() << " as interference")
        }

        return signal.getReceptionEnd();
    }
}

int DeciderVlc::getSignalState(AirFrame* frame) {

    if (signalStates.find(frame) == signalStates.end()) {
        return NEW;
    }
    else {
        return signalStates[frame];
    }
}

void DeciderVlc::calculateSinrAndSnrMapping(AirFrame* frame, Mapping **sinrMap, Mapping **snrMap) {

    /* calculate Noise-Strength-Mapping */
    Signal& signal = frame->getSignal();

    simtime_t start = signal.getReceptionStart();
    simtime_t end   = signal.getReceptionEnd();

    //call BaseDecider function to get Noise plus Interference mapping
    Mapping* noiseInterferenceMap = calculateRSSIMapping(start, end, frame);
    assert(noiseInterferenceMap);
    //call calculateNoiseRSSIMapping() to get Noise only mapping
    Mapping* noiseMap = calculateNoiseRSSIMapping(start, end, frame);
    assert(noiseMap);
    //get power map for frame currently under reception
    ConstMapping* recvPowerMap = signal.getReceivingPower();
    assert(recvPowerMap);

    DECIDER_VLC_DBG("recvPowerMap: " << *recvPowerMap
            << "\nnoiseMap: " << *noiseMap
            << "\nnoiseInterferenceMap: " << *noiseInterferenceMap);

    //TODO: handle noise of zero (must not devide with zero!)
    *sinrMap = MappingUtils::divide( *recvPowerMap, *noiseInterferenceMap, Argument::MappedZero() );
    *snrMap = MappingUtils::divide( *recvPowerMap, *noiseMap, Argument::MappedZero() );

    delete noiseInterferenceMap;
    noiseInterferenceMap = 0;
    delete noiseMap;
    noiseMap = 0;

}

Mapping* DeciderVlc::calculateNoiseRSSIMapping(simtime_t_cref start, simtime_t_cref end, AirFrame *exclude) {

    // create an empty mapping
    Mapping* resultMap = MappingUtils::createMapping(Argument::MappedZero(), DimensionSet::timeDomain());

    // add thermal noise
    ConstMapping* thermalNoise = phy->getThermalNoise(start, end);
    if (thermalNoise) {
        // FIXME: workaround needed to make *really* sure that the resultMap is defined for the range of the exclude-frame
        const ConstMapping* excludePwr = exclude ? exclude->getSignal().getReceivingPower() : 0;
        if (excludePwr) {
            Mapping* p1 = resultMap;
            // p2 = exclude + thermal
            Mapping* p2 = MappingUtils::add(*excludePwr, *thermalNoise);
            // p3 = p2 - exclude
            Mapping* p3 = MappingUtils::subtract(*p2, *excludePwr);
            // result = result + p3
            resultMap = MappingUtils::add(*resultMap, *p3);
            delete p3;
            delete p2;
            delete p1;
        }
        else {
            Mapping* p1 = resultMap;
            resultMap = MappingUtils::add(*resultMap, *thermalNoise);
            delete p1;
        }
    }

    return resultMap;

}

DeciderResult* DeciderVlc::checkIfSignalOk(AirFrame* frame) {

    Mapping* sinrMap = 0;
    Mapping *snrMap = 0;

    if (collectCollisionStats) {
        calculateSinrAndSnrMapping(frame, &sinrMap, &snrMap);
        assert(snrMap);
    }
    else {
        // calculate SINR
        sinrMap = calculateSnrMapping(frame);
    }
    assert(sinrMap);

    Signal& s = frame->getSignal();
    simtime_t start = s.getReceptionStart();
    simtime_t end = s.getReceptionEnd();

    //compute receive power
    Argument st(DimensionSet::timeFreqDomain());
    st.setTime(s.getReceptionStart());
    st.setArgValue(Dimension::frequency(), centerFrequency);
    double recvPower_dBm = 10*log10(s.getReceivingPower()->getValue(st));

//    start = start + PHY_HDR_PREAMBLE_DURATION; //its ok if something in the training phase is broken
    start = start + PHY_VLC_SHR/bitrate;

    Argument min(DimensionSet::timeFreqDomain());
    min.setTime(start);
    min.setArgValue(Dimension::frequency(), centerFrequency - bandwidth/2);
    Argument max(DimensionSet::timeFreqDomain());
    max.setTime(end);
    max.setArgValue(Dimension::frequency(), centerFrequency + bandwidth/2);

    double snirMin = MappingUtils::findMin(*sinrMap, min, max);
    double snrMin;
    if (collectCollisionStats) {
        snrMin = MappingUtils::findMin(*snrMap, min, max);
    }
    else {
        //just set to any value. if collectCollisionStats != true
        //it will be ignored by packetOk
        snrMin = 1e200;
    }

    ConstMappingIterator* bitrateIt = s.getBitrate()->createConstIterator();
    bitrateIt->next(); //iterate to payload's bitrate indicator
    double payloadBitrate = bitrateIt->getValue();
    delete bitrateIt;

    DeciderResult80211* result = 0;

    enum PACKET_OK_RESULT packetOkRes = packetOk(snirMin, snrMin, frame->getBitLength(), payloadBitrate, check_and_cast<AirFrameVlc *>(frame));

    switch (packetOkRes) {

        case DECODED:
            DECIDER_VLC_DBG("Packet is fine! We can decode it")
            result = new DeciderResult80211(true, payloadBitrate, snirMin, recvPower_dBm, false);
            break;

        case NOT_DECODED:
            if (!collectCollisionStats) {
                DECIDER_VLC_DBG("Packet has bit Errors. Lost ")
            }
            else {
                DECIDER_VLC_DBG("Packet has bit Errors due to low power. Lost ")
            }
            result = new DeciderResult80211(false, payloadBitrate, snirMin, recvPower_dBm, false);
            break;

        case COLLISION:
            DECIDER_VLC_DBG("Packet has bit Errors due to collision. Lost ")
            collisions++;
            result = new DeciderResult80211(false, payloadBitrate, snirMin, recvPower_dBm, true);
            break;

        default:
            ASSERT2(false, "Impossible packet result returned by packetOk(). Check the code.");
            break;

    }

    delete sinrMap;
    if (snrMap)
        delete snrMap;
    return result;
}

enum DeciderVlc::PACKET_OK_RESULT DeciderVlc::packetOk(double snirMin, double snrMin, int lengthMPDU, double bitrate, AirFrameVlc* frame) {
    //compute success rate depending on mcs and bw
    double packetOkSinr = getOokPdr(snirMin, lengthMPDU);    // PDR w/o FEC

    //check if header is broken
    double headerOkSinr = getOokPdr(snirMin, PHY_VLC_SHR);

    double packetOkSnr;
    double headerOkSnr;

    //compute PER also for SNR only
    if (collectCollisionStats) {

        packetOkSnr = getOokPdr(snrMin, lengthMPDU);

        headerOkSnr = getOokPdr(snrMin, PHY_VLC_SHR);

        //the probability of correct reception without considering the interference
        //MUST be greater or equal than when consider it

        ASSERT( close(packetOkSnr, packetOkSinr) || (packetOkSnr > packetOkSinr) );
        ASSERT( close(headerOkSnr, headerOkSinr) || (headerOkSnr > headerOkSinr) );
    }

    //probability of no bit error in the PLCP header
    double rand = RNGCONTEXT dblrand();

    // Header part
    if (!collectCollisionStats) {
        if (rand > headerOkSinr)
            return NOT_DECODED;
    }
    else {

        if (rand > headerOkSinr) {
            //ups, we have a header error. is that due to interference?
            if (rand > headerOkSnr) {
                //no. we would have not been able to receive that even
                //without interference
                return NOT_DECODED;
            }
            else {
                //yes. we would have decoded that without interference
                return COLLISION;
            }

        }
    }

    //probability of no bit error in the rest of the packet
    rand = RNGCONTEXT dblrand();

    // Packet part
    if (!collectCollisionStats) {
        if (rand > packetOkSinr) {
            return NOT_DECODED;
        }
        else {
            return DECODED;
        }
    }
    else {

        if (rand > packetOkSinr) {
            //ups, we have an error in the payload. is that due to interference?
            if (rand > packetOkSnr) {
                //no. we would have not been able to receive that even
                //without interference
                return NOT_DECODED;
            }
            else {
                //yes. we would have decoded that without interference
                return COLLISION;
            }

        }
        else {
            return DECODED;
        }

    }

}

simtime_t DeciderVlc::processSignalEnd(AirFrame* msg) {
    DECIDER_VLC_DBG("processSignalEnd()")
    AirFrameVlc *frame = check_and_cast<AirFrameVlc *>(msg);

    // here the Signal is finally processed
    Signal& signal = frame->getSignal();

    Argument start(DimensionSet::timeFreqDomain());
    start.setTime(signal.getReceptionStart());
    start.setArgValue(Dimension::frequency(), centerFrequency);

    // Forget the state information about this frame
    signalStates.erase(frame);

    DeciderResult* result;

    if (frame->getUnderVlcSensitivity()) {
        //this frame was not even detected by the radio card
        result = new DeciderResult(false);
    }
    else {  // If detected by the radio
        //first check whether this is the frame NIC is currently synced to
        if (frame == currentSignal.first) {
            // Try to decode the frame, i.e., check if the Decider received the frame correctly
            result = checkIfSignalOk(frame);

            //after having tried to decode the frame, the NIC is no more synced to the frame
            //and it is ready for syncing on a new one
            currentSignal.first = 0;
        }
        else {
            //if this is not the frame we are synced on, we cannot receive it
            result = new DeciderResult(false);
        }
    }

    if (result->isSignalCorrect()) {
        DECIDER_VLC_DBG("Packet was received correctly, it is now handed to upper layer")

        // go on with processing this AirFrame, send it to the Mac-Layer
        phy->sendUp(frame, result);
    }
    else {
        // To investigate why the packet was not received correctly
        delete result;
    }

    return notAgain;
}

void DeciderVlc::finish() {
}

DeciderVlc::~DeciderVlc() {}
;
