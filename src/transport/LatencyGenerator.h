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

#ifndef TRANSPORT_LATENCYGENERATOR_H_
#define TRANSPORT_LATENCYGENERATOR_H_

#include <omnetpp.h>

using namespace omnetpp;
using namespace std;

class LatencyGenerator {
private:

public:
    LatencyGenerator();
    simtime_t generator_latency();
    virtual ~LatencyGenerator();
};

#endif /* TRANSPORT_LATENCYGENERATOR_H_ */
