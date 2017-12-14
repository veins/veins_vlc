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

#include "veins/modules/vlc/utility/Utils.h"

double traci2omnetAngle2(double angle){
    // angle returned in radians

    if (angle >= -M_PI && angle < 0) angle = -angle;
    else if (angle >= 0 && angle <= M_PI) angle = 2*M_PI - angle;
    else throw cRuntimeError("unexpected angle");

    return angle;
}

double omnet2traciAngle2(double angle){
    // angle returned in radians

    if (angle >= 0 && angle <= M_PI) angle = -angle;
    else if (angle > M_PI && angle <= 2*M_PI) angle = 2*M_PI - angle;
    else throw cRuntimeError("unexpected angle");

    return angle;
}

double reverseTraci(double angle){
    // angle is in radians

    // no need to normalize because traci always returns between -180 and 180
    if (angle < 0) angle += M_PI;
    else if (angle >= 0) angle -= M_PI;
    else throw cRuntimeError("unexpected angle");

    return angle;
}

double rad2deg(double angleRad){
    return angleRad * 180.0 / M_PI;
}

double deg2rad(double angleDeg){
    return angleDeg * M_PI / 180.0;
}

double traci2cartesianAngle(double angleRad){

    double angle = angleRad;

    if (angle >= -M_PI && angle < 0) angle += 2*M_PI;
    else if (angle >= 0 && angle < M_PI) angle = angle;
    else throw cRuntimeError("unexpected angle");

    return angle;
}

double utilTrunc(double number){
    // keep only up to third decimal
    return std::trunc(number * 1000.0)/1000.0;
}

/*
 * same as FWMath::close(), except EPSILON
 */
bool close(double one, double two){
    return fabs(one-two)<0.0000001;
}
