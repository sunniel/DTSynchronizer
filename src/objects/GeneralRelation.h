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

#ifndef OBJECTS_GENERALRELATION_H_
#define OBJECTS_GENERALRELATION_H_

#include <iostream>
#include "SituationRelation.h"

using namespace std;

class GeneralRelation {
public:
    enum Relation {
        AND, OR
    };

    long src;
    long dest;
    Relation relation;
    double weight;
public:
    GeneralRelation();
    GeneralRelation(SituationRelation::Relation sr, long src = 0, long dest = 0,
            double weight = 1.0);
    virtual ~GeneralRelation();
};

inline std::ostream& operator<<(std::ostream &os, const GeneralRelation &r) {
    return os << "General relation (" << r.src << " -> " << r.dest
            << "): relation " << r.relation << ", weight " << r.weight << endl;
}

#endif /* OBJECTS_GENERALRELATION_H_ */
