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

#ifndef OBJECTS_SITUATIONGRAPH_H_
#define OBJECTS_SITUATIONGRAPH_H_

#include <vector>
#include <utility>
#include <map>
#include <fstream>
#include <nlohmann/json.hpp>
#include "../common/Util.h"
#include "SituationNode.h"
#include "SituationRelation.h"
#include "DirectedGraph.h"

using namespace std;
using json = nlohmann::json;

// forward declaration
class SituationEvolution;

class SituationGraph {
private:
    /*
     * reachability index
     */
    vector<vector<bool>> *ri;
    map<long, map<long, bool>> cri;
public:
    map<long, SituationNode> situationMap;
    typedef pair<long, long> edge_id;
    map<edge_id, SituationRelation> relationMap;
    // For a n-layer SG graph, layers[n-1] is the bottom layer and layers[0] is the top layer
    vector<DirectedGraph> layers;
    string modelName;
private:
    vector<vector<bool>> _boolMatrixPower(vector<vector<bool>> &mat, int n);
    void _boolMatrixAdd(vector<vector<bool>> *result,
            vector<vector<bool>> &mat1, vector<vector<bool>> &mat2);
    void _buildReachabilityMatrix(set<long> &vertices, set<edge_id> &edges);
    void _buildCausalReachabilityMatrix();
public:
    SituationGraph();
    vector<long> getAllOperationalSitutions();
    vector<long> getOperationalSitutions(long topNodeId);
    vector<long> getHypotheses(long evidence);
    /*
     * A deprecated method, replaced by isCausallyReachable
     */
    bool isReachable(long src, long dest);
    // check whether argument evidence is the evidence of h on the situation graph
    bool isEvidential(long e, long h);
    bool isCausallyReachable(long src, long dest);
    void loadModel(const std::string &filename, SituationEvolution *se);
    void changeModel(const std::string &filename);
    DirectedGraph getLayer(int index);
    int modelHeight();
    SituationNode getNode(long id);
    SituationRelation getRelation(long src, long dest);
    int numOfNodes();
    vector<SituationGraph> getWeaklyConnectedComponents();
    void print();
    void clear();
    virtual ~SituationGraph();
};

#endif /* OBJECTS_SITUATIONGRAPH_H_ */
