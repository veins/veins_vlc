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
 * Based on Decider80211p.h from David Eckhoff
 * and modifications by Bastian Bloessl, Stefan Joerer, Michele Segata
 *
 * For the description of the methods see the similar functions in Decider80211p.h
 */

#ifndef DECIDERVLC_H_
#define DECIDERVLC_H_

#include "veins/base/phyLayer/BaseDecider.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"

using Veins::AirFrame;
using Veins::TraCIMobility;

#define DECIDER_VLC_DBG(word) if(debug) {std::cout << "\tModule: [" << myPhy << ".decider" <<"]: " << word << std::endl;}

class AirFrameVlc;

class DeciderVlc: public BaseDecider {
    public:
        enum PACKET_OK_RESULT {DECODED, NOT_DECODED, COLLISION};

    protected:
        bool debug;
        double centerFrequency;
        double bandwidth;
        double bitrate;
        std::string direction;

        double myBusyTime;
        double myStartTime;
        std::string myPhy;
        std::map<AirFrame*,int> signalStates;
        bool collectCollisionStats;
        unsigned int collisions;

    protected:
        DeciderResult* checkIfSignalOk(AirFrame* frame);
        virtual simtime_t processNewSignal(AirFrame* frame);
        virtual simtime_t processSignalEnd(AirFrame* frame);
        void calculateSinrAndSnrMapping(AirFrame* frame, Mapping **sinrMap, Mapping **snrMap);
        Mapping* calculateNoiseRSSIMapping(simtime_t_cref start, simtime_t_cref end, AirFrame *exclude);
        enum DeciderVlc::PACKET_OK_RESULT packetOk(double snirMin, double snrMin, int lengthMPDU, double bitrate, AirFrameVlc* frame);

    public:
        DeciderVlc(DeciderToPhyInterface* phy,
                      double sensitivity,
                      double centerFrequency,
                      double bw,
                      double bRate,
                      const std::string lightDir,
                      int myIndex = -1,
                      bool coreDebug = false,
                      bool collectCollisionStatistics = false,
                      bool debug = false):
            BaseDecider(phy, sensitivity, myIndex, coreDebug),
            centerFrequency(centerFrequency),
            bandwidth(bw),
            bitrate(bRate),
            direction(lightDir),
            collectCollisionStats(collectCollisionStatistics),
            debug(debug),
            myBusyTime(0),
            myStartTime(simTime().dbl())
            {
                DECIDER_VLC_DBG("DeciderVLC constructor called!");
                cModule *mod = check_and_cast<cModule *>(phy);
                myPhy = mod->getFullPath();
            }

        int getSignalState(AirFrame* frame);
        virtual ~DeciderVlc();
        virtual void finish();

};

#endif
