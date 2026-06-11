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
    numOfNodes = 0;
    single = -1;
}

BNInferenceEngine::~BNInferenceEngine() {

}

void BNInferenceEngine::reason(SituationGraph sg,
        std::map<long, SituationInstance> &instanceMap, simtime_t current) {

    std::vector<SituationGraph> graphs = sg.getWeaklyConnectedComponents();

    cout << "number of weakly connected components: " << graphs.size() << endl;

    for(auto graph : graphs){
        // the informed graph size must be greater than any node index, ruled by dlib
        numOfNodes = (sg.modelHeight() + 1) * 100;
        this->current = current;
        for (auto instance : instanceMap) {
            if(graph.situationMap.count(instance.first) > 0){
                extInstanceMap[instance.first] = instance.second;
            }
        }
        _extendSG(graph);

        _buildCPT(esg);

    //    _identifyNetwork(instanceMap);
        _updateState();

        for (auto &instance : instanceMap) {
            if(graph.situationMap.count(instance.first) > 0){
                instance.second.state = extInstanceMap[instance.first].state;
                instance.second.counter = extInstanceMap[instance.first].counter;
                instance.second.next_start = extInstanceMap[instance.first].next_start;
            }
        }

        // housekeeping
        BNet.clearSolution();
        esg.nodeMap.clear();
        esg.relationMap.clear();

        /*
         * for test purpose
         */
//        cout << endl << "print extInstanceMap" << endl;
//        for (auto m : extInstanceMap) {
//            cout << m.second;
//        }
//        cout << endl;

        extInstanceMap.clear();
        single = -1;
    }
}

void BNInferenceEngine::_extendSG(SituationGraph sg) {
    for (auto entry : sg.situationMap) {
        long id = entry.first;
        SituationNode sn = entry.second;
        std::vector<long> causes = sn.causes;
        std::vector<long> evidences = sn.evidences;
        int numOfEvidence = causes.size();
        int numOfCause = evidences.size();
        GeneralNode node;
        node.id = id;
        node.threshold = sn.threshold;

        if (numOfEvidence > 0 && numOfCause > 0) {
            /*
             * Add intermediary nodes for the mixed relation type
             */
            GeneralNode causeNode;
            causeNode.id = numOfNodes++;
            // TODO what should be the threshold value of intermediary node?
            causeNode.threshold = 0.01;
            node.predecessors.push_back(causeNode.id);
            // let the weight of intermediary node be 1
            _addRelation(causeNode.id, node.id, 1, SituationRelation::AND);
            for (auto causeId : causes) {
                SituationRelation sr = sg.getRelation(causeId, id);
                _addRelation(causeId, causeNode.id, sr.weight, sr.relation);
                causeNode.predecessors.push_back(causeId);
            }
            esg.nodeMap[causeNode.id] = causeNode;
            SituationInstance andNodeInstance(causeNode.id);
            andNodeInstance.state = SituationInstance::UNDETERMINED;
            extInstanceMap[causeNode.id] = andNodeInstance;

            GeneralNode evidenceNode;
            evidenceNode.id = numOfNodes++;
            // TODO what should be the threshold value of intermediary node?
            evidenceNode.threshold = 0.01;
            node.predecessors.push_back(evidenceNode.id);
            // let the weight of intermediary node be 1
            _addRelation(evidenceNode.id, node.id, 1, SituationRelation::AND);
            for (auto evidenceId : evidences) {
                SituationRelation sr = sg.getRelation(evidenceId, id);
                _addRelation(evidenceId, evidenceNode.id, sr.weight,
                        sr.relation);
                evidenceNode.predecessors.push_back(evidenceId);
            }
            esg.nodeMap[evidenceNode.id] = evidenceNode;
            SituationInstance orNodeInstance(evidenceNode.id);
            orNodeInstance.state = SituationInstance::UNDETERMINED;
            extInstanceMap[evidenceNode.id] = orNodeInstance;
        } else {
            /*
             * merge the predecessors of single relation type
             */
            for (auto causeId : causes) {
                SituationRelation sr = sg.getRelation(causeId, id);
                _addRelation(causeId, node.id, sr.weight, sr.relation);
                node.predecessors.push_back(causeId);
            }
            for (auto evidenceId : evidences) {
                SituationRelation sr = sg.getRelation(evidenceId, id);
                _addRelation(evidenceId, node.id, sr.weight, sr.relation);
                node.predecessors.push_back(evidenceId);
            }
        }

        esg.nodeMap[id] = node;
    }

    // for test purpose
//    esg.print();
}

void BNInferenceEngine::_addRelation(long src, long dest, double weight,
        SituationRelation::Relation relation) {
    GeneralRelation er(relation, src, dest, weight);
    ExtendedGraph::edge_id eid;
    eid.first = src;
    eid.second = dest;
    esg.relationMap[eid] = er;
}

GeneralRelation BNInferenceEngine::_getRelation(long src, long dest) {
    ExtendedGraph::edge_id eid;
    eid.first = src;
    eid.second = dest;
    return esg.relationMap[eid];
}

void BNInferenceEngine::_buildCPT(ExtendedGraph esg) {
    /*
     * Initialize Bayesian network
     */
    cout << "number_of_nodes: " << numOfNodes << endl;
    std::set<pair<long, long>> edges;
    for (auto relation : esg.relationMap) {
        edges.insert(relation.first);
    }
    /*
     * for single node network
     */
    if(edges.empty()){
        if(esg.nodeMap.size() > 0){
            long id = esg.nodeMap.begin()->first;
            single = id;
            edges.insert(make_pair(0, 0));
        }
    }

    /*
     * Construct the CPT of the ESG;
     * The key of each entry in CPT is a tuple<child, c_state, set<parent, p_state>>,
     */
    std::map<std::tuple<long, long, std::set<std::pair<long, long>>>, double> CPT;
    for (auto entry : esg.nodeMap) {
        GeneralNode node = entry.second;
        long id = (single == -1 ? entry.first : 0);

//        cout << "set probability of node " << node << endl;

        std::vector<long> predecessors = node.predecessors;
        if (predecessors.empty()) {

//            cout << "set priori probability of node " << node << endl;

            // Clear out parent state so that it doesn't have any of the previous assignment
            std::tuple<long, long, std::set<std::pair<long, long>>> setting_0(
                    id, 0, std::set<std::pair<long, long>>());
            CPT[setting_0] = 0.5;
            std::tuple<long, long, std::set<std::pair<long, long>>> setting_1(
                    id, 1, std::set<std::pair<long, long>>());
            CPT[setting_1] = 0.5;

        } else {
            /*
             * Preparation
             */
            GeneralRelation gr = _getRelation(predecessors[0], id);
            const short size = predecessors.size();
            // conditional probabilities of each individual predecessor: p[i].first = p(B|-A) = 0, p[i].second = p(B|A) = weight
            pair<double, double> p[size];
            for (int i = 0; i < size; i++) {
                ExtendedGraph::edge_id eid;
                eid.first = predecessors[i];
                eid.second = id;
                GeneralRelation sr = esg.relationMap[eid];
                p[i].first = 0;
                p[i].second = sr.weight;
            }

            /*
             * relation-sensitive calculation of CPT
             */
            if (gr.relation == GeneralRelation::AND) { // also include the sole predecessor case
                // 2^n binary combinations
                const short totalCombinations = 1 << size;
                for (int i = 0; i < totalCombinations; i++) {
                    std::set<std::pair<long, long>> parent_state;
                    // Be careful! It means the number of cause of a situation cannot exceed 32
                    bitset<32> binary(i);
                    // conditional probability of multiple causes
                    double p_cond = 1;
                    for (int i = 0; i < size; i++) {

                        // cout << "predecessors[i]: " << predecessors[i] << endl;

                        std::pair<long, long> parent;
                        parent.first = predecessors[i];
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

                    std::tuple<long, long, std::set<std::pair<long, long>>> setting_1(
                            id, 1, parent_state);
                    CPT[setting_1] = p_cond;
                    std::tuple<long, long, std::set<std::pair<long, long>>> setting_0(
                            id, 0, parent_state);
                    CPT[setting_0] = 1 - p_cond;

//                                    cout << "print CPT of node (AND) " << id << endl;
//                                    cout << "condition: ";
//                                    for(int i= 0; i < size; i++){
//                                      cout << binary[i] << ", ";
//                                    }
//                                    cout << endl;
//                                    cout << "conditional probability of p(triggered): " << p_cond << endl;
//                                    cout << "conditional probability of p(untriggered): " << 1- p_cond << endl;
                }
            } else if (gr.relation == GeneralRelation::OR) {
                // 2^n binary combinations
                const short totalCombinations = 1 << size;
                for (int i = 0; i < totalCombinations; i++) {
                    std::set<std::pair<long, long>> parent_state;
                    // Be careful! It means the number of cause of a situation cannot exceed 32
                    bitset<32> binary(i);
                    // conditional probability of multiple causes
                    double p_cond = 1;
                    for (int i = 0; i < size; i++) {

                        // cout << "predecessors[i]: " << predecessors[i] << endl;

                        std::pair<long, long> parent;
                        parent.first = predecessors[i];
                        if (binary[i]) {
                            p_cond *= 1 - p[i].second;
                            parent.second = 1;
                            parent_state.insert(parent);
                        } else {
                            parent.second = 0;
                        }
                        parent_state.insert(parent);
                    }

                    std::tuple<long, long, std::set<std::pair<long, long>>> setting_1(
                            id, 1, parent_state);
                    CPT[setting_1] = 1 - p_cond;
                    std::tuple<long, long, std::set<std::pair<long, long>>> setting_0(
                            id, 0, parent_state);
                    CPT[setting_0] = p_cond;

//                                    cout << "print CPT of node (OR) " << id << endl;
//                                    cout << "condition: ";
//                                    for(int i= 0; i < size; i++){
//                                      cout << binary[i] << ", ";
//                                    }
//                                    cout << endl;
//                                    cout << "conditional probability of p(triggered): " << 1 - p_cond << endl;
//                                    cout << "conditional probability of p(untriggered): " << p_cond << endl;
                }
            }
        }
    }
    BNet.BuildNetwork(numOfNodes, edges, CPT);
}

void BNInferenceEngine::_updateState() {

    /*
     * Build a Bayesian network solution
     */
    std::map<long, long> evidences;
    for (auto instance : extInstanceMap) {
        long sid = (single == -1 ? instance.first : 0);
        SituationInstance si = instance.second;
        if (si.state == SituationInstance::TRIGGERING
                || si.state == SituationInstance::TRIGGERED) {
            // TODO here, instance alignment is included, correct and fully implemented?
            evidences[sid] = 1;

            cout << "set evidence of node " << instance.first << ": " << 1 << endl;
        } else if (si.state == SituationInstance::UNTRIGGERED) {
            evidences[sid] = 0;

            cout << "set evidence of node " << instance.first << ": " << 0 << endl;
        }
    }
    BNet.buildSolution(evidences);

    /*
     * Bayesian network-based state inference
     */
    for (auto &instance : extInstanceMap) {
        long sid = instance.first;
        long nid = (single == -1 ? sid : 0);
        SituationInstance &si = instance.second;
        // probability of triggering
        double p_tr = BNet.getProbability(nid, 1);
        if (si.state == SituationInstance::UNDETERMINED) {
            if (p_tr >= esg.nodeMap[sid].threshold) {
                si.state = SituationInstance::TRIGGERING;
                si.counter++;
                si.next_start = current;
            } else {
                si.state = SituationInstance::UNTRIGGERED;
            }

            cout << "probability of triggering node " << sid << ": " << p_tr
                    << endl;
            cout << "state of undetermined node " << sid << ": " << si.state
                    << endl;
            cout << "counter of node " << sid << ": " << si.counter << endl;
        }
    }
}
