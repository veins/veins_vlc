#include "veins/modules/analogueModel/VehicleObstacleShadowing.h"

#define debugEV (ev.isDisabled()||!debug) ? ev : ev << "PhyLayer(VehicleObstacleShadowing): "

VehicleObstacleShadowing::VehicleObstacleShadowing(ObstacleControl& obstacleControl, double carrierFrequency, bool useTorus, const Coord& playgroundSize, bool debug, bool enableVlc) :
	obstacleControl(obstacleControl),
	carrierFrequency(carrierFrequency),
	useTorus(useTorus),
	playgroundSize(playgroundSize),
	debug(debug),
    enableVlc(enableVlc)
{
	if (useTorus) throw cRuntimeError("VehicleObstacleShadowing does not work on torus-shaped playgrounds");
}


void VehicleObstacleShadowing::filterSignal(AirFrame *frame, const Coord& senderPos, const Coord& receiverPos){
	Signal& s = frame->getSignal();
	double factor = obstacleControl.calculateVehicleAttenuation(senderPos, receiverPos, s, enableVlc);
	bool hasFrequency = s.getTransmissionPower()->getDimensionSet().hasDimension(Dimension::frequency());
	const DimensionSet& domain = hasFrequency ? DimensionSet::timeFreqDomain() : DimensionSet::timeDomain();
	ConstantSimpleConstMapping* attMapping = new ConstantSimpleConstMapping(domain, factor);
	s.addAttenuation(attMapping);

	EV << "value is: " << factor << endl;
}
