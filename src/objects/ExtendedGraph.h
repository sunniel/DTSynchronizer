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

#ifndef OBJECTS_EXTENDEDGRAPH_H_
#define OBJECTS_EXTENDEDGRAPH_H_

#include <vector>
#include <map>
#include <fstream>
#include "GeneralNode.h"
#include "GeneralRelation.h"

class ExtendedGraph {
public:
    map<long, GeneralNode> nodeMap;
    typedef pair<long, long> edge_id;
    map<edge_id, GeneralRelation> relationMap;
public:
    ExtendedGraph();
    GeneralNode getNode(long id);
    int numOfNodes();
    void print();
    virtual ~ExtendedGraph();
};

#endif /* OBJECTS_EXTENDEDGRAPH_H_ */
