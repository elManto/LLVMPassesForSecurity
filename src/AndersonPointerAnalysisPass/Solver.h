#include "Utils.h"


#include <queue>
#include <set>
#include <map>

using namespace std;


class PointsToNode {
    private:
        set<int> successors;
        set<int> loadTo;
        set<int> storeFrom;
        set<int> ptsSet;
    public:
        void addSuccessor(int idx) {
            successors.insert(idx);
        }

        void addLoad(int idx) {
            loadTo.insert(idx);
        }

        void addStore(int idx) {
            storeFrom.insert(idx);
        }
    
        bool addPointee(int pointee) {
            if (ptsSet.count(pointee))
                return false;
            ptsSet.insert(pointee);
            return true;
        }

        bool addPointee(set<int> pointeeSet) {
            bool isNovel = false;
            for (int idx : pointeeSet) {
                if (!ptsSet.count(idx)) {
                    isNovel = true;
                    ptsSet.insert(pointeeSet.begin(), pointeeSet.end());
                    return isNovel;
                }
            }
            return isNovel;
        }

        set<int>& getSuccessors() {
            return successors;
        }
    
        set<int>& getLoads() {
            return loadTo;
        }

        set<int>& getStores() {
            return storeFrom;
        }   

        set<int>& getPtsSet() {
            return ptsSet;
        }

        bool hasSuccessor(int succ) {
            return successors.count(succ);
        }
};

class AndersonGraph {
    private:
        vector<PointsToNode> graph;
        queue<int> workList;
        
        void insertEdge(int src, int dest);
        void propagate(int dst, set<int>& src);
        void propagate(int dst, int src);
    public:
        AndersonGraph(unsigned n, vector<MyConstraint>& constraints);
        void solve();
        vector<PointsToNode>& getGraph();
        void dumpGraph();
        void graph2map(map<int, vector<int>>* res);
};




