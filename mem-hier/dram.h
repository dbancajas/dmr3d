

/*
 * description: implements the interface to DRAMSim2
 * initial author: Dean Michael Ancajas
 *
 */


#ifndef _DRAM_H_
#define _DRAM_H_

#include "MemorySystem.h" 
#include "device.h"

class mem_hier_t;
template <class prot_t, class msg_t>
class dram_t : public device_t {

 public:
	typedef event_t<msg_t,dram_t> _event_t;
	typedef map<uint32_t,_event_t*,std::less <int> > TransactionMap;
	typedef map<uint32_t,uint64_t,std::less<int> > LatencyMap;
	typedef map<uint64_t,uint64_t,std::less<int> > AddrLatencyMap;	
	
	
	dram_t(string name, uint32 num_links);
	virtual ~dram_t() { }
	virtual stall_status_t message_arrival(message_t *message);
	
	virtual void from_file(FILE *file);
	virtual void to_file(FILE *file);
	virtual bool is_quiet();
	void advance_cycle();
	void printStats();

	static const uint8 cache_link = 0; //understand this
	static const uint8 io_link = 1;//understand this	

 protected:
        MemorySystem *mem;
 	int transId; //unique transaction Id for all transactions sent to DRAM
	int newTransId;
	int num_processors;
	tick_t g_cycles;
	TransactionMap tMap;//transaction map
	LatencyMap lMap;
	LatencyMap numaMap; //numa additional latency map
	AddrLatencyMap returnMap;//transaction address and cycle return from dram
	AddrLatencyMap returnLatency;
	tick_t latency;
	uint32 num_requests; //current number of outstanding requests
	vector<msg_t> queue; //queue of transaction waiting for callbacks before firing event	

	int findIndex(int transId); //finds index in queue of messages

	void write_memory(addr_t addr, size_t size, uint8 *data);
	void read_memory(addr_t addr, size_t size, uint8 *data);
	static void event_handler(_event_t *e);
	//tick_t get_latency(message_t *message);

	void read_complete(uint id, uint64_t address, uint64_t clock_cycle, uint32_t transId);//callback for dramsim
	void write_complete(uint id, uint64_t address, uint64_t clock_cycle, uint32_t transId);//callback for dramsim
	void power_callback(double a, double b, double c, double d);//power callback for dramsim
	int get_add_latency(unsigned int proc, uint64_t addr);//returns additional latency due to NUMA

	//stats stuff
	st_entry_t *stat_requests;	
	st_entry_t *stat_neighbor;
	st_entry_t *stat_remote;
	st_entry_t *stat_local;
	histo_1d_t *stat_dramlatency_histo;
	histo_1d_t *stat_request_distrib;

	bool stats_print_enqueued; //figure out what this is for
};

#include "dram.cc"

#endif //_DRAM_H_
