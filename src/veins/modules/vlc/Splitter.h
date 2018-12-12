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

#pragma once

#include <omnetpp.h>

#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/utility/TimerManager.h"
#include "veins/modules/world/annotations/AnnotationManager.h"

#include "veins/modules/vlc/PhyLayerVlc.h"

using Veins::AnnotationManager;
using Veins::AnnotationManagerAccess;
using Veins::TraCIMobility;

namespace Veins {

class Splitter : public cSimpleModule {
public:
    Splitter();
    ~Splitter();

protected:
    // Gates
    int toApplication;
    int fromApplication;
    int toDsrcNic;
    int fromDsrcNic;
    int toVlcTail;
    int fromVlcTail;
    int toVlcHead;
    int fromVlcHead;

    // Variables
    bool debug;
    bool collectStatistics;
    bool draw;
    double headHalfAngle;
    double tailHalfAngle;
    TraCIMobility* mobility;
    AnnotationManager* annotationManager;
    Veins::TimerManager timerManager{this};
    std::vector<PhyLayerVlc*> vlcPhys;

    // Statistics
    int headlightPacketsSent = 0;
    int taillightPacketsSent = 0;
    int vlcPacketsSent = 0;
    int headlightPacketsReceived = 0;
    int taillightPacketsReceived = 0;
    int vlcPacketsReceived = 0;

    // Signals
    simsignal_t totalVlcDelaySignal;
    simsignal_t headVlcDelaySignal;
    simsignal_t tailVlcDelaySignal;

    virtual void initialize();
    virtual void handleMessage(cMessage* msg);
    virtual void finish();
    void handleUpperMessage(cMessage* msg);
    void handleLowerMessage(cMessage* msg);

    void drawRayLine(Coord pos, int length, double halfAngle, bool reverse = false);
};

} // namespace Veins
