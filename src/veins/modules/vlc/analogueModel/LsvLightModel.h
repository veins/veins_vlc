//
// Copyright (C) 2018-2018 Julien Jahneke <julien.jahneke@ccs-labs.org>
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

#include <cassert>

#include "veins/base/phyLayer/AnalogueModel.h"
#include "veins/modules/vlc/utility/ConstsVlc.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/world/annotations/AnnotationManager.h"
#include "veins/modules/vlc/utility/Utils.h"

#include "veins/modules/vlc/PhyLayerVlc.h"
#include "veins/modules/vlc/PhotoDiode.h"
#include "veins/modules/vlc/RadiationPattern.h"

using Veins::AirFrame;
using Veins::AnnotationManager;
using Veins::AnnotationManagerAccess;
using Veins::TraCIMobility;

namespace Veins {

/**
 * @brief This class returns the received power on the
 * receiving module, based on the headlight data provided by Lsv
 *
 * @ ingroup analogueModels
 *
 * The values are recorded as observed from the spectrum analyzer
 * to which the PD is connected -- this is electrical power
 */

class VEINS_API LsvLightModel : public AnalogueModel {
protected:
    AnnotationManager* annotations;

    bool debug = true;
    double sensitivity_dbm;

public:
    LsvLightModel(std::map<std::string, RadiationPattern>* RadiationPattern_Map, std::map<std::string, PhotoDiode>* PhotoDiode_Map, double sensitivity)
        : sensitivity_dbm(sensitivity)
        , RP_Map(RadiationPattern_Map)
        , PD_Map(PhotoDiode_Map)
    {
        annotations = AnnotationManagerAccess().getIfExists();
        ASSERT(annotations);
    };

    virtual void filterSignal(Signal* signal, const Coord& sendersPos, const Coord& receiverPos);
    virtual bool neverIncreasesPower() override
    {
        return true;
    }
    RadiationPattern* getRadiationPatternFromKey(std::string key);
    PhotoDiode* getPhotoDiodeFromKey(std::string key);

    double getFromMatrix(int LeftOrRight, double irradianceTheta, double irradiancePhi);
    bool inFOV(int LeftOrRight, double irradianceTheta, double irradiancePhi);
    void rotatePos(Coord& C, double rotAngle, double deltaX, double deltaY, double deltaZ);
    double getOpticalPower(double irradiance, double incidenceTheta, double incidencePhi);
    double getElectricalPowermW(double opticalPower);
    int getHeading(cModule* module);
    double getCurrentFactor();

    std::map<std::string, RadiationPattern>* RP_Map;
    std::map<std::string, PhotoDiode>* PD_Map;
    RadiationPattern* RP;
    PhotoDiode* PD;
};
} // namespace Veins
