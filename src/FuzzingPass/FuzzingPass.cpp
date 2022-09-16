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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

//#define DEBUG 1

#ifdef DEBUG
#define DEBUG(X)                                                               \
  do {                                                                         \
    X;                                                                         \
  } while (false)
#else
#define DEBUG(X) ((void)0)
#endif

#define MIN_FCN_SIZE 1
#define MAP_SIZE_POW2 16
#define MAP_SIZE (1U << MAP_SIZE_POW2)

#define CTX 1


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

struct FuzzingModulePass : public ModulePass {
  private:

	LLVMContext* C;

	Type *Int8Ty, *Int32Ty,*Int8PTy;



  public:
    

  static char ID;

  explicit FuzzingModulePass() : ModulePass(ID) {}


  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
  }

  bool doInitialization(Module &M) override {

    struct timeval tv;
    struct timezone tz;
    uint32_t rand_seed;
    gettimeofday(&tv, &tz);
    rand_seed = tv.tv_sec ^ tv.tv_usec ^ getpid();
    srandom(rand_seed);

	return true;
  }
  
  bool runOnModule(Module &M) override {

    C = &(M.getContext());

    Int8Ty = IntegerType::get(*C, 8);
    Int32Ty = IntegerType::get(*C, 32);
    Int8PTy  = PointerType::get(Int8Ty, 0);

    Constant* OneConst = ConstantInt::get(Int8Ty, 1);

    unsigned int cur_loc = 0;
    unsigned int ctx = CTX;
    uint32_t map_size = MAP_SIZE;

    GlobalVariable *AFLPrevLocation = new GlobalVariable(M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__afl_prev_loc", 0, GlobalVariable::GeneralDynamicTLSModel, 0, false);
    GlobalVariable *AFLBitmap = new GlobalVariable(M, Int8PTy, false, GlobalValue::ExternalLinkage, 0, "__afl_area_ptr");

    GlobalVariable *AFLContext = nullptr;

    if (ctx) {

        AFLContext = new GlobalVariable(M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__afl_prev_ctx", 0, GlobalVariable::GeneralDynamicTLSModel, 0, false);
    }



    for (auto &F: M) {

        if (isBlacklisted(F) || F.size() < MIN_FCN_SIZE)
            continue;

        int are_there_calls = 0;
        for (auto &BB : F) {
            Value* PrevCtx = nullptr;

            BasicBlock::iterator InsertionPoint = BB.getFirstInsertionPt();
            IRBuilder<> IRB(&(*InsertionPoint));

            if (ctx) {
                if (&BB == &F.getEntryBlock()) {
                    // We skip leaf functions

                    for (auto &SecondBB : F) {
                        if (are_there_calls)
                            break;
                        
                        for (auto &I : SecondBB) {
                            if (CallInst* CI = dyn_cast<CallInst>(&I)) {
                                Function *Callee = CI->getCalledFunction();
                                if (!Callee || Callee->size() < MIN_FCN_SIZE)
                                    continue;
                                else {
                                    are_there_calls = 1;
                                    break;
                                }
                            }
                        }
                    }
                    if (are_there_calls) {

                        LoadInst* PrevCtxLoad = IRB.CreateLoad(AFLContext);
                        PrevCtx = static_cast<Value*>(PrevCtxLoad);
    
                        unsigned cur_ctx = random() % MAP_SIZE;
                        Value* CurCtx = ConstantInt::get(Int32Ty, cur_ctx);
                        
                        StoreInst* StoreCtx = IRB.CreateStore(IRB.CreateXor(PrevCtx, CurCtx), AFLContext);
                    
                    }
                }
            }

            cur_loc = random() % map_size;
            Constant* CurLoc = ConstantInt::get(Int32Ty, cur_loc);
            
            LoadInst* LoadPrevLoc = IRB.CreateLoad(AFLPrevLocation);
            LoadPrevLoc->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(*C, None));
            Value* PrevLoc = static_cast<Value*>(LoadPrevLoc);

            if (ctx) {
                PrevLoc = IRB.CreateZExt(IRB.CreateXor(PrevLoc, PrevCtx), Int32Ty);
            }

            LoadInst* MapPtr = IRB.CreateLoad(AFLBitmap);
            MapPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(*C, None));

            Value* MapPtrIdx = IRB.CreateGEP(MapPtr, IRB.CreateXor(PrevLoc, CurLoc));

            LoadInst* NumberOfHits = IRB.CreateLoad(MapPtrIdx);
            NumberOfHits->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(*C, None));

            Value* Add = IRB.CreateAdd(NumberOfHits, OneConst);
            
            // Never Zero from afl++
            Constant* ZeroConst = ConstantInt::get(Int8Ty, 0);
            auto comparisonFlag = IRB.CreateICmpEQ(Add, ZeroConst);
            auto carry = IRB.CreateZExt(comparisonFlag, Int8Ty);
            Add = IRB.CreateAdd(Add, carry);

            StoreInst* UpdateHits = IRB.CreateStore(Add, MapPtrIdx);
            UpdateHits->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(*C, None));

            StoreInst* UpdatePrevLocation = IRB.CreateStore(ConstantInt::get(Int32Ty, cur_loc >> 1), AFLPrevLocation);
            UpdatePrevLocation->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(*C, None));

            if (ctx && are_there_calls) {
                //restore old ctx when return
                Instruction* I = BB.getTerminator();
                if (isa<ReturnInst>(I)) {
                    IRBuilder<> Restore_IRB(I);
                    StoreInst* Restore = Restore_IRB.CreateStore(PrevCtx, AFLContext);
                    
                }
            }

        }
        
    
    }
    

    return false;
  }
}; 

char FuzzingModulePass::ID = 0;

static void registerFuzzingPass(const PassManagerBuilder &,
                               legacy::PassManagerBase &PM) {

  PM.add(new FuzzingModulePass());

}

static RegisterStandardPasses RegisterFuzzingPass(
    PassManagerBuilder::EP_OptimizerLast, registerFuzzingPass);

static RegisterStandardPasses RegisterFuzzingPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerFuzzingPass);

static RegisterPass<FuzzingModulePass>
    X("Fuzzing", "FuzzingPass",
      false,
      false
    );
