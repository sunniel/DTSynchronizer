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

#include <omnetpp.h>
#include <stack>
#include "SituationEvolution.h"
#include "SituationGraph.h"

SituationGraph::SituationGraph() {

}

SituationGraph::~SituationGraph() {
    // TODO why cannot release pointer here?
//    if(ri){
//        ri->clear();
//        delete ri;
//        ri = NULL;
//    }
}

vector<long> SituationGraph::getAllOperationalSitutions() {
    vector<long> operational_situations;
    DirectedGraph &bottom = layers[layers.size() - 1];
    vector<long> bottom_nodes = bottom.topo_sort();
    for (auto node : bottom_nodes) {
        SituationNode &operational_situation = situationMap[node];
        operational_situations.push_back(operational_situation.id);
    }
    return operational_situations;
}

vector<long> SituationGraph::getOperationalSitutions(long topNodeId) {
    vector<long> operational_situations;

    SituationNode &topNode = situationMap[topNodeId];
    stack<SituationNode> toChecks;
    toChecks.push(topNode);
    while (!toChecks.empty()) {
        SituationNode toCheck = toChecks.top();
        toChecks.pop();
        if (!toCheck.evidences.empty()) {
            for (auto evidenceId : toCheck.evidences) {
                SituationNode evidence = situationMap[evidenceId];
                toChecks.push(evidence);
            }
        } else {
            operational_situations.push_back(toCheck.id);
        }
    }

    return operational_situations;
}

vector<long> SituationGraph::getHypotheses(long evidence){
    vector<long> hypotheses;
    for(auto entry : relationMap){
        long src = entry.first.first;
        long dest = entry.first.second;
        SituationRelation relation = entry.second;
        if(src == evidence){
            if(relation.type == SituationRelation::V){
                hypotheses.push_back(dest);
            }
        }
    }
    return hypotheses;
}

bool SituationGraph::isReachable(long src, long dest) {
    int i = situationMap[src].index;
    int j = situationMap[dest].index;
    return ri->at(i)[j];
}

bool SituationGraph::isEvidential(long e, long h){
    bool isEvidential = false;
    deque<long> hypotheses;
    vector<long> parents = getHypotheses(e);
    hypotheses.insert(hypotheses.end(), parents.begin(), parents.end());
    while(!hypotheses.empty()){
        long hypothesis = hypotheses.front();
        hypotheses.pop_front();
        if(hypothesis != h){
            parents = getHypotheses(hypothesis);
            hypotheses.insert(hypotheses.end(), parents.begin(), parents.end());
        }else{
            isEvidential = true;
        }
    }

    return isEvidential;
}

bool SituationGraph::isCausallyReachable(long src, long dest){
    return cri[src][dest];
}

/*
 * For square matrix only
 */
vector<vector<bool>> SituationGraph::_boolMatrixPower(vector<vector<bool>> &mat,
        int n) {
    vector<vector<bool>> mat_n = mat;
    int k = mat_n.size();

    for (int pow = 0; pow < n - 1; pow++) {
        vector<vector<bool>> temp = mat_n;
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < k; j++) {
                bool value = false;
                for (int m = 0; m < k; m++) {
                    value = value || (temp[i][m] && mat[m][j]);
                    if (value)
                        // early out
                        break;
                }
                mat_n[i][j] = value;
            }
        }
    }

    return mat_n;
}

/*
 * For square matrix only
 */
void SituationGraph::_boolMatrixAdd(vector<vector<bool>> *result,
        vector<vector<bool>> &mat1, vector<vector<bool>> &mat2) {
    int n = mat1.size();

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            result->at(i)[j] = mat1[i][j] || mat2[i][j];
        }
    }
}

void SituationGraph::_buildReachabilityMatrix(set<long> &vertices,
        set<edge_id> &edges) {

    cout << "in _buildReachabilityMatrix" << endl;

    /*
     * initialize reachability matrix
     */
    int size = vertices.size();
    ri = new vector<vector<bool>>(size, vector<bool>(size, false));

    /*
     * build adjacency matrix
     */

    cout << "build adjacency matrix" << endl;

    vector<vector<bool>> adjMatrix(size, vector<bool>(size, false));
    for (auto src : vertices) {
        for (auto dest : vertices) {
            if (src != dest) {
                edge_id eid;
                eid.first = src;
                eid.second = dest;
                if (edges.count(eid)) {
                    int i = situationMap[src].index;
                    int j = situationMap[dest].index;
                    adjMatrix[i][j] = true;
                }
            }
        }
    }

    cout << "build reachability matrix" << endl;

    /*
     * build reachability matrix
     */
    for (int i = 1; i <= size; i++) {
        cout << "index: " << i << endl;
        vector<vector<bool>> adjMatrixPow = _boolMatrixPower(adjMatrix, i);
        vector<vector<bool>> temp = *ri;
        _boolMatrixAdd(ri, temp, adjMatrixPow);
    }
}

void SituationGraph::_buildCausalReachabilityMatrix(){

    cout << "in _buildReachabilityMatrix" << endl;

    vector<long> ops = getAllOperationalSitutions();
    for(auto op : ops){
        for(auto op2 : ops){
            bool causallyReachable = false;
            if(isReachable(op, op2)){
                causallyReachable = true;
            }else{
                deque<long> hypotheses;
                vector<long> parents = getHypotheses(op2);
                hypotheses.insert(hypotheses.end(), parents.begin(), parents.end());
                while(!hypotheses.empty()){
                    long hypothesis = hypotheses.front();
                    hypotheses.pop_front();
                    if(isEvidential(op, hypothesis)){
                        break;
                    }else if(isReachable(op, hypothesis)){
                        causallyReachable = true;
                        break;
                    }else{
                        parents = getHypotheses(hypothesis);
                        hypotheses.insert(hypotheses.end(), parents.begin(), parents.end());
                    }
                }
            }
            cri[op][op2] = causallyReachable;
        }
    }
}

void SituationGraph::loadModel(const std::string &filename,
        SituationEvolution *se) {
    modelName = filename;
    std::ifstream f(filename);
    json data = json::parse(f);

    /*
     * init model
     */
    int index = 0;
    this->clear();

    /*
     * for constructing reachability index
     */
    set<long> vertices;
    set<edge_id> edges;

    /*
     * 1. Create situation graph (SG)
     */
    for (const auto &layer : data["layers"].items()) {

        std::map<long, SituationNode> layerMap;

        /*
         * 1.1 Construct SG nodes and edges
         */
        for (const auto &node : layer.value().items()) {

            SituationNode situation;
            long id = node.value()["ID"].get<long>();
            situation.id = id;
            vertices.insert(id);
            situation.index = index;
            index++;

            /*
             * 1.1.0 build situation instances
             */
            double duration = node.value()["Duration"].get<double>() / 1000.0;
            double init = node.value()["Init"].get<double>() / 1000.0;
            SituationInstance::Type type =
                    (SituationInstance::Type) node.value()["type"].get<short>();
            if (!node.value()["Cycle"].is_null()) {
                // cycle is in millisecond
                double cycle = node.value()["Cycle"].get<double>() / 1000.0;
                se->addInstance(id, type, SimTime(duration), SimTime(init), SimTime(cycle));
            } else {
                se->addInstance(id, type, SimTime(duration), SimTime(init));
            }
            if (node.value().contains("BaseBelief")) {
                se->getInstance(id).belief = node.value()["BaseBelief"].get<
                        double>();
            }
            if(node.value().contains("latency")){
                se->getInstance(id).latency = node.value()["latency"].get<double>();
            }

            // build threshold
            if(node.value().contains("Threshold")){
                situation.threshold = node.value()["Threshold"].get<double>();
            }

            /*
             * 1.1.1 build cause-consequence relations
             */
            if (!node.value()["Predecessors"].empty()
                    && !node.value()["Predecessors"].is_null()) {
                for (const auto &pre : node.value()["Predecessors"].items()) {
                    SituationRelation relation;
                    long src = pre.value()["ID"].get<long>();
                    relation.src = src;
                    relation.dest = situation.id;
                    situation.causes.push_back(src);
                    relation.type = SituationRelation::H;
                    short relationValue = pre.value()["Relation"].get<short>();
                    switch (relationValue) {
                    case 1:
                        relation.relation = SituationRelation::AND;
                        break;
                    case 2:
                        relation.relation = SituationRelation::OR;
                        break;
                    default:
                        relation.relation = SituationRelation::SOLE;
                    }
                    relation.weight = pre.value()["Weight-x"].get<double>();
                    edge_id eid;
                    eid.first = src;
                    eid.second = relation.dest;
                    relationMap[eid] = relation;
                    edges.insert(eid);
                }
            }

            /*
             * 1.1.2 build parent-child relations
             */
            if (!node.value()["Children"].empty()
                    && !node.value()["Children"].is_null()) {
                for (const auto &chd : node.value()["Children"].items()) {
                    SituationRelation relation;
                    long src = chd.value()["ID"].get<long>();
                    relation.src = src;
                    relation.dest = situation.id;
                    situation.evidences.push_back(
                            chd.value()["ID"].get<long>());
                    relation.type = SituationRelation::V;
                    short relationValue = chd.value()["Relation"].get<short>();
                    switch (relationValue) {
                    case 1:
                        relation.relation = SituationRelation::AND;
                        break;
                    case 2:
                        relation.relation = SituationRelation::OR;
                        break;
                    default:
                        relation.relation = SituationRelation::SOLE;
                    }
                    relation.weight = chd.value()["Weight-y"].get<double>();
                    edge_id eid;
                    eid.first = src;
                    eid.second = relation.dest;
                    relationMap[eid] = relation;
//                    edge_id reid;
//                    reid.first = relation.dest;
//                    reid.second = src;
                    edges.insert(eid);
//                    edges.insert(reid);
                }
            }

            layerMap[situation.id] = situation;
        }

        /*
         * 1.2 Construct SG layers
         */
        DirectedGraph graph;
        for (auto m : layerMap) {
            graph.add_vertex(m.first);
            SituationNode &node = m.second;
            for (auto p : node.causes) {
                graph.add_edge(p, node.id);
            }
        }
        graph.print();
        layers.push_back(graph);

        // Create mapping relations
        situationMap.insert(layerMap.begin(), layerMap.end());
    }

    /*
     * 2. Create reachability index
     */
    _buildReachabilityMatrix(vertices, edges);
    cout << "print reachability matrix" << endl;
//    for(auto row : *ri){
//        for(auto col : row){
//            cout << col << "  ";
//        }
//        cout << endl;
//    }
    _buildCausalReachabilityMatrix();
    cout << "print causal reachability matrix" << endl;
//    for(auto element : cri){
//        cout << "key = " << element.first << endl;
//        util::printMap(element.second);
//    }
//    cout << endl;
}

void SituationGraph::changeModel(const std::string &filename){
    modelName = filename;
    std::ifstream f(filename);
    json data = json::parse(f);

    /*
     * init model
     */
    int index = 0;
    this->clear();

    /*
     * for constructing reachability index
     */
    set<long> vertices;
    set<edge_id> edges;

    /*
     * 1. Create situation graph (SG)
     */
    for (const auto &layer : data["layers"].items()) {

        std::map<long, SituationNode> layerMap;

        /*
         * 1.1 Construct SG nodes and edges
         */
        for (const auto &node : layer.value().items()) {

            SituationNode situation;
            long id = node.value()["ID"].get<long>();
            situation.id = id;
            vertices.insert(id);
            situation.index = index;
            index++;

            // build threshold
            if(node.value().contains("Threshold")){
                situation.threshold = node.value()["Threshold"].get<double>();
            }

             /*
             * 1.1.1 build cause-consequence relations
             */
            if (!node.value()["Predecessors"].empty()
                    && !node.value()["Predecessors"].is_null()) {
                for (const auto &pre : node.value()["Predecessors"].items()) {
                    SituationRelation relation;
                    long src = pre.value()["ID"].get<long>();
                    relation.src = src;
                    relation.dest = situation.id;
                    situation.causes.push_back(src);
                    relation.type = SituationRelation::H;
                    short relationValue = pre.value()["Relation"].get<short>();
                    switch (relationValue) {
                    case 1:
                        relation.relation = SituationRelation::AND;
                        break;
                    case 2:
                        relation.relation = SituationRelation::OR;
                        break;
                    default:
                        relation.relation = SituationRelation::SOLE;
                    }
                    relation.weight = pre.value()["Weight-x"].get<double>();
                    edge_id eid;
                    eid.first = src;
                    eid.second = relation.dest;
                    relationMap[eid] = relation;
                    edges.insert(eid);
                }
            }

            /*
             * 1.1.2 build parent-child relations
             */
            if (!node.value()["Children"].empty()
                    && !node.value()["Children"].is_null()) {
                for (const auto &chd : node.value()["Children"].items()) {
                    SituationRelation relation;
                    long src = chd.value()["ID"].get<long>();
                    relation.src = src;
                    relation.dest = situation.id;
                    situation.evidences.push_back(
                            chd.value()["ID"].get<long>());
                    relation.type = SituationRelation::V;
                    short relationValue = chd.value()["Relation"].get<short>();
                    switch (relationValue) {
                    case 1:
                        relation.relation = SituationRelation::AND;
                        break;
                    case 2:
                        relation.relation = SituationRelation::OR;
                        break;
                    default:
                        relation.relation = SituationRelation::SOLE;
                    }
                    relation.weight = chd.value()["Weight-y"].get<double>();
                    edge_id eid;
                    eid.first = src;
                    eid.second = relation.dest;
                    relationMap[eid] = relation;
//                    edge_id reid;
//                    reid.first = relation.dest;
//                    reid.second = src;
                    edges.insert(eid);
//                    edges.insert(reid);
                }
            }

            layerMap[situation.id] = situation;
        }

        /*
         * 1.2 Construct SG layers
         */
        DirectedGraph graph;
        for (auto m : layerMap) {
            graph.add_vertex(m.first);
            SituationNode &node = m.second;
            for (auto p : node.causes) {
                graph.add_edge(p, node.id);
            }
        }
//        graph.print();
        layers.push_back(graph);

        // Create mapping relations
        situationMap.insert(layerMap.begin(), layerMap.end());
    }

    /*
     * 2. Create reachability index
     */
    _buildReachabilityMatrix(vertices, edges);
    _buildCausalReachabilityMatrix();

}

vector<SituationGraph> SituationGraph::getWeaklyConnectedComponents(){
    vector<SituationGraph> graphs;
    map<long, SituationNode> tempMap = situationMap;
    while(!tempMap.empty()){
        SituationGraph sg;
        vector<long> toRemoves;
        set<long> visited;
        deque<long> toSearchs;
        toSearchs.push_back(tempMap.begin()->first);
        while(!toSearchs.empty()){
            long first = toSearchs.front();
            toSearchs.pop_front();

            if (visited.count(first) > 0) {
                continue;
            } else {
                visited.insert(first);
            }

            sg.situationMap[first] = tempMap[first];
            toRemoves.push_back(first);
            for(auto temp : tempMap){
                long node = temp.first;
                if(node != first){
                    if(relationMap.count(make_pair(first, node)) > 0){
                        if(visited.count(node) == 0){
                            toSearchs.push_back(node);
                        }
                        sg.relationMap[make_pair(first, node)] = relationMap[make_pair(first, node)];
                    }else if(relationMap.count(make_pair(node, first)) > 0){
                        if(visited.count(node) == 0){
                            toSearchs.push_back(node);
                        }
                        sg.relationMap[make_pair(node, first)] = relationMap[make_pair(node, first)];
                    }
                }
            }
        }
        graphs.push_back(sg);
        for(auto toRemove : toRemoves){
            tempMap.erase(toRemove);
        }
        toRemoves.clear();
    }
    return graphs;
}

DirectedGraph SituationGraph::getLayer(int index) {
    return layers[index];
}

SituationNode SituationGraph::getNode(long id) {
    return situationMap[id];
}

SituationRelation SituationGraph::getRelation(long src, long dest){
    edge_id eid = std::make_pair(src, dest);
    return relationMap[eid];
}

int SituationGraph::modelHeight() {
    return layers.size();
}

int SituationGraph::numOfNodes() {
    return situationMap.size();
}

void SituationGraph::print() {
    for (auto m : situationMap) {
        cout << m.second;
    }
}

void SituationGraph::clear(){
    cri.clear();
    situationMap.clear();
    relationMap.clear();
    layers.clear();
}
