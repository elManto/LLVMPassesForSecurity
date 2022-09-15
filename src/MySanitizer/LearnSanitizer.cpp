#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <vector>

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
using namespace std;

static bool isBlacklisted(const Function& F) {

    static const char *Blacklist[] = {

        "asan.", "llvm.", "sancov.", "__ubsan_handle_", "ign.", "__afl_",
        "_fini", "__libc_csu", "__asan",  "__msan", "msan.", "__sanitize"

    };

    for (auto const &BlacklistFunc : Blacklist) {

      if (F.getName().startswith(BlacklistFunc)) return true;

    }
    
    if (F.getName() == "_start") return true;

    return false;

}

struct LearnSanitizerModulePass : public ModulePass {
  private:

    FunctionCallee hook_load, hook_store, hook_malloc, 
                   hook_free, hook_entry, hook_exit;

	LLVMContext* C;

	Type *VoidTy, *Int8Ty, *Int16Ty, *Int32Ty, *Int64Ty, 
        *Int8PTy, *Int16PTy, *Int32PTy, *Int64PTy, *VoidPTy;

    DataLayout* Layout;

    void ReplaceHeapFunctions(vector<CallInst*>& HeapFunctions, FunctionCallee& SanitizerFunction) {    
        for (CallInst* Call : HeapFunctions) {
            
            Call->setCalledFunction(SanitizerFunction);
        }
    }


    void InstrumentMemoryAccesses(vector<Instruction*>& Accesses, Module& M, FunctionCallee SanitizerFunction) {

	    for (Instruction* Access : Accesses) {
            vector<Value*> args;
            Value* memoryPointer = nullptr;
            if (StoreInst* ST = dyn_cast<StoreInst>(Access))
                memoryPointer = ST->getPointerOperand();
            else if (LoadInst* LO = dyn_cast<LoadInst>(Access))
                memoryPointer = LO->getPointerOperand();


            PointerType* pointerType = cast<PointerType>(memoryPointer->getType());                         
            uint64_t storeSize = Layout->getTypeStoreSize(pointerType->getPointerElementType());
            Constant* Size = ConstantInt::get(pointerType->getPointerElementType(), storeSize);
            
            IRBuilder<> IRB(Access);
            args.push_back(memoryPointer);
            args.push_back(Size);
            CallInst* hook = IRB.CreateCall(SanitizerFunction, args);
	    	hook->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(*C, None));
	    }
    }


    void InstrumentEntryOrExitPoints(vector<Instruction*> EntryExitPoints, Module& M, FunctionCallee HookRtn) {
        for (Instruction* I : EntryExitPoints) {
            Function* ParentFunc = I->getFunction();
            IRBuilder<> IRB(I);
            vector<Value*> args;            

            Constant* fcn_name = ConstantDataArray::getString(*C, ParentFunc->getName(), true);
            AllocaInst* Alloca = IRB.CreateAlloca(Int8PTy);
            Value* FcnNameVal = static_cast<Value*>(Alloca);
            IRB.CreateStore(fcn_name, FcnNameVal);

            args.push_back(FcnNameVal);
            CallInst* call = IRB.CreateCall(HookRtn, args); 
	    	call->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(*C, None));
            DEBUG(errs() << "Installed a new hoook point\n");
        } 

    }



  public:
    

  static char ID;
  //Hello() : ModulePass(ID) {}
  explicit LearnSanitizerModulePass() : ModulePass(ID) {}


  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
  }

  bool doInitialization(Module &M) override {
    DEBUG(errs() << "INITIALIZATION \n");
	return true;
  }
  
  bool runOnModule(Module &M) override {

    //_M(M);

    C = &(M.getContext());
    Layout = new DataLayout(&M);
  
    VoidTy = Type::getVoidTy(*C);
  
    Int8Ty = IntegerType::get(*C, 8);
    Int16Ty = IntegerType::get(*C, 16);
    Int32Ty = IntegerType::get(*C, 32);
    Int64Ty = IntegerType::get(*C, 64);  

    
    Int8PTy  = PointerType::get(Int8Ty, 0);
    Int16PTy = PointerType::get(Int16Ty, 0);
    Int32PTy = PointerType::get(Int32Ty, 0);
    Int64PTy = PointerType::get(Int64Ty, 0); 

    VoidPTy = PointerType::get(VoidTy, 0);
 
    hook_store = M.getOrInsertFunction("__hook_store", VoidTy, Int64Ty, Int64Ty);
    hook_load = M.getOrInsertFunction("__hook_load", VoidTy, Int64Ty, Int64Ty);
    hook_malloc = M.getOrInsertFunction("__hook_malloc", Int8PTy, Int64Ty);
    hook_free = M.getOrInsertFunction("__hook_free", VoidTy, Int64Ty);
    hook_entry = M.getOrInsertFunction("__hook_entry", VoidTy, Int8PTy);
    hook_exit = M.getOrInsertFunction("__hook_exit", VoidTy, Int8PTy);
    

    vector<Instruction*> Stores, Loads;
    vector<CallInst*> Mallocs, Frees;
    vector<Instruction*> EntryPoints;
    vector<Instruction*> ReturnInstructions;
    

    for (Function& F : M) {
        if (isBlacklisted(F))
            continue;
        for (BasicBlock& BB : F) {

            if (&F.getEntryBlock() == &BB)
                EntryPoints.push_back(&(BB.front()));

            for (Instruction& I : BB) {

                if (StoreInst* ST = dyn_cast<StoreInst>(&I)) {
                    Stores.push_back(ST);
                }
                else if (LoadInst* LO = dyn_cast<LoadInst>(&I)) {
                    Loads.push_back(LO);
                }
                else if (CallInst* Call = dyn_cast<CallInst>(&I)) {

                    if (!Call->getCalledFunction()->getName().compare("malloc")) {
                        Mallocs.push_back(Call);
                    }
                    else if (!Call->getCalledFunction()->getName().compare("free")) {
                        Frees.push_back(Call);
                    }
                    else 
                        // other heap functions (realloc, ..)
                        continue;

                }
                else if (ReturnInst* RI = dyn_cast<ReturnInst>(&I)) {
                    ReturnInstructions.push_back(RI);
                }
            }
        }
    }

    InstrumentEntryOrExitPoints(EntryPoints, M, hook_entry);

    ReplaceHeapFunctions(Mallocs, hook_malloc);
    ReplaceHeapFunctions(Frees, hook_free);

    InstrumentMemoryAccesses(Stores, M, hook_store);
    InstrumentMemoryAccesses(Loads, M, hook_load);

    InstrumentEntryOrExitPoints(ReturnInstructions, M, hook_exit);
    
 
    return false;
  }
}; 

char LearnSanitizerModulePass::ID = 0;

static void registerLearnSanitizerPass(const PassManagerBuilder &,
                               legacy::PassManagerBase &PM) {

  PM.add(new LearnSanitizerModulePass());

}

static RegisterStandardPasses RegisterLearnSanitizerPass(
    PassManagerBuilder::EP_OptimizerLast, registerLearnSanitizerPass);

static RegisterStandardPasses RegisterLearnSanitizerPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerLearnSanitizerPass);

static RegisterPass<LearnSanitizerModulePass>
    X("LearnSanitizer", "LearnSanitizerPass",
      false,
      false
    );
