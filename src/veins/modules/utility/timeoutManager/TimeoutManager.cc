//
// Copyright (C) 2006-2017 Florian Hagenauer <hagenauer@ccs-labs.org>
//
// Documentation for these modules is at http://veins.car2x.org/
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

#include "TimeoutManager.h"

Define_Module(TimeoutManager);

TimeoutManager::TimeoutManager() {
	debug = false;
}

TimeoutManager::~TimeoutManager() {
}

void TimeoutManager::setTimeout(TimeoutObserver* sender, std::string name, short kind, simtime_t delay, bool cancel) {
	Enter_Method("setTimeout");

	cMessage* msg = getTimeout(sender, name, kind);

	if (msg != NULL) {
		if(cancel){
		    // There is such timeout but needs to be canceled, reset it
            cancelEvent(msg);
            scheduleAt(simTime() + delay, msg);
		}
		else{
            msg = new cMessage(name.c_str());
            msg->setKind(kind);
            scheduleAt(simTime() + delay, msg);     // Set the timeout to self (TimoutObserver)
            timeoutTable.insert(std::make_pair(TimeoutKey(sender, name, kind), msg));
            reverseTimeoutTable[msg->getId()] = sender;
		}
	} else if (msg == NULL) {
		// There is no such timeout, create it.
		msg = new cMessage(name.c_str());
		msg->setKind(kind);
		scheduleAt(simTime() + delay, msg);     // Set the timeout
		timeoutTable.insert(std::make_pair(TimeoutKey(sender, name, kind), msg));
		reverseTimeoutTable[msg->getId()] = sender;
	}
}

void TimeoutManager::setTimeout(TimeoutObserver* sender, short kind, simtime_t delay) {
	Enter_Method
	("setTimeout");
	setTimeout(sender, "", kind, delay);
}

/* Use this method to send the message exactly at the given time interval of absolute simTime() */
void TimeoutManager::setIntervalTimeout(TimeoutObserver* sender, short kind, simtime_t lowerBound, simtime_t upperBound){
    Enter_Method("setIntervalTimeout");

    simtime_t timeOfArrival = simTime() - floor(simTime());
    simtime_t delay;

    if (timeOfArrival < lowerBound){
        delay = uniform((lowerBound - timeOfArrival) , (upperBound - timeOfArrival));
    }
    else if ( (timeOfArrival >= lowerBound) && (timeOfArrival < upperBound) ) {
        delay = uniform(0.0, (upperBound - timeOfArrival));
    }
    else if (timeOfArrival >= upperBound){
        std::cerr<< "The timeout arrived late!" << simTime() << std::endl;
    }
    else{
//        throw cRuntimeError("Message arrived after the supposed deadline (upperBound)");
    }
    setTimeout(sender, "", kind, delay);
}

void TimeoutManager::removeEntry(TimeoutMessageTable::iterator it) {
	Enter_Method("removeEntry");
	cMessage *msg = it->second;
	reverseTimeoutTable.erase(msg->getId());
	timeoutTable.erase(it);
	cancelAndDelete(msg);
}

void TimeoutManager::unsetTimeout(TimeoutObserver* sender, short kind) {
	Enter_Method("unsetTimeout");
	unsetTimeout(sender, "", kind);
}

void TimeoutManager::unsetTimeout(TimeoutObserver* sender, std::string name, short kind) {
	Enter_Method("unsetTimeout");
	TimeoutKey key(sender, name, kind);
	TimeoutMessageTable::iterator it = timeoutTable.find(key);
	if (it != timeoutTable.end()) {
		removeEntry(it);
	}
}

void TimeoutManager::unsetAll(TimeoutObserver* sender) {
	Enter_Method("unsetAll");
	TimeoutKey key(sender);
	std::pair<TimeoutMessageTable::iterator, TimeoutMessageTable::iterator> range =
	        timeoutTable.equal_range(key);
	if (range.first != range.second) {
		for (TimeoutMessageTable::iterator it = range.first; it != range.second; it++) {
			removeEntry(it);
		}
	}
}

bool TimeoutManager::hasTimeout(TimeoutObserver* sender, std::string name, short kind) {
	Enter_Method("hasTimeout");
	return getTimeout(sender, name, kind) != NULL;
}

cMessage* TimeoutManager::getTimeout(TimeoutObserver* sender, std::string name, short kind) {
	Enter_Method("getTimeout");
	TimeoutKey key(sender, name, kind);
	TimeoutMessageTable::iterator it = timeoutTable.find(key);

	if (it != timeoutTable.end()) {
		return it->second;
	}
	return NULL;
}

bool TimeoutManager::handleTimeoutMessage(cMessage* msg) {
	Enter_Method("handleTimeoutMessage");

	std::map<long, TimeoutObserver*>::iterator it = reverseTimeoutTable.find(msg->getId());

	if (it == reverseTimeoutTable.end()) {
		return false;
	}

	TimeoutObserver* too = it->second;
	std::string name = msg->getName();
	short kind = msg->getKind();

	timeoutTable.erase(TimeoutKey(it->second, msg->getName(), msg->getKind()));
	reverseTimeoutTable.erase(it->first);

	too->onTimeout(name, kind);
	cancelAndDelete(msg);
	return true;
}

void TimeoutManager::setDebug(bool debug) {
	Enter_Method("setDebug");
	this->debug = debug;
}

void TimeoutManager::finish(){
    // tidy up events
    for (TimeoutMessageTable::iterator it = timeoutTable.begin(); it != timeoutTable.end();) {
        removeEntry(it++);
    }
}
