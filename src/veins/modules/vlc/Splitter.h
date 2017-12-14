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

#ifndef __VEINS_SPLITTER_H_
#define __VEINS_SPLITTER_H_

#include <omnetpp.h>

#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/utility/timeoutManager/TimeoutObserver.h"
#include "veins/modules/utility/timeoutManager/TimeoutManager.h"
#include "veins/modules/world/annotations/AnnotationManager.h"

using Veins::TraCIMobility;
using Veins::AnnotationManager;
using Veins::AnnotationManagerAccess;

#define DRAW_CONES 1

#define SPOUT(word) if(debug) {std::cout << "Module: [" << getFullPath() <<"]: " << word << std::endl;}

class Splitter : public cSimpleModule, public TimeoutObserver
{
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
    TimeoutManager* timeoutManager;
    AnnotationManager* annotationManager;

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
    virtual void handleMessage(cMessage *msg);
    virtual void finish();
    void handleUpperMessage(cMessage *msg);
    void handleLowerMessage(cMessage *msg);

    virtual void onTimeout(std::string name, short int kind);

    void drawCone(int length, double halfAngle, bool reverse=false);
};

#endif
