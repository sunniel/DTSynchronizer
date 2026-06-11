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

#ifndef OBJECTS_SITUATIONRENDER_H_
#define OBJECTS_SITUATIONRENDER_H_

#include "SituationEvolution.h"

using namespace omnetpp;
using namespace std;

class SituationRender: public SituationEvolution {
public:
    SituationRender();
    virtual ~SituationRender();
    double rateOfOpConsistency(map<pair<long, int>, int> instanceArrivals);
};

#endif /* OBJECTS_SITUATIONRENDER_H_ */
