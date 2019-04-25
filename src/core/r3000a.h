/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#pragma once

#include <memory>

#include "core/psxbios.h"
#include "core/psxcounters.h"
#include "core/psxemulator.h"
#include "core/psxhle.h"
#include "core/psxmem.h"

namespace PCSX {

typedef union {
#if defined(__BIGENDIAN__)
    struct {
        uint8_t h3, h2, h, l;
    } b;
    struct {
        int8_t h3, h2, h, l;
    } sb;
    struct {
        uint16_t h, l;
    } w;
    struct {
        int16_t h, l;
    } sw;
#else
    struct {
        uint8_t l, h, h2, h3;
    } b;
    struct {
        uint16_t l, h;
    } w;
    struct {
        int8_t l, h, h2, h3;
    } sb;
    struct {
        int16_t l, h;
    } sw;
#endif
    uint32_t d;
    int32_t sd;
} PAIR;

typedef union {
    struct {
        uint32_t r0, at, v0, v1, a0, a1, a2, a3;
        uint32_t t0, t1, t2, t3, t4, t5, t6, t7;
        uint32_t s0, s1, s2, s3, s4, s5, s6, s7;
        uint32_t t8, t9, k0, k1, gp, sp, s8, ra;
        uint32_t lo, hi;
    } n;
    uint32_t r[34]; /* Lo, Hi in r[32] and r[33] */
    PAIR p[34];
} psxGPRRegs;

typedef union {
    struct {
        uint32_t Index, Random, EntryLo0, BPC, Context, BDA, PIDMask, DCIC;
        uint32_t BadVAddr, BDAM, EntryHi, BPCM, Status, Cause, EPC, PRid;
        uint32_t Config, LLAddr, WatchLO, WatchHI, XContext, Reserved1, Reserved2, Reserved3;
        uint32_t Reserved4, Reserved5, ECC, CacheErr, TagLo, TagHi, ErrorEPC, Reserved6;
    } n;
    uint32_t r[32];
} psxCP0Regs;

typedef struct {
    int16_t x, y;
} SVector2D;

typedef struct {
    int16_t z, unused;
} SVector2Dz;

typedef struct {
    int16_t x, y, z, unused;
} SVector3D;

typedef struct {
    int16_t x, y, z, unused;
} LVector3D;

typedef struct {
    uint8_t r, g, b, c;
} CBGR;

typedef struct {
    int16_t m11, m12, m13, m21, m22, m23, m31, m32, m33, unused;
} SMatrix3D;

typedef union {
    struct {
        SVector3D v0, v1, v2;
        CBGR rgb;
        int32_t otz;
        int32_t ir0, ir1, ir2, ir3;
        SVector2D sxy0, sxy1, sxy2, sxyp;
        SVector2Dz sz0, sz1, sz2, sz3;
        CBGR rgb0, rgb1, rgb2;
        int32_t reserved;
        int32_t mac0, mac1, mac2, mac3;
        uint32_t irgb, orgb;
        int32_t lzcs, lzcr;
    } n;
    uint32_t r[32];
    PAIR p[32];
} psxCP2Data;

typedef union {
    struct {
        SMatrix3D rMatrix;
        int32_t trX, trY, trZ;
        SMatrix3D lMatrix;
        int32_t rbk, gbk, bbk;
        SMatrix3D cMatrix;
        int32_t rfc, gfc, bfc;
        int32_t ofx, ofy;
        int32_t h;
        int32_t dqa, dqb;
        int32_t zsf3, zsf4;
        int32_t flag;
    } n;
    uint32_t r[32];
    PAIR p[32];
} psxCP2Ctrl;

enum {
    PSXINT_SIO = 0,
    PSXINT_CDR,
    PSXINT_CDREAD,
    PSXINT_GPUDMA,
    PSXINT_MDECOUTDMA,
    PSXINT_SPUDMA,
    PSXINT_GPUBUSY,
    PSXINT_MDECINDMA,
    PSXINT_GPUOTCDMA,
    PSXINT_CDRDMA,
    PSXINT_SPUASYNC,
    PSXINT_CDRDBUF,
    PSXINT_CDRLID,
    PSXINT_CDRPLAY
};

typedef struct {
    psxGPRRegs GPR;  /* General Purpose Registers */
    psxCP0Regs CP0;  /* Coprocessor0 Registers */
    psxCP2Data CP2D; /* Cop2 data registers */
    psxCP2Ctrl CP2C; /* Cop2 control registers */
    uint32_t pc;     /* Program counter */
    uint32_t code;   /* The instruction */
    uint32_t cycle;
    uint32_t interrupt;
    struct {
        uint32_t sCycle, cycle;
    } intCycle[32];
    uint8_t ICache_Addr[0x1000];
    uint8_t ICache_Code[0x1000];
    bool ICache_valid;
} psxRegisters;

// U64 and S64 are used to wrap long integer constants.
#define U64(val) val##ULL
#define S64(val) val##LL

#if defined(__BIGENDIAN__)

#define _i32(x) reinterpret_cast<int32_t *>(&x)[0]
#define _u32(x) reinterpret_cast<uint32_t *>(&x)[0]

#define _i16(x) reinterpret_cast<int16_t *>(&x)[1]
#define _u16(x) reinterpret_cast<uint16_t *>(&x)[1]

#define _i8(x) reinterpret_cast<int8_t *>(&x)[3]
#define _u8(x) reinterpret_cast<uint8_t *>(&x)[3]

#else

#define _i32(x) reinterpret_cast<int32_t *>(&x)[0]
#define _u32(x) reinterpret_cast<uint32_t *>(&x)[0]

#define _i16(x) reinterpret_cast<int16_t *>(&x)[0]
#define _u16(x) reinterpret_cast<uint16_t *>(&x)[0]

#define _i8(x) reinterpret_cast<int8_t *>(&x)[0]
#define _u8(x) reinterpret_cast<uint8_t *>(&x)[0]

#endif

/**** R3000A Instruction Macros ****/
#define _PC_ PCSX::g_emulator.m_psxCpu->m_psxRegs.pc  // The next PC to be executed

#define _fOp_(code) ((code >> 26))           // The opcode part of the instruction register
#define _fFunct_(code) ((code)&0x3F)         // The funct part of the instruction register
#define _fRd_(code) ((code >> 11) & 0x1F)    // The rd part of the instruction register
#define _fRt_(code) ((code >> 16) & 0x1F)    // The rt part of the instruction register
#define _fRs_(code) ((code >> 21) & 0x1F)    // The rs part of the instruction register
#define _fSa_(code) ((code >> 6) & 0x1F)     // The sa part of the instruction register
#define _fIm_(code) ((uint16_t)code)         // The immediate part of the instruction register
#define _fTarget_(code) (code & 0x03ffffff)  // The target part of the instruction register

#define _fImm_(code) ((int16_t)code)   // sign-extended immediate
#define _fImmU_(code) (code & 0xffff)  // zero-extended immediate
#define _fImmLU_(code) (code << 16)    // LUI

#define _Op_ _fOp_(PCSX::g_emulator.m_psxCpu->m_psxRegs.code)
#define _Funct_ _fFunct_(PCSX::g_emulator.m_psxCpu->m_psxRegs.code)
#define _Rd_ _fRd_(PCSX::g_emulator.m_psxCpu->m_psxRegs.code)
#define _Rt_ _fRt_(PCSX::g_emulator.m_psxCpu->m_psxRegs.code)
#define _Rs_ _fRs_(PCSX::g_emulator.m_psxCpu->m_psxRegs.code)
#define _Sa_ _fSa_(PCSX::g_emulator.m_psxCpu->m_psxRegs.code)
#define _Im_ _fIm_(PCSX::g_emulator.m_psxCpu->m_psxRegs.code)
#define _Target_ _fTarget_(PCSX::g_emulator.m_psxCpu->m_psxRegs.code)

#define _Imm_ _fImm_(PCSX::g_emulator.m_psxCpu->m_psxRegs.code)
#define _ImmU_ _fImmU_(PCSX::g_emulator.m_psxCpu->m_psxRegs.code)
#define _ImmLU_ _fImmLU_(PCSX::g_emulator.m_psxCpu->m_psxRegs.code)

#define _rRs_ PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.r[_Rs_]  // Rs register
#define _rRt_ PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.r[_Rt_]  // Rt register
#define _rRd_ PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.r[_Rd_]  // Rd register
#define _rSa_ PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.r[_Sa_]  // Sa register
#define _rFs_ PCSX::g_emulator.m_psxCpu->m_psxRegs.CP0.r[_Rd_]  // Fs register

#define _c2dRs_ PCSX::g_emulator.m_psxCpu->m_psxRegs.CP2D.r[_Rs_]  // Rs cop2 data register
#define _c2dRt_ PCSX::g_emulator.m_psxCpu->m_psxRegs.CP2D.r[_Rt_]  // Rt cop2 data register
#define _c2dRd_ PCSX::g_emulator.m_psxCpu->m_psxRegs.CP2D.r[_Rd_]  // Rd cop2 data register
#define _c2dSa_ PCSX::g_emulator.m_psxCpu->m_psxRegs.CP2D.r[_Sa_]  // Sa cop2 data register

#define _rHi_ PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.n.hi  // The HI register
#define _rLo_ PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.n.lo  // The LO register

#define _JumpTarget_ ((_Target_ * 4) + (_PC_ & 0xf0000000))  // Calculates the target during a jump instruction
#define _BranchTarget_ ((int16_t)_Im_ * 4 + _PC_)            // Calculates the target during a branch instruction

#define _SetLink(x) \
    PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.r[x] = _PC_ + 4;  // Sets the return address in the link register

class R3000Acpu {
  public:
    virtual ~R3000Acpu() {}
    virtual bool Init() { return false; }
    virtual void Reset() = 0;
    virtual void Execute() = 0;      /* executes up to a break */
    virtual void ExecuteBlock() = 0; /* executes up to a jump */
    virtual void Clear(uint32_t Addr, uint32_t Size) = 0;
    virtual void Shutdown() = 0;
    virtual bool Implemented() { return false; }

    const std::string &getName() { return m_name; }

  public:
    static int psxInit();
    void psxReset();
    void psxShutdown();
    void psxException(uint32_t code, uint32_t bd);
    void psxBranchTest();
    void psxExecuteBios();
    void psxJumpTest();

    psxRegisters m_psxRegs;

  protected:
    R3000Acpu(const std::string &name) : m_name(name) {}
    inline void InterceptConsole() {
        // Intercept puts and putchar, even if running the binary bios.
        if (m_psxRegs.pc == 0xa0) {
            switch (m_psxRegs.GPR.n.t1) {
                case 0x3e:
                    psxHLEt[1]();
                    break;
            }
        }

        if (m_psxRegs.pc == 0xb0) {
            switch (m_psxRegs.GPR.n.t1) {
                case 0x3d:
                case 0x3f:
                    psxHLEt[2]();
                    break;
            }
        }
    }

  public:
    /*
Formula One 2001
- Use old CPU cache code when the RAM location is
  updated with new code (affects in-game racing)

TODO:
- I-cache / D-cache swapping
- Isolate D-cache from RAM
*/

    inline uint32_t *Read_ICache(uint32_t pc, bool isolate) {
        uint32_t pc_bank, pc_offset, pc_cache;
        uint8_t *IAddr, *ICode;

        pc_bank = pc >> 24;
        pc_offset = pc & 0xffffff;
        pc_cache = pc & 0xfff;

        IAddr = m_psxRegs.ICache_Addr;
        ICode = m_psxRegs.ICache_Code;

        // clear I-cache
        if (!m_psxRegs.ICache_valid) {
            memset(m_psxRegs.ICache_Addr, 0xff, sizeof(m_psxRegs.ICache_Addr));
            memset(m_psxRegs.ICache_Code, 0xff, sizeof(m_psxRegs.ICache_Code));

            m_psxRegs.ICache_valid = true;
        }

        // uncached
        if (pc_bank >= 0xa0) return (uint32_t *)PSXM(pc);

        // cached - RAM
        if (pc_bank == 0x80 || pc_bank == 0x00) {
            if (SWAP_LE32(*(uint32_t *)(IAddr + pc_cache)) == pc_offset) {
                // Cache hit - return last opcode used
                return (uint32_t *)(ICode + pc_cache);
            } else {
                // Cache miss - addresses don't match
                // - default: 0xffffffff (not init)

                if (!isolate) {
                    // cache line is 4 bytes wide
                    pc_offset &= ~0xf;
                    pc_cache &= ~0xf;

                    // address line
                    *(uint32_t *)(IAddr + pc_cache + 0x0) = SWAP_LE32(pc_offset + 0x0);
                    *(uint32_t *)(IAddr + pc_cache + 0x4) = SWAP_LE32(pc_offset + 0x4);
                    *(uint32_t *)(IAddr + pc_cache + 0x8) = SWAP_LE32(pc_offset + 0x8);
                    *(uint32_t *)(IAddr + pc_cache + 0xc) = SWAP_LE32(pc_offset + 0xc);

                    // opcode line
                    pc_offset = pc & ~0xf;
                    *(uint32_t *)(ICode + pc_cache + 0x0) = psxMu32ref(pc_offset + 0x0);
                    *(uint32_t *)(ICode + pc_cache + 0x4) = psxMu32ref(pc_offset + 0x4);
                    *(uint32_t *)(ICode + pc_cache + 0x8) = psxMu32ref(pc_offset + 0x8);
                    *(uint32_t *)(ICode + pc_cache + 0xc) = psxMu32ref(pc_offset + 0xc);
                }

                // normal code
                return (uint32_t *)PSXM(pc);
            }
        }

        /*
        TODO: Probably should add cached BIOS
        */

        // default
        return (uint32_t *)PSXM(pc);
    }

  private:
    const std::string m_name;
};

/* The dynarec CPU will still call into the interpreted CPU for the delay slot checks. */
class InterpretedCPU : public R3000Acpu {
  public:
    InterpretedCPU() : R3000Acpu("Interpreted") {}
    virtual bool Implemented() final { return true; }
    virtual bool Init() override;
    virtual void Reset() override;
    virtual void Execute() override;
    virtual void ExecuteBlock() override;
    virtual void Clear(uint32_t Addr, uint32_t Size) override;
    virtual void Shutdown() override;

  protected:
    InterpretedCPU(const std::string &name) : R3000Acpu(name) {}

    static int psxTestLoadDelay(int reg, uint32_t tmp);
    void psxDelayTest(int reg, uint32_t bpc);
    void psxTestSWInts();

    static inline const uint32_t g_LWL_MASK[4] = {0xffffff, 0xffff, 0xff, 0};
    static inline const uint32_t g_LWL_SHIFT[4] = {24, 16, 8, 0};
    static inline const uint32_t g_LWR_MASK[4] = {0, 0xff000000, 0xffff0000, 0xffffff00};
    static inline const uint32_t g_LWR_SHIFT[4] = {0, 8, 16, 24};
    static inline const uint32_t g_SWL_MASK[4] = {0xffffff00, 0xffff0000, 0xff000000, 0};
    static inline const uint32_t g_SWL_SHIFT[4] = {24, 16, 8, 0};
    static inline const uint32_t g_SWR_MASK[4] = {0, 0xff, 0xffff, 0xffffff};
    static inline const uint32_t g_SWR_SHIFT[4] = {0, 8, 16, 24};

  private:
    typedef void (InterpretedCPU::*intFunc_t)();
    typedef const intFunc_t cIntFunc_t;

    int s_branch = 0;
    int s_branch2 = 0;
    uint32_t s_branchPC;

    cIntFunc_t *s_pPsxBSC = NULL;
    cIntFunc_t *s_pPsxSPC = NULL;
    cIntFunc_t *s_pPsxREG = NULL;
    cIntFunc_t *s_pPsxCP0 = NULL;
    cIntFunc_t *s_pPsxCP2 = NULL;
    cIntFunc_t *s_pPsxCP2BSC = NULL;

    void execI();
    void delayRead(int reg, uint32_t bpc);
    void delayWrite(int reg, uint32_t bpc);
    void delayReadWrite(int reg, uint32_t bpc);
    uint32_t psxBranchNoDelay();
    int psxDelayBranchExec(uint32_t tar);
    int psxDelayBranchTest(uint32_t tar1);
    void doBranch(uint32_t tar);

    void MTC0(int reg, uint32_t val);

    /* Arithmetic with immediate operand */
    void psxADDI();
    void psxADDIU();
    void psxANDI();
    void psxORI();
    void psxXORI();
    void psxSLTI();
    void psxSLTIU();

    /* Register arithmetic */
    void psxADD();
    void psxADDU();
    void psxSUB();
    void psxSUBU();
    void psxAND();
    void psxOR();
    void psxXOR();
    void psxNOR();
    void psxSLT();
    void psxSLTU();

    /* Register mult/div & Register trap logic */
    void psxDIV();
    void psxDIVU();
    void psxMULT();
    void psxMULTU();

    /* Register branch logic */
    void psxBGEZ();
    void psxBGEZAL();
    void psxBGTZ();
    void psxBLEZ();
    void psxBLTZ();
    void psxBLTZAL();

    /* Shift arithmetic with constant shift */
    void psxSLL();
    void psxSRA();
    void psxSRL();

    /* Shift arithmetic with variant register shift */
    void psxSLLV();
    void psxSRAV();
    void psxSRLV();

    /* Load higher 16 bits of the first word in GPR with imm */
    void psxLUI();

    /* Move from HI/LO to GPR */
    void psxMFHI();
    void psxMFLO();

    /* Move to GPR to HI/LO & Register jump */
    void psxMTHI();
    void psxMTLO();

    /* Special purpose instructions */
    void psxBREAK();
    void psxSYSCALL();
    void psxRFE();

    /* Register branch logic */
    void psxBEQ();
    void psxBNE();

    /* Jump to target */
    void psxJ();
    void psxJAL();

    /* Register jump */
    void psxJR();
    void psxJALR();

    /* Load and store for GPR */
    void psxLB();
    void psxLBU();
    void psxLH();
    void psxLHU();
    void psxLW();

  private:
    void psxLWL();
    void psxLWR();
    void psxSB();
    void psxSH();
    void psxSW();
    void psxSWL();
    void psxSWR();

    /* Moves between GPR and COPx */
    void psxMFC0();
    void psxCFC0();
    void psxMTC0();
    void psxCTC0();
    void psxMFC2();
    void psxCFC2();

    /* Misc */
    void psxNULL();
    void psxSPECIAL();
    void psxREGIMM();
    void psxCOP0();
    void psxBASIC();
    void psxHLE();

    static const intFunc_t s_psxBSC[64];
    static const intFunc_t s_psxSPC[64];
    static const intFunc_t s_psxREG[32];
    static const intFunc_t s_psxCP0[32];
};

class Cpus {
  public:
    static std::unique_ptr<R3000Acpu> Interpreted();
};

}  // namespace PCSX
