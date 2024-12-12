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

#ifndef OBJECTS_BNINFERENCEENGINE_H_
#define OBJECTS_BNINFERENCEENGINE_H_

#include <map>
#include <set>
#include <vector>
#include <tuple>
#include <utility>
#include <bitset>
#include <omnetpp.h>
#include "SituationInstance.h"
#include "SituationGraph.h"
#include "DirectedGraph.h"
#include "BayesianNetwork.h"

using namespace std;
using namespace omnetpp;

class BNInferenceEngine {
private:
    BayesianNetwork BNet;
//    void constructCPT();
//    void subgraphExtraction();
public:
    void loadModel(SituationGraph sg);
    void reason(SituationGraph sg,
            std::map<int, SituationInstance> &instanceMap, simtime_t current);
    BNInferenceEngine();
    virtual ~BNInferenceEngine();
};

#endif /* OBJECTS_BNINFERENCEENGINE_H_ */
