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
    // TODO Auto-generated constructor stub
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

bool SituationGraph::isReachable(long src, long dest) {
    int i = situationMap[src].index;
    int j = situationMap[dest].index;
    return ri->at(i)[j];
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
    /*
     * initialize reachability matrix
     */
    int size = vertices.size();
    ri = new vector<vector<bool>>(size, vector<bool>(size, false));

    /*
     * build adjacency matrix
     */
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

    /*
     * build reachability matrix
     */
    for (int i = 1; i <= size; i++) {
        vector<vector<bool>> adjMatrixPow = _boolMatrixPower(adjMatrix, i);
        vector<vector<bool>> temp = *ri;
        _boolMatrixAdd(ri, temp, adjMatrixPow);
    }
}

void SituationGraph::loadModel(const std::string &filename,
        SituationEvolution *se) {
    std::ifstream f(filename);
    json data = json::parse(f);

    int index = 0;

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

            double duration = node.value()["Duration"].get<double>() / 1000.0;
            SituationInstance::Type type =
                    (SituationInstance::Type) node.value()["type"].get<short>();
            if (!node.value()["Cycle"].is_null()) {
                // cycle is in millisecond
                double cycle = node.value()["Cycle"].get<double>() / 1000.0;
                se->addInstance(id, type, SimTime(duration), SimTime(cycle));
            } else {
                se->addInstance(id, type, SimTime(duration));
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
                    edge_id reid;
                    reid.first = relation.dest;
                    reid.second = src;
                    edges.insert(eid);
                    edges.insert(reid);
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
//    cout << "print reachability matrix" << endl;
//    for(auto row : *ri){
//        for(auto col : row){
//            cout << col << "  ";
//        }
//        cout << endl;
//    }
}

DirectedGraph SituationGraph::getLayer(int index) {
    return layers[index];
}

SituationNode SituationGraph::getNode(long id) {
    return situationMap[id];
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

SituationGraph::~SituationGraph() {
    // TODO why cannot release pointer here?
//    if(ri){
//        ri->clear();
//        delete ri;
//        ri = NULL;
//    }
}

