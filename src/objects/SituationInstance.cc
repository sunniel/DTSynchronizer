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

#include "SituationInstance.h"

SituationInstance::SituationInstance(long id, Type type, simtime_t duration,
        simtime_t init, simtime_t cycle, double belief, double latency) {
    this->id = id;
    this->counter = 0;
    this->type = type;
    this->state = State::UNTRIGGERED;
    this->duration = duration;
    this->cycle = cycle;
    this->next_start = init;
    this->belief = belief;
    this->latency = latency;
}

SituationInstance::~SituationInstance() {

}

