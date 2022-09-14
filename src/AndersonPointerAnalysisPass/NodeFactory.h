#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>

#include <map>
#include <vector>

using namespace std;
using namespace llvm;

enum NodeType {
    Pointer,
    AllocationSite,
};

class PointNode {
    private:
        int idx;
        Value* value;
        NodeType nodeType;

    public:
        PointNode(int idx, Value* V, NodeType nt) : idx(idx), value(V), nodeType(nt) {}
        const Value* getValue() {
            return value;
        }
};

class NodeFactory {
    private:
        vector<PointNode> nodes;
        map<Value*, unsigned> allocSiteNodes;
        map<Value*, unsigned> pointerNodes;
        map<Value*, unsigned> retNodes;
    public:
        unsigned createAllocSiteNode(Value* V);
        unsigned createPointerNode(Value* V);
        unsigned createRetNode(Value* V);
        unsigned getAllocSiteNode(Value* V);
        unsigned getPointerNode(Value* V);
        unsigned getRetNode(Value* V);
        unsigned getNumNode() {
            return nodes.size();
        };
        const Value* getValueByIdx(int idx) {
            return nodes[idx].getValue();
        }
        
};
