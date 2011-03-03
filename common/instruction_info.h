/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _INSTRUCTION_INFO_H_
#define _INSTRUCTION_INFO_H_

class inst_info_t {
    
    private:
        instruction_type_t       type; // Instruction Type
        /// the opcode of the instruction
        opcodes_t           opcode;
        
        // type of functional unit
        fu_type_t           futype;
        
        // the type of branch
        branch_type_t       branch_type;
        
        // additional info for the instruction (atomic, alternate sp)
        uint32              ainfo;
        
        bool                taken;
        // branch related offset
        int64               offset;
        
        // branch related: annul bit
        bool                annul;
        int64               imm_asi;
        int32               num_srcs, num_dests;
        int32               l_reg  [RI_NUMBER];  // logical registers
        int32               reg_sz [RN_NUMBER];  // # regs for each regbox
        bool                reg_fp [RN_NUMBER];  // fp/int for each regbox
        addr_t              target;
        string              opcode_names[i_opcodes + 1];
        
        int64               rd_reg;
        
        uint64              pc_reg;
		addr_t              pc;
        
    public:
        inst_info_t();
        bool decode_instruction_info(addr_t phys_addr, conf_object_t *cpu);
        uint32 maskBits32 (uint32 data, int start, int stop);
        branch_type_t get_branch_type();
        instruction_type_t get_instruction_type();
        addr_t get_target();
        void clear();
        const char *get_opcode_name();
        bool get_delay_slot();
        opcodes_t get_opcode() {return opcode;} 
		addr_t get_pc() { return pc; }
		
		int32 get_num_dests() { return num_dests; }
		
		void read_output_regs(conf_object_t *cpu, int64 *regs);
		int32 *get_output_reg_nums() { return &l_reg[RD]; }
		bool is_possible_bct();
        
        
};


#endif
