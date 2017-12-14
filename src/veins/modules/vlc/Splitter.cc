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

#include "veins/modules/vlc/Splitter.h"
#include "veins/modules/vlc/messages/VlcMessage_m.h"
#include "veins/modules/vlc/utility/Utils.h"

Define_Module(Splitter);

Splitter::Splitter():
    timeoutManager(NULL),
    annotationManager(NULL)
{}

Splitter::~Splitter(){}

void Splitter::initialize()
{
    // From upper layers --> lower layers
    fromApplication =  findGate("applicationIn");
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
    cModule *tmpMobility = getParentModule()->getSubmodule("veinsmobility");
    mobility = dynamic_cast<TraCIMobility*>(tmpMobility);
    ASSERT(mobility);

    cModule* tmp = getParentModule()->getSubmodule("timeoutManager");
    timeoutManager = dynamic_cast<TimeoutManager*>(tmp);
    ASSERT(timeoutManager);

    annotationManager = AnnotationManagerAccess().getIfExists();
    ASSERT(annotationManager);
}

void Splitter::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()){
        error("Self-message arrived!");
        delete msg; msg = NULL;
    }
    else{
        int arrivalGate = msg->getArrivalGateId();
        if(arrivalGate == fromApplication){
            handleUpperMessage(msg);
        }
        // The arrival gate is not from the application, it'a from lower layers
        // TODO: add annotation drawings based on the destination technology
        else{
            SPOUT("Message from lower layers received!");
            handleLowerMessage(msg);
        }
    }
}

void Splitter::handleUpperMessage(cMessage *msg){
    // Cast the message to a subclass
    VlcMessage *vlcMsg = dynamic_cast<VlcMessage *>(msg);

    // Handle WSMs if the VLC has to be "retrofitted" to non-VLC app
    if (!vlcMsg) {
        WaveShortMessage *wsm = dynamic_cast<WaveShortMessage*>(msg);

        // If not a VlcMessage check whether it is a WSM to send directly
        if (wsm != NULL && !strcmp(msg->getClassName(), "WaveShortMessage") ){
            send(wsm, toDsrcNic);
            return;
        }
        else
            error("Not a VlcMessage, not WaveShortMessage");
    }

    // if (vlcMsg)...
    int networkType = vlcMsg->getAccessTechnology();
    if(networkType == DSRC){
        SPOUT("DSRC message received from upper layer!");
        send(vlcMsg, toDsrcNic);
    }
    else if(networkType == VLC){
        SPOUT("VLC message received from upper layer!");
        if(draw){
            // Won't draw at Simtime() < 0.1 as TraCI is not connected and annotation fails
            timeoutManager->setTimeout(this, DRAW_CONES, 0 + uniform(0.0,0.1) );
        }


        int lightModule = vlcMsg->getHeadOrTail();

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
            case BOTH_LIGHTS:
            {
                vlcPacketsSent++;
                VlcMessage *toHead = vlcMsg->dup();
                toHead->setHeadOrTail(HEADLIGHT);
                send(toHead, toVlcHead);

                VlcMessage *toTail = vlcMsg->dup();
                toTail->setHeadOrTail(TAILLIGHT);
                send(toTail, toVlcTail);
                delete vlcMsg; vlcMsg = NULL;
                break;
            }
            default:
                error("\tThe light module has not been specified in the message!");
                break;
        }
    }
    else{
        error("\tThe access technology has not been specified in the message!");
    }
}

void Splitter::handleLowerMessage(cMessage *msg){
    int lowerGate = msg->getArrivalGateId();

    // !(lowerGate == fromDsrcNic) --> (lowerGate  == fromVlcHead || fromVlcTail)
    // If the message is from any of the VLC modules and we need to collect statistics
    if(!(lowerGate == fromDsrcNic) && collectStatistics){
        vlcPacketsReceived++;
        VlcMessage *vlcMsg = dynamic_cast<VlcMessage *>(msg);

        emit(totalVlcDelaySignal, simTime() - vlcMsg->getSentAt());

        int srclightModule = vlcMsg->getHeadOrTail();
        if(srclightModule == HEADLIGHT){
            headlightPacketsReceived++;
            emit(headVlcDelaySignal, simTime() - vlcMsg->getSentAt());
        }
        else if(srclightModule == TAILLIGHT){
            taillightPacketsReceived++;
            emit(tailVlcDelaySignal, simTime() - vlcMsg->getSentAt());
        }
        else
            error("neither `head` nor `tail`");
    }

    send(msg, toApplication);
}

void Splitter::onTimeout(std::string name, short int kind){
    Enter_Method("onTimeout");

    switch (kind) {
        case DRAW_CONES:
        {
            //timeoutManager->setTimeout(this, DRAW_CONES, 0.1 + uniform(0.0,0.1) );

            // Headlight, right
            drawCone(100, headHalfAngle);
            // left
            drawCone(100, -headHalfAngle);
            // Taillight, left
            drawCone(30, tailHalfAngle, true);
            // right
            drawCone(30, -tailHalfAngle, true);
            break;
        }
        default:
        {
            // None
            break;
        }

    }
}

void Splitter::drawCone(int length, double halfAngle, bool reverse){
    double heading = mobility->getAngleRad();
    // This is for the cone of the tail
    if (reverse) heading = reverseTraci(heading);

    annotationManager->scheduleErase(0.1,
            annotationManager->drawLine(
                    mobility->getCurrentPosition(),
                    mobility->getCurrentPosition() +
                        Coord(length*cos( halfAngle + traci2omnetAngle2( heading ) ),
                                length*sin( halfAngle + traci2omnetAngle2( heading ) ) ) ,
                    "white") );
}

void Splitter::finish(){
    // 'headlightPacketsSent' and 'taillightPacketsSent' will be zero if packets are not explicitly sent via them
    recordScalar("headlightPacketsSent", headlightPacketsSent);
    recordScalar("headlightPacketsReceived", headlightPacketsReceived);
    recordScalar("taillightPacketsSent", taillightPacketsSent);
    recordScalar("taillightPacketsReceived", taillightPacketsReceived);
    recordScalar("vlcPacketsSent", vlcPacketsSent);
    recordScalar("vlcPacketsReceived", vlcPacketsReceived);
}
