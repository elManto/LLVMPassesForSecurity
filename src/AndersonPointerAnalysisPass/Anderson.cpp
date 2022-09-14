#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"


#include "NodeFactory.h"
#include "Utils.h"
#include "Solver.h"

//#define DEBUG 1

#ifdef DEBUG
#define DEBUG(X)                                                               \
  do {                                                                         \
    X;                                                                         \
  } while (false)
#else
#define DEBUG(X) ((void)0)
#endif

using namespace llvm;

static std::string getValueName (const Value *v) {
  // If we can get name directly
  if (v->getName().str().length() > 0) {
      return v->getName().str();
  } else if (isa<Instruction>(v)) {
      std::string s = "";
      raw_string_ostream *strm = new raw_string_ostream(s);
      v->print(*strm);
      std::string inst = strm->str();
      size_t idx1 = inst.find("%");
      size_t idx2 = inst.find(" ", idx1);
      if (idx1 != std::string::npos && idx2 != std::string::npos && idx1 == 2) {
          return inst.substr(idx1, idx2 - idx1);
      } else {
          // nothing match
          return "";
      }
  } else if (const ConstantInt *cint = dyn_cast<ConstantInt>(v)) {
      std::string s = "";
      raw_string_ostream *strm = new raw_string_ostream(s);
      cint->getValue().print(*strm, true);
      return strm->str();
  } else {
      std::string s = "";
      raw_string_ostream *strm = new raw_string_ostream(s);
      v->print(*strm);
      std::string inst = strm->str();
      return "\"" + inst + "\"";
  }
}

struct AndersonAnalysisModulePass : public ModulePass {
  private:
    void AddFunctionReturnNodes(Module &M) {
        for (Function &F: M) {
            if (F.isIntrinsic() || F.isDeclaration())
                continue;
            if (F.getType()->isPointerTy()) {
                Value* Ret = static_cast<Value*>(&F);
                NF.createRetNode(Ret);
            }
        }
    }

    void AddFunctionBodyConstraints(Module &M) {

        for (Function &F : M) {
            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    if (I.getType() && I.getType()->isPointerTy()) {
                        NF.createPointerNode(&I);
                    }
                }
            }
            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    AddInstructionConstraints(I);
                }
            }
        }     
    }

    void AddInstructionConstraints(Instruction &I) {
        if (isa<AllocaInst>(&I)) {
            if(!I.getType()->isPointerTy())
                return;
            int srcIdx = NF.createAllocSiteNode(&I);
            int destIdx = NF.getPointerNode(&I);
            AllConstraints.emplace_back(destIdx, srcIdx, AddressOf); 
        }
        else if (isa<LoadInst>(&I)) {
            if (!I.getType()->isPointerTy())
                return;
            int srcIdx = NF.getPointerNode(I.getOperand(0));
            int destIdx = NF.getPointerNode(&I);
            AllConstraints.emplace_back(destIdx, srcIdx, Load);
        }
        else if (isa<StoreInst>(&I)) {
            if(I.getOperand(0)->getType()->isPointerTy()) {
                int srcIdx = NF.getPointerNode(I.getOperand(0));
                int destIdx = NF.getPointerNode(I.getOperand(1));
                AllConstraints.emplace_back(destIdx, srcIdx, Store);
            }
        }
        else if (isa<PHINode>(&I)) {
            if (!I.getType()->isPointerTy())
                return;
            PHINode* phi = static_cast<PHINode*>(&I);
            int destIdx = NF.getPointerNode(&I);
            for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
                int srcIdx = NF.getPointerNode(phi->getIncomingValue(i));
                AllConstraints.emplace_back(destIdx, srcIdx, Copy);
            }
        }
        else if (isa<CallInst>(&I) || isa<InvokeInst>(&I)) {
            CallBase* CB = static_cast<CallBase*>(&I);
            if (CB->isIndirectCall())
                return;
            Function* calledFunction = CB->getCalledFunction();
            if (calledFunction->isIntrinsic() || calledFunction->isDeclaration())
                return;

            if (!CB->getFunctionType()->isPointerTy())
                return;

            int destIdx = NF.getPointerNode(&I);
            int srcIdx;
            if (calledFunction->getName().compare("malloc")) {
                // Naive way to check if it is a malloc and also
                // we should extend to calloc, realloc, handle the free, ..
                srcIdx = NF.getAllocSiteNode(&I);
                AllConstraints.emplace_back(destIdx, srcIdx, AddressOf);
            }
            else {
                srcIdx = NF.getRetNode(calledFunction);
                AllConstraints.emplace_back(destIdx, srcIdx, Copy);
            }


            AddArgConstraints(CB, calledFunction);

        }
        else if (isa<ReturnInst>(&I)) {
            if (I.getNumOperands() > 0 && I.getOperand(0)->getType()->isPointerTy()) {

                int destIdx = NF.getRetNode(I.getParent()->getParent());
                int srcIdx = NF.getPointerNode(I.getOperand(0));
                AllConstraints.emplace_back(destIdx, srcIdx, Copy);
            }
        }
        else if (isa<GetElementPtrInst>(&I)) {
            int destIdx = NF.getPointerNode(&I);
            int srcIdx = NF.getPointerNode(I.getOperand(0));
            AllConstraints.emplace_back(destIdx, srcIdx, Copy);
        }
        else
            return;

    }

    void AddArgConstraints(CallBase* CB, Function* F) {
        auto argumentIterator = CB->arg_begin();
        auto parameterIterator = F->arg_begin();
        
        while (argumentIterator != CB->arg_end() && parameterIterator != F->arg_end()) {
            Value* argument = *argumentIterator;
            Value* parameter = &*parameterIterator;
            if (argument->getType()->isPointerTy() && parameter->getType()->isPointerTy()) {
                int destIdx = NF.getPointerNode(parameter);
                int srcIdx = NF.getPointerNode(argument);
                AllConstraints.emplace_back(destIdx, srcIdx, Copy);
            }
            argumentIterator++;
            parameterIterator++;
        }
    }

    void dumpConstraints() {
      errs() << "Constraints " << AllConstraints.size() << "\n";
      for(auto &item: AllConstraints) {
        auto srcStr = getValueName(NF.getValueByIdx(item.src));
        auto destStr = getValueName(NF.getValueByIdx(item.dest));
        // auto srcStr = item.getSrc();
        // auto destStr = item.getDest();
        switch(item.type) {
          case AddressOf:
            errs() << destStr << " <- &" << srcStr << "\n";
            break;
          case Copy:
            errs() << destStr << " <- " << srcStr << "\n";
            break;
          case Load:
            errs() << destStr << " <- *" << srcStr << "\n";
            break;
          case Store:
            errs() << "*" << destStr << " <- " << srcStr << "\n";
            break;
        }
      }
    }


  public:
    
  NodeFactory NF;
  vector<MyConstraint> AllConstraints; 

  static char ID;
  //Hello() : ModulePass(ID) {}
  explicit AndersonAnalysisModulePass() : ModulePass(ID) {}


  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
  }

  bool doInitialization(Module &M) override {
    errs() << "INITIALIZATION \n";
    return true;
  }
  
  bool runOnModule(Module &M) override {
    AddFunctionReturnNodes(M);
    AddFunctionBodyConstraints(M);
    
    unsigned n = NF.getNumNode();

    DEBUG(dumpConstraints());
    
    AndersonGraph anderson(n, AllConstraints);
    anderson.solve();

    //anderson.dumpGraph();

    map<int, vector<int>> nodes_map;
    anderson.graph2map(&nodes_map);

    for (auto it = nodes_map.begin(); it != nodes_map.end(); ++it) {
        int idx = it->first;
        const Value* V = NF.getValueByIdx(idx);
        errs() << "Node with value name : " << getValueName(V) << "\n";
        vector<int> pointees = it->second;
        for (int p : pointees) {
            const Value* pointee = NF.getValueByIdx(p);
            errs() << "\t" << getValueName(pointee) << "\n";
        }
    }

    return false;
  }
}; 

char AndersonAnalysisModulePass::ID = 0;

static void registerAndersonAnalysisPass(const PassManagerBuilder &,
                               legacy::PassManagerBase &PM) {

  PM.add(new AndersonAnalysisModulePass());

}

static RegisterStandardPasses RegisterAndersonAnalysisPass(
    PassManagerBuilder::EP_OptimizerLast, registerAndersonAnalysisPass);

static RegisterStandardPasses RegisterAndersonAnalysisPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerAndersonAnalysisPass);

static RegisterPass<AndersonAnalysisModulePass>
    X("Anderson", "AndrersonAnalysisPass",
      false,
      false
    );
