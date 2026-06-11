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

#include "SituationEvolution.h"

SituationEvolution::SituationEvolution() {

}

SituationEvolution::~SituationEvolution() {

}

void SituationEvolution::initModel(const char *model_path_sync) {
    sg.loadModel(model_path_sync, this);
    original = sg;
}

void SituationEvolution::initOriginalModel(const char *model_path_src){
    original.loadModel(model_path_src, this);
}

void SituationEvolution::addInstance(long id, SituationInstance::Type type,
        simtime_t duration, simtime_t init, simtime_t cycle) {
    SituationInstance si(id, type, duration, init, cycle);
    instanceMap[si.id] = si;
}

int SituationEvolution::numOfConsistentOperation(bool type){

//    cout << "in numOfConsistentOperation with type = " << type << endl;

    int consistency = 0;
    vector<long> operations = original.getAllOperationalSitutions();
    for(auto op : operations){

        if(type && instanceMap[op].type == SituationInstance::HIDDEN){

//            cout << "one hidden operation " << op << " in the numOfConsistentOperation method" << endl;

            continue;
        }

        /*
         * Check both explicit cause and implicit cause
         */
        for(auto op2 : operations){

            if (op2 != op && original.isCausallyReachable(op2, op)) {

//                cout << "from op2 " << op2 << " to op " << op << " is unilaterally reachable" << endl;

                int causeCounter = instanceMap[op2].counter;
                if(causeCounter >= instanceMap[op].counter){

//                    cout << "cause " << op2 << " counter " << causeCounter
//                            << " more than situation " << op << " counter "
//                            << instanceMap[op].counter << endl;

                    consistency++;
                }else{
//                    cout << "An inconsistent cause found" << endl;
                }
            }else{
//                cout << "from op2 " << op2 << " to op " << op << " is not unilaterally reachable" << endl;
            }
        }
    }
    return consistency;
}

SituationInstance& SituationEvolution::getInstance(long id) {
    return instanceMap[id];
}

SituationGraph& SituationEvolution::getModel() {
    return sg;
}

void SituationEvolution::print() {
    for (auto m : instanceMap) {
        cout << m.second;
    }
}
