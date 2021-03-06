//===- MipsDisassembler.cpp - Disassembler for Mips -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is part of the Mips Disassembler.
//
//===----------------------------------------------------------------------===//

#include "Mips.h"
#include "MipsRegisterInfo.h"
#include "MipsSubtarget.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/MC/MCFixedLenDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/MemoryObject.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

typedef MCDisassembler::DecodeStatus DecodeStatus;

namespace {

/// MipsDisassemblerBase - a disasembler class for Mips.
class MipsDisassemblerBase : public MCDisassembler {
public:
  /// Constructor     - Initializes the disassembler.
  ///
  MipsDisassemblerBase(const MCSubtargetInfo &STI, const MCRegisterInfo *Info,
                       bool bigEndian) :
    MCDisassembler(STI), RegInfo(Info),
    IsN64(STI.getFeatureBits() & Mips::FeatureN64), isBigEndian(bigEndian) {}

  virtual ~MipsDisassemblerBase() {}

  const MCRegisterInfo *getRegInfo() const { return RegInfo.get(); }

  bool isN64() const { return IsN64; }

private:
  OwningPtr<const MCRegisterInfo> RegInfo;
  bool IsN64;
protected:
  bool isBigEndian;
};

/// MipsDisassembler - a disasembler class for Mips32.
class MipsDisassembler : public MipsDisassemblerBase {
  bool IsMicroMips;
public:
  /// Constructor     - Initializes the disassembler.
  ///
  MipsDisassembler(const MCSubtargetInfo &STI, const MCRegisterInfo *Info,
                   bool bigEndian) :
    MipsDisassemblerBase(STI, Info, bigEndian) {
      IsMicroMips = STI.getFeatureBits() & Mips::FeatureMicroMips;
    }

  /// getInstruction - See MCDisassembler.
  virtual DecodeStatus getInstruction(MCInst &instr,
                                      uint64_t &size,
                                      const MemoryObject &region,
                                      uint64_t address,
                                      raw_ostream &vStream,
                                      raw_ostream &cStream) const;
};


/// Mips64Disassembler - a disasembler class for Mips64.
class Mips64Disassembler : public MipsDisassemblerBase {
public:
  /// Constructor     - Initializes the disassembler.
  ///
  Mips64Disassembler(const MCSubtargetInfo &STI, const MCRegisterInfo *Info,
                     bool bigEndian) :
    MipsDisassemblerBase(STI, Info, bigEndian) {}

  /// getInstruction - See MCDisassembler.
  virtual DecodeStatus getInstruction(MCInst &instr,
                                      uint64_t &size,
                                      const MemoryObject &region,
                                      uint64_t address,
                                      raw_ostream &vStream,
                                      raw_ostream &cStream) const;
};

} // end anonymous namespace

// Forward declare these because the autogenerated code will reference them.
// Definitions are further down.
static DecodeStatus DecodeGPR64RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder);

static DecodeStatus DecodeCPU16RegsRegisterClass(MCInst &Inst,
                                                 unsigned RegNo,
                                                 uint64_t Address,
                                                 const void *Decoder);

static DecodeStatus DecodeGPR32RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder);

static DecodeStatus DecodePtrRegisterClass(MCInst &Inst,
                                           unsigned Insn,
                                           uint64_t Address,
                                           const void *Decoder);

static DecodeStatus DecodeDSPRRegisterClass(MCInst &Inst,
                                            unsigned RegNo,
                                            uint64_t Address,
                                            const void *Decoder);

static DecodeStatus DecodeFGR64RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder);

static DecodeStatus DecodeFGR32RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder);

static DecodeStatus DecodeFGRH32RegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder);

static DecodeStatus DecodeCCRRegisterClass(MCInst &Inst,
                                           unsigned RegNo,
                                           uint64_t Address,
                                           const void *Decoder);

static DecodeStatus DecodeFCCRegisterClass(MCInst &Inst,
                                           unsigned RegNo,
                                           uint64_t Address,
                                           const void *Decoder);

static DecodeStatus DecodeHWRegsRegisterClass(MCInst &Inst,
                                              unsigned Insn,
                                              uint64_t Address,
                                              const void *Decoder);

static DecodeStatus DecodeAFGR64RegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder);

static DecodeStatus DecodeACC64DSPRegisterClass(MCInst &Inst,
                                                unsigned RegNo,
                                                uint64_t Address,
                                                const void *Decoder);

static DecodeStatus DecodeHI32DSPRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);

static DecodeStatus DecodeLO32DSPRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);

static DecodeStatus DecodeMSA128BRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);

static DecodeStatus DecodeMSA128HRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);

static DecodeStatus DecodeMSA128WRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);

static DecodeStatus DecodeMSA128DRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);

static DecodeStatus DecodeMSACtrlRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);

static DecodeStatus DecodeBranchTarget(MCInst &Inst,
                                       unsigned Offset,
                                       uint64_t Address,
                                       const void *Decoder);

static DecodeStatus DecodeJumpTarget(MCInst &Inst,
                                     unsigned Insn,
                                     uint64_t Address,
                                     const void *Decoder);

// DecodeBranchTargetMM - Decode microMIPS branch offset, which is
// shifted left by 1 bit.
static DecodeStatus DecodeBranchTargetMM(MCInst &Inst,
                                         unsigned Offset,
                                         uint64_t Address,
                                         const void *Decoder);

// DecodeJumpTargetMM - Decode microMIPS jump target, which is
// shifted left by 1 bit.
static DecodeStatus DecodeJumpTargetMM(MCInst &Inst,
                                       unsigned Insn,
                                       uint64_t Address,
                                       const void *Decoder);

static DecodeStatus DecodeMem(MCInst &Inst,
                              unsigned Insn,
                              uint64_t Address,
                              const void *Decoder);

static DecodeStatus DecodeMSA128Mem(MCInst &Inst, unsigned Insn,
                                    uint64_t Address, const void *Decoder);

static DecodeStatus DecodeMemMMImm12(MCInst &Inst,
                                     unsigned Insn,
                                     uint64_t Address,
                                     const void *Decoder);

static DecodeStatus DecodeMemMMImm16(MCInst &Inst,
                                     unsigned Insn,
                                     uint64_t Address,
                                     const void *Decoder);

static DecodeStatus DecodeFMem(MCInst &Inst, unsigned Insn,
                               uint64_t Address,
                               const void *Decoder);

static DecodeStatus DecodeSimm16(MCInst &Inst,
                                 unsigned Insn,
                                 uint64_t Address,
                                 const void *Decoder);

// Decode the immediate field of an LSA instruction which
// is off by one.
static DecodeStatus DecodeLSAImm(MCInst &Inst,
                                 unsigned Insn,
                                 uint64_t Address,
                                 const void *Decoder);

static DecodeStatus DecodeInsSize(MCInst &Inst,
                                  unsigned Insn,
                                  uint64_t Address,
                                  const void *Decoder);

static DecodeStatus DecodeExtSize(MCInst &Inst,
                                  unsigned Insn,
                                  uint64_t Address,
                                  const void *Decoder);

/// INSVE_[BHWD] have an implicit operand that the generated decoder doesn't
/// handle.
template <typename InsnType>
static DecodeStatus DecodeINSVE_DF(MCInst &MI, InsnType insn, uint64_t Address,
                                   const void *Decoder);
namespace llvm {
extern Target TheMipselTarget, TheMipsTarget, TheMips64Target,
              TheMips64elTarget;
}

static MCDisassembler *createMipsDisassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new MipsDisassembler(STI, T.createMCRegInfo(""), true);
}

static MCDisassembler *createMipselDisassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new MipsDisassembler(STI, T.createMCRegInfo(""), false);
}

static MCDisassembler *createMips64Disassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new Mips64Disassembler(STI, T.createMCRegInfo(""), true);
}

static MCDisassembler *createMips64elDisassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new Mips64Disassembler(STI, T.createMCRegInfo(""), false);
}

extern "C" void LLVMInitializeMipsDisassembler() {
  // Register the disassembler.
  TargetRegistry::RegisterMCDisassembler(TheMipsTarget,
                                         createMipsDisassembler);
  TargetRegistry::RegisterMCDisassembler(TheMipselTarget,
                                         createMipselDisassembler);
  TargetRegistry::RegisterMCDisassembler(TheMips64Target,
                                         createMips64Disassembler);
  TargetRegistry::RegisterMCDisassembler(TheMips64elTarget,
                                         createMips64elDisassembler);
}

#include "MipsGenDisassemblerTables.inc"

template <typename InsnType>
static DecodeStatus DecodeINSVE_DF(MCInst &MI, InsnType insn, uint64_t Address,
                                   const void *Decoder) {
  typedef DecodeStatus (*DecodeFN)(MCInst &, unsigned, uint64_t, const void *);
  // The size of the n field depends on the element size
  // The register class also depends on this.
  InsnType tmp = fieldFromInstruction(insn, 17, 5);
  unsigned NSize = 0;
  DecodeFN RegDecoder = nullptr;
  if ((tmp & 0x18) == 0x00) { // INSVE_B
    NSize = 4;
    RegDecoder = DecodeMSA128BRegisterClass;
  } else if ((tmp & 0x1c) == 0x10) { // INSVE_H
    NSize = 3;
    RegDecoder = DecodeMSA128HRegisterClass;
  } else if ((tmp & 0x1e) == 0x18) { // INSVE_W
    NSize = 2;
    RegDecoder = DecodeMSA128WRegisterClass;
  } else if ((tmp & 0x1f) == 0x1c) { // INSVE_D
    NSize = 1;
    RegDecoder = DecodeMSA128DRegisterClass;
  } else
    llvm_unreachable("Invalid encoding");

  assert(NSize != 0 && RegDecoder != nullptr);

  // $wd
  tmp = fieldFromInstruction(insn, 6, 5);
  if (RegDecoder(MI, tmp, Address, Decoder) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  // $wd_in
  if (RegDecoder(MI, tmp, Address, Decoder) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  // $n
  tmp = fieldFromInstruction(insn, 16, NSize);
  MI.addOperand(MCOperand::CreateImm(tmp));
  // $ws
  tmp = fieldFromInstruction(insn, 11, 5);
  if (RegDecoder(MI, tmp, Address, Decoder) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  // $n2
  MI.addOperand(MCOperand::CreateImm(0));

  return MCDisassembler::Success;
}

  /// readInstruction - read four bytes from the MemoryObject
  /// and return 32 bit word sorted according to the given endianess
static DecodeStatus readInstruction32(const MemoryObject &region,
                                      uint64_t address,
                                      uint64_t &size,
                                      uint32_t &insn,
                                      bool isBigEndian,
                                      bool IsMicroMips) {
  uint8_t Bytes[4];

  // We want to read exactly 4 Bytes of data.
  if (region.readBytes(address, 4, Bytes) == -1) {
    size = 0;
    return MCDisassembler::Fail;
  }

  if (isBigEndian) {
    // Encoded as a big-endian 32-bit word in the stream.
    insn = (Bytes[3] <<  0) |
           (Bytes[2] <<  8) |
           (Bytes[1] << 16) |
           (Bytes[0] << 24);
  }
  else {
    // Encoded as a small-endian 32-bit word in the stream.
    // Little-endian byte ordering:
    //   mips32r2:   4 | 3 | 2 | 1
    //   microMIPS:  2 | 1 | 4 | 3
    if (IsMicroMips) {
      insn = (Bytes[2] <<  0) |
             (Bytes[3] <<  8) |
             (Bytes[0] << 16) |
             (Bytes[1] << 24);
    } else {
      insn = (Bytes[0] <<  0) |
             (Bytes[1] <<  8) |
             (Bytes[2] << 16) |
             (Bytes[3] << 24);
    }
  }

  return MCDisassembler::Success;
}

DecodeStatus
MipsDisassembler::getInstruction(MCInst &instr,
                                 uint64_t &Size,
                                 const MemoryObject &Region,
                                 uint64_t Address,
                                 raw_ostream &vStream,
                                 raw_ostream &cStream) const {
  uint32_t Insn;

  DecodeStatus Result = readInstruction32(Region, Address, Size,
                                          Insn, isBigEndian, IsMicroMips);
  if (Result == MCDisassembler::Fail)
    return MCDisassembler::Fail;

  if (IsMicroMips) {
    // Calling the auto-generated decoder function.
    Result = decodeInstruction(DecoderTableMicroMips32, instr, Insn, Address,
                               this, STI);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      return Result;
    }
    return MCDisassembler::Fail;
  }

  // Calling the auto-generated decoder function.
  Result = decodeInstruction(DecoderTableMips32, instr, Insn, Address,
                             this, STI);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  return MCDisassembler::Fail;
}

DecodeStatus
Mips64Disassembler::getInstruction(MCInst &instr,
                                   uint64_t &Size,
                                   const MemoryObject &Region,
                                   uint64_t Address,
                                   raw_ostream &vStream,
                                   raw_ostream &cStream) const {
  uint32_t Insn;

  DecodeStatus Result = readInstruction32(Region, Address, Size,
                                          Insn, isBigEndian, false);
  if (Result == MCDisassembler::Fail)
    return MCDisassembler::Fail;

  // Calling the auto-generated decoder function.
  Result = decodeInstruction(DecoderTableMips6432, instr, Insn, Address,
                             this, STI);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }
  // If we fail to decode in Mips64 decoder space we can try in Mips32
  Result = decodeInstruction(DecoderTableMips32, instr, Insn, Address,
                             this, STI);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  return MCDisassembler::Fail;
}

static unsigned getReg(const void *D, unsigned RC, unsigned RegNo) {
  const MipsDisassemblerBase *Dis = static_cast<const MipsDisassemblerBase*>(D);
  return *(Dis->getRegInfo()->getRegClass(RC).begin() + RegNo);
}

static DecodeStatus DecodeCPU16RegsRegisterClass(MCInst &Inst,
                                                 unsigned RegNo,
                                                 uint64_t Address,
                                                 const void *Decoder) {

  return MCDisassembler::Fail;

}

static DecodeStatus DecodeGPR64RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder) {

  if (RegNo > 31)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Mips::GPR64RegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeGPR32RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;
  unsigned Reg = getReg(Decoder, Mips::GPR32RegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodePtrRegisterClass(MCInst &Inst,
                                           unsigned RegNo,
                                           uint64_t Address,
                                           const void *Decoder) {
  if (static_cast<const MipsDisassembler *>(Decoder)->isN64())
    return DecodeGPR64RegisterClass(Inst, RegNo, Address, Decoder);

  return DecodeGPR32RegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeDSPRRegisterClass(MCInst &Inst,
                                            unsigned RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return DecodeGPR32RegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeFGR64RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Mips::FGR64RegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeFGR32RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Mips::FGR32RegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeFGRH32RegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Mips::FGRH32RegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeCCRRegisterClass(MCInst &Inst,
                                           unsigned RegNo,
                                           uint64_t Address,
                                           const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;
  unsigned Reg = getReg(Decoder, Mips::CCRRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeFCCRegisterClass(MCInst &Inst,
                                           unsigned RegNo,
                                           uint64_t Address,
                                           const void *Decoder) {
  if (RegNo > 7)
    return MCDisassembler::Fail;
  unsigned Reg = getReg(Decoder, Mips::FCCRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeMem(MCInst &Inst,
                              unsigned Insn,
                              uint64_t Address,
                              const void *Decoder) {
  int Offset = SignExtend32<16>(Insn & 0xffff);
  unsigned Reg = fieldFromInstruction(Insn, 16, 5);
  unsigned Base = fieldFromInstruction(Insn, 21, 5);

  Reg = getReg(Decoder, Mips::GPR32RegClassID, Reg);
  Base = getReg(Decoder, Mips::GPR32RegClassID, Base);

  if(Inst.getOpcode() == Mips::SC){
    Inst.addOperand(MCOperand::CreateReg(Reg));
  }

  Inst.addOperand(MCOperand::CreateReg(Reg));
  Inst.addOperand(MCOperand::CreateReg(Base));
  Inst.addOperand(MCOperand::CreateImm(Offset));

  return MCDisassembler::Success;
}

static DecodeStatus DecodeMSA128Mem(MCInst &Inst, unsigned Insn,
                                    uint64_t Address, const void *Decoder) {
  int Offset = SignExtend32<10>(fieldFromInstruction(Insn, 16, 10));
  unsigned Reg = fieldFromInstruction(Insn, 6, 5);
  unsigned Base = fieldFromInstruction(Insn, 11, 5);

  Reg = getReg(Decoder, Mips::MSA128BRegClassID, Reg);
  Base = getReg(Decoder, Mips::GPR32RegClassID, Base);

  Inst.addOperand(MCOperand::CreateReg(Reg));
  Inst.addOperand(MCOperand::CreateReg(Base));

  // The immediate field of an LD/ST instruction is scaled which means it must
  // be multiplied (when decoding) by the size (in bytes) of the instructions'
  // data format.
  // .b - 1 byte
  // .h - 2 bytes
  // .w - 4 bytes
  // .d - 8 bytes
  switch(Inst.getOpcode())
  {
  default:
    assert (0 && "Unexpected instruction");
    return MCDisassembler::Fail;
    break;
  case Mips::LD_B:
  case Mips::ST_B:
    Inst.addOperand(MCOperand::CreateImm(Offset));
    break;
  case Mips::LD_H:
  case Mips::ST_H:
    Inst.addOperand(MCOperand::CreateImm(Offset << 1));
    break;
  case Mips::LD_W:
  case Mips::ST_W:
    Inst.addOperand(MCOperand::CreateImm(Offset << 2));
    break;
  case Mips::LD_D:
  case Mips::ST_D:
    Inst.addOperand(MCOperand::CreateImm(Offset << 3));
    break;
  }

  return MCDisassembler::Success;
}

static DecodeStatus DecodeMemMMImm12(MCInst &Inst,
                                     unsigned Insn,
                                     uint64_t Address,
                                     const void *Decoder) {
  int Offset = SignExtend32<12>(Insn & 0x0fff);
  unsigned Reg = fieldFromInstruction(Insn, 21, 5);
  unsigned Base = fieldFromInstruction(Insn, 16, 5);

  Reg = getReg(Decoder, Mips::GPR32RegClassID, Reg);
  Base = getReg(Decoder, Mips::GPR32RegClassID, Base);

  if (Inst.getOpcode() == Mips::SC_MM)
    Inst.addOperand(MCOperand::CreateReg(Reg));

  Inst.addOperand(MCOperand::CreateReg(Reg));
  Inst.addOperand(MCOperand::CreateReg(Base));
  Inst.addOperand(MCOperand::CreateImm(Offset));

  return MCDisassembler::Success;
}

static DecodeStatus DecodeMemMMImm16(MCInst &Inst,
                                     unsigned Insn,
                                     uint64_t Address,
                                     const void *Decoder) {
  int Offset = SignExtend32<16>(Insn & 0xffff);
  unsigned Reg = fieldFromInstruction(Insn, 21, 5);
  unsigned Base = fieldFromInstruction(Insn, 16, 5);

  Reg = getReg(Decoder, Mips::GPR32RegClassID, Reg);
  Base = getReg(Decoder, Mips::GPR32RegClassID, Base);

  Inst.addOperand(MCOperand::CreateReg(Reg));
  Inst.addOperand(MCOperand::CreateReg(Base));
  Inst.addOperand(MCOperand::CreateImm(Offset));

  return MCDisassembler::Success;
}

static DecodeStatus DecodeFMem(MCInst &Inst,
                               unsigned Insn,
                               uint64_t Address,
                               const void *Decoder) {
  int Offset = SignExtend32<16>(Insn & 0xffff);
  unsigned Reg = fieldFromInstruction(Insn, 16, 5);
  unsigned Base = fieldFromInstruction(Insn, 21, 5);

  Reg = getReg(Decoder, Mips::FGR64RegClassID, Reg);
  Base = getReg(Decoder, Mips::GPR32RegClassID, Base);

  Inst.addOperand(MCOperand::CreateReg(Reg));
  Inst.addOperand(MCOperand::CreateReg(Base));
  Inst.addOperand(MCOperand::CreateImm(Offset));

  return MCDisassembler::Success;
}


static DecodeStatus DecodeHWRegsRegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder) {
  // Currently only hardware register 29 is supported.
  if (RegNo != 29)
    return  MCDisassembler::Fail;
  Inst.addOperand(MCOperand::CreateReg(Mips::HWR29));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeAFGR64RegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder) {
  if (RegNo > 30 || RegNo %2)
    return MCDisassembler::Fail;

  ;
  unsigned Reg = getReg(Decoder, Mips::AFGR64RegClassID, RegNo /2);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeACC64DSPRegisterClass(MCInst &Inst,
                                                unsigned RegNo,
                                                uint64_t Address,
                                                const void *Decoder) {
  if (RegNo >= 4)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Mips::ACC64DSPRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeHI32DSPRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo >= 4)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Mips::HI32DSPRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeLO32DSPRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo >= 4)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Mips::LO32DSPRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeMSA128BRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Mips::MSA128BRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeMSA128HRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Mips::MSA128HRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeMSA128WRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Mips::MSA128WRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeMSA128DRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Mips::MSA128DRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeMSACtrlRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo > 7)
    return MCDisassembler::Fail;

  unsigned Reg = getReg(Decoder, Mips::MSACtrlRegClassID, RegNo);
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeBranchTarget(MCInst &Inst,
                                       unsigned Offset,
                                       uint64_t Address,
                                       const void *Decoder) {
  unsigned BranchOffset = Offset & 0xffff;
  BranchOffset = SignExtend32<18>(BranchOffset << 2) + 4;
  Inst.addOperand(MCOperand::CreateImm(BranchOffset));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeJumpTarget(MCInst &Inst,
                                     unsigned Insn,
                                     uint64_t Address,
                                     const void *Decoder) {

  unsigned JumpOffset = fieldFromInstruction(Insn, 0, 26) << 2;
  Inst.addOperand(MCOperand::CreateImm(JumpOffset));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeBranchTargetMM(MCInst &Inst,
                                         unsigned Offset,
                                         uint64_t Address,
                                         const void *Decoder) {
  unsigned BranchOffset = Offset & 0xffff;
  BranchOffset = SignExtend32<18>(BranchOffset << 1);
  Inst.addOperand(MCOperand::CreateImm(BranchOffset));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeJumpTargetMM(MCInst &Inst,
                                       unsigned Insn,
                                       uint64_t Address,
                                       const void *Decoder) {
  unsigned JumpOffset = fieldFromInstruction(Insn, 0, 26) << 1;
  Inst.addOperand(MCOperand::CreateImm(JumpOffset));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeSimm16(MCInst &Inst,
                                 unsigned Insn,
                                 uint64_t Address,
                                 const void *Decoder) {
  Inst.addOperand(MCOperand::CreateImm(SignExtend32<16>(Insn)));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeLSAImm(MCInst &Inst,
                                 unsigned Insn,
                                 uint64_t Address,
                                 const void *Decoder) {
  // We add one to the immediate field as it was encoded as 'imm - 1'.
  Inst.addOperand(MCOperand::CreateImm(Insn + 1));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeInsSize(MCInst &Inst,
                                  unsigned Insn,
                                  uint64_t Address,
                                  const void *Decoder) {
  // First we need to grab the pos(lsb) from MCInst.
  int Pos = Inst.getOperand(2).getImm();
  int Size = (int) Insn - Pos + 1;
  Inst.addOperand(MCOperand::CreateImm(SignExtend32<16>(Size)));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeExtSize(MCInst &Inst,
                                  unsigned Insn,
                                  uint64_t Address,
                                  const void *Decoder) {
  int Size = (int) Insn  + 1;
  Inst.addOperand(MCOperand::CreateImm(SignExtend32<16>(Size)));
  return MCDisassembler::Success;
}
