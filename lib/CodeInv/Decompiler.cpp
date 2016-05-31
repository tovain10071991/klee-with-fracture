//===--- Decompiler - Decompiles machine basic blocks -----------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class uses the disassembler and inverse instruction selector classes
// to decompile target specific code into LLVM IR and return function objects
// back to the user.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: August 28, 2013
//===----------------------------------------------------------------------===//

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CFG.h"

#include "CodeInv/Decompiler.h"

#include <err.h>

#include "Helper/LLDBHelper.h"

using namespace llvm;

namespace fracture {

Decompiler::Decompiler(Disassembler *NewDis, Module *NewMod, raw_ostream &InfoOut, raw_ostream &ErrOut) :
    Dis(NewDis), Mod(NewMod), Infos(InfoOut), Errs(ErrOut){

  assert(NewDis && "Cannot initialize decompiler with null Disassembler!");
  Context = Dis->getMCDirector()->getContext();
  Emitter = new IREmitter(this, Infos, Errs);
  if (Mod == NULL) {
    std::string ModID = Dis->getExecutable()->getFileName().data();
    ModID += "-IR";
    Mod = new Module(StringRef(ModID), *(Dis->getMCDirector()->getContext()));
    initModule();
  }
}

Decompiler::~Decompiler() {
  delete Context;
  delete Mod;
  delete Dis;
  delete Emitter;
}

void Decompiler::initModule()
{
  const TargetMachine* TM = Dis->getMCDirector()->getTargetMachine();
  Mod->setTargetTriple(TM->getTargetTriple());
  Mod->setDataLayout(TM->getDataLayout()->getStringRepresentation());

  const MCRegisterInfo* MRI = Dis->getMCDirector()->getMCRegisterInfo();

  for(unsigned i = 1; i < MRI->getNumRegs(); ++i)
  {
    MCSuperRegIterator super_reg_iter(i, MRI);
    if(!super_reg_iter.isValid())
    {
      Mod->getOrInsertGlobal(MRI->getName(i), Type::getIntNTy(*Context, Dis->getMCDirector()->getRegType(i).getSizeInBits()));
    }
  }

  Mod->getOrInsertGlobal("OF", Type::getInt1Ty(*Context));
  Mod->getOrInsertGlobal("SF", Type::getInt1Ty(*Context));
  Mod->getOrInsertGlobal("ZF", Type::getInt1Ty(*Context));
  Mod->getOrInsertGlobal("AF", Type::getInt1Ty(*Context));
  Mod->getOrInsertGlobal("PF", Type::getInt1Ty(*Context));
  Mod->getOrInsertGlobal("CF", Type::getInt1Ty(*Context));
  Mod->getOrInsertGlobal("TF", Type::getInt1Ty(*Context));
  Mod->getOrInsertGlobal("IF", Type::getInt1Ty(*Context));
  Mod->getOrInsertGlobal("DF", Type::getInt1Ty(*Context));
  Mod->getOrInsertGlobal("NT", Type::getInt1Ty(*Context));
  Mod->getOrInsertGlobal("RF", Type::getInt1Ty(*Context));
  
  Mod->getOrInsertFunction("saib_collect_indirect", FunctionType::get(Type::getVoidTy(*Context), {Type::getInt64Ty(*Context)}, false));
}

void Decompiler::decompile(unsigned long Address) {
  std::vector<unsigned long> Children;
  Children.push_back(Address);

  do {
    //size_t ChildrenSize = Children.size();
    //errs() << "Size: " << ChildrenSize << "\n";
    Function* CurFunc = decompileFunction(Children.back());
    Children.pop_back();
    if (CurFunc == NULL) {
      continue;
    }
    // Scan Current Function for children (should probably record children
    // during decompile...)
    for (Function::iterator BI = CurFunc->begin(), BE = CurFunc->end();
         BI != BE; ++BI) {
      for (BasicBlock::iterator I = BI->begin(), E = BI->end();
           I != E; ++I) {
        CallInst *CI = dyn_cast<CallInst>(I);
        //outs() << "------CI------\n";
        if (CI == NULL || !CI->getCalledFunction() || !CI->getCalledFunction()->hasFnAttribute("Address")) {
          //outs() << "Continue?\n";
          continue;
        }
        //CI->dump();
        StringRef AddrStr =
          CI->getCalledFunction()->getFnAttribute("Address").getValueAsString();
        uint64_t Addr;
        AddrStr.getAsInteger(10, Addr);
        DEBUG(outs() << "Read Address as: " << format("%1" PRIx64, Addr)
          << ", " << AddrStr << "\n");
        StringRef FName = Dis->getFunctionName(Addr);
        // Change sections to check if function address is paired with a 
        // relocated function and then set function name accordingly
        StringRef SectionName;
        object::SectionRef Section = Dis->getSectionByAddress(Addr);
        Dis->setSection(Section);
        Dis->getRelocFunctionName(Addr, FName);
        Section = Dis->getSectionByAddress(Address);
        Dis->setSection(Section);
        CI->getCalledFunction()->setName(FName);
        Function *NF = Mod->getFunction(FName);
        if (Addr != 0 && (NF == NULL || NF->empty())) {
          Children.push_back(Addr);
        }
      }
    }
  } while (Children.size() != 0); // While there are children, decompile
}

Function* Decompiler::decompileFunction(unsigned long Address) {
  // Check that Address is inside the current section.
  // TODO: Find a better way to do this check. What we really care about is
  // avoiding reads to library calls and areas of memory we can't "see".
  const object::SectionRef Sect = Dis->getCurrentSection();
  uint64_t SectStart, SectEnd;
#if LLVM_VERSION_CODE == LLVM_VERSION(3, 4)
  error_code ec;
  ec = Sect.getAddress(SectStart);
  assert(!ec && "get addr failed in Decompiler::decompileFunction");
  ec = Sect.getSize(SectEnd);
  assert(!ec && "get size failed in Decompiler::decompileFunction");
#else
  SectStart = Sect.getAddress();
  SectEnd = Sect.getSize();
#endif
  SectEnd += SectStart;
  if (Address < SectStart || Address > SectEnd) {
    errs() << "Address out of bounds for section (is this a library call?): "
           << format("%1" PRIx64, Address) << "\n";
    return NULL;
  }

  MachineFunction *MF = Dis->disassemble(Address);

  // Get Function Name
  // TODO: Determine Function Type
  FunctionType *FType = FunctionType::get(Type::getPrimitiveType(*Context,
      Type::VoidTyID), false);
  AttributeSet AS;
  AS = AS.addAttribute(*Context, AttributeSet::FunctionIndex, "Address", Twine(Address).str());
  Function *F =
    cast<Function>(Mod->getOrInsertFunction(MF->getName(), FType, AS));

  if (!F->empty()) {
    return F;
  }
  F->setLinkage(GlobalValue::LinkageTypes::WeakAnyLinkage);
  // For each basic block
  MachineFunction::iterator BI = MF->begin(), BE = MF->end();
  while (BI != BE) {
    //outs() << "-----BI------\n";
    //BI->dump();
    getOrCreateBasicBlock(BI->getName(), F);
    ++BI;
  }

  BI = MF->begin();
  while (BI != BE) {
//    BI->dump();
    if (decompileBasicBlock(BI, F) == NULL) {
      printError("Unable to decompile basic block!");
    }
    ++BI;
  }

  // During Decompilation, did any "in-between" basic blocks get created?
  // Nothing ever splits the entry block, so we skip it.
  for (Function::iterator I = ++F->begin(), E = F->end(); I != E; ++I) {
    if (!(I->empty())) {
      continue;
    }

    // Right now, the only way to get the right offset is to parse its name
    // it sucks, but it works.
    StringRef Name = I->getName();

//    size_t Off = F->getName().size() + 1;
//    size_t Size = Name.size() - 3;

    //outs() << "-----I------\n";
    //outs() << "Name: " << Name << " Offset: " << Off << " Size: " << Size << "\n";
    //I->dump();

    StringRef BBAddrStr = Name.substr(3, Name.size() - 3);
    unsigned long long BBAddr;
    getAsUnsignedInteger(BBAddrStr, 10, BBAddr);
    DEBUG(errs() << "Split Target: " << Name << "\t Address: "
                 << BBAddr << "\n");
    // split Block at AddrStr
    Function::iterator SB;      // Split basic block
    BasicBlock::iterator SI, SE;    // Split instruction
    // Note the ++, nothing ever splits the entry block.
    for (SB = ++F->begin(); SB != E; ++SB) {
      DEBUG(SB->dump());
      if (SB->empty() || BBAddr < getBasicBlockAddress(SB)) {
        continue;
      }
      assert(SB->getTerminator() && "Decompiler::decompileFunction - getTerminator (missing llvm unreachable?)");
      DEBUG(outs() << "SB: " << SB->getName()
              << "\tRange: " << Dis->getDebugOffset(SB->begin()->getDebugLoc())
              << " " << Dis->getDebugOffset(SB->getTerminator()->getDebugLoc())
              << "\n");
      if (BBAddr > Dis->getDebugOffset(SB->getTerminator()->getDebugLoc())) {
        continue;
      }

      // Reorder instructions based on Debug Location
      sortBasicBlock(SB);
      DEBUG(errs() << "Found Split Block: " << SB->getName() << "\n");
      // Find iterator to split on.
      for (SI = SB->begin(), SE = SB->end(); SI != SE; ++SI) {
        // outs() << "SI: " << SI->getDebugLoc().getLine() << "\n";
        if (Dis->getDebugOffset(SI->getDebugLoc()) == BBAddr) break;
        if (Dis->getDebugOffset(SI->getDebugLoc()) > BBAddr) {
          errs() << "Could not find address inside basic block!\n"
                 << "SI: " << Dis->getDebugOffset(SI->getDebugLoc()) << "\n"
                 << "BBAddr: " << BBAddr << "\n";
          break;
        }
      }
      break;
    }
    if (!SB || SI == SE || SB == E) {
      errs() << "Decompiler: Failed to find instruction offset in function!\n";
      continue;
    }
    outs() << SB->getName() << " -> " << I->getName() << "\nCreating Block...\n";
    splitBasicBlockIntoBlock(SB, SI, I);
  }

  return F;
}

BasicBlock* Decompiler::getOrCreateBasicBlock(unsigned long Address, Function *F) {
  std::string TBName;
  // Find and/or create the basic block
  raw_string_ostream TBOut(TBName);
  TBOut << "bb_" << Address;

  return getOrCreateBasicBlock(StringRef(TBOut.str()), F);
}

BasicBlock* Decompiler::getOrCreateBasicBlock(StringRef BBName, Function *F) {
  // Set this basic block as the target
  BasicBlock *BBTgt = NULL;
  Function::iterator BI = F->begin(), BE = F->end();
  while (BI->getName() != BBName && BI != BE) ++BI;
  if (BI == BE) {
    BBTgt =
      BasicBlock::Create(*(Dis->getMCDirector()->getContext()), BBName, F);
  } else {
    BBTgt = &(*BI);
  }

  return BBTgt;
}

BasicBlock* Decompiler::decompileBasicBlock(MachineBasicBlock *MBB,
  Function *F) {
  // Create a new basic block (if necessary)
  BasicBlock *BB = getOrCreateBasicBlock(MBB->getName(), F);

  for (MachineBasicBlock::iterator I = MBB->instr_begin(), E = MBB->instr_end();
       I != E; ++I) {
    Emitter->EmitIR(BB, I);
  }

  if(!BB->getTerminator())
  {
    Emitter->getIRB()->SetInsertPoint(BB);
    Emitter->getIRB()->CreateUnreachable();
  }

  return BB;
}

// Note: Users should not use this function if the BB is empty.
uint64_t Decompiler::getBasicBlockAddress(BasicBlock *BB) {
  if (BB->empty()) {
    errs() << "Empty basic block encountered, these do not have addresses!\n";
    return 0;                   // In theory, a BB could have an address of 0
                                // in practice it is invalid.
  } else {
    return Dis->getDebugOffset(BB->begin()->getDebugLoc());
  }
}

void Decompiler::sortBasicBlock(BasicBlock *BB) {
  BasicBlock::InstListType *Cur = &BB->getInstList();
  BasicBlock::InstListType::iterator P, I, E, S;
  I = Cur->begin();
  E = Cur->end();
  while (I != E) {
    P = I;
    if (++I == E) {
      break; // Note the terminator is always last instruction
    }
    if (Dis->getDebugOffset(P->getDebugLoc())
      <= Dis->getDebugOffset(I->getDebugLoc())) {
      continue;
    }
    while (--P != Cur->begin()
      && Dis->getDebugOffset(P->getDebugLoc())
      > Dis->getDebugOffset(I->getDebugLoc())) {
      // Do nothing.
    }
    // Insert at P, remove at I
    S = I;
    ++S;
    Instruction *Tmp = &(*I);
    Cur->remove(I);
    Cur->insertAfter(P, Tmp);
    I = S;
  }
  I = Cur->begin();
  E = Cur->end();
  while (I != E) {
    // outs() << "Line #: " << I->getDebugLoc().getLine() << "\n";
    ++I;
  }
}

// This is basically the split basic block function but it does not create
// a new basic block.
void Decompiler::splitBasicBlockIntoBlock(Function::iterator Src,
  BasicBlock::iterator FirstInst, BasicBlock *Tgt) {
  assert(Src->getTerminator() && "Can't use splitBasicBlock on degenerate BB!");
  assert(FirstInst != Src->end() &&
         "Trying to get me to create degenerate basic block!");

  Tgt->moveAfter(Src);

  // Move all of the specified instructions from the original basic block into
  // the new basic block.
  Tgt->getInstList().splice(Tgt->end(), Src->getInstList(),
    FirstInst, Src->end());

  // Add a branch instruction to the newly formed basic block.
  BranchInst *BI = BranchInst::Create(Tgt, Src);
  // Set debugLoc to the instruction before the terminator's DebugLoc.
  // Note the pre-inc which can confuse folks.
  BI->setDebugLoc((++Src->rbegin())->getDebugLoc());

  // Now we must loop through all of the successors of the New block (which
  // _were_ the successors of the 'this' block), and update any PHI nodes in
  // successors.  If there were PHI nodes in the successors, then they need to
  // know that incoming branches will be from New, not from Old.
  //
  for (succ_iterator I = succ_begin(Tgt), E = succ_end(Tgt); I != E; ++I) {
    // Loop over any phi nodes in the basic block, updating the BB field of
    // incoming values...
    BasicBlock *Successor = *I;
    PHINode *PN;
    for (BasicBlock::iterator II = Successor->begin();
         (PN = dyn_cast<PHINode>(II)); ++II) {
      int IDX = PN->getBasicBlockIndex(Src);
      while (IDX != -1) {
        PN->setIncomingBlock((unsigned)IDX, Tgt);
        IDX = PN->getBasicBlockIndex(Src);
      }
    }
  }
}

Function* Decompiler::getFunctionByAddr(uint64_t addr)
{
  for(auto iter = Mod->begin(); iter != Mod->end(); ++iter)
  {
    StringRef AddrStr = iter->getFnAttribute("Address").getValueAsString();
    uint64_t Addr;
    AddrStr.getAsInteger(10, Addr);
    if(Addr==addr)
      return iter;
  }
  FunctionType *FType = FunctionType::get(Type::getPrimitiveType(*Context, Type::VoidTyID), false);
  AttributeSet AS;
  AS = AS.addAttribute(*Context, AttributeSet::FunctionIndex, "Address", Twine(addr).str());
  Function *F = cast<Function>(Mod->getOrInsertFunction(get_func_name(addr), FType, AS));
  return F;
}

} // End namespace fracture
