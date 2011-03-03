// Compile time configuration Version 

CONF_V_ARRAY_INT (fu_types, "# Types of Functional Units", 30, 14, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12)
CONF_V_ARRAY_INT (fu_count, "# of functional units",          30, 14, 2, 2, 2,  4, 4, 2, 2, 2, 2,  1,  1, 2, 2)
CONF_V_ARRAY_INT (fu_latencies, "functional units latencies", 30,  14, 1, 1, 1, 4, 20, 1, 2, 2, 4, 12, 24, 1, 1, 1)

CONF_V_INTEGER (fetch_stages, "# of pipeline stages for fetch", 3)
CONF_V_INTEGER (decode_stages, "# of pipeline stages for decode", 2)
CONF_V_INTEGER (rename_stages, "# of pipeline stages for rename", 2)
CONF_V_INTEGER (commit_stages, "# of pipeline stages for commit", 3)
CONF_V_INTEGER (window_size, "# of entries in instruction window", 128)
CONF_V_INTEGER (lsq_load_size, "# of entries in load queue", 32)
CONF_V_INTEGER (lsq_store_size, "# of entries in store queue", 32)
CONF_V_INTEGER (stbuf_size, "# of entries in store buffer", 16)

CONF_V_INTEGER (max_fetch, "# of instructions to fetch", 4)
CONF_V_INTEGER (max_issue, "# of instructions to issue", 4)
CONF_V_INTEGER (max_commit, "# of instructions to commit", 4)

CONF_V_BOOL (kernel_params, "separate kernel params", false)

CONF_V_INTEGER (yags_choice_bits, "# of choice bits for YAGS", 14)
CONF_V_INTEGER (yags_exception_bits, "# of exception bits for YAGS", 12)
CONF_V_INTEGER (yags_tag_bits, "# of tag bits for YAGS", 6)

CONF_V_INTEGER (cas_filter_bits, "# of filter bits for CAS", 8)
CONF_V_INTEGER (cas_exception_bits, "# of exception bits for CAS", 10)
CONF_V_INTEGER (cas_exception_shift, "# of exception shift for CAS", 8)

CONF_V_INTEGER (max_latency, "internal. cycles to hold", 5)

CONF_V_INTEGER (ras_table_bits, "# of bits for RAS", 6)
CONF_V_INTEGER (ras_filter_bits, "# of bits for RAS filter", 7)

CONF_V_INTEGER (stats_print_cycles, "# of cycles to elapse for stats. 0 to disable", 10000000)
CONF_V_INTEGER (run_cycles, "# of cycles to run simulation. 0 to disable", 0)
CONF_V_INTEGER (run_commits, "# of instructions to commit. 0 to disable", 0)
CONF_V_INTEGER (run_user_commits, "# of instructions to commit for user. 0 to disable", 0)
CONF_V_BOOL (overwrite_stats, "overwrite stats log", true)
CONF_V_STRING (stats_log, "stats output file", "stats.log")

CONF_V_BOOL (disable_stbuffer, "disable store buffer", false)

CONF_V_BOOL (initiate_loads_commit, "initiate loads at commit", false)

CONF_V_BOOL (issue_inorder, "issue inorder", false)

CONF_V_BOOL (issue_load_commit, "issue loads at commit", false)
CONF_V_BOOL (issue_store_commit, "issue stores at commit", false)

CONF_V_BOOL (issue_non_spec, "issue non-speculatively", false)
CONF_V_BOOL (issue_loads_non_spec, "issue loads non-speculatively", false) 
CONF_V_BOOL (issue_stores_non_spec, "issue stores non-speculatively", false)

CONF_V_BOOL (disable_complete_bypass, "disable complete bypass", false)
CONF_V_BOOL (disable_extended_bypass, "disable extended bypass", false)
CONF_V_BOOL (use_fetch_buffer, "Use of fetch buffer", false)
CONF_V_INTEGER (fetch_buffer_size, "Size of fetch buffer", 4)

CONF_V_INTEGER (wait_list_size, "internal. wait list size", 64)
CONF_V_BOOL (retry, "retry instead of replay", true)

CONF_V_BOOL (check_lsq_bypass, "check lsq bypass path", false)
CONF_V_BOOL (initiate_loads_unresolved, "initiate loads with older unresolved stores", false)
CONF_V_BOOL (store_prefetch, "prefetch store cache lines", false)

CONF_V_BOOL (handle_interrupt_early, "early handling of interrupts", true)
CONF_V_BOOL (kernel_stats, "separate kernel stats", false)

CONF_V_BOOL (stall_mem_access, "stall mem access", false)
CONF_V_INTEGER (stall_mem_access_cycles, "# of cycles to stall mem access", 12)

CONF_V_INTEGER (check_freq, "forward progress check frequency (cycles)",  1000000)
CONF_V_INTEGER (eventq_size, "eventq size", 50)

// window size x 2 
CONF_V_INTEGER (dead_list_size, "dead list size", 256)
CONF_V_INTEGER (recycle_list_size, "recycle list size", 256)

CONF_V_INTEGER (trap_levels, "internal. # of trap levels", 5)
CONF_V_BOOL (recycle_instr, "recycle instructions", true)

CONF_V_BOOL (use_internal_lsq, "use simics internal lsq", false)

CONF_V_BOOL (heart_beat, "enable heart beat", true)
CONF_V_INTEGER (heart_beat_cycles, "heart beat cycles", 5000000)

CONF_V_BOOL (use_fastsim, "use fastsim", true)
CONF_V_INTEGER (fastsim_icount, "fastsim instruction count", 1000)

CONF_V_BOOL (compressed_stats, "Compressed Stats ", true)

CONF_V_BOOL (separate_kbp, "separate kernel branch predictor", false)

CONF_V_BOOL (use_mem_hier, "use ms2sim memory hierarchy", true)
CONF_V_BOOL (use_processor, "use ms2sim processor", true)

CONF_V_BOOL (disambig_prediction, "use memory disambiguation prediction", false)
CONF_V_INTEGER (disambig_size, "size of memory disambig predictor", 128)
CONF_V_BOOL (prefetch_unresolved, "prefetch unresolved/safe loads", false)

CONF_V_BOOL (sync2_runahead, "don't squash and block fetch until sync2 at head", false)


// Checkpoint Stuff
CONF_V_STRING(processor_checkpoint_in, "Intermediate CheckPoint File", "")
CONF_V_STRING(processor_checkpoint_out, "Next Checkpoint File", "")

CONF_V_BOOL (use_mmu, "use ms2sim mmu (U2 only)", false)
CONF_V_BOOL (mmu_hw_fill, "try to hardware fill mmu misses (Linux only)", false)

CONF_V_INTEGER (memory_image_size, "memory image size. 0 for auto-probe.", 0)

CONF_V_BOOL (trace_commits, "print disassembled commits", false)
CONF_V_BOOL (trace_pipe, "print disassembled instr at each stage", false)
CONF_V_INTEGER (trace_cpu, "only trace this CPU (> num_cpus to trace all)", 999)

CONF_V_BOOL (line_prefetch, "Initiate next line prefetch from the fetch buffer", true)
CONF_V_BOOL (straight_line_prefetch, "Contiguous Line Prediction only", true)
CONF_V_INTEGER (line_pred_bits, "Prediction Table Size of Line Predictor", 10)

CONF_V_INTEGER (extra_seq, "Number of 'extra' sequencers", 0)

CONF_V_BOOL (separate_user_os, "Separate OS/User execution among cores", false)
CONF_V_INTEGER (num_user_seq, "Number of sequencers dedicated to user code", 0)
CONF_V_INTEGER (thread_preempt, "Cycles before preempting thread", 50000)


CONF_V_ARRAY_INT (chip_design, "Chip design specification", 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
CONF_V_ARRAY_INT (spin_loop_pcs, "PCs of spin loops", 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
CONF_V_INTEGER (mutex_locked_pc, "VPC of Solaris jump when mutex is locked", 0)
CONF_V_INTEGER (kernel_idle_enter, "VPC of entering kernel idle loop", 0)
CONF_V_INTEGER (kernel_idle_exit, "VPC of leaving kernel idle loop", 0)
CONF_V_INTEGER (kernel_scheduler_pc, "VPC of Solaris scheduler", 0)

CONF_V_ARRAY_INT (syscall_provision, "Syscall requiring multiple h/w contexts", 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
CONF_V_INTEGER (scheduling_algorithm, "Algorithm to use while mapping threads", 0)
CONF_V_BOOL (cache_only_provision, "Only study the cache behavior under provisioning", false)
CONF_V_INTEGER (thread_hop_interval, "Cycles between threads hop in asymmetric provision", 1000000)
CONF_V_ARRAY_INT (syscall_coupling, "Syscalls which match against each other ", 80, 3, 3, 4, 5, 6, 121, 122, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

CONF_V_BOOL (per_syscall_kern_stack, "Emulate a per-systemcall LWP stack (Solaris)", false)
CONF_V_BOOL (per_syscall_kern_map, "Emulate a per-systemcall LWP map (Solaris)", false)
CONF_V_BOOL (share_br_pred, "share the branch predictor", false)

CONF_V_BOOL (csp_use_mmu, "CSP uses mmu's correctly", false)
CONF_V_BOOL (csp_stop_tick, "Stop tick/stick updates when VCPUs are paused", false)


CONF_V_INTEGER (spinloop_threshold, "Number of unique store ", 5)
CONF_V_INTEGER (spin_check_interval, "Committed Instruction count between spinloop", 1000)
CONF_V_INTEGER (spin_detection_algorithm, "Algorithm for spin detection", 0)
CONF_V_BOOL (trigger_spin, "trigger on spin detection", true)
CONF_V_INTEGER (user_spin_factor, "Number of successive spin detection to trigger user spin", 0)
CONF_V_BOOL (spin_proc_mode, "No spin detection w/ proc_mode change", true)

CONF_V_INTEGER (store_count_factor, "Factor for store count", 1000)
CONF_V_INTEGER (load_count_factor, "Factor for load count", 1)
CONF_V_INTEGER (adaptive_pc_threshold, "Threshold for Adaptive PCs", 0)

CONF_V_BOOL (cache_vcpu_state, "Cache VCPU state_size, else use threaed_switch_lat", true)
CONF_V_INTEGER (state_size, "Processor state in 8byte words", 277)
CONF_V_INTEGER (thread_state_offset, "Address offset of VCPU states", 2944)
CONF_V_INTEGER (max_outstanding_thread_state, "Number of outstanding thread-switch reqs", 64)
CONF_V_BOOL (optimize_thread_state_addr, "Optimize addresses when sending thread state", true)
CONF_V_INTEGER (thread_switch_latency, "Cost of a thread switch (cycles)", 10)

CONF_V_INTEGER (periodic_interrupt, "Interrupt seq every, reguardless of switching time", 0)
CONF_V_INTEGER (long_thread_preempt, " Long thread preeempt", 0)
CONF_V_BOOL (no_os_pause, "Do not pause on OS for the first preempt", false)
CONF_V_BOOL (safe_trap, "Trap handling considered safe state for preemption", false)

// Related to early interrupt handling
CONF_V_INTEGER (transaction_leak_interval, "cycles between leaking transactions", 0)
CONF_V_BOOL (paused_vcpu_interrupt, "Determines early handling of paused VCPU interrupt. 0 to disable", false)
CONF_V_BOOL (software_interrupt_only, "Only handle on software interrupt", false)
CONF_V_INTEGER (interrupt_handle_cycle, "Cycles to execute for interrupt handling", 1000)

CONF_V_BOOL (observe_kstack, "Kernel stack operation", false)

// Temperature management
CONF_V_BOOL(enable_disable_core, "Periodically enable disable core", false)
CONF_V_INTEGER (disable_interval, "Interval between disabling core", 1000000)
CONF_V_INTEGER (disable_period, "Period of disabling core", 100000)
CONF_V_INTEGER (disable_event_count, "Number of disable events", 100000)
CONF_V_INTEGER (disable_interval_stddev, " standard deviation of disable", 1000)
CONF_V_INTEGER (disable_interval_mean, " Mean of disable_interval", 1000000)


// Faulty core management
CONF_V_INTEGER(disable_faulty_cores, "Disable N faulty cores at beginning", 0)
CONF_V_BOOL (optimize_faulty_core, "Optimize the Faulty Cores", false)
CONF_V_INTEGER (minm_successive_preempt, "Minimum interval between successive preempt", 5000)

// Unsafe store
CONF_V_BOOL (asi_ie_safe, "Inverse endian ASIs safe", false)
CONF_V_BOOL (asi_sec_safe, "Secondary and as-user ASIs safe", false)
CONF_V_BOOL (asi_blk_safe, "Block Access ASIs safe", false)
CONF_V_BOOL (ldd_std_safe, "LDDs and STDs safe", false)

// Interrupts
CONF_V_BOOL (stat_os_entry_only, "sys stats for OS entry only", false)
CONF_V_BOOL (interrupt_core_change, "Core change for interrupts recieved in a syscall", false)

CONF_V_INTEGER (num_machines, "number of consolidated machines", 1)
CONF_V_BOOL (override_idsr_read, "Override IDSR read to return 0 always", false)
CONF_V_BOOL (allow_tlb_shootdown, "Allow TLB Shootdown cross calls", false)

// Tracing stats
CONF_V_INTEGER (print_commit_stat_trace, "Print commits every N cycles", 0)
CONF_V_INTEGER (print_l1miss_stat_trace, "Print L1 cache miss every N cycles", 0)
CONF_V_INTEGER (print_l2miss_stat_trace, "Print L2 cache miss every N cycles", 0)
CONF_V_INTEGER (print_sync_stat_trace, "Print sync instr every N cycles", 0)

CONF_V_INTEGER(l2stripe_threshold, "Misses before switch is worthwhile", 0);

// SSP . TSP
CONF_V_BOOL (avoid_idle_context, "Avoid Idle context", false)
CONF_V_BOOL (eager_yield, "Eagerly yield thread when switching user/OS", true)
CONF_V_BOOL (copy_brpred_state, " Copy branch predictor state", true)
CONF_V_INTEGER (max_ssp_context, "Maximum h/w context for SAP", 4)
CONF_V_BOOL (avoid_idle_os, " Avoid Idle OS Cores ", true)


// Function Management
CONF_V_BOOL (v9_binary, " Binary is V9", false)
CONF_V_ARRAY_INT (function_map, "Function Map into different context", 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
CONF_V_STRING (function_file, "File containing function addresses", "")
CONF_V_ARRAY_INT (function_lib_offset, "File with Functions", 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, )
CONF_V_STRING (function_lib1, "File with first library", "")
CONF_V_STRING (function_lib2, "File with second library", "")
CONF_V_STRING (function_lib3, "File with third library", "")

// Operand Width Analysis
CONF_V_BOOL (operand_width_analysis, "Analysis of operand width", false)
CONF_V_BOOL (ignore_sllx, " Ignore sllx operations for 64 bits", false)
CONF_V_BOOL (ignore_cmp, " Ignore cmp operations ", true)
CONF_V_BOOL (operand_trace, "Print a Trace of operand width violations", false)
CONF_V_INTEGER (operand_dump_interval, "log (interval)", 10)
CONF_V_INTEGER (wide_operand_penalty, "Additional penalty for executing wide operand", 2)
CONF_V_ARRAY_INT (vary_datapath_width, " Varying Size for the data path width", 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

// Register Read Write
CONF_V_BOOL (analyze_register_dependency, "Analyze the register dependency for CR", false)

// Heterogeneous multicore
CONF_V_ARRAY_INT (vary_max_issue, "Varying Max Issue", 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
CONF_V_ARRAY_INT (vary_max_fetch, "Varying Max Fetch", 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
CONF_V_ARRAY_INT (vary_max_commit, "Varying Max Commit", 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
CONF_V_ARRAY_INT (vary_issue_inorder, "Varying Issue Type", 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)


// Power


CONF_V_BOOL (power_profile, "Power profile", false)
CONF_V_INTEGER (opcode_length, "Length of the opcode in the ISA", 8)
CONF_V_INTEGER (inst_length, "Length of the instruction in the ISA", 32)
CONF_V_INTEGER (data_width, "Length of the data", 64)
CONF_V_INTEGER (addr_width, " Address width", 64)
CONF_V_INTEGER (technology_node, "Technology node in nm", 70)
CONF_V_INTEGER (vdd_scaling_factor, " VDD scaling factor * 1000", 1000)
// di/dti
CONF_V_INTEGER (didt_interval, " Resonant period for didt calculation", 0)


// HOTSPOT
CONF_V_STRING (floorplan_file, "File for the core floorplan", "/afs/cs.wisc.edu/p/prometheus/private/kchak1/comp_spread/home/condor/core_new.45.flp")
CONF_V_STRING (hotspot_init_root, "File for the initial temperature", "/afs/cs.wisc.edu/p/prometheus/private/kchak1/comp_spread/home/condor/hotspot/base")
CONF_V_STRING (hotspot_init_suffix, "File for the initial temperature", "core_init_temp")
CONF_V_STRING (hotspot_steady_file, "Steady Temperature in Hotspot", "/afs/cs.wisc.edu/p/prometheus/private/kchak1/comp_spread/home/condor/core.steady")
CONF_V_INTEGER (temperature_update_interval, "log Cycles between invoking Hotspot 0 disables", 15)
CONF_V_BOOL (power_trace, "Put out Power Trace", false)

CONF_V_INTEGER (heatrun_migration_epoch, "Log Cycles between Heat Run Migration", 0)
CONF_V_INTEGER (chip_frequency, "Clock Frequency of the Chip in Mhz", 3000)
CONF_V_INTEGER (leakage_cycle_adjust, "Leakage factor adjustment for frequency scaling * 1000", 1000)

// Narrow core
CONF_V_BOOL (narrow_core_switch, "Switching into narrow core", false)
CONF_V_STRING (nc_migration_root, "directory containing PC lists for narrow core migration", "/afs/cs.wisc.edu/p/prometheus/private/kchak1/comp_spread/home/condor/nc_migration/")

