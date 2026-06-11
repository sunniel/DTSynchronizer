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

#include "../common/Util.h"
#include "SituationReasoner.h"

SituationReasoner::SituationReasoner() :
        SituationEvolution() {
}

SituationReasoner::~SituationReasoner() {

}

bool SituationReasoner::_determineState(long instanceId, SituationInstance::State newState,
        simtime_t current) {
    bool updated = false;
    SituationInstance &instance = instanceMap[instanceId];

    if (newState == SituationInstance::TRIGGERING) {
        // triggering state has the highest priority
        if (instance.state != SituationInstance::TRIGGERING) {
            instance.counter++;
            instance.next_start = current;
            instance.state = SituationInstance::TRIGGERING;
            updated = true;
        }
    } else  {
        if(instance.state != SituationInstance::TRIGGERING){
            if(instance.state != newState){
                instance.state = newState;
                updated = true;
            }
        }
    }

    return updated;
}

void SituationReasoner::_beliefPropagate(simtime_t current) {
    int layers = sg.modelHeight();

    /*
     * Process each layer from bottom to top in the situation graph;
     * Evidence nodes are in lower layers, hypothesis nodes are in upper layers
     */
    for (int i = layers - 1; i > 0; i--) {
        // layers[layers-1] access the bottom layer
        DirectedGraph g = sg.getLayer(i - 1);
        std::vector<long> hypotheses = g.topo_sort();

        // Process each node in the current layer as a hypothesis
        for (auto hId : hypotheses) {
            bool toTrigger = false;

            SituationNode hNode = sg.getNode(hId);
            SituationInstance &hypothesis = instanceMap[hId];
            // Collect evidences nodes from the child layer
            std::vector<long> evidences = hNode.evidences;

            // assume evidences is non-empty
            SituationRelation relation = sg.getRelation(evidences[0], hId);
            if (relation.relation == SituationRelation::SOLE) {
                /*
                 * Case 1: Single evidence with SOLE relation
                 * Belief = evidence_belief * relation_weight
                 */
                SituationInstance evidence = instanceMap[evidences[0]];
                hypothesis.belief = evidence.belief * relation.weight;
                // combine evidence state
                if (evidence.counter > hypothesis.counter) {
                    toTrigger = true;
                }

                // for test purpose
//                cout << "node [" << hId << "]in SOLE, belief： " << hypothesis.belief << endl;

            } else if (relation.relation == SituationRelation::OR) {
                /*
                 * Cases 2: Multiple evidence nodes with OR relations
                 * Belief = max(evidence_belief * relation_weight)
                 */
                double maxWeightedBelief = 0.0;
                for (long eId : evidences) {
                    relation = sg.getRelation(eId, hId);
                    SituationInstance evidence = instanceMap[eId];
                    maxWeightedBelief = std::max(maxWeightedBelief,
                            evidence.belief * relation.weight);
                    // combine evidence states
                    if (evidence.counter <= hypothesis.counter) {
                        toTrigger |= false;
                    } else {
                        toTrigger |= true;
                    }
                }
                hypothesis.belief = maxWeightedBelief;

                // for test purpose
//                cout << "node [" << hId << "] in OR, belief： " << hypothesis.belief << endl;

            } else {
                /*
                 * Cases 3: Multiple evidence nodes with AND relations
                 * Belief = Dempster's combination of weighted beliefs
                 */
                double combinedBelief = 1.0;
                toTrigger = true;
                for (long eId : evidences) {
                    relation = sg.getRelation(eId, hId);
                    SituationInstance evidence = instanceMap[eId];
                    combinedBelief *= evidence.belief * relation.weight;
                    // combine evidence states
                    if (evidence.counter <= hypothesis.counter) {
                        toTrigger &= false;
                    } else {
                        toTrigger &= true;
                    }
                }
                hypothesis.belief = combinedBelief;

                // for test purpose
//                cout << "node [" << hId << "] in AND, belief： " << hypothesis.belief << endl;

            }

            // for test purpose
//            cout << "node [" << hId << "] threshold： " << hNode.threshold << endl;

            /*
             * set node to TRIGGERED if belief > threshold
             */
            if (toTrigger && hypothesis.belief >= hNode.threshold) {
                hypothesis.state = SituationInstance::TRIGGERING;
                hypothesis.counter++;
                hypothesis.next_start = current;
            }
        }
    }
}

bool SituationReasoner::_backwardRetrospect(simtime_t current) {
    bool toIter = false;
    int layers = sg.modelHeight();

    /*
     * Process each layer from top to bottom
     * NOTE: Both TRIGGERED and TRIGGERING instances are considered for instance alignment
     */
    for (int layer = 0; layer < layers; layer++) {
        DirectedGraph currentLayer = sg.getLayer(layer);
        std::vector<long> nodes = currentLayer.topo_sort();
        std::queue<long> triggeredNodes;
        for (auto node : nodes) {
            if (instanceMap[node].state == SituationInstance::TRIGGERING
                    || instanceMap[node].state
                            == SituationInstance::UNDETERMINED
                    || instanceMap[node].state
                            == SituationInstance::TRIGGERED) {
                triggeredNodes.push(node);
            }
        }
        while (!triggeredNodes.empty()) {
            long triggered = triggeredNodes.front();
            triggeredNodes.pop();

            // for test purpose
//            cout << "check the cause of node [" << triggered << "]" << endl;

            for (auto causeId : sg.getNode(triggered).causes) {
                // use trigger counter to check cause state
                if (instanceMap[causeId].counter
                        < instanceMap[triggered].counter) {
                    /*
                     * Determine cause state
                     */
                    SituationInstance::State newState;
                    // This cause is the SOLE cause of the given situation
                    bool condition_1 = false;
                    // All causes have AND relations
                    bool condition_2 = false;
                    // All causes have OR relations and none is UNTRIGGERED
                    bool condition_3 = false;
                    /*
                     * Condition 1: Check if this cause is the SOLE cause of the input effect
                     */
                    if (sg.getNode(triggered).causes.size() == 1) {
                        condition_1 = true;
                    }
                    /*
                     * Condition 2: All causes of this effect have AND relations
                     */
                    if (sg.getRelation(causeId, triggered).relation
                            == SituationRelation::AND) {
                        condition_2 = true;
                    }
                    /*
                     * Condition 3: All causes of the given situation have OR relations, and the rest causes are UNTRIGGERED
                     */
                    if(sg.getRelation(causeId, triggered).relation
                            == SituationRelation::OR){
                        std::vector<long>causes = sg.getNode(triggered).causes;
                        bool noCauseTriggerred = true;
                        for (auto cause : causes) {
                            if (cause != causeId
                                    && (instanceMap[cause].counter
                                            >= instanceMap[triggered].counter)) {
                                noCauseTriggerred = false;
                                break;
                            }
                        }
                        condition_3 = noCauseTriggerred;
                    }

                    if (condition_1 || condition_2 || condition_3) {
                        if (_determineState(causeId, SituationInstance::TRIGGERING, current)){
                            toIter = true;
                        }
                    } else {
                        // in retrospection, the state will only be triggering or undetermined
                        if (_determineState(causeId, SituationInstance::UNDETERMINED, current)){
                            toIter = true;
                        }
                    }

                    // for test purpose
//                    cout << "current cause [" << causeId << "]  state in BR: " << instanceMap[causeId].state << endl;

                    // Iteratively check cause state
                    if (instanceMap[causeId].state
                                                == SituationInstance::TRIGGERING || instanceMap[causeId].state
                                                == SituationInstance::UNDETERMINED) {
                        triggeredNodes.push(causeId);
                    }
                } else {
                    /*
                     * instance alignment
                     */
                    if ((instanceMap[causeId].counter
                            > instanceMap[triggered].counter
                            && instanceMap[causeId].state
                                    == SituationInstance::UNTRIGGERED)
                            || (instanceMap[triggered].state
                                    == SituationInstance::TRIGGERING
                                    && instanceMap[causeId].state
                                            == SituationInstance::UNTRIGGERED)) {
                        instanceMap[causeId].state =
                                SituationInstance::TRIGGERED;
                        toIter = true;
                    } else if (instanceMap[triggered].state
                            == SituationInstance::UNDETERMINED
                            && instanceMap[causeId].state
                                    == SituationInstance::UNTRIGGERED) {
                        instanceMap[causeId].state =
                                SituationInstance::UNDETERMINED;
                        toIter = true;
                    } else if(instanceMap[triggered].state
                            == SituationInstance::TRIGGERED
                            && instanceMap[causeId].state
                                    == SituationInstance::UNTRIGGERED){
                        // for triggered state backward propagation
                        instanceMap[causeId].state =
                                SituationInstance::TRIGGERED;
                        toIter = true;
                    }
                }
            }
        }
    }

    return toIter;
}

bool SituationReasoner::_downwardRetrospection(simtime_t current) {
    bool toIter = false;
    int layers = sg.modelHeight();
    /*
     * Process each layer from top to bottom, skipping the bottom layer
     */
    for (int layer = 0; layer < layers - 1; layer++) {
        DirectedGraph graph = sg.getLayer(layer);
        std::vector<long> nodes = graph.topo_sort();

        /*
         * Collect triggered situations in this layer
         * NOTE: Both TRIGGERED and TRIGGERING instances are considered for instance alignment
         */
        std::queue<long> triggeredNodes;
        for (long node : nodes) {
            if (instanceMap[node].state == SituationInstance::TRIGGERING
                    || instanceMap[node].state
                            == SituationInstance::UNDETERMINED
                    || instanceMap[node].state
                            == SituationInstance::TRIGGERED) {
                triggeredNodes.push(node);
            }
        }

        while (!triggeredNodes.empty()) {
            long triggered = triggeredNodes.front();
            triggeredNodes.pop();

            SituationInstance triggeredInstance = instanceMap[triggered];
            for (long evidenceId : sg.getNode(triggered).evidences) {
                // use trigger counter to check evidence state
                if (instanceMap[evidenceId].counter
                        < instanceMap[triggered].counter) {
                    /*
                     * Determine evidence state
                     */
                    // This evidence is the SOLE cause of the given situation
                    bool condition_1 = false;
                    // All evidences have AND relations
                    bool condition_2 = false;
                    // All evidences have OR relations and none is UNTRIGGERED
                    bool condition_3 = false;
                    if (sg.getNode(triggered).evidences.size() == 1) {
                        condition_1 = true;
                    }
                    if (sg.getRelation(evidenceId, triggered).relation
                            == SituationRelation::AND) {
                        condition_2 = true;
                    }
                    /*
                     * Condition 3: All evidences of the given situation have OR relations, and the rest evidences are UNTRIGGERED
                     */
                    if(sg.getRelation(evidenceId, triggered).relation
                            == SituationRelation::OR){
                        std::vector<long>evidences = sg.getNode(triggered).causes;
                        bool noEvidenceTriggerred = true;
                        for (auto evidence : evidences) {
                            if (evidence != evidenceId
                                    && (instanceMap[evidence].counter
                                            >= instanceMap[triggered].counter)) {
                                noEvidenceTriggerred = false;
                                break;
                            }
                        }
                        condition_3 = noEvidenceTriggerred;
                    }

                    if (condition_1 || condition_2 || condition_3) {
                        if (_determineState(evidenceId, SituationInstance::TRIGGERING, current)){
                            toIter = true;
                        }
                    } else {
                        if (_determineState(evidenceId, SituationInstance::UNDETERMINED, current)){
                            toIter = true;
                        }
                    }
                } else {
                    /*
                     * instance alignment
                     */
                    if ((instanceMap[evidenceId].counter
                            > instanceMap[triggered].counter
                            && instanceMap[evidenceId].state
                                    == SituationInstance::UNTRIGGERED)
                            || (instanceMap[triggered].state
                                    == SituationInstance::TRIGGERING
                                    && instanceMap[evidenceId].state
                                            == SituationInstance::UNTRIGGERED)) {
                        instanceMap[evidenceId].state =
                                SituationInstance::TRIGGERED;
                        toIter = true;
                    } else if (instanceMap[triggered].state
                            == SituationInstance::UNDETERMINED
                            && instanceMap[evidenceId].state
                                    == SituationInstance::UNTRIGGERED) {
                        instanceMap[evidenceId].state =
                                SituationInstance::UNDETERMINED;
                        toIter = true;
                    } else if(instanceMap[triggered].state
                            == SituationInstance::TRIGGERED
                            && instanceMap[evidenceId].state
                                    == SituationInstance::UNTRIGGERED){
                        // for triggered state downward propagation
                        instanceMap[evidenceId].state =
                                SituationInstance::TRIGGERED;
                        toIter = true;
                    }
                }
            }
        }
    }

    return toIter;
}

bool SituationReasoner::_statePropagation(){
    bool toIter = false;
    int layers = sg.modelHeight();
    for (int i = layers - 1; i > 0; i--) {
        DirectedGraph currentLayer = sg.getLayer(i);
        std::vector<long> nodes = currentLayer.topo_sort();
        for (auto nodeId : nodes) {
            if (instanceMap[nodeId].state == SituationInstance::UNDETERMINED) {
                // find all hypotheses
                std::vector<long> hypotheses = sg.getHypotheses(nodeId);
                for (auto hId : hypotheses) {
                    if (instanceMap[hId].counter <= instanceMap[nodeId].counter
                            && instanceMap[hId].state
                                    != SituationInstance::UNDETERMINED) {
                        instanceMap[hId].state =
                                SituationInstance::UNDETERMINED;
                        toIter = true;
                    }
                }
            } else if (instanceMap[nodeId].state
                    == SituationInstance::TRIGGERED) {
                // find all hypotheses
                std::vector<long> hypotheses = sg.getHypotheses(nodeId);
                for (auto hId : hypotheses) {
                    if (instanceMap[hId].counter >= instanceMap[nodeId].counter
                            && instanceMap[hId].state
                                    == SituationInstance::UNTRIGGERED) {
                        instanceMap[hId].state = SituationInstance::TRIGGERED;
                        toIter = true;
                    }
                }
            }
        }
    }
    return toIter;
}

std::set<long> SituationReasoner::reason(std::set<long> triggered,
        simtime_t current) {
    std::set<long> tOperational;

    cout << "show triggered: ";
    util::printSet(triggered);

    int layers = sg.modelHeight();

    /*
     * 1. Trigger bottom layer situations
     */
    // layers[layers-1] accesses the bottom layer; layers[0] accesses the top layer
    DirectedGraph g = sg.getLayer(layers - 1);
    std::vector<long> bottoms = g.topo_sort();
    for (auto bottom : bottoms) {
        SituationInstance &instance = instanceMap[bottom];
        if (triggered.find(bottom) != triggered.end()) {
            instance.state = SituationInstance::TRIGGERING;
            instance.counter++;
            instance.next_start = current;
        }
    }

    /*
     * 2. Perform belief propagation
     */
    _beliefPropagate(current);
    /*
     * for test purpose
     */
    cout << endl;
    cout << "========== print situations after BP =============" << endl;
//    print();
    cout << endl;

    bool toIter = true;
    while(toIter){
        /*
         * 3. Perform backward-retrospection & downward-retrospection
         */
        toIter = _backwardRetrospect(current);
        /*
         * for test purpose
         */
        cout << endl;
        cout << "========== print situations after BR =============" << endl;
//        print();
        cout << endl;

        toIter = _downwardRetrospection(current);
        /*
         * for test purpose
         */
        cout << endl;
        cout << "========== print situations after DR =============" << endl;
//        print();
        cout << endl;

        toIter = _statePropagation();
        /*
         * for test purpose
         */
        cout << endl;
        cout << "========== print situations after SP =============" << endl;
//        print();
        cout << endl;
    }

    /*
     * 4. Update refinement
     */
    bool needRefinement = false;
    for (auto instance : instanceMap) {
        if (instance.second.state == SituationInstance::UNDETERMINED) {
            needRefinement = true;
            break;
        }
    }
    if (needRefinement) {
        BNInferenceEngine engine;
        engine.reason(sg, instanceMap, current);

        /*
         * for test purpose
         */
        cout << endl;
        cout << "========== print situations after Bayesian network inference =============" << endl;
//        print();
        cout << endl;
    }

    /*
     * 5. Get operational situations to return
     */
    for (auto bottom : bottoms) {
        SituationInstance &instance = instanceMap[bottom];
        if (instance.state == SituationInstance::TRIGGERING
                && instance.next_start == current) {
            tOperational.insert(instance.id);
        }
    }

    /*
     * 6. reset transient situations, durable situations is cyclically reset from Synchronizer
     */
    checkState(current);

    /*
     * Reset triggered situation to untriggered state
     */
    for (auto &instance : instanceMap) {
        if (instance.second.state == SituationInstance::TRIGGERED) {
            instance.second.state = SituationInstance::UNTRIGGERED;
        }
    }

    /*
     * for test purpose
     */
    cout << endl;
    cout << "========== print situation graph instance after reasoning =============" << endl;
    print();
    cout << endl;

    return tOperational;
}

void SituationReasoner::checkState(simtime_t current) {

//    cout << "check state at: " << current << endl;

    // reset persistent situations
    for (auto &si : instanceMap) {
        if (si.second.next_start + si.second.duration <= current
                && si.second.state == SituationInstance::TRIGGERING) {
            si.second.state = SituationInstance::UNTRIGGERED;

            cout << "reset node " << si.first << endl;
        }
    }
}
