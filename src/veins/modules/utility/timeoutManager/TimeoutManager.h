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

#ifndef __VEINS_TIMEOUTMANAGER_H_
#define __VEINS_TIMEOUTMANAGER_H_

#include <map>

#include <omnetpp.h>

#include "veins/modules/utility/timeoutManager/TimeoutObserver.h"

using namespace omnetpp;

/*
 * @brief A timeout manager for OMNeT++.
 *
 * This is a timeout manager for OMNeT++ which allows to cancel timeouts and reschedule them,
 * therefore providing more control. Components that wish to use it have to implement the
 * TimeoutObserver class.
 *
 * @author Patrick Baldemaier, Florian Hagenauer
 */

class TimeoutManager : public cSimpleModule{

	private:
        class TimeoutKey;
        bool debug;
        typedef std::multimap<TimeoutKey, cMessage*> TimeoutMessageTable;
    public:
        TimeoutManager();
        virtual ~TimeoutManager();
        virtual void finish();

        void setTimeout(TimeoutObserver* sender, std::string name, short kind, simtime_t delay, bool cancel = true);
        void setTimeout(TimeoutObserver* sender, short kind, simtime_t delay);
        void setIntervalTimeout(TimeoutObserver* sender, short kind, simtime_t lowerBound, simtime_t upperBound);
        void unsetTimeout(TimeoutObserver* sender, std::string name, short kind);
        void unsetTimeout(TimeoutObserver* sender, short kind);
        void unsetAll(TimeoutObserver* sender);
        bool handleTimeoutMessage(cMessage* msg);
        bool hasTimeout(TimeoutObserver* sender, std::string name, short kind);
        void setDebug(bool debug);

    private:
        cMessage* getTimeout(TimeoutObserver* sender, std::string name, short kind);
        void removeEntry(TimeoutMessageTable::iterator it);
        TimeoutMessageTable timeoutTable;
        std::map<long, TimeoutObserver*> reverseTimeoutTable;

        void handleMessage(cMessage* msg){
        	if(!msg->isSelfMessage()){
        		return;
        	}
        	handleTimeoutMessage(msg);
        }

        class TimeoutKey {
            public:
                TimeoutKey(TimeoutObserver* sender) {
                    this->sender = sender;
                    senderOnly = true;
                }
                TimeoutKey(TimeoutObserver* sender, std::string name, short kind) {
                    this->sender = sender;
                    this->name = name;
                    this->kind = kind;
                    senderOnly = false;
                }

                bool operator <(const TimeoutKey& other) const {
                    if (senderOnly == false) {
                        if (sender == other.sender) {
                            if (name == other.name) {
                                return kind < other.kind;
                            }
                            return name < other.name;
                        }
                    }
                    return sender < other.sender;
                }
            private:
                TimeoutObserver* sender;
                std::string name;
                short kind;
                bool senderOnly;
        };
};

#endif /* TIMEOUTMANAGER_H_ */
