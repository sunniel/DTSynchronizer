//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef OBJECTS_SITUATIONINSTANCE_H_
#define OBJECTS_SITUATIONINSTANCE_H_

#include <iostream>
#include <omnetpp.h>

using namespace std;
using namespace omnetpp;

class SituationInstance {
public:
    enum Type {
        NORMAL, HIDDEN
    };
    enum State {
        UNTRIGGERED, TRIGGERING, UNDETERMINED, TRIGGERED
    };
    long id;
    int counter;
    // 0 - normal source, 1 - hidden source
    Type type;
    // 0 - untriggered, 1 - triggered, 2 - UNDETERMINED
    State state;
    simtime_t duration;
    simtime_t cycle;
    /*
     * A multi-purpose variable:
     * 1) next situation/event start time in Arranger, and
     * 2) current situation start time in Reasoner.
     * Particularly in Arranger, next_start times the next situation for top-layer situation,
     * while it times the next event for bottom-layer situation.
     */
    simtime_t next_start;

    // Current belief from evidence combination, 1 by default
    double belief;

    double latency;
public:
    // init: time of first triggering, i.e., value of next_start
    SituationInstance(long id = -1, Type type = Type::NORMAL,
            simtime_t duration = 0, simtime_t init = 0, simtime_t cycle = 0,
            double belief = 1.0, double latency = 50.0);
    virtual ~SituationInstance();
};

inline std::ostream& operator<<(std::ostream &os, const SituationInstance &si) {
    os << "situation (" << si.id << "): counter " << si.counter << ", type "
            << si.type << ", state " << si.state << ", duration " << si.duration
            << ", cycle " << si.cycle << ", next_start " << si.next_start
            << ", latency " << si.latency << endl;
    return os;
}

#endif /* OBJECTS_SITUATIONINSTANCE_H_ */
