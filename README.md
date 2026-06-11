# DT Synchronization OMNeT++ Simulation （OMNeT++ 6.0）

## 1. Libraries & Dependencies

1) External library linked: Boost (https://www.boost.org/)

2) DLib: For Bayesian network inference.
Note: the network model must be connected. Namely, there should be no partitions.

## 2. Situation Modeling

.json files under the /src/files directory define a situation model (i.e., situation graph). One contains multiple layers, devided into JSON arrayes. The first layer is the top layer of situation graph, called layer 0, while the last layer is the bottom layer.

In each layer, there are multipe JSON objects, representing situation nodes. Each situation is uniquely identified by ID.
 
Predecessors and Children contain the causes and evidences of a situation respectively. They could be null if empty. 

Duration is the length of a situation, which could be 0 for a transient situation. Cycle is the time gap between two sitaution occurrences, which could be set to empty or 0 to represent no-gap.

Currently, ***it is assumed that each observable situaiton is only related to one state variable, which, however, is not restricted in principle***.

Note: A bottom-layer node should not have more than one parent node. Otherwise, it may cause a runtime error. 

The SG_default_large.json contains a model of 240 nodes in total.

## 3. Implementation Notes

Most functions are not fully implemented, or implemented in a workaround way, including: 

1) In Synchronzier, the reasoning result from calling SituationReasoner's *reason* method contains a list of triggered observable situations,  which is supposed to tell TriggeringEventGenerator to generate the corresponding simulation events.

2) In the *generateTriggeringEvents* method of TriggeringEventGenerator, the event merge function is only an over-simplified implementation.

3) The *generateTriggeringEvents* method of TriggeringEventGenerator is supposed to use the function parameter *cycleTriggered* to generate events for sync failure, and add them to mergedEvents, which hasn't been implemented yet.