/*
 * @brief The callback for the timeout manager.
 *
 * This class has to be implemented by all classes that want to use the timeout manager. To signal
 * OMNeT++ that the call comes from another module Enter_Method("onTimeout"); has to be added on top
 * of the implemented onTimeout().
 *
 * @author Patrick Baldemaier, Florian Hagenauer
 */
#ifndef TIMEOUT_OBSERVER_H
#define TIMEOUT_OBSERVER_H

#include <omnetpp.h>

class TimeoutObserver {
	public:
		virtual void onTimeout(std::string name, short kind) = 0;
};

#endif /* TIMEOUT_OBSERVER_H */
