//===-- ARM64CleanupLocalDynamicTLSPass.cpp -----------------------*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Local-dynamic access to thread-local variables proceeds in three stages.
//
// 1. The offset of this Module's thread-local area from TPIDR_EL0 is calculated
//    in much the same way as a general-dynamic TLS-descriptor access against
//    the special symbol _TLS_MODULE_BASE.
// 2. The variable's offset from _TLS_MODULE_BASE_ is calculated using
//    instructions with "dtprel" modifiers.
// 3. These two are added, together with TPIDR_EL0, to obtain the variable's
//    true address.
//
// This is only better than general-dynamic access to the variable if two or
// more of the first stage TLS-descriptor calculations can be combined. This
// pass looks through a function and performs such combinations.
//
//===----------------------------------------------------------------------===//
#include "ARM64.h"
#include "ARM64InstrInfo.h"
#include "ARM64MachineFunctionInfo.h"
#include "ARM64TargetMachine.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
using namespace llvm;

namespace {
struct LDTLSCleanup : public MachineFunctionPass {
  static char ID;
  LDTLSCleanup() : MachineFunctionPass(ID) {}

  virtual bool runOnMachineFunction(MachineFunction &MF) {
    ARM64FunctionInfo *AFI = MF.getInfo<ARM64FunctionInfo>();
    if (AFI->getNumLocalDynamicTLSAccesses() < 2) {
      // No point folding accesses if there isn't at least two.
      return false;
    }

    MachineDominatorTree *DT = &getAnalysis<MachineDominatorTree>();
    return VisitNode(DT->getRootNode(), 0);
  }

  // Visit the dominator subtree rooted at Node in pre-order.
  // If TLSBaseAddrReg is non-null, then use that to replace any
  // TLS_base_addr instructions. Otherwise, create the register
  // when the first such instruction is seen, and then use it
  // as we encounter more instructions.
  bool VisitNode(MachineDomTreeNode *Node, unsigned TLSBaseAddrReg) {
    MachineBasicBlock *BB = Node->getBlock();
    bool Changed = false;

    // Traverse the current block.
    for (MachineBasicBlock::iterator I = BB->begin(), E = BB->end(); I != E;
         ++I) {
      switch (I->getOpcode()) {
      case ARM64::TLSDESC_BLR:
        // Make sure it's a local dynamic access.
        if (!I->getOperand(1).isSymbol() ||
            strcmp(I->getOperand(1).getSymbolName(), "_TLS_MODULE_BASE_"))
          break;

        if (TLSBaseAddrReg)
          I = replaceTLSBaseAddrCall(I, TLSBaseAddrReg);
        else
          I = setRegister(I, &TLSBaseAddrReg);
        Changed = true;
        break;
      default:
        break;
      }
    }

    // Visit the children of this block in the dominator tree.
    for (MachineDomTreeNode::iterator I = Node->begin(), E = Node->end();
         I != E; ++I) {
      Changed |= VisitNode(*I, TLSBaseAddrReg);
    }

    return Changed;
  }

  // Replace the TLS_base_addr instruction I with a copy from
  // TLSBaseAddrReg, returning the new instruction.
  MachineInstr *replaceTLSBaseAddrCall(MachineInstr *I,
                                       unsigned TLSBaseAddrReg) {
    MachineFunction *MF = I->getParent()->getParent();
    const ARM64TargetMachine *TM =
        static_cast<const ARM64TargetMachine *>(&MF->getTarget());
    const ARM64InstrInfo *TII = TM->getInstrInfo();

    // Insert a Copy from TLSBaseAddrReg to x0, which is where the rest of the
    // code sequence assumes the address will be.
    MachineInstr *Copy =
        BuildMI(*I->getParent(), I, I->getDebugLoc(),
                TII->get(TargetOpcode::COPY), ARM64::X0).addReg(TLSBaseAddrReg);

    // Erase the TLS_base_addr instruction.
    I->eraseFromParent();

    return Copy;
  }

  // Create a virtal register in *TLSBaseAddrReg, and populate it by
  // inserting a copy instruction after I. Returns the new instruction.
  MachineInstr *setRegister(MachineInstr *I, unsigned *TLSBaseAddrReg) {
    MachineFunction *MF = I->getParent()->getParent();
    const ARM64TargetMachine *TM =
        static_cast<const ARM64TargetMachine *>(&MF->getTarget());
    const ARM64InstrInfo *TII = TM->getInstrInfo();

    // Create a virtual register for the TLS base address.
    MachineRegisterInfo &RegInfo = MF->getRegInfo();
    *TLSBaseAddrReg = RegInfo.createVirtualRegister(&ARM64::GPR64RegClass);

    // Insert a copy from X0 to TLSBaseAddrReg for later.
    MachineInstr *Next = I->getNextNode();
    MachineInstr *Copy = BuildMI(*I->getParent(), Next, I->getDebugLoc(),
                                 TII->get(TargetOpcode::COPY),
                                 *TLSBaseAddrReg).addReg(ARM64::X0);

    return Copy;
  }

  virtual const char *getPassName() const {
    return "Local Dynamic TLS Access Clean-up";
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.addRequired<MachineDominatorTree>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
};
}

char LDTLSCleanup::ID = 0;
FunctionPass *llvm::createARM64CleanupLocalDynamicTLSPass() {
  return new LDTLSCleanup();
}
