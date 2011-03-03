// Select the memory topology and protocols
// See README.txt for current reasonable options
//CONF_V_STRING(memory_topology, "mem topology and protocol", "unip-one-default")
CONF_V_STRING(memory_topology, "mem topology and protocol", "unip-one-two")
CONF_V_BOOL(hasdram, "uses dram", true)

// Not all cache params below are used for all protocols/configs
CONF_V_INTEGER(l1d_latency, "L1D hit latency", 1)
CONF_V_INTEGER(l1d_assoc, "L1D associativity", 2)
CONF_V_INTEGER(l1d_size, "L1D size (Kbytes)", 128)
CONF_V_INTEGER(l1d_banks, "# L1D banks", 2)
CONF_V_INTEGER(l1d_bank_bw_inv, "Inv of L1D bank b/w (cycles/req/bank); 0 for inf", 1)
CONF_V_INTEGER(l1d_req_q_size, "# of request queue entries per L1D bank", 0)
CONF_V_INTEGER(l1d_lsize, "L1D line size (bytes)", 64)
CONF_V_INTEGER(l1d_num_mshrs, "# L1D MSHRs", 8)
CONF_V_INTEGER(l1d_requests_per_mshr, "# L1D piggybacked requests per MSHR", 8)
CONF_V_INTEGER(l1d_writeback_buffers, "# L1D write-back buffer slote", 8);

CONF_V_INTEGER(l1i_latency, "L1I hit latency", 0)
CONF_V_INTEGER(l1i_assoc, "L1I associativity", 2)
CONF_V_INTEGER(l1i_size, "L1I size (Kbytes)", 128)
CONF_V_INTEGER(l1i_banks, "# L1I banks", 2)
CONF_V_INTEGER(l1i_bank_bw_inv, "Inv of L1I bank b/w (cycles/req/bank); 0 for inf", 1)
CONF_V_INTEGER(l1i_req_q_size, "# of request queue entries per L1I bank", 0)
CONF_V_INTEGER(l1i_lsize, "L1I line size (bytes)", 64)
CONF_V_INTEGER(l1i_num_mshrs, "# L1I MSHRs", 4)
CONF_V_INTEGER(l1i_requests_per_mshr, "# L1I piggybacked requests per MSHR", 8)
CONF_V_BOOL(l1i_next_line_prefetch, "L1I Perform next line miss prefetching", false)
CONF_V_INTEGER(l1i_next_line_pref_num, "L1I # of lines to prefetch", 1)

CONF_V_INTEGER(l2_latency, "L2 latency", 6)
CONF_V_INTEGER(l2_assoc, "L2 associativity", 4)
CONF_V_INTEGER(l2_size, "L2 size (Kbytes)", 1024)
CONF_V_INTEGER(l2_banks, "# L2 banks", 4)
CONF_V_INTEGER(l2_bank_bw_inv, "Inv of L2 bank b/w (cycles/req/bank); 0 for inf", 4)
CONF_V_INTEGER(l2_req_q_size, "# of request queue entries per L2 bank", 0)
CONF_V_INTEGER(l2_lsize, "L2 line size (bytes)", 64)
CONF_V_INTEGER(l2_num_mshrs, "# L2 of MSHRs", 16)
CONF_V_INTEGER(l2_requests_per_mshr, "# L2 piggybacked requests per MSHR", 8)
CONF_V_INTEGER(l2_writeback_buffers, "# L2 write-back buffer slote", 8)
CONF_V_INTEGER(l2dir_assoc_factor, "# reduction factor for L2 directory associativity", 1)
CONF_V_INTEGER(l2dir_assoc, " Fixed L2 Directory associativty", 0) 
CONF_V_INTEGER(l2dir_size, "FIXED Setup of L2 Directory Size", 0) 

CONF_V_INTEGER(l3_latency, "L3 latency", 20)
CONF_V_INTEGER(l3_assoc, "L3 associativity", 16)
CONF_V_INTEGER(l3_size, "L3 size (Kbytes)", 4096)
CONF_V_INTEGER(l3_banks, "# L3 banks", 4)
CONF_V_INTEGER(l3_bank_bw_inv, "Inv of L3 bank b/w (cycles/req/bank); 0 for inf", 4)
CONF_V_INTEGER(l3_req_q_size, "# of request queue entries per L3 bank", 0)
CONF_V_INTEGER(l3_lsize, "L3 line size (bytes)", 64)
CONF_V_INTEGER(l3_num_mshrs, "# L3 of MSHRs", 32)
CONF_V_INTEGER(l3_requests_per_mshr, "# L3 piggybacked requests per MSHR", 8)
CONF_V_INTEGER(l3_writeback_buffers, "# L3 write-back buffer slote", 8);
CONF_V_BOOL(l3_stride_prefetch, "Single data prefetch in L3", false)

CONF_V_INTEGER(l1dir_latency, "L1 Directory latency", 5)

CONF_V_INTEGER(main_mem_latency, "main memory latency (cycles)", 60)
CONF_V_INTEGER(main_mem_bandwidth, "main memory bandwidth (Bytes/1kcycles)", 8000) // 40 GB/s @ 5 ghz

// TODO: better link specification
CONF_V_INTEGER(default_link_latency, "default link latency (cycles)", 1)
CONF_V_INTEGER(default_messages_per_link, "default max messages per link", 0)
// Bus width specification
CONF_V_INTEGER(num_addr_bus, "Number of address bus", 4)
CONF_V_INTEGER(num_data_bus, "Number of data bus", 4)

// Should data be cached in the memhier?  Don't use.
CONF_V_BOOL(cache_data, "cache data in mem hier", false)
CONF_V_BOOL(break_data_mismatch, "break with a mem-hier/simics data mismatch", false);

// Should mem-hier cache accesses to cacheable I/O space?  Safe if !cache_data
CONF_V_BOOL(cache_io_space, "cache cacheable I/O space ops", true)
// Should mem-hier handle I/O device requests?  Safe to leave off if !cache_data
CONF_V_BOOL(handle_io_devices, "handle requests from I/O devices", false)
// Should mem-hier handle reads from I/O devices?  Safe to leave off even if !cache_data
CONF_V_BOOL(handle_iodev_reads, "handle read requests from I/O devices", false)
// Treat atomics as one transaction (for simpler protocols)
CONF_V_BOOL(single_trans_atomics, "treat atomics as a single transaction", true)
// Ignore uncacheable accesses (use for simpler protocols)
CONF_V_BOOL(ignore_uncacheable, "ignore uncacheable transactions", true)

// Insert a zero-cycle delay event on a L1 cache hit.  don't use anymore
CONF_V_BOOL(delay_zero_latency, "internal. zero-cycle delay for l1 hits", false)

CONF_V_BOOL(copy_transaction, "internal. make a copy of all transactions", false)

CONF_V_BOOL(randomize_mm, "randomize main memory latency", false)
CONF_V_INTEGER(random_mm_dist, "width of uniform random mem latency", 10)
CONF_V_INTEGER(random_seed, "random seed", 0)

CONF_V_BOOL(shadow_timing, "Change timing to minimize shadow-sim diffs", false)

// Warming up mem-hier
CONF_V_INTEGER(mem_hier_warmup, "Number of cycles to warm up and discard stats. 0 to disable", 0)
CONF_V_BOOL(fetch_profile, "Gathers various fetch related stats", false)
CONF_V_BOOL(watch_idle_loop, "Keep stats on Linux idle loop", false)
CONF_V_INTEGER(idle_min_pc, "Min ifetch addr considered idle", 99744) //0x0185a0
CONF_V_INTEGER(idle_max_pc, "Max ifetch addr considered idle", 99824) //0x0185f0

// mem-hier Response Tracking mechanism
CONF_V_BOOL(mem_hier_response_tracking, "Tracks Request Response of Mem-Hier", false)
CONF_V_INTEGER(mem_hier_response_threshold, "Maximum delay between request and response", 10000)
CONF_V_INTEGER(mem_hier_check_frequency, "Response Check Frequency", 10)

// Some boring stuff...
CONF_V_INTEGER(stats_print_cycle, "cycles between stats printing", 1000000)
CONF_V_INTEGER(quiesce_delta, "internal. checking for quiet system", 100)
CONF_V_INTEGER(max_classname_len, "internal. max chars in typeid name", 100)
CONF_V_BOOL(print_transition_stats, "Print list of all state transitions", false)

CONF_V_BOOL(profile_mem_addresses, "Keep stats on each mem address", false)
CONF_V_INTEGER(prof_ma_granularity_bits, "Log2 granularity", 6) // 64-bytes
CONF_V_INTEGER(prof_mostly_kernel_ratio, "Kernel % considered mostly kernel", 97)
CONF_V_INTEGER(prof_mostly_user_ratio, "User % considered mostly user", 95)
CONF_V_BOOL(prof_ma_ignore_ifetch, "Ignore instruction fetch accesses", false)
CONF_V_BOOL(prof_ma_ignore_stores, "Ignore store accesses", false)
CONF_V_BOOL(prof_ma_regwin_as_user, "Consider user spill/fills traps as user access", true)
CONF_V_BOOL(prof_ma_print_addresses, "Print each address line", false)

CONF_V_BOOL(profile_spin_locks, "Keep stats on spin locks", false)
CONF_V_BOOL(prof_sl_print_each_lock, "Print stats for each lock", false)
CONF_V_BOOL(profile_codeshare, "Profile code sharing", false)

// check Point
CONF_V_STRING(mem_hier_checkpoint_in, "Intermediate CheckPoint File", "")
CONF_V_STRING(mem_hier_checkpoint_out, "Next Checkpoint File", "")
CONF_V_STRING(workload_checkpoint, "Workload checkpoint", "")
// Warmup -- takes effect when run with processor disabled
CONF_V_INTEGER(warmup_transactions, "Number of transactions to warmup the mem-hier", 10)
CONF_V_INTEGER(random_addresses, "Number of Addresses to consider while generating random transactions", 0)
CONF_V_INTEGER (random_transaction_traffic, "Number of random requests sent ", 0)
CONF_V_INTEGER(random_transaction_frequency, "Number of cycles between bursts of Random Traffic", 1)

// Protocol specific options
CONF_V_BOOL(cmp_incl_l2_read_fwd, "L2 forwards shared reads to best L1", false);
CONF_V_INTEGER(cmp_interconnect_latency, "CMP interconnect latency (cycles)", 5)
CONF_V_INTEGER (cmp_additional_c2c_latency, "Additional Latency in Cache to Cache", 0)
CONF_V_INTEGER(cmp_interconnect_bw_addr, "Cycles to xmit an address message", 0)
CONF_V_INTEGER(cmp_interconnect_bw_data, "Cycles to xmit a data message", 0)
CONF_V_INTEGER(cmp_interconnect_topo, "Topology of CMP interconnect", 0)
CONF_V_BOOL(evict_singlets, "Evict blocks with minimum sharers", false)

// trans array size
CONF_V_INTEGER(mem_trans_count, "Number of mem trans to initiate", 500000)
CONF_V_INTEGER(warmup_l1_size, "Size of L1 in the warmup checkpoints", 32)
CONF_V_INTEGER(warmup_l2_size, "Size of L2 in the warmup checkpoints", 1024)
CONF_V_INTEGER(warmup_l3_size, "Size of L3 in the warmup checkpoints", 8192)

// inclusion in I-cache
CONF_V_BOOL(icache_inclusion, "enforce inclusion on I-cache", true)

CONF_V_BOOL(perfect_icache, "Use perfect Icache", false);
CONF_V_BOOL(perfect_dcache, "Use perfect dcache", false);
CONF_V_BOOL(perfect_store_access, "Perfect Access for Stores", false);
CONF_V_BOOL(no_c2c_latency, "No additional interconnect latency for C2C", false);
CONF_V_BOOL(mem_ignore_os, "Ignore OS memory refs", false);
CONF_V_BOOL(mem_ignore_user, "Ignore user memory refs", false);

CONF_V_BOOL(keep_thread_state, "Try to keep thread state on chip", true);
CONF_V_BOOL (tstate_l1_bypass, "Bypass L1 for thread_state trans", true)
CONF_V_BOOL(sep_kern_cache, "Separate private caches for user and OS", false);
CONF_V_BOOL(optimize_os_user_spills, "Send user Spills inside a syscall to user cache", false);
CONF_V_BOOL(optimize_os_lwp, "Send lwp accesses to home cache", false);
CONF_V_BOOL(machines_share_osinstr, "Multiple machines share OS instructions", 0)


CONF_V_INTEGER(sms_region_bits, "log2 of region size for SMS", 11)

CONF_V_BOOL(stackheap_profile, "Profile Stack and Heap Accesses", false)
CONF_V_BOOL (optimize_stack_write, "Optimize Stack Writes", false)
CONF_V_INTEGER(print_c2c_stat_trace, "Print cache2cache transfers", 0)
CONF_V_INTEGER(print_l1miss_addr_trace, "Print l1 miss addresses", 0)
CONF_V_INTEGER(print_l1miss_bank_trace, "Print l1 misses to banks", 0)
CONF_V_INTEGER(l2_stripe_size, "Size of bank address stripe (bits)", 0)

CONF_V_BOOL(ino_decode_instr_info, "Decode instruction info with inorder mode", false)
CONF_V_BOOL(ino_fake_cache_trace_mode, "Fake cache-trace-mode when in instr-trace-mode", false)
CONF_V_BOOL(check_silent_stores, "Check for silent stores", false)

CONF_V_BOOL(write_warmup_trace, "Create a mem access trace for warmup", false)
CONF_V_INTEGER(write_warmup_trace_num, "Number of trace transactions", 20000000)
CONF_V_INTEGER(write_warmup_trace_norepeat, "Avoid recent repeats", 50)
CONF_V_BOOL(read_warmup_trace, "Read a mem access trace to create warmup chkpt", false)
CONF_V_STRING(warmup_trace_file, "Trace file for mem access trace", "")


// POWER PROFILE
CONF_V_INTEGER (l1i_bank_leak, "Leakage of each L1 I cache banks power (mw)",  0)
CONF_V_INTEGER (l1d_bank_leak, "Leakage of each L1 D cache banks power (mw)",  0)
CONF_V_INTEGER (l2_bank_leak, "Leakage of each L2 cache banks power (mw)",  0)
CONF_V_INTEGER (cache_idle_leak, "Leakage factor in a drowsy cache * 1000", 200)
CONF_V_INTEGER (cache_power_calibration, " Power calibration for caches ", 1)
