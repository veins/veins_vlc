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

#include <omnetpp.h>
#include <veins-vlc/application/simpleVlcApp/SimpleVlcApp.h>

using namespace veins;

Define_Module(veins::SimpleVlcApp);

SimpleVlcApp::SimpleVlcApp()
    : traciManager(NULL)
    , mobility(NULL)
    , annotations(NULL)
{
}

SimpleVlcApp::~SimpleVlcApp()
{
}

void SimpleVlcApp::initialize(int stage)
{
    if (stage == 0) {
        toLower = findGate("lowerLayerOut");
        fromLower = findGate("lowerLayerIn");

        // Pointers to simulation modules
        traciManager = TraCIScenarioManagerAccess().get();
        ASSERT(traciManager);

        cModule* tmpMobility = getParentModule()->getSubmodule("veinsmobility");
        mobility = dynamic_cast<TraCIMobility*>(tmpMobility);
        ASSERT(mobility);

        annotations = AnnotationManagerAccess().getIfExists();
        ASSERT(annotations);

        sumoId = mobility->getExternalId();
        debug = par("debug").boolValue();
        byteLength = par("packetByteLength");
        transmissionPeriod = 1 / par("beaconingFrequency").doubleValue();
    }
    if (stage == 3) {
        auto dsrc = [this]() {
            VlcMessage* vlcMsg = new VlcMessage();
            vlcMsg->setAccessTechnology(DSRC);
            send(vlcMsg, toLower);
        };
        int vlcModule = BOTH_LIGHTS;
        auto vlc = [this, vlcModule]() {
            EV_INFO << "Sending VLC message of type: " << vlcModule << std::endl;
            VlcMessage* vlcMsg = generateVlcMessage(VLC, vlcModule);
            send(vlcMsg, toLower);
        };
        timerManager.create(veins::TimerSpecification(vlc).oneshotAt(SimTime(20, SIMTIME_S)));
    }
}

void SimpleVlcApp::handleMessage(cMessage* msg)
{
    // To handle the timer
    if (timerManager.handleMessage(msg)) return;

    if (msg->isSelfMessage()) {
        throw cRuntimeError("This module does not use custom self messages");
        return;
    }
    else {
        VlcMessage* vlcMsg = check_and_cast<VlcMessage*>(msg);
        int accessTech = vlcMsg->getAccessTechnology();
        switch (accessTech) {
        case DSRC: {
            EV_INFO << "DSRC message received!" << std::endl;
            delete vlcMsg;
            break;
        }
        case VLC: {
            EV_INFO << "VLC message received from: " << vlcMsg->getSourceNode() << std::endl;
            delete vlcMsg;
            break;
        }
        default:
            error("message neither from DSRC nor VLC");
            break;
        }
    }
}

VlcMessage* SimpleVlcApp::generateVlcMessage(int accessTechnology, int sendingModule)
{
    VlcMessage* vlcMsg = new VlcMessage();

    // OMNeT-specific
    vlcMsg->setName("vlcMessage");

    // WSM fields

    // HeterogeneousMessage specific
    vlcMsg->setSourceNode(this->sumoId.c_str());
    vlcMsg->setDestinationNode("BROADCAST");
    vlcMsg->setAccessTechnology(accessTechnology);
    vlcMsg->setTransmissionModule(sendingModule);
    vlcMsg->setSentAt(simTime()); // There is timestamp field in WSM too

    // Set application layer packet length
    vlcMsg->setByteLength(byteLength);

    return vlcMsg;
}
