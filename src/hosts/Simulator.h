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

#ifndef __DTSYNCHRONIZER_SIMULATOR_H_
#define __DTSYNCHRONIZER_SIMULATOR_H_

#include "../objects/SituationRender.h"

using namespace omnetpp;
using namespace std;

class Simulator: public cSimpleModule {
private:
    // <pair of instance counter, operation_index>
    map<pair<long, int>, int> opArrivals;
    int opIndex;
    SituationRender sr;
protected:
    int toltalOperations;
    virtual void initialize() override;
    virtual void finish() override;
    virtual void handleMessage(cMessage *msg) override;
public:
    Simulator();
};

#endif
