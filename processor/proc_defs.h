/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _PROC_DEFS_H_
#define _PROC_DEFS_H_

// class definitions 
class iwindow_t;
class dynamic_instr_t;
class mai_t;
class mai_instruction_t;
class sequencer_t;
class chip_t;
class debugio_t;
class fu_t;
class yags_t;
class cascade_t;
class gpc_t;
class bpred_state_t;
class ctrl_flow_t;
class ras_t;
class ras_state_t;
class lsq_t;
class disambig_t;
class fetch_line_entry_t;
class fetch_buffer_t;
class st_buffer_t;
class mem_xaction_t;
class v9_mem_xaction_t;
class wait_list_t;
class mem_hier_handle_t;
class mem_hier_handle_iface_t;
class proc_object_t;
class proc_stats_t;
class fastsim_t;
template <class T> class window_t;

class mmu_t;
class mmu_asi_interface_t;
class mmu_info_t;
typedef struct cmp_mmu_iface cmp_mmu_iface_t;

// mem hier related class definitions
class mem_hier_interface_t;
class mem_trans_t;
class invalidate_addr_t;

class eventq_t;

class thread_scheduler_t;
class thread_context_t;

class csp_algorithm_t;
class base_algorithm_t;
class tsp_algorithm_t;
class ssp_algorithm_t;
class esp_algorithm_t;
class msp_algoruthm_t;
class bank_algorithm_t;
class heatrun_algorithm_t;

class spin_heuristic_t;
class disable_timer_t;


/* register index for a dynamic_instr_t */
enum reg_index_t {
    RS1       = 0,
    RS2       = 4,   
    RS_RD     = 8,    // for mov/st instrs (rd), leave space of 16 regs for stdfa
    RS_CC     = 24, 
    RD        = 25,   // leave space of 16 regs for lddfa
    RD_CC     = 41,
    RI_NUMBER = 42,  
};  
#define RD_NUMBER (RI_NUMBER - RD)
const int32 CC     = 100; /* conditional code register */
const int32 SP     = 14; /* stack pointer register, o6 */
const int32 FP     = 30; /* frame pointer register, i6 */
const int32 REG_NA = -1; /* invalid register number */
const int32 CWP_NA = -1; /* invalid CWP register */
enum reginfo_index_t {
    RN_RS1,
    RN_RS2,
    RN_RS_RD,
    RN_RD,
    RN_NUMBER
};
const int32 regbox_index[RN_NUMBER] = {RS1, RS2, RS_RD, RD};

/* register window, physical/logical register numbers */
const int32 NWINDOW  = 8;  // number of register windows
const int32 N_GLOBAL = 8;  // number of global reg (not windowed)
const int32 N_GPR    = 32; // number of integer GPR
const int32 N_GPR_PER_WINDOW = 16; // number of GPR reg per window (in + local)
/* XXX currently don't map FPR */
const int32 NLOGICAL = (NWINDOW * N_GPR_PER_WINDOW + N_GLOBAL) + 1;  // +1 for CC
const int32 N_FPR    = 64; // number of single-precision floating-point reg
// const int32 NLOGICAL = (NWINDOW * N_GPR_PER_WINDOW + N_GLOBAL) + N_FPR;

/* indicate if the value in a phy-register is ready */
enum value_ready_t {
   	VR_NO,
   	VR_YES,
   	VR_SPEC,    // for value speculation 
	VR_NUMBER,  
}; 

// functional units
enum fu_type_t {
	FU_NONE = 0, 
	FU_INTALU, 
	FU_INTMULT, 
	FU_INTDIV, 
	FU_BRANCH, 
	FU_FLOATADD, 
	FU_FLOATCMP, 
	FU_FLOATCVT, 
	FU_FLOATMULT, 
	FU_FLOATDIV, 
	FU_FLOATSQRT, 
	FU_MEMRDPORT, 
	FU_MEMWRPORT,
	FU_TYPES
};


enum pipe_stage_t {
	PIPE_NONE,
	PIPE_FETCH,
	PIPE_FETCH_MEM_ACCESS,
	PIPE_INSERT,
	PIPE_PRE_DECODE,
	PIPE_DECODE,
	PIPE_RENAME, 
	PIPE_WAIT,
	PIPE_EXECUTE,
	PIPE_EXECUTE_DONE,
	// mem access not started
	PIPE_MEM_ACCESS,
	// mem access not started. should start when safe
	PIPE_MEM_ACCESS_SAFE,
	// mem access aborted. should restart
	PIPE_MEM_ACCESS_RETRY,
	// mem access in progress
	PIPE_MEM_ACCESS_PROGRESS,
	// mem access in progress, stage 2
	PIPE_MEM_ACCESS_PROGRESS_S2, 
	// mem access soon to complete,
	PIPE_MEM_ACCESS_REEXEC,
	// mem access complete
	PIPE_COMMIT,
	PIPE_COMMIT_CONTINUE,
	// other ``interesting'' stages
	PIPE_EXCEPTION,
	PIPE_SPEC_EXCEPTION,
	PIPE_REPLAY,
	// stages when dinstr not in iwindow
	PIPE_ST_BUF_MEM_ACCESS_PROGRESS,
	PIPE_END,
	PIPE_STAGES
};	

enum gran_t {
	GRAN_BYTE = 0, 
	GRAN_HALF, 
	GRAN_WORD,
	GRAN_QUAD, 
	GRAN_BLOCK, 
	GRAN_GRANS
};

// instruction types
enum instruction_type_t {
	IT_NONE,
	IT_EXECUTE,
	IT_CONTROL,
	IT_LOAD,
	IT_STORE,
	IT_PREFETCH,
	IT_TYPES
};

const uint32 AI_NONE      = 0x0;
const uint32 AI_ATOMIC    = 0x1;
const uint32 AI_ALT_SPACE = 0x2;
const uint32 AI_PREFETCH  = 0x4;

const uint32 PENDING_EVENTQ           = 0x1;
const uint32 PENDING_MEM_HIER         = 0x2;
const uint32 PENDING_WAIT_LIST        = 0x4;

// branch types
enum branch_type_t {
    BRANCH_NONE,        // not a branch 
    BRANCH_UNCOND,      // unconditional branch
    BRANCH_COND,        // conditional branch
    BRANCH_PCOND,       // predicted conditional branch
    BRANCH_CALL,        // call 
    BRANCH_RETURN,      // return from call
    BRANCH_INDIRECT,    // indirect call
    BRANCH_CWP,         // current window pointer update
    BRANCH_TRAP_RETURN, // return from trap
    BRANCH_TRAP,        // 
    BRANCH_PRIV,        // privilege level change
    BRANCH_TYPES
};

// mai instruction stages
enum mai_instr_stage_t {
	INSTR_CREATED,
	INSTR_FETCHED,
	INSTR_DECODED,
	INSTR_EXECUTED,
	INSTR_ST_RETIRED,
	INSTR_COMMITTED,
	INSTR_STAGES
};

// instruction events
enum instr_event_t {
	EVENT_OK,
	EVENT_FAILED,
	EVENT_EXCEPTION,
	EVENT_SPEC_EXCEPTION,
	EVENT_STALL,
	EVENT_SQUASH,
	EVENT_SYNC,
	EVENT_UNRESOLVED_DEP,
	EVENT_UNRESOLVED,
    EVENT_SPECULATIVE,
	EVENT_EVENTS
};

// sync type 
enum sync_type_t {
	SYNC_NONE = 0,
	SYNC_ONE,
	SYNC_TWO,
	SYNC_TYPES
};

enum snoop_status_t {	
	SNOOP_NO_CONFLICT,
	SNOOP_BYPASS_COMPLETE, 
	SNOOP_BYPASS_EXTND, 
	SNOOP_REPLAY, 
	SNOOP_UNRESOLVED_ST,
	SNOOP_PRED_SAFE_ST,
	SNOOP_UNSAFE,
	SNOOP_UNKNOWN, 
	SNOOP_STATUS
};

enum ts_yield_reason_t {
	YIELD_NONE,
	YIELD_DONE_RETRY,
	YIELD_EXCEPTION,
	YIELD_PAGE_FAULT,
	YIELD_LONG_RUNNING,
	YIELD_MUTEX_LOCKED,
	YIELD_IDLE_ENTER,
	YIELD_SYSCALL_SWITCH,
    YIELD_PAUSED_THREAD_INTERRUPT,
    YIELD_EXTRA_LONG_RUNNING,
    YIELD_PROC_MODE_CHANGE,
    YIELD_DISABLE_CORE,
    YIELD_FUNCTION,
    YIELD_HEATRUN_MIGRATE,
    YIELD_NARROW_CORE,
	YIELD_ENTRIES,
};


const uint64 SYS_NUM_SW_INTR    = 300;
const uint64 SYS_NUM_HW_INTR    = 301;
const uint64 SYS_NUM_PAGE_FAULT = 400;
const uint64 SYS_NUM_OS_SCHED   = 500;
const uint64 SYS_UNKNOWN_BOOT   = 600;
const uint64 SYS_UNKNOWN_SCHED  = 700;
const uint64 SYS_UNKNOWN_INTR   = 800;

const addr_t THREAD_STATE_BASE  =  0xbeef00000000000ULL;


// data type definitions

typedef  int64 stick_t;
typedef uint64 tag_t;
typedef uint32 direct_state_t;
typedef uint32 indirect_state_t;

typedef uint64 counter_t;

typedef struct {
	uint64 value;
	bool   valid;

	void initialize (uint64 value = 0) {
		value = 0;
		valid = false;
	}

	void set (uint64 _v) {
		value = _v;
		valid = true;
	}

	bool is_valid () {
		return valid;
	}

	uint64 get () {
		ASSERT (valid);
		return value;
	}

	void invalidate () {
		valid = false;
	}

} safe_uint64;

//#undef STAT_INC

#endif // _PROC_DEFS_H_

