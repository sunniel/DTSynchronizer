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

#include "../common/Constants.h"
#include "../common/Util.h"
#include "../common/RandomClass.h"
#include "../messages/IoTEvent_m.h"
#include "../messages/SimEvent_m.h"
#include "Synchronizer.h"

// ================ TODO for testing purpose =====================
#include "../objects/BNInferenceEngine.h"

Define_Module(Synchronizer);

Synchronizer::Synchronizer() {
    receivedOperations = 0;
    receivedSituations = 0;
    sentOperations = 0;
    slice = 0;

    // 500 ms
    check_cycle = 0.5;

    SETimeout = new cMessage(msg::SE_TIMEOUT);
    SCTimeout = new cMessage(msg::SC_TIMEOUT);
    timer = new cMessage(msg::TIMER_START);
}

Synchronizer::~Synchronizer() {
    if (SETimeout != NULL) {
        cancelAndDelete(SETimeout);
    }
    if (SCTimeout != NULL) {
        cancelAndDelete(SCTimeout);
    }
}

void Synchronizer::initialize() {
    /*
     * Construct a situation graph and a situation inference engine
     */
    const char* SITUATION_MODEL_SYNC = par("situation_file_sync").stringValue();
    sr.initModel(SITUATION_MODEL_SYNC);
    const char* SITUATION_MODEL_SRC = par("situation_file_src").stringValue();
    sr.initOriginalModel(SITUATION_MODEL_SRC);
    // 3000 ms
    slice_cycle = par("TIME_SLICE_LENGTH").doubleValue();
    sog.setModel(sr.getModel());
    sog.setModelInstance(&sr);

    /*
     * ================ testing situation graph =====================
     */
//    SituationGraph sg = sr.getModel();
//    long src = 102;
//    long dest = 104;
//    bool reachable = sg.isCausallyReachable(src, dest);
//    cout << endl << "from op " << src << " to op " << dest << " is reachable" << endl << endl;

    /*
     * ================ testing Bayesian network inference =====================
     */
//        sr.getInstance(100).state = SituationInstance::UNTRIGGERED;
//        sr.getInstance(101).state = SituationInstance::UNTRIGGERED;
//        sr.getInstance(102).state = SituationInstance::UNTRIGGERED;
//        sr.getInstance(103).state = SituationInstance::UNDETERMINED;
//        sr.getInstance(104).state = SituationInstance::UNDETERMINED;
//        sr.getInstance(105).state = SituationInstance::TRIGGERED;
//
//        sr.getInstance(106).state = SituationInstance::UNDETERMINED;
//        sr.getInstance(107).state = SituationInstance::UNTRIGGERED;
//        sr.getInstance(108).state = SituationInstance::UNTRIGGERED;
//        sr.getInstance(109).state = SituationInstance::UNDETERMINED;
//        sr.getInstance(110).state = SituationInstance::UNTRIGGERED;
//        sr.getInstance(111).state = SituationInstance::UNTRIGGERED;
//        sr.getInstance(112).state = SituationInstance::UNTRIGGERED;
//        sr.getInstance(113).state = SituationInstance::UNDETERMINED;
//        sr.getInstance(114).state = SituationInstance::UNDETERMINED;
//        sr.getInstance(115).state = SituationInstance::UNTRIGGERED;
//
//        sr.getInstance(116).state = SituationInstance::TRIGGERING;
//
//        sr.getInstance(117).state = SituationInstance::UNTRIGGERED;
//        sr.getInstance(118).state = SituationInstance::UNDETERMINED;
//        sr.getInstance(119).state = SituationInstance::UNTRIGGERED;

//        sr.getInstance(201).state = SituationInstance::TRIGGERED;
//        sr.getInstance(202).state = SituationInstance::TRIGGERING;
//        sr.getInstance(203).state = SituationInstance::TRIGGERING;
//        BNInferenceEngine engine;
//        engine.reason(sr.getModel(), sr.instanceMap, simTime());

        /*
         * ================ testing Situation Inference =====================
         */
//        std::set<long> triggered;
//        triggered.insert(100);
//        triggered.insert(101);
//    //    triggered.insert(102);
//        triggered.insert(103);
//        triggered.insert(115);
//        triggered.insert(116);
//    //    triggered.insert(104);
//    //    triggered.insert(105);
//    //    triggered.insert(106);
//    //    triggered.insert(108);
//        // create an instance with Triggered state for DR
//    //    sr.getInstance(100).counter = 1;
//        // create an instance with Triggered state for BR
//    //    sr.getInstance(101).counter = 1;
//    //    sr.getInstance(102).counter = 1;
//    //    sr.getInstance(201).counter = 1;
//    //    sr.getInstance(200).counter = 1;
//        sr.getInstance(113).state = SituationInstance::UNDETERMINED;
//        sr.getInstance(114).state = SituationInstance::UNDETERMINED;
//        sr.reason(triggered, simTime());

    // schedule situation evolution
    scheduleAt(check_cycle, SCTimeout);
    scheduleAt(slice_cycle, SETimeout);
    scheduleAt(0, timer);
}

void Synchronizer::finish() {
    recordScalar("Received Operations", receivedOperations);
    recordScalar("Received Operational Situations", receivedSituations);
    int consistency = sr.numOfConsistentOperation(true);
    recordScalar("Recognized Consistent Operations", consistency);
    int fullConsistent = sr.numOfConsistentOperation(false);
    recordScalar("full Consistent Operations", fullConsistent);
    recordScalar("Recognized Hidden Consistent Operations", fullConsistent - consistency);

    /*
     * Calculate recall & precision
     */
    std::vector<long> operations = sr.getModel().getAllOperationalSitutions();
    std::map<int, double> recalls;
    std::map<int, double> precisions;
    std::map<int, double> redundancies;
    double recall = 0;
    double precision = 0;
    double redundancy = 0;
    for(auto op : operations){
        double so_count = (double)sr.getInstance(op).counter;
        double ao_count = 0;
        if(actOBCounters.count(op) > 0){
            ao_count = (double)actOBCounters[op];
        }
        if(ao_count >= so_count){
            recalls[op] = (double)so_count / ao_count;
            precisions[op] = 1;
            redundancies[op] = 0;
        }else{
            recalls[op] = 1;
            precisions[op] = (double) ao_count / so_count;
            redundancies[op] = ((double) so_count - ao_count)
                    / ((double) so_count);
        }
        recall += recalls[op];
        precision += precisions[op];
        redundancy += redundancies[op];

        cout << "accuracy of situation " << op << " = " << recalls[op] << endl;
//        cout << "precision of situation " << op << " = " << precisions[op] << endl;
        cout << "redundancy of operation " << op << " = " << redundancies[op] << endl;
    }
    recall = recall / operations.size();
    precision = precision / operations.size();
    redundancy = redundancy / operations.size();
    recordScalar("Simulation accuracy", recall);
    recordScalar("Operation redundancy", redundancy);
    double F1 = 2.0 * (recall * precision) / (recall + precision);
    recordScalar("Simulation F1 score", F1);
    cout << endl;

    /*
     * Calculate situation occurrence fidelity
     * TODO: currently, simulated and actual observable situations are not fully aligned here
     */
//    std::vector<long> operations = sr.getModel().getAllOperationalSitutions();
    double sum_diff = 0;
    double sum_situation = 0;
    for(auto op : operations){
        double so_count = (double)sr.getInstance(op).counter;
        double ao_count = 0;
        if(actOBCounters.count(op) > 0){
            ao_count = (double)actOBCounters[op];
        }
        sum_diff += abs(so_count - ao_count) / ao_count;
        sum_situation += ao_count;
        cout << "actual situation " << op << " count = " << ao_count << endl;
        cout << "simulated situation " << op << " count = " << so_count << endl;
    }
    cout << "sum_diff: " << sum_diff << ", sum_situation: " << sum_situation <<endl;
    double fidelity_occ = 1- sum_diff / sum_situation;
    recordScalar("Situation Occurrence Fidelity", fidelity_occ);
    cout << endl;

    /*
     * Calculate situation alignment fidelity
     * Misalignment may be caused by higher cause frequency that put consequence and late cause into the same time slice
     */
//    double sum_sqr_max_diff = 0;
    for(auto simCauseCounts : m_simCauseCounts){
        long id = simCauseCounts.first.first;
        long count = simCauseCounts.first.second;
//        cout << "alignment fidelity calculation on "
//                << simCauseCounts.first.first << ", "
//                << simCauseCounts.first.second << endl;
        si_id iid(id, count);
//        std::vector<int> countDiff;
        for(auto causeCount : simCauseCounts.second){
            long cause = causeCount.first;
            int simCount = causeCount.second;
            int actCount = m_actCauseCounts[iid][cause];
            int diff = simCount - actCount;
            SAF[diff] = SAF[diff] + 1;
            diff = simCount - count;
            SAF2[diff] = SAF2[diff] + 1;

//            countDiff.push_back(abs(simCount - actCount));
//            cout << "situation " << id << " count " << count << " cause " << causeCount.first << " simulated count: " << simCount << endl;
//            cout << "situation " << id << " count " << count << " cause " << causeCount.first << " actual count: " << actCount << endl;
        }

//        if(countDiff.size() > 0){
//            int max_diff = *std::max_element(countDiff.begin(), countDiff.end());
//            sum_sqr_max_diff += pow(max_diff, 2);
////            cout << "max situation (" << id << ", " << count << ") max misalignment = " << max_diff << endl;
//        }else{
////            cout << "no cause of situation (" << id << ", " << count << ")" << endl;
//        }
    }

    cout << endl << "print situation alignment fidelity: " << endl;
    util::printMap(SAF);
//    cout << endl << "print situation alignment fidelity 2: " << endl;
//    util::printMap(SAF2);

//    double fidelity_ali = 1 - sqrt(sum_sqr_max_diff / (double)m_simCauseCounts.size());
//    recordScalar("Situation Alignment Fidelity", fidelity_ali);

    recordScalar("Sent Operations", sentOperations);

    // Record the ending time point
    std::chrono::steady_clock::time_point end =
            std::chrono::steady_clock::now();
    // Calculate the duration,
    // Use duration_cast to convert the difference to your desired unit (e.g., milliseconds)
    std::chrono::milliseconds duration = std::chrono::duration_cast<
            std::chrono::milliseconds>(end - start);
    std::cout << "Execution time: " << duration.count() << " ms" << std::endl;
}

void Synchronizer::handleMessage(cMessage *msg) {
    if (msg->isName(msg::IOT_EVENT)) {
        receivedOperations++;
        IoTEvent *event = check_and_cast<IoTEvent*>(msg);
        long id = event->getEventID();

        cout << "IoT event (" << id << "): toTrigger "
                << event->getToTrigger() << ", counter " << event->getCounter()
                << ", type " << event->getType() << ", cause counts "
                << event->getCauseCounts() << ", timestamp "
                << event->getTimestamp() << endl;

        /*
         * toTrigger: determines whether it is a situational event
         * type: determines whether the event is observable or hidden
         *
         * By rights, all received IoT events need to be cached for regression if needed.
         * Here, temporarily only triggering events are maintained for simplicity.
         */
        if (event->getToTrigger() && event->getType() == SituationInstance::NORMAL) {
            sog.cacheEvent(event->getEventID(), event->getToTrigger(),
                    event->getTimestamp());
            bufferCounters[id]++;
        }

        /*
         * Update actual observable situation counter and cause counters for occurrence fidelity analysis
         */
        if (event->getToTrigger()){
            receivedSituations++;

            actOBCounters.count(id) > 0 ? actOBCounters[id]++ : (actOBCounters[id] = 1);
            json j_causeCounts = json::parse(string(event->getCauseCounts()));
            std::map<long, int> causeCounts = j_causeCounts.get<std::map<long, int>>();
//            std::vector<si_id> actOBCauseCounts;
//            std::copy(causeCounts.begin(), causeCounts.end(), std::back_inserter(actOBCauseCounts));
            int count = actOBCounters[id];
            si_id actOBId(id, count);
            m_actCauseCounts[actOBId] = causeCounts;
        }

        // free up memory space
        delete event;
    } else if (msg->isName(msg::SE_TIMEOUT)) {

        /*
         * Situation evolution cycle is reached
         */

        simtime_t current = simTime();
        slice = (int) (current / slice_cycle);
        cout << endl << "current time: " << current << ", time slice: " <<  slice << endl;

        std::set<long> triggered;

//        cout << "print buffer counters: ";
//        util::printMap(bufferCounters);

        /*
         * 1. Get triggered observable situations
         */
        for (auto bufferCounter : bufferCounters) {
            if (bufferCounter.second > 0) {

                cout << "triggered ID " << bufferCounter.first << ", counter " << bufferCounter.second << endl;

                triggered.insert(bufferCounter.first);
                bufferCounters[bufferCounter.first]--;
            }
        }

        /*
         * 2. Situation inference
         * The inference result contains a list of triggered observable situations,
         * which is supposed to tell SOG to generate the corresponding simulation events.
         */
        std::set<long> tOperations = sr.reason(triggered, current);

        /*
         * Update simulated observable situation counter and cause counters for alignment fidelity analysis
         */
        for(auto op : tOperations){
            std::map<long, int> causeCounts;
            /*
             * Check the explicit cause only
             */
//            std::vector<long> causes = sr.getModel().getNode(op).causes;
//            for(auto cause : causes){
//                causeCounts[cause] = sr.getInstance(cause).counter;
//            }
            /*
             * Check both explicit cause and implicit cause
             */
            std::vector<long> operations = sr.getModel().getAllOperationalSitutions();
            for(auto op2 : operations){
                if (op2 != op && sr.getModel().isCausallyReachable(op2, op)) {
                    causeCounts[op2] = sr.getInstance(op2).counter;
                }
            }
//            std::vector<si_id> simOBCauseCounts;
//            std::copy(causeCounts.begin(), causeCounts.end(), std::back_inserter(simOBCauseCounts));
            int count = sr.getInstance(op).counter;
            si_id simOBId(op, count);
            m_simCauseCounts[simOBId] = causeCounts;
        }

        /*
         * 3. Simulation operation generation
         */
        std::queue<std::vector<VirtualOperation>> opSets = sog.generateOperations(
                tOperations, current);
        cout << "Operation sets are: " << endl;
        util::printComplexQueue(opSets);

        /*
         * 4. Send operations to simulator
         */
        while (!opSets.empty()) {
            std::vector<VirtualOperation> operations = opSets.front();
            for (auto op : operations) {
                SimEvent *event = new SimEvent(msg::SIM_EVENT);
                event->setEventID(op.id);
                event->setTimestamp(op.timestamp);
                event->setCount(op.count);
                simtime_t latency = lg.generator_latency(50);
                // send out the message
                sendDelayed(event, latency, "out");

                sentOperations++;
            }

            opSets.pop();
        }
        scheduleAt(simTime() + slice_cycle, SETimeout);
    } else if (msg->isName(msg::SC_TIMEOUT)) {
        sr.checkState(simTime());
        scheduleAt(simTime() + check_cycle, SCTimeout);
    } else if(msg->isName(msg::TIMER_START)){
        // Record the starting time point
        std::cout<< "Start timing the simulation" << std::endl;
        start = std::chrono::steady_clock::now();
        cancelAndDelete(timer);
    }
}
