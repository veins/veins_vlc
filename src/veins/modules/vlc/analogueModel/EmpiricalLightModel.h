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

// Based on previous implementation from Hua-Yen Tseng

#ifndef PER_MODEL_H
#define PER_MODEL_H

#include <cassert>

#include "veins/base/utils/MiXiMDefs.h"
#include "veins/base/phyLayer/AnalogueModel.h"
#include "veins/modules/vlc/utility/ConstsVlc.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/world/annotations/AnnotationManager.h"
#include "veins/modules/vlc/utility/Utils.h"

using Veins::AirFrame;
using Veins::TraCIMobility;
using Veins::AnnotationManager;
using Veins::AnnotationManagerAccess;

#define ELMDEBUG(word) if(debug) {std::cout << "\t\tELM: " << word << std::endl;}

/**
 * @brief This class returns the received power on the
 * receiving module, based on the empirical measurements
 * with taillight and headlight
 *
 * The values are recorded as observed from the spectrum analyzer
 * to which the PD is connected -- this is electrical power
 */
class MIXIM_API EmpiricalLightModel : public AnalogueModel {
protected:
    AnnotationManager* annotations;

    bool debug;
    double sensitivity_dbm;
    double headlightMaxTxRange;
    double taillightMaxTxRange;
    double headlightMaxTxAngle;
    double taillightMaxTxAngle;

public:
	EmpiricalLightModel(bool debug, double rxSensitivity_dbm, double m_headlightMaxTxRange, double m_taillightMaxTxRange, double m_headlightMaxTxAngle, double m_taillightMaxTxAngle):
	    debug(debug),
	    sensitivity_dbm(rxSensitivity_dbm),
	    headlightMaxTxRange(m_headlightMaxTxRange),
	    taillightMaxTxRange(m_taillightMaxTxRange),
	    headlightMaxTxAngle(m_headlightMaxTxAngle),
	    taillightMaxTxAngle(m_taillightMaxTxAngle)
    {
        // Transform degrees into radians
        headlightMaxTxAngle = deg2rad(headlightMaxTxAngle);
        taillightMaxTxAngle = deg2rad(taillightMaxTxAngle);

        ELMDEBUG("\tEmpiricalLightModel() constructor called...")
        ELMDEBUG("\trxSensitivity_dbm: " << sensitivity_dbm);
        ELMDEBUG("\theadlightMaxTxRange: " << headlightMaxTxRange);
        ELMDEBUG("\ttaillightMaxTxRange: " << taillightMaxTxRange);
        ELMDEBUG("\theadlightMaxTxAngle: " << headlightMaxTxAngle);
        ELMDEBUG("\ttaillightMaxTxAngle: " << taillightMaxTxAngle);

        annotations = AnnotationManagerAccess().getIfExists();
        ASSERT(annotations);
	};

	virtual void filterSignal(AirFrame *, const Coord&, const Coord&);

	int getHeading(cModule* module);

	bool isRecvPowerUnderSensitivity(int senderHeading, double distanceFromSenderToReceiver, const Coord& vectorFromTx2Rx, const Coord& vectorTxHeading, const Coord& vectorRxHeading);
    double calcReceivedPower(int senderHeading, double distanceFromSenderToReceiver, const Coord& vectorFromTx2Rx, const Coord& vectorTxHeading, const Coord& vectorRxHeading);
    double calcFittedReceivedPower(double distanceFromSenderToReceiver, const Coord& vectorFromTx2Rx, const Coord& vectorTxHeading);
};

#endif
