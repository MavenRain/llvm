//===-- SystemZMCCodeEmitter.cpp - Convert SystemZ code to machine code ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the SystemZMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/SystemZMCTargetDesc.h"
#include "MCTargetDesc/SystemZMCFixups.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"

using namespace llvm;

#define DEBUG_TYPE "mccodeemitter"

namespace {
class SystemZMCCodeEmitter : public MCCodeEmitter {
  const MCInstrInfo &MCII;
  MCContext &Ctx;

public:
  SystemZMCCodeEmitter(const MCInstrInfo &mcii, MCContext &ctx)
    : MCII(mcii), Ctx(ctx) {
  }

  ~SystemZMCCodeEmitter() override {}

  // OVerride MCCodeEmitter.
  void encodeInstruction(const MCInst &MI, raw_ostream &OS,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

private:
  // Automatically generated by TableGen.
  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  // Called by the TableGen code to get the binary encoding of operand
  // MO in MI.  Fixups is the list of fixups against MI.
  uint64_t getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  // Called by the TableGen code to get the binary encoding of an address.
  // The index or length, if any, is encoded first, followed by the base,
  // followed by the displacement.  In a 20-bit displacement,
  // the low 12 bits are encoded before the high 8 bits.
  uint64_t getBDAddr12Encoding(const MCInst &MI, unsigned OpNum,
                               SmallVectorImpl<MCFixup> &Fixups,
                               const MCSubtargetInfo &STI) const;
  uint64_t getBDAddr20Encoding(const MCInst &MI, unsigned OpNum,
                               SmallVectorImpl<MCFixup> &Fixups,
                               const MCSubtargetInfo &STI) const;
  uint64_t getBDXAddr12Encoding(const MCInst &MI, unsigned OpNum,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const;
  uint64_t getBDXAddr20Encoding(const MCInst &MI, unsigned OpNum,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const;
  uint64_t getBDLAddr12Len8Encoding(const MCInst &MI, unsigned OpNum,
                                    SmallVectorImpl<MCFixup> &Fixups,
                                    const MCSubtargetInfo &STI) const;
  uint64_t getBDVAddr12Encoding(const MCInst &MI, unsigned OpNum,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const;

  // Operand OpNum of MI needs a PC-relative fixup of kind Kind at
  // Offset bytes from the start of MI.  Add the fixup to Fixups
  // and return the in-place addend, which since we're a RELA target
  // is always 0.  If AllowTLS is true and optional operand OpNum + 1
  // is present, also emit a TLS call fixup for it.
  uint64_t getPCRelEncoding(const MCInst &MI, unsigned OpNum,
                            SmallVectorImpl<MCFixup> &Fixups,
                            unsigned Kind, int64_t Offset,
                            bool AllowTLS) const;

  uint64_t getPC16DBLEncoding(const MCInst &MI, unsigned OpNum,
                              SmallVectorImpl<MCFixup> &Fixups,
                              const MCSubtargetInfo &STI) const {
    return getPCRelEncoding(MI, OpNum, Fixups,
                            SystemZ::FK_390_PC16DBL, 2, false);
  }
  uint64_t getPC32DBLEncoding(const MCInst &MI, unsigned OpNum,
                              SmallVectorImpl<MCFixup> &Fixups,
                              const MCSubtargetInfo &STI) const {
    return getPCRelEncoding(MI, OpNum, Fixups,
                            SystemZ::FK_390_PC32DBL, 2, false);
  }
  uint64_t getPC16DBLTLSEncoding(const MCInst &MI, unsigned OpNum,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const {
    return getPCRelEncoding(MI, OpNum, Fixups,
                            SystemZ::FK_390_PC16DBL, 2, true);
  }
  uint64_t getPC32DBLTLSEncoding(const MCInst &MI, unsigned OpNum,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const {
    return getPCRelEncoding(MI, OpNum, Fixups,
                            SystemZ::FK_390_PC32DBL, 2, true);
  }
};
} // end anonymous namespace

MCCodeEmitter *llvm::createSystemZMCCodeEmitter(const MCInstrInfo &MCII,
                                                const MCRegisterInfo &MRI,
                                                MCContext &Ctx) {
  return new SystemZMCCodeEmitter(MCII, Ctx);
}

void SystemZMCCodeEmitter::
encodeInstruction(const MCInst &MI, raw_ostream &OS,
                  SmallVectorImpl<MCFixup> &Fixups,
                  const MCSubtargetInfo &STI) const {
  uint64_t Bits = getBinaryCodeForInstr(MI, Fixups, STI);
  unsigned Size = MCII.get(MI.getOpcode()).getSize();
  // Big-endian insertion of Size bytes.
  unsigned ShiftValue = (Size * 8) - 8;
  for (unsigned I = 0; I != Size; ++I) {
    OS << uint8_t(Bits >> ShiftValue);
    ShiftValue -= 8;
  }
}

uint64_t SystemZMCCodeEmitter::
getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                  SmallVectorImpl<MCFixup> &Fixups,
                  const MCSubtargetInfo &STI) const {
  if (MO.isReg())
    return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());
  if (MO.isImm())
    return static_cast<uint64_t>(MO.getImm());
  llvm_unreachable("Unexpected operand type!");
}

uint64_t SystemZMCCodeEmitter::
getBDAddr12Encoding(const MCInst &MI, unsigned OpNum,
                    SmallVectorImpl<MCFixup> &Fixups,
                    const MCSubtargetInfo &STI) const {
  uint64_t Base = getMachineOpValue(MI, MI.getOperand(OpNum), Fixups, STI);
  uint64_t Disp = getMachineOpValue(MI, MI.getOperand(OpNum + 1), Fixups, STI);
  assert(isUInt<4>(Base) && isUInt<12>(Disp));
  return (Base << 12) | Disp;
}

uint64_t SystemZMCCodeEmitter::
getBDAddr20Encoding(const MCInst &MI, unsigned OpNum,
                    SmallVectorImpl<MCFixup> &Fixups,
                    const MCSubtargetInfo &STI) const {
  uint64_t Base = getMachineOpValue(MI, MI.getOperand(OpNum), Fixups, STI);
  uint64_t Disp = getMachineOpValue(MI, MI.getOperand(OpNum + 1), Fixups, STI);
  assert(isUInt<4>(Base) && isInt<20>(Disp));
  return (Base << 20) | ((Disp & 0xfff) << 8) | ((Disp & 0xff000) >> 12);
}

uint64_t SystemZMCCodeEmitter::
getBDXAddr12Encoding(const MCInst &MI, unsigned OpNum,
                     SmallVectorImpl<MCFixup> &Fixups,
                     const MCSubtargetInfo &STI) const {
  uint64_t Base = getMachineOpValue(MI, MI.getOperand(OpNum), Fixups, STI);
  uint64_t Disp = getMachineOpValue(MI, MI.getOperand(OpNum + 1), Fixups, STI);
  uint64_t Index = getMachineOpValue(MI, MI.getOperand(OpNum + 2), Fixups, STI);
  assert(isUInt<4>(Base) && isUInt<12>(Disp) && isUInt<4>(Index));
  return (Index << 16) | (Base << 12) | Disp;
}

uint64_t SystemZMCCodeEmitter::
getBDXAddr20Encoding(const MCInst &MI, unsigned OpNum,
                     SmallVectorImpl<MCFixup> &Fixups,
                     const MCSubtargetInfo &STI) const {
  uint64_t Base = getMachineOpValue(MI, MI.getOperand(OpNum), Fixups, STI);
  uint64_t Disp = getMachineOpValue(MI, MI.getOperand(OpNum + 1), Fixups, STI);
  uint64_t Index = getMachineOpValue(MI, MI.getOperand(OpNum + 2), Fixups, STI);
  assert(isUInt<4>(Base) && isInt<20>(Disp) && isUInt<4>(Index));
  return (Index << 24) | (Base << 20) | ((Disp & 0xfff) << 8)
    | ((Disp & 0xff000) >> 12);
}

uint64_t SystemZMCCodeEmitter::
getBDLAddr12Len8Encoding(const MCInst &MI, unsigned OpNum,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const {
  uint64_t Base = getMachineOpValue(MI, MI.getOperand(OpNum), Fixups, STI);
  uint64_t Disp = getMachineOpValue(MI, MI.getOperand(OpNum + 1), Fixups, STI);
  uint64_t Len  = getMachineOpValue(MI, MI.getOperand(OpNum + 2), Fixups, STI) - 1;
  assert(isUInt<4>(Base) && isUInt<12>(Disp) && isUInt<8>(Len));
  return (Len << 16) | (Base << 12) | Disp;
}

uint64_t SystemZMCCodeEmitter::
getBDVAddr12Encoding(const MCInst &MI, unsigned OpNum,
                     SmallVectorImpl<MCFixup> &Fixups,
                     const MCSubtargetInfo &STI) const {
  uint64_t Base = getMachineOpValue(MI, MI.getOperand(OpNum), Fixups, STI);
  uint64_t Disp = getMachineOpValue(MI, MI.getOperand(OpNum + 1), Fixups, STI);
  uint64_t Index = getMachineOpValue(MI, MI.getOperand(OpNum + 2), Fixups, STI);
  assert(isUInt<4>(Base) && isUInt<12>(Disp) && isUInt<5>(Index));
  return (Index << 16) | (Base << 12) | Disp;
}

uint64_t
SystemZMCCodeEmitter::getPCRelEncoding(const MCInst &MI, unsigned OpNum,
                                       SmallVectorImpl<MCFixup> &Fixups,
                                       unsigned Kind, int64_t Offset,
                                       bool AllowTLS) const {
  const MCOperand &MO = MI.getOperand(OpNum);
  const MCExpr *Expr;
  if (MO.isImm())
    Expr = MCConstantExpr::Create(MO.getImm() + Offset, Ctx);
  else {
    Expr = MO.getExpr();
    if (Offset) {
      // The operand value is relative to the start of MI, but the fixup
      // is relative to the operand field itself, which is Offset bytes
      // into MI.  Add Offset to the relocation value to cancel out
      // this difference.
      const MCExpr *OffsetExpr = MCConstantExpr::Create(Offset, Ctx);
      Expr = MCBinaryExpr::CreateAdd(Expr, OffsetExpr, Ctx);
    }
  }
  Fixups.push_back(MCFixup::create(Offset, Expr, (MCFixupKind)Kind));

  // Output the fixup for the TLS marker if present.
  if (AllowTLS && OpNum + 1 < MI.getNumOperands()) {
    const MCOperand &MOTLS = MI.getOperand(OpNum + 1);
    Fixups.push_back(MCFixup::create(0, MOTLS.getExpr(),
                                     (MCFixupKind)SystemZ::FK_390_TLS_CALL));
  }
  return 0;
}

#include "SystemZGenMCCodeEmitter.inc"
