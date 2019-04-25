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

/*
 * PSX assembly interpreter.
 */

#include "core/psxemulator.h"
#include "core/psxhle.h"
#include "core/r3000a.h"

void PCSX::InterpretedCPU::delayRead(int reg, uint32_t bpc) {
    uint32_t rold, rnew;

    //  PCSX::g_system->printf("delayRead at %x!\n", PCSX::g_emulator.m_psxCpu->m_psxRegs.pc);

    rold = PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.r[reg];
    cIntFunc_t func = s_pPsxBSC[PCSX::g_emulator.m_psxCpu->m_psxRegs.code >> 26];
    (*this.*func)();  // branch delay load
    rnew = PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.r[reg];

    PCSX::g_emulator.m_psxCpu->m_psxRegs.pc = bpc;

    s_branch = 0;

    PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.r[reg] = rold;
    execI();  // first branch opcode
    PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.r[reg] = rnew;

    PCSX::g_emulator.m_psxCpu->psxBranchTest();
}

void PCSX::InterpretedCPU::delayWrite(int reg, uint32_t bpc) {
    /*  PCSX::g_system->printf("delayWrite at %x!\n", PCSX::g_emulator.m_psxCpu->m_psxRegs.pc);

            PCSX::g_system->printf("%s\n", disR3000AF(PCSX::g_emulator.m_psxCpu->m_psxRegs.code,
       PCSX::g_emulator.m_psxCpu->m_psxRegs.pc-4)); PCSX::g_system->printf("%s\n", disR3000AF(PSXMu32(bpc), bpc));*/

    // no changes from normal behavior
    cIntFunc_t func = s_pPsxBSC[PCSX::g_emulator.m_psxCpu->m_psxRegs.code >> 26];
    (*this.*func)();

    s_branch = 0;
    PCSX::g_emulator.m_psxCpu->m_psxRegs.pc = bpc;

    PCSX::g_emulator.m_psxCpu->psxBranchTest();
}

void PCSX::InterpretedCPU::delayReadWrite(int reg, uint32_t bpc) {
    //  PCSX::g_system->printf("delayReadWrite at %x!\n", PCSX::g_emulator.m_psxCpu->m_psxRegs.pc);

    // the branch delay load is skipped

    s_branch = 0;
    PCSX::g_emulator.m_psxCpu->m_psxRegs.pc = bpc;

    PCSX::g_emulator.m_psxCpu->psxBranchTest();
}

// this defines shall be used with the tmp
// of the next func (instead of _Funct_...)
#define _tFunct_ ((tmp)&0x3F)       // The funct part of the instruction register
#define _tRd_ ((tmp >> 11) & 0x1F)  // The rd part of the instruction register
#define _tRt_ ((tmp >> 16) & 0x1F)  // The rt part of the instruction register
#define _tRs_ ((tmp >> 21) & 0x1F)  // The rs part of the instruction register
#define _tSa_ ((tmp >> 6) & 0x1F)   // The sa part of the instruction register

int PCSX::InterpretedCPU::psxTestLoadDelay(int reg, uint32_t tmp) {
    if (tmp == 0) return 0;  // NOP
    switch (tmp >> 26) {
        case 0x00:  // SPECIAL
            switch (_tFunct_) {
                case 0x00:  // SLL
                case 0x02:
                case 0x03:  // SRL/SRA
                    if (_tRd_ == reg && _tRt_ == reg)
                        return 1;
                    else if (_tRt_ == reg)
                        return 2;
                    else if (_tRd_ == reg)
                        return 3;
                    break;

                case 0x08:  // JR
                    if (_tRs_ == reg) return 2;
                    break;
                case 0x09:  // JALR
                    if (_tRd_ == reg && _tRs_ == reg)
                        return 1;
                    else if (_tRs_ == reg)
                        return 2;
                    else if (_tRd_ == reg)
                        return 3;
                    break;

                    // SYSCALL/BREAK just a break;

                case 0x20:
                case 0x21:
                case 0x22:
                case 0x23:
                case 0x24:
                case 0x25:
                case 0x26:
                case 0x27:
                case 0x2a:
                case 0x2b:  // ADD/ADDU...
                case 0x04:
                case 0x06:
                case 0x07:  // SLLV...
                    if (_tRd_ == reg && (_tRt_ == reg || _tRs_ == reg))
                        return 1;
                    else if (_tRt_ == reg || _tRs_ == reg)
                        return 2;
                    else if (_tRd_ == reg)
                        return 3;
                    break;

                case 0x10:
                case 0x12:  // MFHI/MFLO
                    if (_tRd_ == reg) return 3;
                    break;
                case 0x11:
                case 0x13:  // MTHI/MTLO
                    if (_tRs_ == reg) return 2;
                    break;

                case 0x18:
                case 0x19:
                case 0x1a:
                case 0x1b:  // MULT/DIV...
                    if (_tRt_ == reg || _tRs_ == reg) return 2;
                    break;
            }
            break;

        case 0x01:  // REGIMM
            switch (_tRt_) {
                case 0x00:
                case 0x01:
                case 0x10:
                case 0x11:  // BLTZ/BGEZ...
                    // Xenogears - lbu v0 / beq v0
                    // - no load delay (fixes battle loading)
                    break;

                    if (_tRs_ == reg) return 2;
                    break;
            }
            break;

        // J would be just a break;
        case 0x03:  // JAL
            if (31 == reg) return 3;
            break;

        case 0x04:
        case 0x05:  // BEQ/BNE
            // Xenogears - lbu v0 / beq v0
            // - no load delay (fixes battle loading)
            break;

            if (_tRs_ == reg || _tRt_ == reg) return 2;
            break;

        case 0x06:
        case 0x07:  // BLEZ/BGTZ
            // Xenogears - lbu v0 / beq v0
            // - no load delay (fixes battle loading)
            break;

            if (_tRs_ == reg) return 2;
            break;

        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b:
        case 0x0c:
        case 0x0d:
        case 0x0e:  // ADDI/ADDIU...
            if (_tRt_ == reg && _tRs_ == reg)
                return 1;
            else if (_tRs_ == reg)
                return 2;
            else if (_tRt_ == reg)
                return 3;
            break;

        case 0x0f:  // LUI
            if (_tRt_ == reg) return 3;
            break;

        case 0x10:  // COP0
            switch (_tFunct_) {
                case 0x00:  // MFC0
                    if (_tRt_ == reg) return 3;
                    break;
                case 0x02:  // CFC0
                    if (_tRt_ == reg) return 3;
                    break;
                case 0x04:  // MTC0
                    if (_tRt_ == reg) return 2;
                    break;
                case 0x06:  // CTC0
                    if (_tRt_ == reg) return 2;
                    break;
                    // RFE just a break;
            }
            break;

        case 0x12:  // COP2
            switch (_tFunct_) {
                case 0x00:
                    switch (_tRs_) {
                        case 0x00:  // MFC2
                            if (_tRt_ == reg) return 3;
                            break;
                        case 0x02:  // CFC2
                            if (_tRt_ == reg) return 3;
                            break;
                        case 0x04:  // MTC2
                            if (_tRt_ == reg) return 2;
                            break;
                        case 0x06:  // CTC2
                            if (_tRt_ == reg) return 2;
                            break;
                    }
                    break;
                    // RTPS... break;
            }
            break;

        case 0x22:
        case 0x26:  // LWL/LWR
            if (_tRt_ == reg)
                return 3;
            else if (_tRs_ == reg)
                return 2;
            break;

        case 0x20:
        case 0x21:
        case 0x23:
        case 0x24:
        case 0x25:  // LB/LH/LW/LBU/LHU
            if (_tRt_ == reg && _tRs_ == reg)
                return 1;
            else if (_tRs_ == reg)
                return 2;
            else if (_tRt_ == reg)
                return 3;
            break;

        case 0x28:
        case 0x29:
        case 0x2a:
        case 0x2b:
        case 0x2e:  // SB/SH/SWL/SW/SWR
            if (_tRt_ == reg || _tRs_ == reg) return 2;
            break;

        case 0x32:
        case 0x3a:  // LWC2/SWC2
            if (_tRs_ == reg) return 2;
            break;
    }

    return 0;
}

void PCSX::InterpretedCPU::psxDelayTest(int reg, uint32_t bpc) {
    uint32_t *code;
    uint32_t tmp;

    // Don't execute yet - just peek
    code = PCSX::g_emulator.m_psxCpu->Read_ICache(bpc, true);

    tmp = ((code == NULL) ? 0 : SWAP_LE32(*code));
    s_branch = 1;

    switch (psxTestLoadDelay(reg, tmp)) {
        case 1:
            delayReadWrite(reg, bpc);
            return;
        case 2:
            delayRead(reg, bpc);
            return;
        case 3:
            delayWrite(reg, bpc);
            return;
    }
    cIntFunc_t func = s_pPsxBSC[m_psxRegs.code >> 26];
    (*this.*func)();

    s_branch = 0;
    m_psxRegs.pc = bpc;

    psxBranchTest();
}

uint32_t PCSX::InterpretedCPU::psxBranchNoDelay() {
    uint32_t *code;
    uint32_t temp;

    code = PCSX::g_emulator.m_psxCpu->Read_ICache(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc, true);
    PCSX::g_emulator.m_psxCpu->m_psxRegs.code = ((code == NULL) ? 0 : SWAP_LE32(*code));
    switch (_Op_) {
        case 0x00:  // SPECIAL
            switch (_Funct_) {
                case 0x08:  // JR
                    return _u32(_rRs_);
                case 0x09:  // JALR
                    temp = _u32(_rRs_);
                    if (_Rd_) {
                        _SetLink(_Rd_);
                    }
                    return temp;
            }
            break;
        case 0x01:  // REGIMM
            switch (_Rt_) {
                case 0x00:  // BLTZ
                    if (_i32(_rRs_) < 0) return _BranchTarget_;
                    break;
                case 0x01:  // BGEZ
                    if (_i32(_rRs_) >= 0) return _BranchTarget_;
                    break;
                case 0x08:  // BLTZAL
                    if (_i32(_rRs_) < 0) {
                        _SetLink(31);
                        return _BranchTarget_;
                    }
                    break;
                case 0x09:  // BGEZAL
                    if (_i32(_rRs_) >= 0) {
                        _SetLink(31);
                        return _BranchTarget_;
                    }
                    break;
            }
            break;
        case 0x02:  // J
            return _JumpTarget_;
        case 0x03:  // JAL
            _SetLink(31);
            return _JumpTarget_;
        case 0x04:  // BEQ
            if (_i32(_rRs_) == _i32(_rRt_)) return _BranchTarget_;
            break;
        case 0x05:  // BNE
            if (_i32(_rRs_) != _i32(_rRt_)) return _BranchTarget_;
            break;
        case 0x06:  // BLEZ
            if (_i32(_rRs_) <= 0) return _BranchTarget_;
            break;
        case 0x07:  // BGTZ
            if (_i32(_rRs_) > 0) return _BranchTarget_;
            break;
    }

    return (uint32_t)-1;
}

int PCSX::InterpretedCPU::psxDelayBranchExec(uint32_t tar) {
    execI();

    s_branch = 0;
    PCSX::g_emulator.m_psxCpu->m_psxRegs.pc = tar;
    PCSX::g_emulator.m_psxCpu->m_psxRegs.cycle += PCSX::Emulator::BIAS;
    PCSX::g_emulator.m_psxCpu->psxBranchTest();
    return 1;
}

int PCSX::InterpretedCPU::psxDelayBranchTest(uint32_t tar1) {
    uint32_t tar2, tmp1, tmp2;

    tar2 = psxBranchNoDelay();
    if (tar2 == (uint32_t)-1) return 0;

    /*
     * Branch in delay slot:
     * - execute 1 instruction at tar1
     * - jump to tar2 (target of branch in delay slot; this branch
     *   has no normal delay slot, instruction at tar1 was fetched instead)
     */
    PCSX::g_emulator.m_psxCpu->m_psxRegs.pc = tar1;
    tmp1 = psxBranchNoDelay();
    if (tmp1 == (uint32_t)-1) {
        return psxDelayBranchExec(tar2);
    }
    PCSX::g_emulator.m_psxCpu->m_psxRegs.cycle += PCSX::Emulator::BIAS;

    /*
     * Got a branch at tar1:
     * - execute 1 instruction at tar2
     * - jump to target of that branch (tmp1)
     */
    PCSX::g_emulator.m_psxCpu->m_psxRegs.pc = tar2;
    tmp2 = psxBranchNoDelay();
    if (tmp2 == (uint32_t)-1) {
        return psxDelayBranchExec(tmp1);
    }
    PCSX::g_emulator.m_psxCpu->m_psxRegs.cycle += PCSX::Emulator::BIAS;

    /*
     * Got a branch at tar2:
     * - execute 1 instruction at tmp1
     * - jump to target of that branch (tmp2)
     */
    PCSX::g_emulator.m_psxCpu->m_psxRegs.pc = tmp1;
    return psxDelayBranchExec(tmp2);
}

inline void PCSX::InterpretedCPU::doBranch(uint32_t tar) {
    uint32_t *code;
    uint32_t tmp;

    s_branch2 = s_branch = 1;
    s_branchPC = tar;

    // notaz: check for branch in delay slot
    if (psxDelayBranchTest(tar)) return;

    // branch delay slot
    code = PCSX::g_emulator.m_psxCpu->Read_ICache(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc, true);

    PCSX::g_emulator.m_psxCpu->m_psxRegs.code = ((code == NULL) ? 0 : SWAP_LE32(*code));

    PCSX::g_emulator.m_psxCpu->m_psxRegs.pc += 4;
    PCSX::g_emulator.m_psxCpu->m_psxRegs.cycle += PCSX::Emulator::BIAS;

    // check for load delay
    tmp = PCSX::g_emulator.m_psxCpu->m_psxRegs.code >> 26;
    switch (tmp) {
        case 0x10:  // COP0
            switch (_Rs_) {
                case 0x00:  // MFC0
                case 0x02:  // CFC0
                    psxDelayTest(_Rt_, s_branchPC);
                    return;
            }
            break;
        case 0x12:  // COP2
            switch (_Funct_) {
                case 0x00:
                    switch (_Rs_) {
                        case 0x00:  // MFC2
                        case 0x02:  // CFC2
                            psxDelayTest(_Rt_, s_branchPC);
                            return;
                    }
                    break;
            }
            break;
        case 0x32:  // LWC2
            psxDelayTest(_Rt_, s_branchPC);
            return;
        default:
            if (tmp >= 0x20 && tmp <= 0x26) {  // LB/LH/LWL/LW/LBU/LHU/LWR
                psxDelayTest(_Rt_, s_branchPC);
                return;
            }
            break;
    }

    cIntFunc_t func = s_pPsxBSC[PCSX::g_emulator.m_psxCpu->m_psxRegs.code >> 26];
    (*this.*func)();

    s_branch = 0;
    PCSX::g_emulator.m_psxCpu->m_psxRegs.pc = s_branchPC;

    PCSX::g_emulator.m_psxCpu->psxBranchTest();
}

/*********************************************************
 * Arithmetic with immediate operand                      *
 * Format:  OP rt, rs, immediate                          *
 *********************************************************/
void PCSX::InterpretedCPU::psxADDI() {
    if (!_Rt_) return;
    _rRt_ = _u32(_rRs_) + _Imm_;
}  // Rt = Rs + Im      (Exception on Integer Overflow)
void PCSX::InterpretedCPU::psxADDIU() {
    if (!_Rt_) return;
    _rRt_ = _u32(_rRs_) + _Imm_;
}  // Rt = Rs + Im
void PCSX::InterpretedCPU::psxANDI() {
    if (!_Rt_) return;
    _rRt_ = _u32(_rRs_) & _ImmU_;
}  // Rt = Rs And Im
void PCSX::InterpretedCPU::psxORI() {
    if (!_Rt_) return;
    _rRt_ = _u32(_rRs_) | _ImmU_;
}  // Rt = Rs Or  Im
void PCSX::InterpretedCPU::psxXORI() {
    if (!_Rt_) return;
    _rRt_ = _u32(_rRs_) ^ _ImmU_;
}  // Rt = Rs Xor Im
void PCSX::InterpretedCPU::psxSLTI() {
    if (!_Rt_) return;
    _rRt_ = _i32(_rRs_) < _Imm_;
}  // Rt = Rs < Im              (Signed)
void PCSX::InterpretedCPU::psxSLTIU() {
    if (!_Rt_) return;
    _rRt_ = _u32(_rRs_) < ((uint32_t)_Imm_);
}  // Rt = Rs < Im              (Unsigned)

/*********************************************************
 * Register arithmetic                                    *
 * Format:  OP rd, rs, rt                                 *
 *********************************************************/
void PCSX::InterpretedCPU::psxADD() {
    if (!_Rd_) return;
    _rRd_ = _u32(_rRs_) + _u32(_rRt_);
}  // Rd = Rs + Rt              (Exception on Integer Overflow)
void PCSX::InterpretedCPU::psxADDU() {
    if (!_Rd_) return;
    _rRd_ = _u32(_rRs_) + _u32(_rRt_);
}  // Rd = Rs + Rt
void PCSX::InterpretedCPU::psxSUB() {
    if (!_Rd_) return;
    _rRd_ = _u32(_rRs_) - _u32(_rRt_);
}  // Rd = Rs - Rt              (Exception on Integer Overflow)
void PCSX::InterpretedCPU::psxSUBU() {
    if (!_Rd_) return;
    _rRd_ = _u32(_rRs_) - _u32(_rRt_);
}  // Rd = Rs - Rt
void PCSX::InterpretedCPU::psxAND() {
    if (!_Rd_) return;
    _rRd_ = _u32(_rRs_) & _u32(_rRt_);
}  // Rd = Rs And Rt
void PCSX::InterpretedCPU::psxOR() {
    if (!_Rd_) return;
    _rRd_ = _u32(_rRs_) | _u32(_rRt_);
}  // Rd = Rs Or  Rt
void PCSX::InterpretedCPU::psxXOR() {
    if (!_Rd_) return;
    _rRd_ = _u32(_rRs_) ^ _u32(_rRt_);
}  // Rd = Rs Xor Rt
void PCSX::InterpretedCPU::psxNOR() {
    if (!_Rd_) return;
    _rRd_ = ~(_u32(_rRs_) | _u32(_rRt_));
}  // Rd = Rs Nor Rt
void PCSX::InterpretedCPU::psxSLT() {
    if (!_Rd_) return;
    _rRd_ = _i32(_rRs_) < _i32(_rRt_);
}  // Rd = Rs < Rt              (Signed)
void PCSX::InterpretedCPU::psxSLTU() {
    if (!_Rd_) return;
    _rRd_ = _u32(_rRs_) < _u32(_rRt_);
}  // Rd = Rs < Rt              (Unsigned)

/*********************************************************
 * Register mult/div & Register trap logic                *
 * Format:  OP rs, rt                                     *
 *********************************************************/
void PCSX::InterpretedCPU::psxDIV() {
    if (!_i32(_rRt_)) {
        if (_i32(_rRs_) & 0x80000000) {
            _i32(_rLo_) = 1;
        } else {
            _i32(_rLo_) = 0xFFFFFFFF;
            _i32(_rHi_) = _i32(_rRs_);
        }
    } else if (_i32(_rRs_) == 0x80000000 && _i32(_rRt_) == 0xFFFFFFFF) {
        _i32(_rLo_) = 0x80000000;
        _i32(_rHi_) = 0;
    } else {
        _i32(_rLo_) = _i32(_rRs_) / _i32(_rRt_);
        _i32(_rHi_) = _i32(_rRs_) % _i32(_rRt_);
    }
}

void PCSX::InterpretedCPU::psxDIVU() {
    if (_rRt_ != 0) {
        _rLo_ = _rRs_ / _rRt_;
        _rHi_ = _rRs_ % _rRt_;
    } else {
        _rLo_ = 0xffffffff;
        _rHi_ = _rRs_;
    }
}

void PCSX::InterpretedCPU::psxMULT() {
    uint64_t res = (int64_t)((int64_t)_i32(_rRs_) * (int64_t)_i32(_rRt_));

    PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.n.lo = (uint32_t)(res & 0xffffffff);
    PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.n.hi = (uint32_t)((res >> 32) & 0xffffffff);
}

void PCSX::InterpretedCPU::psxMULTU() {
    uint64_t res = (uint64_t)((uint64_t)_u32(_rRs_) * (uint64_t)_u32(_rRt_));

    PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.n.lo = (uint32_t)(res & 0xffffffff);
    PCSX::g_emulator.m_psxCpu->m_psxRegs.GPR.n.hi = (uint32_t)((res >> 32) & 0xffffffff);
}

/*********************************************************
 * Register branch logic                                  *
 * Format:  OP rs, offset                                 *
 *********************************************************/
#define RepZBranchi32(op) \
    if (_i32(_rRs_) op 0) doBranch(_BranchTarget_);
#define RepZBranchLinki32(op)     \
    if (_i32(_rRs_) op 0) {       \
        _SetLink(31);             \
        doBranch(_BranchTarget_); \
    }

void PCSX::InterpretedCPU::psxBGEZ() { RepZBranchi32(>=) }        // Branch if Rs >= 0
void PCSX::InterpretedCPU::psxBGEZAL() { RepZBranchLinki32(>=) }  // Branch if Rs >= 0 and link
void PCSX::InterpretedCPU::psxBGTZ() { RepZBranchi32(>) }         // Branch if Rs >  0
void PCSX::InterpretedCPU::psxBLEZ() { RepZBranchi32(<=) }        // Branch if Rs <= 0
void PCSX::InterpretedCPU::psxBLTZ() { RepZBranchi32(<) }         // Branch if Rs <  0
void PCSX::InterpretedCPU::psxBLTZAL() { RepZBranchLinki32(<) }   // Branch if Rs <  0 and link

/*********************************************************
 * Shift arithmetic with constant shift                   *
 * Format:  OP rd, rt, sa                                 *
 *********************************************************/
void PCSX::InterpretedCPU::psxSLL() {
    if (!_Rd_) return;
    _u32(_rRd_) = _u32(_rRt_) << _Sa_;
}  // Rd = Rt << sa
void PCSX::InterpretedCPU::psxSRA() {
    if (!_Rd_) return;
    _i32(_rRd_) = _i32(_rRt_) >> _Sa_;
}  // Rd = Rt >> sa (arithmetic)
void PCSX::InterpretedCPU::psxSRL() {
    if (!_Rd_) return;
    _u32(_rRd_) = _u32(_rRt_) >> _Sa_;
}  // Rd = Rt >> sa (logical)

/*********************************************************
 * Shift arithmetic with variant register shift           *
 * Format:  OP rd, rt, rs                                 *
 *********************************************************/
void PCSX::InterpretedCPU::psxSLLV() {
    if (!_Rd_) return;
    _u32(_rRd_) = _u32(_rRt_) << _u32(_rRs_);
}  // Rd = Rt << rs
void PCSX::InterpretedCPU::psxSRAV() {
    if (!_Rd_) return;
    _i32(_rRd_) = _i32(_rRt_) >> _u32(_rRs_);
}  // Rd = Rt >> rs (arithmetic)
void PCSX::InterpretedCPU::psxSRLV() {
    if (!_Rd_) return;
    _u32(_rRd_) = _u32(_rRt_) >> _u32(_rRs_);
}  // Rd = Rt >> rs (logical)

/*********************************************************
 * Load higher 16 bits of the first word in GPR with imm  *
 * Format:  OP rt, immediate                              *
 *********************************************************/
void PCSX::InterpretedCPU::psxLUI() {
    if (!_Rt_) return;
    _u32(_rRt_) = _ImmLU_;
}  // Upper halfword of Rt = Im

/*********************************************************
 * Move from HI/LO to GPR                                 *
 * Format:  OP rd                                         *
 *********************************************************/
void PCSX::InterpretedCPU::psxMFHI() {
    if (!_Rd_) return;
    _rRd_ = _rHi_;
}  // Rd = Hi
void PCSX::InterpretedCPU::psxMFLO() {
    if (!_Rd_) return;
    _rRd_ = _rLo_;
}  // Rd = Lo

/*********************************************************
 * Move to GPR to HI/LO & Register jump                   *
 * Format:  OP rs                                         *
 *********************************************************/
void PCSX::InterpretedCPU::psxMTHI() { _rHi_ = _rRs_; }  // Hi = Rs
void PCSX::InterpretedCPU::psxMTLO() { _rLo_ = _rRs_; }  // Lo = Rs

/*********************************************************
 * Special purpose instructions                           *
 * Format:  OP                                            *
 *********************************************************/
void PCSX::InterpretedCPU::psxBREAK() {
    // Break exception - psx rom doens't handles this
}

void PCSX::InterpretedCPU::psxSYSCALL() {
    PCSX::g_emulator.m_psxCpu->m_psxRegs.pc -= 4;
    PCSX::g_emulator.m_psxCpu->psxException(0x20, s_branch);
}

void PCSX::InterpretedCPU::psxRFE() {
    //  PCSX::g_system->printf("psxRFE\n");
    PCSX::g_emulator.m_psxCpu->m_psxRegs.CP0.n.Status =
        (PCSX::g_emulator.m_psxCpu->m_psxRegs.CP0.n.Status & 0xfffffff0) |
        ((PCSX::g_emulator.m_psxCpu->m_psxRegs.CP0.n.Status & 0x3c) >> 2);
}

/*********************************************************
 * Register branch logic                                  *
 * Format:  OP rs, rt, offset                             *
 *********************************************************/
#define RepBranchi32(op) \
    if (_i32(_rRs_) op _i32(_rRt_)) doBranch(_BranchTarget_);

void PCSX::InterpretedCPU::psxBEQ() { RepBranchi32(==) }  // Branch if Rs == Rt
void PCSX::InterpretedCPU::psxBNE() { RepBranchi32(!=) }  // Branch if Rs != Rt

/*********************************************************
 * Jump to target                                         *
 * Format:  OP target                                     *
 *********************************************************/
void PCSX::InterpretedCPU::psxJ() { doBranch(_JumpTarget_); }
void PCSX::InterpretedCPU::psxJAL() {
    _SetLink(31);
    doBranch(_JumpTarget_);
}

/*********************************************************
 * Register jump                                          *
 * Format:  OP rs, rd                                     *
 *********************************************************/
void PCSX::InterpretedCPU::psxJR() {
    doBranch(_u32(_rRs_));
    PCSX::g_emulator.m_psxCpu->psxJumpTest();
}

void PCSX::InterpretedCPU::psxJALR() {
    uint32_t temp = _u32(_rRs_);
    if (_Rd_) {
        _SetLink(_Rd_);
    }
    doBranch(temp);
}

/*********************************************************
 * Load and store for GPR                                 *
 * Format:  OP rt, offset(base)                           *
 *********************************************************/

#define _oB_ (_u32(_rRs_) + _Imm_)

void PCSX::InterpretedCPU::psxLB() {
    // load delay = 1 latency
    if (s_branch == 0) {
        // simulate: beq r0,r0,lw+4 / lw / (delay slot)
        PCSX::g_emulator.m_psxCpu->m_psxRegs.pc -= 4;
        doBranch(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc + 4);

        return;
    }

    if (_Rt_) {
        _i32(_rRt_) = (signed char)PCSX::g_emulator.m_psxMem->psxMemRead8(_oB_);
    } else {
        PCSX::g_emulator.m_psxMem->psxMemRead8(_oB_);
    }
}

void PCSX::InterpretedCPU::psxLBU() {
    // load delay = 1 latency
    if (s_branch == 0) {
        // simulate: beq r0,r0,lw+4 / lw / (delay slot)
        PCSX::g_emulator.m_psxCpu->m_psxRegs.pc -= 4;
        doBranch(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc + 4);

        return;
    }

    if (_Rt_) {
        _u32(_rRt_) = PCSX::g_emulator.m_psxMem->psxMemRead8(_oB_);
    } else {
        PCSX::g_emulator.m_psxMem->psxMemRead8(_oB_);
    }
}

void PCSX::InterpretedCPU::psxLH() {
    // load delay = 1 latency
    if (s_branch == 0) {
        // simulate: beq r0,r0,lw+4 / lw / (delay slot)
        PCSX::g_emulator.m_psxCpu->m_psxRegs.pc -= 4;
        doBranch(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc + 4);

        return;
    }

    if (_Rt_) {
        _i32(_rRt_) = (short)PCSX::g_emulator.m_psxMem->psxMemRead16(_oB_);
    } else {
        PCSX::g_emulator.m_psxMem->psxMemRead16(_oB_);
    }
}

void PCSX::InterpretedCPU::psxLHU() {
    // load delay = 1 latency
    if (s_branch == 0) {
        // simulate: beq r0,r0,lw+4 / lw / (delay slot)
        PCSX::g_emulator.m_psxCpu->m_psxRegs.pc -= 4;
        doBranch(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc + 4);

        return;
    }

    if (_Rt_) {
        _u32(_rRt_) = PCSX::g_emulator.m_psxMem->psxMemRead16(_oB_);
    } else {
        PCSX::g_emulator.m_psxMem->psxMemRead16(_oB_);
    }
}

void PCSX::InterpretedCPU::psxLW() {
    // load delay = 1 latency
    if (s_branch == 0) {
        // simulate: beq r0,r0,lw+4 / lw / (delay slot)
        PCSX::g_emulator.m_psxCpu->m_psxRegs.pc -= 4;
        doBranch(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc + 4);

        return;
    }

    if (_Rt_) {
        _u32(_rRt_) = PCSX::g_emulator.m_psxMem->psxMemRead32(_oB_);
    } else {
        PCSX::g_emulator.m_psxMem->psxMemRead32(_oB_);
    }
}

void PCSX::InterpretedCPU::psxLWL() {
    uint32_t addr = _oB_;
    uint32_t shift = addr & 3;
    uint32_t mem = PCSX::g_emulator.m_psxMem->psxMemRead32(addr & ~3);

    // load delay = 1 latency
    if (s_branch == 0) {
        // simulate: beq r0,r0,lw+4 / lw / (delay slot)
        PCSX::g_emulator.m_psxCpu->m_psxRegs.pc -= 4;
        doBranch(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc + 4);

        return;
    }

    if (!_Rt_) return;
    _u32(_rRt_) = (_u32(_rRt_) & g_LWL_MASK[shift]) | (mem << g_LWL_SHIFT[shift]);

    /*
    Mem = 1234.  Reg = abcd

    0   4bcd   (mem << 24) | (reg & 0x00ffffff)
    1   34cd   (mem << 16) | (reg & 0x0000ffff)
    2   234d   (mem <<  8) | (reg & 0x000000ff)
    3   1234   (mem      ) | (reg & 0x00000000)
    */
}

void PCSX::InterpretedCPU::psxLWR() {
    uint32_t addr = _oB_;
    uint32_t shift = addr & 3;
    uint32_t mem = PCSX::g_emulator.m_psxMem->psxMemRead32(addr & ~3);

    // load delay = 1 latency
    if (s_branch == 0) {
        // simulate: beq r0,r0,lw+4 / lw / (delay slot)
        PCSX::g_emulator.m_psxCpu->m_psxRegs.pc -= 4;
        doBranch(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc + 4);

        return;
    }

    if (!_Rt_) return;
    _u32(_rRt_) = (_u32(_rRt_) & g_LWR_MASK[shift]) | (mem >> g_LWR_SHIFT[shift]);

    /*
    Mem = 1234.  Reg = abcd

    0   1234   (mem      ) | (reg & 0x00000000)
    1   a123   (mem >>  8) | (reg & 0xff000000)
    2   ab12   (mem >> 16) | (reg & 0xffff0000)
    3   abc1   (mem >> 24) | (reg & 0xffffff00)
    */
}

void PCSX::InterpretedCPU::psxSB() { PCSX::g_emulator.m_psxMem->psxMemWrite8(_oB_, _u8(_rRt_)); }
void PCSX::InterpretedCPU::psxSH() { PCSX::g_emulator.m_psxMem->psxMemWrite16(_oB_, _u16(_rRt_)); }
void PCSX::InterpretedCPU::psxSW() { PCSX::g_emulator.m_psxMem->psxMemWrite32(_oB_, _u32(_rRt_)); }

void PCSX::InterpretedCPU::psxSWL() {
    uint32_t addr = _oB_;
    uint32_t shift = addr & 3;
    uint32_t mem = PCSX::g_emulator.m_psxMem->psxMemRead32(addr & ~3);

    PCSX::g_emulator.m_psxMem->psxMemWrite32(addr & ~3,
                                             (_u32(_rRt_) >> g_SWL_SHIFT[shift]) | (mem & g_SWL_MASK[shift]));
    /*
    Mem = 1234.  Reg = abcd

    0   123a   (reg >> 24) | (mem & 0xffffff00)
    1   12ab   (reg >> 16) | (mem & 0xffff0000)
    2   1abc   (reg >>  8) | (mem & 0xff000000)
    3   abcd   (reg      ) | (mem & 0x00000000)
    */
}

void PCSX::InterpretedCPU::psxSWR() {
    uint32_t addr = _oB_;
    uint32_t shift = addr & 3;
    uint32_t mem = PCSX::g_emulator.m_psxMem->psxMemRead32(addr & ~3);

    PCSX::g_emulator.m_psxMem->psxMemWrite32(addr & ~3,
                                             (_u32(_rRt_) << g_SWR_SHIFT[shift]) | (mem & g_SWR_MASK[shift]));

    /*
    Mem = 1234.  Reg = abcd

    0   abcd   (reg      ) | (mem & 0x00000000)
    1   bcd4   (reg <<  8) | (mem & 0x000000ff)
    2   cd34   (reg << 16) | (mem & 0x0000ffff)
    3   d234   (reg << 24) | (mem & 0x00ffffff)
    */
}

/*********************************************************
 * Moves between GPR and COPx                             *
 * Format:  OP rt, fs                                     *
 *********************************************************/
void PCSX::InterpretedCPU::psxMFC0() {
    // load delay = 1 latency
    if (s_branch == 0) {
        // simulate: beq r0,r0,lw+4 / lw / (delay slot)
        PCSX::g_emulator.m_psxCpu->m_psxRegs.pc -= 4;
        doBranch(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc + 4);

        return;
    }

    if (!_Rt_) return;

    _i32(_rRt_) = (int)_rFs_;
}

void PCSX::InterpretedCPU::psxCFC0() {
    // load delay = 1 latency
    if (s_branch == 0) {
        // simulate: beq r0,r0,lw+4 / lw / (delay slot)
        PCSX::g_emulator.m_psxCpu->m_psxRegs.pc -= 4;
        doBranch(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc + 4);

        return;
    }

    if (!_Rt_) return;

    _i32(_rRt_) = (int)_rFs_;
}

void PCSX::InterpretedCPU::psxTestSWInts() {
    // the next code is untested, if u know please
    // tell me if it works ok or not (linuzappz)
    if (m_psxRegs.CP0.n.Cause & m_psxRegs.CP0.n.Status & 0x0300 && m_psxRegs.CP0.n.Status & 0x1) {
        psxException(m_psxRegs.CP0.n.Cause, s_branch);
    }
}

inline void PCSX::InterpretedCPU::MTC0(int reg, uint32_t val) {
    //  PCSX::g_system->printf("MTC0 %d: %x\n", reg, val);
    switch (reg) {
        case 12:  // Status
            PCSX::g_emulator.m_psxCpu->m_psxRegs.CP0.r[12] = val;
            psxTestSWInts();
            break;

        case 13:  // Cause
            PCSX::g_emulator.m_psxCpu->m_psxRegs.CP0.n.Cause = val & ~(0xfc00);
            psxTestSWInts();
            break;

        default:
            PCSX::g_emulator.m_psxCpu->m_psxRegs.CP0.r[reg] = val;
            break;
    }
}

void PCSX::InterpretedCPU::psxMTC0() { MTC0(_Rd_, _u32(_rRt_)); }
void PCSX::InterpretedCPU::psxCTC0() { MTC0(_Rd_, _u32(_rRt_)); }

void PCSX::InterpretedCPU::psxMFC2() {
    // load delay = 1 latency
    if (s_branch == 0) {
        // simulate: beq r0,r0,lw+4 / lw / (delay slot)
        PCSX::g_emulator.m_psxCpu->m_psxRegs.pc -= 4;
        doBranch(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc + 4);

        return;
    }
}

void PCSX::InterpretedCPU::psxCFC2() {
    // load delay = 1 latency
    if (s_branch == 0) {
        // simulate: beq r0,r0,lw+4 / lw / (delay slot)
        PCSX::g_emulator.m_psxCpu->m_psxRegs.pc -= 4;
        doBranch(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc + 4);

        return;
    }
}

/*********************************************************
 * Unknow instruction (would generate an exception)       *
 * Format:  ?                                             *
 *********************************************************/
void PCSX::InterpretedCPU::psxNULL() {
    PSXCPU_LOG("psx: Unimplemented op %x\n", PCSX::g_emulator.m_psxCpu->m_psxRegs.code);
}

void PCSX::InterpretedCPU::psxSPECIAL() { (*this.*(s_pPsxSPC[_Funct_]))(); }

void PCSX::InterpretedCPU::psxREGIMM() { (*this.*(s_pPsxREG[_Rt_]))(); }

void PCSX::InterpretedCPU::psxCOP0() { (*this.*(s_pPsxCP0[_Rs_]))(); }

void PCSX::InterpretedCPU::psxBASIC() { (*this.*(s_pPsxCP2BSC[_Rs_]))(); }

void PCSX::InterpretedCPU::psxHLE() {
    uint32_t hleCode = PCSX::g_emulator.m_psxCpu->m_psxRegs.code & 0x03ffffff;
    if (hleCode >= (sizeof(psxHLEt) / sizeof(psxHLEt[0]))) {
        psxNULL();
    } else {
        psxHLEt[hleCode]();
    }
}

const PCSX::InterpretedCPU::intFunc_t PCSX::InterpretedCPU::s_psxBSC[64] = {
    &InterpretedCPU::psxSPECIAL, &InterpretedCPU::psxREGIMM, &InterpretedCPU::psxJ,    &InterpretedCPU::psxJAL,    // 00
    &InterpretedCPU::psxBEQ,     &InterpretedCPU::psxBNE,    &InterpretedCPU::psxBLEZ, &InterpretedCPU::psxBGTZ,   // 04
    &InterpretedCPU::psxADDI,    &InterpretedCPU::psxADDIU,  &InterpretedCPU::psxSLTI, &InterpretedCPU::psxSLTIU,  // 08
    &InterpretedCPU::psxANDI,    &InterpretedCPU::psxORI,    &InterpretedCPU::psxXORI, &InterpretedCPU::psxLUI,    // 0c
    &InterpretedCPU::psxCOP0,    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,   // 10
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,   // 14
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,   // 18
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,   // 1c
    &InterpretedCPU::psxLB,      &InterpretedCPU::psxLH,     &InterpretedCPU::psxLWL,  &InterpretedCPU::psxLW,     // 20
    &InterpretedCPU::psxLBU,     &InterpretedCPU::psxLHU,    &InterpretedCPU::psxLWR,  &InterpretedCPU::psxNULL,   // 24
    &InterpretedCPU::psxSB,      &InterpretedCPU::psxSH,     &InterpretedCPU::psxSWL,  &InterpretedCPU::psxSW,     // 28
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxSWR,  &InterpretedCPU::psxNULL,   // 2c
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,   // 30
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,   // 34
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxHLE,    // 38
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,   // 3c
};

const PCSX::InterpretedCPU::intFunc_t PCSX::InterpretedCPU::s_psxSPC[64] = {
    &InterpretedCPU::psxSLL,     &InterpretedCPU::psxNULL,  &InterpretedCPU::psxSRL,  &InterpretedCPU::psxSRA,   // 00
    &InterpretedCPU::psxSLLV,    &InterpretedCPU::psxNULL,  &InterpretedCPU::psxSRLV, &InterpretedCPU::psxSRAV,  // 04
    &InterpretedCPU::psxJR,      &InterpretedCPU::psxJALR,  &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 08
    &InterpretedCPU::psxSYSCALL, &InterpretedCPU::psxBREAK, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 0c
    &InterpretedCPU::psxMFHI,    &InterpretedCPU::psxMTHI,  &InterpretedCPU::psxMFLO, &InterpretedCPU::psxMTLO,  // 10
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,  &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 14
    &InterpretedCPU::psxMULT,    &InterpretedCPU::psxMULTU, &InterpretedCPU::psxDIV,  &InterpretedCPU::psxDIVU,  // 18
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,  &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 1c
    &InterpretedCPU::psxADD,     &InterpretedCPU::psxADDU,  &InterpretedCPU::psxSUB,  &InterpretedCPU::psxSUBU,  // 20
    &InterpretedCPU::psxAND,     &InterpretedCPU::psxOR,    &InterpretedCPU::psxXOR,  &InterpretedCPU::psxNOR,   // 24
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,  &InterpretedCPU::psxSLT,  &InterpretedCPU::psxSLTU,  // 28
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,  &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 2c
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,  &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 30
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,  &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 34
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,  &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 38
    &InterpretedCPU::psxNULL,    &InterpretedCPU::psxNULL,  &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 3c
};

const PCSX::InterpretedCPU::intFunc_t PCSX::InterpretedCPU::s_psxREG[32] = {
    &InterpretedCPU::psxBLTZ,   &InterpretedCPU::psxBGEZ,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 00
    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 04
    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 08
    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 0c
    &InterpretedCPU::psxBLTZAL, &InterpretedCPU::psxBGEZAL, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 10
    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 14
    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 18
    &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL,   &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 1c
};

const PCSX::InterpretedCPU::intFunc_t PCSX::InterpretedCPU::s_psxCP0[32] = {
    &InterpretedCPU::psxMFC0, &InterpretedCPU::psxNULL, &InterpretedCPU::psxCFC0, &InterpretedCPU::psxNULL,  // 00
    &InterpretedCPU::psxMTC0, &InterpretedCPU::psxNULL, &InterpretedCPU::psxCTC0, &InterpretedCPU::psxNULL,  // 04
    &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 08
    &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 0c
    &InterpretedCPU::psxRFE,  &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 10
    &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 14
    &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 18
    &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL, &InterpretedCPU::psxNULL,  // 1c
};

///////////////////////////////////////////

bool PCSX::InterpretedCPU::Init() {
    s_pPsxBSC = s_psxBSC;
    s_pPsxSPC = s_psxSPC;
    s_pPsxREG = s_psxREG;
    s_pPsxCP0 = s_psxCP0;
    return true;
}
void PCSX::InterpretedCPU::Reset() { PCSX::g_emulator.m_psxCpu->m_psxRegs.ICache_valid = false; }
void PCSX::InterpretedCPU::Execute() {
    while (PCSX::g_system->running()) execI();
}
void PCSX::InterpretedCPU::ExecuteBlock() {
    s_branch2 = 0;
    while (!s_branch2) execI();
}
void PCSX::InterpretedCPU::Clear(uint32_t Addr, uint32_t Size) {}
void PCSX::InterpretedCPU::Shutdown() {}
// interpreter execution
inline void PCSX::InterpretedCPU::execI() {
    InterceptConsole();
    uint32_t *code = PCSX::g_emulator.m_psxCpu->Read_ICache(PCSX::g_emulator.m_psxCpu->m_psxRegs.pc, false);
    PCSX::g_emulator.m_psxCpu->m_psxRegs.code = ((code == NULL) ? 0 : SWAP_LE32(*code));

    PCSX::g_emulator.m_psxCpu->m_psxRegs.pc += 4;
    PCSX::g_emulator.m_psxCpu->m_psxRegs.cycle += PCSX::Emulator::BIAS;

    cIntFunc_t func = s_pPsxBSC[PCSX::g_emulator.m_psxCpu->m_psxRegs.code >> 26];
    (*this.*func)();
}
