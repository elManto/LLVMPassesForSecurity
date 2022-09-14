#include "Solver.h"
#include <iostream>

using namespace std;

AndersonGraph::AndersonGraph(unsigned n, vector<MyConstraint>& constraints) : graph(n) {
    for (auto& constraint : constraints) {
        switch (constraint.type) {
            case ConstraintType::Copy :
                graph[constraint.src].addSuccessor(constraint.dest);
                break;
            case ConstraintType::Load :
                graph[constraint.src].addLoad(constraint.dest);
                break;
            case ConstraintType::Store :
                graph[constraint.dest].addStore(constraint.src);
                break;
            case ConstraintType::AddressOf :
                int destIdx = constraint.dest;
                graph[destIdx].addPointee(constraint.src);
                workList.push(destIdx);
                break;
        }
    }
}

vector<PointsToNode>& AndersonGraph::getGraph() {
    return graph;
}

void AndersonGraph::solve() {
    while (!workList.empty()) {
        int idx = workList.front();
        workList.pop();
        PointsToNode node = graph[idx];
        for (int successor : node.getSuccessors())
            propagate(successor, node.getPtsSet());

        for (int pointee : node.getPtsSet()) {
            for (int load : node.getLoads())
                insertEdge(pointee, load);
            for (int store : node.getStores())
                insertEdge(store, pointee);
        }
        
    }

}

void AndersonGraph::propagate(int dest, set<int>& s) {
    bool isChanged = graph[dest].addPointee(s);
    if (isChanged)
        workList.push(dest);
}

void AndersonGraph::propagate(int dest, int src) {
    bool isChanged = graph[dest].addPointee(src);
    if (isChanged) 
        workList.push(dest);
}

void AndersonGraph::insertEdge(int src, int dest) {
    PointsToNode& srcNode = graph[src];
    if (!srcNode.hasSuccessor(dest)) {
        srcNode.addSuccessor(dest);
        if (!srcNode.getPtsSet().empty())
            workList.push(src);
    }
}

void AndersonGraph::dumpGraph() {
    unsigned idx = 0;
    for (PointsToNode node : graph) {
        cout << "node " << idx << "\n";
        set<int> pointsToSet = node.getPtsSet();

        cout << "\tPointees are: ";
        for (int pointee : pointsToSet) {
            cout << "" << pointee << " ";
        }
        cout << "\n\n";
        idx++;
    }
}

void AndersonGraph::graph2map(map<int, vector<int>>* res) {
    //map<int, vector<int>>* res = new map<int, vector<int>>();
    unsigned idx = 0;
    for (PointsToNode node : graph) {

        set<int> pointsToSet = node.getPtsSet();
        if (pointsToSet.empty()) {
            idx++;
            continue;
        }

        for (int pointee : pointsToSet) {

            (*res)[idx].push_back(pointee);
        }
        idx++;

    }

}
