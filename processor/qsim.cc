/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID ("$Id: qsim.cc,v 1.1.1.1.14.1 2005/10/28 18:28:03 pwells Exp $");

#include <assert.h>
#include "definitions.h"
#include "isa.h"
#include "fu.h"
#include "chip.h"
#include "dynamic.h"
#include "sequencer.h"
#include "iwindow.h"
#include "mai_instr.h"


/* set the following parameters */
const int fetch_bw   = 1;
const int execute_bw = 1;
const int commit_bw  = 1;
/* end parameters */

static int mm_id     = -1;
sequencer_t *seq = 0;

typedef struct {
	/* qsim object */
	conf_object_t obj;

	/* cpu number: obtained from the simics config file */
	conf_object_t *cpu;

	/* interrupt vector number that needs to be handled */
	int           interrupt_vector;

	/* fetch, execute and commit bandwidth. User parameters */
	int           fetch_bw;
	int           execute_bw;
	int           commit_bw;

	/* if set, stops the front end. Used for interrupts. */
	int           stop_front_end;

	/* global cycle counter */
    int           cycles;

	/* some stats */
	int           commits;
	int           executes;
	int           fetches;

	/* dummy variable: used to debug */
	int           dummy;

	proc_tm_t     *p;
} qsim_object_t;

typedef struct {
	/* branch related: stores the value returned by is_branch () function  */
	int               is_branch;

	/* is annul bit set */
	int               annul_bit;

	/* if set instruction not allowed to commit */
	int               annul;

	/* if set instruction fetched delay slot */
	int               delay_slot;

	/* points to the child instruction */
	instruction_id_t  child;

	/* when was this instruction fetched */
	int               cycle_fetched;

	/* dynamic_instr_t */
	dynamic_instr_t   *d_instr;

} instruction_ud_t;

/* kind of branch */
typedef enum {INSTR_NOBR,       /* not a branch */
              INSTR_BR_COND,    /* branch is conditional */
              INSTR_BR_UNCOND,  /* branch is unconditional */
              INSTR_BR_NOANNUL  /* call, jmpl, return; no annul bit */
             } instr_brtype_t;

instr_brtype_t is_branch(conf_object_t *cp, instruction_id_t ii)
{
	sim_exception_t ex;
	int op;

	SIM_clear_exception ();

	op = SIM_instruction_get_field_value(cp, ii, "op");
	if ((ex = SIM_get_pending_exception ())) {
		SIM_clear_exception();
		return INSTR_NOBR;
	}

	if (op == 0) { /* rel branches */
		int op2 = SIM_instruction_get_field_value(cp, ii, "op2");
		if ((ex = SIM_get_pending_exception())) {
			SIM_clear_exception();
			return INSTR_NOBR;
		} else {
			if (op2 == 1 || op2 == 2 || op2 == 5 || op2 == 6) {
				int cond = SIM_instruction_get_field_value (cp, ii, "cond");
				if (cond == 0 || cond == 8) return INSTR_BR_UNCOND;
			}

			if (op2 != 0 && op2 != 4 && op2 != 7) return INSTR_BR_COND;
		}
	} else if (op == 2) { /* jmpl and return */
		int op3 = SIM_instruction_get_field_value(cp, ii, "op3");
		if ((ex = SIM_get_pending_exception())) {
			SIM_clear_exception();
			return INSTR_NOBR;
		} else {
			if (op3 == 0x38 || op3 == 0x39)
				return INSTR_BR_NOANNUL;
		}
	} else /* call */
		if (op == 1) 
			return INSTR_BR_NOANNUL;

	return INSTR_NOBR;		
}

void get_instruction_info (qsim_object_t *q,
                           conf_object_t *cpu, instruction_id_t instr) {
	int           i;
	reg_info_t    *info;
	attr_value_t  pc;
	processor_t   *p; 
	instruction_id_t child;

	i   = 0;
	p   = SIM_proc_no_2_ptr ( SIM_get_proc_no (q->cpu) );
	pc  = SIM_instruction_read_input_reg (cpu, instr, Sim_Reg_Id_PC);

	assert (pc.kind == Sim_Val_Integer);

	pr ("%35s", (SIM_disassemble (p, pc.u.integer, 1))->string);
#if 0
	while ((info = SIM_instruction_get_reg_info (cpu, instr, i++))) 
		pr ("| %3d %4s %4s ", info->id, 
		(info->in) ? "in" : "  ", 
		(info->out) ? "out" : "   ");

	pr ("\n");	

    child = SIM_instruction_child (cpu, instr, 0);
	if (child) {
		pc = SIM_instruction_read_input_reg (cpu, child, Sim_Reg_Id_PC);
		pr ("%35s\n", (SIM_disassemble (p, pc.u.integer, 1))->string);
	}
#endif
}

void print_reorder_buffer (qsim_object_t *q, conf_object_t *cpu) {
	int              i;
	instruction_id_t instr;

	attr_value_t     ipc;
	attr_value_t     inpc;
	attr_value_t     opc;
	attr_value_t     onpc;

	i = 0; 
	while ((instr = SIM_instruction_nth_id (cpu, i))) {
		ipc = SIM_instruction_read_input_reg (cpu, instr, Sim_Reg_Id_PC);
		inpc= SIM_instruction_read_input_reg (cpu, instr, Sim_Reg_Id_NPC);
		opc = SIM_instruction_read_output_reg (cpu, instr, Sim_Reg_Id_PC);
		onpc= SIM_instruction_read_output_reg (cpu, instr, Sim_Reg_Id_NPC);

		pr ("%d, i: %d, status: %d sync: %d branch: %d spec: %d exception: %d\n", 
		i,  instr,
		SIM_instruction_phase (cpu, instr), 
		SIM_instruction_is_sync (cpu, instr),
		is_branch (cpu, instr), 
		SIM_instruction_wrong_path (cpu, instr), 
		SIM_instruction_status (cpu, instr) & Sim_IS_Faulting);

		pr ("ipc: %lld, inpc: %lld, opc: %lld, onpc: %lld\n", 
		ipc.u.integer, inpc.u.integer, opc.u.integer, onpc.u.integer);

		i++;	
	}
}

void interrupt_check (qsim_object_t *q, conf_object_t *cpu) {
	if (q->interrupt_vector != No_Exception) {
		switch (SIM_instruction_handle_interrupt (cpu, q->interrupt_vector)) {
			case Sim_IE_OK:
				q->interrupt_vector = No_Exception;
				break;

			case Sim_IE_Interrupts_Disabled:
				break;

			case Sim_IE_Illegal_Interrupt_Point:
				q->stop_front_end = 1;
				break;

			default:
				break;
		}
	}
}

void commit_stage (qsim_object_t *q, conf_object_t *cpu) {

	int              i;
	instruction_id_t instr;
	instruction_ud_t *ud;

	for (i = 0; i < q->commit_bw; i++) {

		/* get root instruction */
		instr = SIM_instruction_nth_id (cpu, 0);

		/* no instructions */
		if (!instr) return;
	
		/* if instruction not executed return (inorder commit) */
		if (! (SIM_instruction_phase (cpu, instr) == Sim_Phase_Executed)) return;

		/* get user data on the instruction */
		ud = (instruction_ud_t *) SIM_instruction_get_user_data (cpu, instr);

		/* if annul bit set squash the instruction */
		if (ud->annul) {
			if (SIM_instruction_squash (cpu, instr) != Sim_IE_OK) assert (0); 
			continue;
		}
		
		/* if instruction in wrong-path, squash and return */
		if (SIM_instruction_wrong_path (cpu, instr)) assert (0);

		/* panic if sync instruction is not alone */
		if (SIM_instruction_is_sync (cpu, instr) == 2 && 
				SIM_instruction_child (cpu, instr, 0)) assert (0);

		/* commit instruction */
		switch (SIM_instruction_commit (cpu, instr)) {
			case Sim_IE_OK:
				q->commits++;
				break;

			case Sim_IE_Code_Breakpoint:
				assert (0);

			default:	
				assert (0);
		}

		/* end the instruction */
		delete ud->d_instr;
		MM_FREE (ud); 

		switch (SIM_instruction_end (cpu, instr)) {
			case Sim_IE_OK:
				break;

			default:
				assert (0);
		}
	}
}

void execute_stage (qsim_object_t *q, conf_object_t *cpu) {
	instruction_id_t instr;
	instruction_id_t child;
	int              i;
	instruction_ud_t *ud;

	 /* exception handler: decode exceptions comes single, execute
	  * exception comes with children. Kill them! */
	instr = SIM_instruction_nth_id (cpu, 0);
	if (instr && SIM_instruction_status (cpu, instr) & Sim_IS_Faulting) {
		if (!SIM_instruction_child (cpu, instr, 0) && 
		    !SIM_instruction_parent (cpu, instr)) {
			if (SIM_instruction_handle_exception (cpu, instr) != Sim_IE_OK) assert (0);
			SIM_instruction_end (cpu, instr);
			return;
		} else assert (0);
	}

	/* executes */
	
	i = 0;
	while ( (i < q->execute_bw) && 
	        (instr = SIM_instruction_nth_id (cpu, i)) ) {

		if (SIM_instruction_phase (cpu, instr) == Sim_Phase_Decoded &&
			SIM_instruction_status (cpu, instr) & Sim_IS_Ready) {

			if (SIM_instruction_is_sync (cpu, instr) == 2) {
				 q->stop_front_end = 1;

				child = SIM_instruction_child (cpu, instr, 0);
				if (child && SIM_instruction_squash (cpu, child) != Sim_IE_OK) assert (0);
			}
			
			switch (SIM_instruction_execute (cpu, instr)) {
			case Sim_IE_OK:
				q->executes++;

				ud = (instruction_ud_t *) (SIM_instruction_get_user_data (cpu, instr));

				/* if branch and has child instruction */
				if (ud->is_branch && ud->child) {
					instruction_ud_t    *ds_ud; 
					instruction_id_t    ds;

					/* get child user data */
					ds    = ud->child;
					ds_ud = (instruction_ud_t *) (SIM_instruction_get_user_data (cpu, ds));

					/* if child is delay slot */
					if (ds_ud->delay_slot) {

						/* if the branch has annul bit set */
						if (ud->annul_bit) {
							/* if conditional branch */
							if (ud->is_branch == INSTR_BR_COND && 
 							    ((SIM_instruction_status (cpu, instr) & Sim_IS_Branch_Taken) != Sim_IS_Branch_Taken)) {
								ds_ud->annul = 1;
							} else
							/* if unconditional branch */
							if (ud->is_branch == INSTR_BR_UNCOND &&
							    ((SIM_instruction_status (cpu, instr) & Sim_IS_Branch_Taken) == Sim_IS_Branch_Taken)) {
								ds_ud->annul = 1;
							}
						}
						
						/* take action on delay slot */
						if (!ds_ud->annul) {
							/* make sure pc's match */
							assert ((SIM_instruction_read_output_reg (cpu, instr, Sim_Reg_Id_PC)).u.integer 
									== (SIM_instruction_read_input_reg (cpu, ds, Sim_Reg_Id_PC)).u.integer); 
							
							/* force write npc */
							SIM_instruction_write_input_reg (cpu, ds, Sim_Reg_Id_NPC, 
							SIM_instruction_read_output_reg (cpu, instr, Sim_Reg_Id_NPC));

							/* make the delay slot non-speculative */
							SIM_instruction_set_right_path (cpu, ds);  
							
						} else {
							/* annul the instruction by squashing ds */
							SIM_instruction_squash (cpu, ds);
							return; /* TODO: not needed */
						}
					}
				}
				break;

			case Sim_IE_Exception: 
				/* handle exception, by squashing all children,
				 * and stopping the frontend. */
				q->stop_front_end = 1;
				child = SIM_instruction_child (cpu, instr, 0);
				if (child && SIM_instruction_squash (cpu, child) != Sim_IE_OK) assert (0);

				return;

			case Sim_IE_Stalling:
				return; /* TODO: not needed */

			case Sim_IE_Sync_Instruction:
				return;

			default:
				assert (0);
			}
		} else break;
		i++;		
	}
}

void fetch_stage (qsim_object_t *q, conf_object_t *cpu) {

	/* only one instruction can be in the tree with Phase = Sim_Phase_Initiated.
	   The fetch unit stalls when there is an i-cache miss. Multiple 
	   outstanding i-cache misses are not possible with the code 
	   that follows. This assumption is fairly ok since qsim models 
	   a simple multiple-issue inorder processor. */
	   
	instruction_id_t   instr = 0;
	instruction_id_t   parent;
	attr_value_t       parent_pc;
	attr_value_t       parent_npc;
	int                i;
	instruction_ud_t  *user_data = 0;
	instruction_ud_t  *parent_info = 0;

	/* stop_front_end code removed. Modified, and included in each
	   of the if-then-else branches */

	/* descend tree, get to the last instruction */
	parent = SIM_instruction_nth_id (cpu, 0);
	for (parent = SIM_instruction_nth_id (cpu, 0); 
		 parent && (SIM_instruction_child (cpu, parent, 0));
	   	 parent = SIM_instruction_child (cpu, parent, 0));

	/* fetch & decode bandwidth */
	for (i = 0; i < q->fetch_bw; parent = instr) {
		i++;

		if (parent &&
		SIM_instruction_phase (cpu, parent) < Sim_Phase_Decoded) {
			// unimplemented: cache miss case
			ASSERT (0);
			
			/* not decoded */
			instr = parent;

		} else if (parent && 
		SIM_instruction_phase (cpu, parent) < Sim_Phase_Executed) {
			/* not executed */
			if (q->stop_front_end) return;
			
			/* get parent information */
			parent_info = (instruction_ud_t *) SIM_instruction_get_user_data (cpu, parent);
			assert (parent_info != NULL);

			/* if parent is unexecuted delay slot, cannot proceed */
			if (parent_info->delay_slot) return;
		
			/* proceed with the fetch */

#if 0
			instr = SIM_instruction_begin (cpu);
			if (!instr) return;
			SIM_instruction_insert (cpu, parent, instr);
#endif
			if (!seq->get_iwindow ()->slot_avail ()) return;

			dynamic_instr_t *d_instr = new dynamic_instr_t (seq);
			instr = d_instr->get_mai_instruction ()->get_instr_id ();
			if (!instr) assert (0);

			/* set child */
			parent_info->child = instr;
		
			/* write pc and npc */
			parent_pc  = SIM_instruction_read_input_reg (cpu, parent, Sim_Reg_Id_PC);
			parent_npc = SIM_instruction_read_input_reg (cpu, parent, Sim_Reg_Id_NPC);
			parent_pc.u.integer = parent_npc.u.integer;
			parent_npc.u.integer = parent_npc.u.integer + 4;
			SIM_instruction_write_input_reg (cpu, instr, Sim_Reg_Id_PC, parent_pc);
			SIM_instruction_write_input_reg (cpu, instr, Sim_Reg_Id_NPC, parent_npc); 
		
			/* set user data = cycle when the instruction was fetched */
			user_data =	MM_ZALLOC (1, instruction_ud_t);
			user_data->cycle_fetched = q->cycles;

			/* if parent is branch and not executed, fetching delay slot */
			if (parent_info->is_branch) user_data->delay_slot = 1;
			
			SIM_instruction_set_user_data (cpu, instr, (lang_void *) user_data);
			
			d_instr->fetch ();
			
		} else if (!parent || 
		    (parent && SIM_instruction_phase (cpu, parent) >= Sim_Phase_Executed)) {
			/* parent executed or no parent: here i don't care about branch
			or delay slot. Reason: br and delay slot instructions are executed, 
			s, pc and npc are known. */
			if (q->stop_front_end) return;
		
			/* set child */
			if (parent) {
				parent_info = 
				(instruction_ud_t *) SIM_instruction_get_user_data (cpu, parent);
			}
#if 0
			instr = SIM_instruction_begin (cpu);
			if (!instr) return;
			SIM_instruction_insert (cpu, parent, instr);
#endif
			if (!seq->get_iwindow ()->slot_avail ()) return;

			dynamic_instr_t *d_instr = new dynamic_instr_t (seq);
			instr = d_instr->get_mai_instruction ()->get_instr_id ();
			if (!instr) assert (0);
					
			/* if parent exists, set child */
	 		if (parent) parent_info->child = instr;

			/* user data */
			user_data =	MM_ZALLOC (1, instruction_ud_t);
			user_data->cycle_fetched = q->cycles;
			//user_data->d_instr = d_instr;

			SIM_instruction_set_user_data (cpu, instr, (lang_void *) user_data);
			
			d_instr->fetch ();
		}

		/* decode instruction */
		
		ASSERT (seq->get_iwindow ()->todecode () == user_data->d_instr);
		switch (SIM_instruction_decode (cpu, instr)) {
		case Sim_IE_OK:
			parent_pc = SIM_instruction_read_input_reg (cpu, instr, Sim_Reg_Id_PC);
			q->fetches++;
			
			/* branch related */
			user_data->is_branch = is_branch (cpu, instr);

			/* if branch has annul bit read it */
			if (user_data->is_branch && user_data->is_branch != INSTR_BR_NOANNUL) 
				user_data->annul_bit = SIM_instruction_get_field_value (cpu, instr, "a");
			
			// user_data->d_instr->decode ();
			break;

		case Sim_IE_Stalling:
			return;

		case Sim_IE_Exception:
			q->stop_front_end = 1;
			return;

		case Sim_IE_Unresolved_Dependencies:
			assert (0);

		default:
			assert (0);
		}
	}
}

void run_cycle (qsim_object_t *q) {
	conf_object_t *cpu = q->cpu;

	/* check to see if the front-end can be started */
	if (q->stop_front_end && SIM_instruction_nth_id (cpu, 0) == 0)
		q->stop_front_end = 0;

	/* handle interrupt */
	interrupt_check (q, cpu); 

	/* commit */
	commit_stage (q, cpu);

	/* execute */
	execute_stage (q, cpu);

	/* fetch and decode */
	fetch_stage (q, cpu);

	/* print info */
	if (q->cycles % 1000000 == 0) { 
		pr ("[cpu%d] %12d c, %12d e, %12d f&d in %14d cycles\n", 
		SIM_get_proc_no (q->cpu), q->commits, q->executes, q->fetches, q->cycles);	
	} 

	/* increment global cycles */
	q->cycles++;
}

static void qsim_cycle_handler (conf_object_t *cpu, qsim_object_t *obj) {
	run_cycle (obj);

	SIM_time_post_cycle (cpu, 1, Sim_Sync_Processor, (event_handler_t) qsim_cycle_handler, obj);
}

static void qsim_interrupt_handler (void *_qsim_obj, void *cpu, integer_t vector) {
	qsim_object_t *qsim_obj = (qsim_object_t *) (_qsim_obj);
	processor_t *p = (processor_t *) cpu;

	if ((SIM_get_proc_no (p)) == SIM_get_proc_no (qsim_obj->cpu)) {
		qsim_obj->interrupt_vector = (int) vector;
	}
}

static void initial_config_hap (void *obj) {
	qsim_object_t *qsim_obj = (qsim_object_t *) obj;

	conf_object_t *cpu = qsim_obj->cpu;

	SIM_time_post_cycle (cpu, 0, Sim_Sync_Processor, (event_handler_t) qsim_cycle_handler, obj);

	SIM_hap_register_callback ("Core_Asynchronous_Trap", 
	(str_hap_func_t) qsim_interrupt_handler, obj);

}

static conf_object_t *qsim_new_instance (parse_object_t *p) {
	qsim_object_t *qsim_obj = MM_ZALLOC (1, qsim_object_t);
	SIM_object_constructor ((conf_object_t *) qsim_obj, p);

	if (SIM_initial_configuration_ok ()) 
		initial_config_hap (qsim_obj);
	else 
		SIM_hap_register_callback ("Core_Initial_Configuration", 
		initial_config_hap, qsim_obj);

	/* initalize qsim object */
	qsim_obj->interrupt_vector = No_Exception;

	/* fetch, execute and commit bandwidth */
	qsim_obj->fetch_bw         = fetch_bw;
	qsim_obj->execute_bw       = execute_bw;
	qsim_obj->commit_bw        = commit_bw;

	qsim_obj->stop_front_end   = 0;

	return (conf_object_t *) qsim_obj;
}

static int qsim_delete_instance (conf_object_t *obj) {
	return 0;
}

set_error_t
set_cpu (void *dont_care, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{   
	conf_object_t *cpu;
    qsim_object_t *qsim_obj = (qsim_object_t *) obj;

    if (val->kind != Sim_Val_String) return Sim_Set_Illegal_Value;

	cpu = SIM_get_object (val->u.string);
    qsim_obj->cpu = cpu;
	
	qsim_obj->p = new proc_tm_t (qsim_obj->cpu);
	seq = qsim_obj->p->get_sequencer ();

    return Sim_Set_Ok;
}   

attr_value_t 
get_cpu (void *dont_care, conf_object_t *obj, attr_value_t *idx) {
    attr_value_t ret;
    qsim_object_t *qsim_obj = (qsim_object_t *) obj;
   
    ret.kind = Sim_Val_String;
    ret.u.string = qsim_obj->cpu->name;
    
    return ret;
}   
extern "C" {

DLL_EXPORT void
init_local () {
	class_data_t class_data;
	conf_class_t *qsim_class;

	memset (&class_data, 0, sizeof (class_data_t));
	class_data.new_instance = qsim_new_instance;
	class_data.delete_instance = qsim_delete_instance;
	class_data.description = "processor user module.";

	qsim_class = SIM_register_class ("processor", &class_data);

	SIM_register_attribute (qsim_class, "cpu", get_cpu, 0, 
	set_cpu, 0, Sim_Attr_Required, "CPU");

}

DLL_EXPORT void
fini_local () {
}
}
