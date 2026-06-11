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

#include "SituationRender.h"

SituationRender::SituationRender() {
    // TODO Auto-generated constructor stub

}

SituationRender::~SituationRender() {
    // TODO Auto-generated destructor stub
}

double SituationRender::rateOfOpConsistency(
        map<pair<long, int>, int> instanceArrivals) {
    double rate = 0.0;
    double consistency = 0.0;
    double total = 0.0;
    for (auto instanceArrival : instanceArrivals) {
        long id_1 = instanceArrival.first.first;
        int count_1 = instanceArrival.first.second;
        pair<long, int> instance_1;
        instance_1.first = id_1;
        instance_1.second = count_1;
        int arrival_1 = instanceArrival.second;

        for (auto instanceArrival_2 : instanceArrivals) {
            long id_2 = instanceArrival_2.first.first;
            int count_2 = instanceArrival_2.first.second;
            pair<long, int> instance_2;
            instance_2.first = id_2;
            instance_2.second = count_2;
            int arrival_2 = instanceArrival_2.second;
            if (instance_1 != instance_2) {

                if (sg.isCausallyReachable(id_2, id_1) && count_2 == count_1) {
                    total++;
                    if (arrival_2 < arrival_1) {
                        consistency++;

//                        cout << "operation instance [" << id_1 << "," << count_1
//                                << "," << arrival_1 << "] replayed after ["
//                                << id_2 << "," << count_2 << "," << arrival_2
//                                << "]" << endl;

                    } else {
//                        cout << "operation instance [" << id_1 << "," << count_1
//                                << "," << arrival_1 << "] replayed before ["
//                                << id_2 << "," << count_2 << "," << arrival_2
//                                << "]" << endl;
                    }

                } else {

                }
            }
        }
    }

    cout << endl << "consistency: " << consistency << ", total: " << total
            << endl;

    rate = consistency / total;
    return rate;
}
