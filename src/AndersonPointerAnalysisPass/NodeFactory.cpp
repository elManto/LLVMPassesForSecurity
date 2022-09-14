#include <stdlib.h>

#include "NodeFactory.h"

#define NODE_ALREADY_PRESENT -10
#define NODE_NOT_PRESENT -20
using namespace llvm;


unsigned NodeFactory::createPointerNode(Value* V) {

    //errs() << "==> Creating Pointer Node\n";
    int idx = nodes.size();
    nodes.emplace_back(idx, V, Pointer);
    if (pointerNodes.find(V) == pointerNodes.end()) {
        pointerNodes.emplace(V, idx);
        //errs() << "<== Created\n";
        return idx;
    }
    //errs() << "<== Creation failed\n";
    exit(NODE_ALREADY_PRESENT);
}


unsigned NodeFactory::createAllocSiteNode(Value* V) {
    int idx = nodes.size();
    nodes.emplace_back(idx, V, AllocationSite);
    if (allocSiteNodes.find(V) == allocSiteNodes.end()) {
        allocSiteNodes.emplace(V, idx);
        return idx;
    }
    exit(NODE_ALREADY_PRESENT);
}

unsigned NodeFactory::createRetNode(Value* V) {
    int idx = nodes.size();
    nodes.emplace_back(idx, V, Pointer);
    if (retNodes.find(V) == retNodes.end()) {
        retNodes.emplace(V, idx);
        return idx;
    }
    exit(NODE_ALREADY_PRESENT);
}

unsigned NodeFactory::getAllocSiteNode(Value* V) {
    if (allocSiteNodes.find(V) != allocSiteNodes.end())
        return allocSiteNodes.at(V);
    exit(NODE_NOT_PRESENT);
}


unsigned NodeFactory::getPointerNode(Value* V) {
    if (pointerNodes.find(V) != pointerNodes.end())
        return pointerNodes.at(V);
    exit(NODE_NOT_PRESENT);
}


unsigned NodeFactory::getRetNode(Value* V) {
    if (retNodes.find(V) != retNodes.end())
        return retNodes.at(V);
    exit(NODE_NOT_PRESENT);
}



