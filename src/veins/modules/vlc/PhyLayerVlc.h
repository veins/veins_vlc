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
 * Based on PhyLayer80211p.h from David Eckhoff
 */

#ifndef PHYLAYERVLC_H_
#define PHYLAYERVLC_H_

#include "veins/base/phyLayer/BasePhyLayer.h"
#include "veins/modules/phy/SNRThresholdDecider.h"
#include "veins/base/connectionManager/BaseConnectionManager.h"
#include "veins/base/utils/Move.h"

#include "veins/modules/mac/ieee80211p/Mac80211pToPhy11pInterface.h"
#include "veins/modules/vlc/DeciderVlc.h"
#include "veins/modules/vlc/analogueModel/EmpiricalLightModel.h"
#include "veins/modules/phy/Decider80211pToPhy80211pInterface.h"

#include "veins/modules/utility/ConstsPhy.h"
#include "veins/modules/vlc/utility/ConstsVlc.h"

using Veins::AirFrame;

#ifndef DBG
#define DBG EV
#define VLCDEBUG(word) if(debug) {std::cout << "Module: [" << getFullPath() <<"]: " << word << std::endl;}
#endif

class PhyLayerVlc   :   public BasePhyLayer
{
	public:
		void initialize(int stage);
	protected:
		/** @brief enable/disable detection of packet collisions */
		bool collectCollisionStatistics;

        double bitrate;
        double bandwidth;
        double frequency;
        double power;
        bool debug;
        bool deciderDebug;
        std::string direction;

        virtual void handleMessage(cMessage* msg);
        virtual void handleSelfMessage(cMessage* msg);

		/**
		 * @brief Creates and returns an instance of the AnalogueModel with the
		 * specified name.
		 *
		 * Is able to initialize the following AnalogueModels:
		 */
		virtual AnalogueModel* getAnalogueModelFromName(std::string name, ParameterMap& params);

        /**
         * @brief Creates and initializes a VehicleObstacleShadowing with the
         * passed parameter values.
         */
        AnalogueModel* initializeVehicleObstacleShadowing(ParameterMap& params);

        /**
         * @brief Creates and initializes a SimpleObstacleShadowing with the
         * passed parameter values.
         */
        AnalogueModel* initializeSimpleObstacleShadowing(ParameterMap& params);

        /**
         * @brief Creates and initializes the EmpiricalLightModel with the
         * passed parameter values.
         */
        AnalogueModel* initializeEmpiricalLightModel(ParameterMap& params);

		/**
		 * @brief Creates and returns an instance of the Decider with the specified
		 * name.
		 */
		virtual Decider* getDeciderFromName(std::string name, ParameterMap& params);

		virtual Decider* initializeDeciderVlc(ParameterMap& params);

		virtual simtime_t setRadioState(int rs);

		int setDirection(const std::string& direction);

		// Methods below are from Mac1609_4
		/**
         * @brief This function encapsulates messages from the upper layer into an
         * AirFrame and sets all necessary attributes.
         */
        virtual AirFrame *encapsMsg(cPacket *msg);
        simtime_t getFrameDuration(int payloadLengthBits) const;

};

#endif /* PHYLAYERVLC_H_ */
