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

#include "GeneralRelation.h"

GeneralRelation::GeneralRelation() {
    src = 0;
    dest = 0;
    relation = Relation::AND;
    weight = 1;
}

GeneralRelation::GeneralRelation(SituationRelation::Relation sr, long src, long dest,
        double weight) {
    switch (sr) {
    case SituationRelation::OR:
        relation = Relation::OR;
        break;
    default:
        relation = Relation::AND;
    }
    this->src = src;
    this->dest = dest;
    this->weight = weight;
}

GeneralRelation::~GeneralRelation() {

}

