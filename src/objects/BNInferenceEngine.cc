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

#include "BNInferenceEngine.h"

BNInferenceEngine::BNInferenceEngine() {
    // TODO Auto-generated constructor stub

}

BNInferenceEngine::~BNInferenceEngine() {
    // TODO Auto-generated destructor stub
}

void BNInferenceEngine::loadModel(SituationGraph sg) {
    /*
     * Initialize Bayesian network
     */
    // the informed graph size must be greater than any node index
    long numOfNodes = (sg.modelHeight() + 1) * 100;
    cout << "number_of_nodes: " << numOfNodes << endl;
    std::set<pair<long, long>> edges;
    for (auto relation : sg.relationMap) {
        edges.insert(relation.first);
    }

    /*
     * Construct the CPT of the SG
     */
    std::map<std::tuple<long, long, std::set<std::pair<long, long>>>, double> CPT;
    DirectedGraph g = sg.getLayer(0);
    std::vector<long> sortedNodes = g.topo_sort();
    for (auto node : sortedNodes) {

//        cout << "set probability of node " << node << endl;

        std::vector<long> &causes = sg.situationMap[node].causes;
        if (causes.empty()) {

//            cout << "set priori probability of node " << node << endl;

            // Clear out parent state so that it doesn't have any of the previous assignment
            std::tuple<long, long, std::set<std::pair<long, long>>> setting_0(node, 0, std::set<std::pair<long, long>>());
            CPT[setting_0] = 0.5;
            std::tuple<long, long, std::set<std::pair<long, long>>> setting_1(node, 1, std::set<std::pair<long, long>>());
            CPT[setting_1] = 0.5;

        } else {
            const short size = causes.size();
            // conditional probabilities of a single cause: p(B|A) = weight, p(B|-A) = 0
            pair<double, double> p[size];
            for (int i = 0; i < size; i++) {
                SituationGraph::edge_id eid;
                eid.first = causes[i];
                eid.second = node;
                SituationRelation sr = sg.relationMap[eid];
                p[i].first = 0;
                p[i].second = sr.weight;
            }

            // 2^n binary combinations
            const short totalCombinations = 1 << size;
            for (int i = 0; i < totalCombinations; i++) {
                std::set<std::pair<long, long>> parent_state;
                // Be careful! It means the number of cause of a situation cannot exceed 32
                bitset<32> binary(i);
                // conditional probability of multiple causes
                double p_cond = 1;
                for (int i = 0; i < size; i++) {

//                    cout << "causes[i]: " << causes[i] << endl;

                    std::pair<long, long> parent;
                    parent.first = causes[i];
                    if (binary[i]) {
                        p_cond *= p[i].second;
                        parent.second = 1;
                        parent_state.insert(parent);
                    } else {
                        p_cond *= p[i].first;
                        parent.second = 0;
                    }
                    parent_state.insert(parent);
                }

                std::tuple<long, long, std::set<std::pair<long, long>>> setting_0(node, 1, parent_state);
                CPT[setting_0] = p_cond;
                std::tuple<long, long, std::set<std::pair<long, long>>> setting_1(node, 0, parent_state);
                CPT[setting_1] = 1 - p_cond;

//                cout << "print CPT of node " << node << endl;
//                cout << "condition: ";
//                for(int i= 0; i < size; i++){
//                  cout << binary[i] << ", ";
//                }
//                cout << endl;
//                cout << "conditional probability of p(triggered): " << p_cond << endl;
//                cout << "conditional probability of p(untriggered): " << 1- p_cond << endl;
            }
        }
    }

    BNet.BuildNetwork(numOfNodes, edges, CPT);
}

void BNInferenceEngine::reason(SituationGraph sg,
        std::map<int, SituationInstance> &instanceMap, simtime_t current) {

    /*
     * Build a Bayesian network solution
     */
    std::map<long, long> evidences;
    for (auto instance : instanceMap) {
        long sid = instance.first;
        SituationInstance si = instance.second;
        if (si.state == SituationInstance::TRIGGERING || si.state == SituationInstance::TRIGGERED) {
            // TODO here, instance alignment is included, correct and fully implemented?
            evidences[sid] = 1;

            cout << "set evidence of node " << sid << ": " << 1 << endl;
        }else if(si.state == SituationInstance::UNTRIGGERED){
            evidences[sid] = 0;

            cout << "set evidence of node " << sid << ": " << 0 << endl;
        }
    }
    BNet.buildSolution(evidences);

    /*
     * Bayesian network-based state inference
     */
    for (auto& instance : instanceMap) {
        long sid = instance.first;
        SituationInstance &si = instance.second;
        // probability of triggering
        double p_tr = BNet.getProbability(sid, 1);
        if(si.state == SituationInstance::UNDETERMINED){
            if (p_tr >= sg.situationMap[sid].threshold) {
                si.state = SituationInstance::TRIGGERING;
                si.counter++;
                si.next_start = current;
            } else {
                si.state = SituationInstance::UNTRIGGERED;
            }

                cout << "probability of triggering node " << sid << ": " << p_tr << endl;
                cout << "state of undetermined node " << sid << ": " << si.state << endl;
                cout << "counter of node " << sid << ": " << si.counter << endl;
        }
    }

    /*
     * Clear the solution
     */
    BNet.clearSolution();
}
