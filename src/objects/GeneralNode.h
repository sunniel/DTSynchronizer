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

#ifndef OBJECTS_GENERALNODE_H_
#define OBJECTS_GENERALNODE_H_

#include <iostream>
#include <vector>

using namespace std;

class GeneralNode {
public:
    long id;
    // triggering state threshold
    double threshold;
    vector<long> predecessors;
public:
    GeneralNode();
    virtual ~GeneralNode();
};

inline std::ostream& operator<<(std::ostream &os, const GeneralNode &s) {
    os << "General node (" << s.id << "): threshold " << s.threshold << endl;
    os << "predecessors (" << s.predecessors.size() << "): ";
    for(auto predecessor : s.predecessors){
        os << predecessor << ", ";
    }
    os << endl;
    return os;
}

#endif /* OBJECTS_GENERALNODE_H_ */
