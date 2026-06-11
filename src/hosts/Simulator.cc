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

#include "../common/Constants.h"
#include "../messages/SimEvent_m.h"
#include "Simulator.h"

Define_Module(Simulator);

Simulator::Simulator(){
    toltalOperations = 0;
    opIndex = 0;
}

void Simulator::initialize() {
    const char* SITUATION_MODEL_SYNC = par("situation_file_src").stringValue();
    sr.initModel(SITUATION_MODEL_SYNC);
}

void Simulator::finish() {
    recordScalar("Simulated Operations", toltalOperations);
    double consistency = sr.rateOfOpConsistency(opArrivals);
    recordScalar("Rate of Operation Consistency", consistency);
}

void Simulator::handleMessage(cMessage *msg) {
    if (msg->isName(msg::SIM_EVENT)) {
        SimEvent *event = check_and_cast<SimEvent*>(msg);
        long id = event->getEventID();
        int count = event->getCount();

        pair<long, int> iid;
        iid.first = id;
        iid.second = count;
        opArrivals[iid] = opIndex++;

        cout << "Simulation event (" << id << "): timestamp "
                << event->getTimestamp() << " count " << count << " index "
                << opIndex << endl;

        toltalOperations++;

        // delete the received msg
        delete event;
    }
}
