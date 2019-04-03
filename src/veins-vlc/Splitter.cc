//
// Copyright (C) 2017 Agon Memedi <memedi@ccs-labs.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later
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

#include "veins-vlc/Splitter.h"
#include "veins-vlc/messages/VlcMessage_m.h"
#include "veins-vlc/utility/Utils.h"

using namespace veins;

Define_Module(veins::Splitter);

Splitter::Splitter()
    : annotationManager(NULL)

{
}

Splitter::~Splitter()
{
}

void Splitter::initialize()
{
    // From upper layers --> lower layers
    fromApplication = findGate("applicationIn");
    toDsrcNic = findGate("nicOut");
    toVlcHead = findGate("nicVlcHeadOut");
    toVlcTail = findGate("nicVlcTailOut");

    // From lower layers --> upper layers
    toApplication = findGate("applicationOut");
    fromDsrcNic = findGate("nicIn");
    fromVlcHead = findGate("nicVlcHeadIn");
    fromVlcTail = findGate("nicVlcTailIn");

    // Module parameters
    draw = par("draw").boolValue();
    headHalfAngle = deg2rad(par("drawHeadHalfAngle").doubleValue());
    tailHalfAngle = deg2rad(par("drawTailHalfAngle").doubleValue());
    collectStatistics = par("collectStatistics").boolValue();
    debug = par("debug").boolValue();

    // Signals
    totalVlcDelaySignal = registerSignal("totalVlcDelay");
    headVlcDelaySignal = registerSignal("headVlcDelay");
    tailVlcDelaySignal = registerSignal("tailVlcDelay");

    // Other simulation modules
    cModule* tmpMobility = getParentModule()->getSubmodule("veinsmobility");
    mobility = dynamic_cast<TraCIMobility*>(tmpMobility);
    ASSERT(mobility);

    vlcPhys = getSubmodulesOfType<PhyLayerVlc>(getParentModule(), true);
    ASSERT(vlcPhys.size() > 0);

    annotationManager = AnnotationManagerAccess().getIfExists();
    ASSERT(annotationManager);
}

void Splitter::handleMessage(cMessage* msg)
{
    if (timerManager.handleMessage(msg)) return;

    if (msg->isSelfMessage()) {
        error("Self-message arrived!");
        delete msg;
        msg = NULL;
    }
    else {
        int arrivalGate = msg->getArrivalGateId();
        if (arrivalGate == fromApplication) {
            handleUpperMessage(msg);
        }
        // The arrival gate is not from the application, it'a from lower layers
        // TODO: add annotation drawings based on the destination technology
        else {
            EV_INFO << "Message from lower layers received!" << std::endl;
            handleLowerMessage(msg);
        }
    }
}

void Splitter::handleUpperMessage(cMessage* msg)
{
    // Cast the message to a subclass
    VlcMessage* vlcMsg = dynamic_cast<VlcMessage*>(msg);

    // Handle WSMs if the VLC has to be "retrofitted" to non-VLC app
    if (!vlcMsg) {
        BaseFrame1609_4* wsm = dynamic_cast<BaseFrame1609_4*>(msg);

        // If not a VlcMessage check whether it is a WSM to send directly
        if (wsm) {
            send(wsm, toDsrcNic);
            return;
        }
        else
            error("Not a VlcMessage, not BaseFrame1609_4");
    }

    // if (vlcMsg)...
    int networkType = vlcMsg->getAccessTechnology();
    if (networkType == DSRC) {
        EV_INFO << "DSRC message received from upper layer!" << std::endl;
        send(vlcMsg, toDsrcNic);
    }
    else if (networkType == VLC) {
        EV_INFO << "VLC message received from upper layer!" << std::endl;
        if (draw) {
            // Won't draw at simTime() < 0.1 as TraCI is not connected and annotation fails
            auto drawCones = [this]() {
                // Headlight, right
                drawRayLine(vlcPhys[0]->getAntennaPosition(), 100, headHalfAngle);
                // left
                drawRayLine(vlcPhys[0]->getAntennaPosition(), 100, -headHalfAngle);
                // Taillight, left
                drawRayLine(vlcPhys[1]->getAntennaPosition(), 30, tailHalfAngle, true);
                // right
                drawRayLine(vlcPhys[1]->getAntennaPosition(), 30, -tailHalfAngle, true);
            };
            // The cones will be drawn immediately as a message is received from the layer above
            timerManager.create(veins::TimerSpecification(drawCones).oneshotAt(simTime()));
        }

        int lightModule = vlcMsg->getTransmissionModule();

        switch (lightModule) {
        case HEADLIGHT:
            headlightPacketsSent++;
            send(vlcMsg, toVlcHead);
            break;
        case TAILLIGHT:
            taillightPacketsSent++;
            send(vlcMsg, toVlcTail);
            break;
        case DONT_CARE:
            error("behavior not specified for DONT_CARE");
            break;
        case BOTH_LIGHTS: {
            vlcPacketsSent++;
            VlcMessage* toHead = vlcMsg->dup();
            toHead->setTransmissionModule(HEADLIGHT);
            send(toHead, toVlcHead);

            VlcMessage* toTail = vlcMsg->dup();
            toTail->setTransmissionModule(TAILLIGHT);
            send(toTail, toVlcTail);
            delete vlcMsg;
            vlcMsg = NULL;
            break;
        }
        default:
            error("\tThe light module has not been specified in the message!");
            break;
        }
    }
    else {
        error("\tThe access technology has not been specified in the message!");
    }
}

void Splitter::handleLowerMessage(cMessage* msg)
{
    int lowerGate = msg->getArrivalGateId();

    // !(lowerGate == fromDsrcNic) --> (lowerGate  == fromVlcHead || fromVlcTail)
    // If the message is from any of the VLC modules and we need to collect statistics
    if (!(lowerGate == fromDsrcNic) && collectStatistics) {
        vlcPacketsReceived++;
        VlcMessage* vlcMsg = dynamic_cast<VlcMessage*>(msg);

        emit(totalVlcDelaySignal, simTime() - vlcMsg->getSentAt());

        int srclightModule = vlcMsg->getTransmissionModule();
        if (srclightModule == HEADLIGHT) {
            headlightPacketsReceived++;
            emit(headVlcDelaySignal, simTime() - vlcMsg->getSentAt());
        }
        else if (srclightModule == TAILLIGHT) {
            taillightPacketsReceived++;
            emit(tailVlcDelaySignal, simTime() - vlcMsg->getSentAt());
        }
        else
            error("neither `head` nor `tail`");
    }

    send(msg, toApplication);
}

void Splitter::drawRayLine(const AntennaPosition& ap, int length, double halfAngle, bool reverse)
{
    double heading = mobility->getHeading().getRad();
    // This is for the cone of the tail
    if (reverse) heading = reverseTraci(heading);

    annotationManager->scheduleErase(0.1,
        annotationManager->drawLine(ap.getPositionAt(),
        ap.getPositionAt() + Coord(length * cos(halfAngle + traci2myAngle(heading)), length * sin(halfAngle + traci2myAngle(heading))),
        "white"));
}

void Splitter::finish()
{
    // 'headlightPacketsSent' and 'taillightPacketsSent' will be zero if packets are not explicitly sent via them
    recordScalar("headlightPacketsSent", headlightPacketsSent);
    recordScalar("headlightPacketsReceived", headlightPacketsReceived);
    recordScalar("taillightPacketsSent", taillightPacketsSent);
    recordScalar("taillightPacketsReceived", taillightPacketsReceived);
    recordScalar("vlcPacketsSent", vlcPacketsSent);
    recordScalar("vlcPacketsReceived", vlcPacketsReceived);
}
