
/* 
 *
 * description:    implements the interface to DRAMSim2
 * initial author: Dean Michael Ancajas
 *
 */

#include "definitions.h"
#include "protocols.h"
#include "device.h"
#include "link.h"
#include "message.h"
#include "event.h"
#include "debugio.h"
#include "verbose_level.h"
#include "transaction.h"
#include "config_extern.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "Callback.h"
#include "MemorySystem.h"

template <class prot_sm_t, class msg_t>
const uint8 dram_t<prot_sm_t, msg_t>::cache_link;
template <class prot_sm_t, class msg_t>
const uint8 dram_t<prot_sm_t, msg_t>::io_link;

//using namespace DRAMSim;

template <class prot_sm_t, class msg_t>
dram_t<prot_sm_t,msg_t>::dram_t(string name, uint32 num_links)
	:device_t(name,num_links)
{
	mem = new MemorySystem(0, "ini/DDR2_micron_16M_8b_x8_sg3E.ini","system.ini","/users/dean/Dropbox/ms2sim_smt/mem-hier/DRAMSim2/","/home/A01491758/Dropbox/ms2sim_smt/mem-hier/resultsfile",8192,4);
       	Callback_t *read_cb = new Callback<dram_t<prot_sm_t,msg_t>, void, uint, uint64_t, uint64_t,uint32_t>(this, &dram_t<prot_sm_t,msg_t>::read_complete);
        Callback_t *write_cb = new Callback<dram_t<prot_sm_t,msg_t>, void, uint, uint64_t, uint64_t,uint32_t>(this, &dram_t<prot_sm_t,msg_t>::write_complete);
	mem->RegisterCallbacks(read_cb, write_cb);

	num_processors = SIM_number_processors();

	g_cycles=0;	
	dmbr=g_conf_dmbr;
	ART.clear();
	LAT.clear();
	toggle.resize(g_conf_num_controllers,0);
	tMap.clear(); 
	lMap.clear(); 
	numaMap.clear();
	returnMap.clear();
	returnLatency.clear();
	transId=0;
	newTransId=0;
	//DRAM Stats
	num_requests = 0;
        stats = new stats_t();
        stats_print_enqueued = false;
        lat_viol = stats->COUNTER_BASIC("lat violations", "# of attempted expansion ART but violated LAT");
        victim_stat = stats->COUNTER_BASIC("victim dram page", "# of usage to victim dram page");
        art_cnt = stats->COUNTER_BASIC("size of ART", "# of created ART entries");
        lat_cnt = stats->COUNTER_BASIC("size of LAT", "# of create LAT entries to track local accesses");
	stat_commits = stats->COUNTER_BASIC("commits-dram","# of commits seen by dramsim");
	stat_hwprefetch = stats->COUNTER_BASIC("HW prefetch", "# of prefetch requests generated by hardware");
        stat_reusepages = stats->COUNTER_BASIC("reuse of ART entry", "# of requests that reuses remapped page ranges");
	stat_expansion = stats->COUNTER_BASIC("remapping by extension","# of expansion requests from ART entry");
        stat_requests = stats->COUNTER_BASIC("d_mem requests", "# of requests made to DRAMSim 2");
        stat_local = stats->COUNTER_BASIC("d_mem local requests", "# of requests made to local controller");
        stat_neighbor = stats->COUNTER_BASIC("d_mem neighbor(1-hop) requests", "# of requests made to neighboring controller");
        stat_remote = stats->COUNTER_BASIC("d_mem remote(2-hops) requests", "# of requests made to a remote controller");
	stat_dramlatency_histo = stats->HISTO_EXP2("dram latency", "distribution of dram latencies",10,1,1);
	stat_dramlatency_histo_local = stats->HISTO_EXP2("dram local latency", "distribution of local dram latencies",10,1,1);
	stat_dramlatency_histo_neighbor = stats->HISTO_EXP2("dram neighbor latency", "distribution of neighbor dram latencies",10,1,128);
	stat_dramlatency_histo_remote = stats->HISTO_EXP2("dram remote latency", "distribution of dram remote latencies",10,1,128);
	//stat_dramlatency_histo = stats->HISTO_UNIFORM("dram latency", "distribution of dram latencies",28,2,8);

	stat_request_distrib = stats->HISTO_UNIFORM("processor request distribution latency", "distribution of request from CPUs",8,1,0);//parametrize this
	stat_local_histo = stats->HISTO_UNIFORM("proc no", "local histo distrib",8,1,0);//parametrize this
	stat_nb_histo = stats->HISTO_UNIFORM("proc no.", "neighbor histo distrib",8,1,0);//parametrize this
	stat_remote_histo = stats->HISTO_UNIFORM("proc no..", "remote histo distrib",8,1,0);//parametrize this
	stat_rank_histo= stats->HISTO_UNIFORM("rank no","rank histo distrib",8,1,0);

//	cout<<"I am here!!!!\n";
}

template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t,msg_t>::printStats(){
	mem->printStats(true);
	//cout<<"total commits:"<<mem_hier_t::ptr()->get_commits()<<endl;
}


template <class prot_sm_t, class msg_t>
stall_status_t
dram_t<prot_sm_t,msg_t>::message_arrival(message_t * message)
{
	//mem->printBitWidths();
	msg_t *msg = static_cast <msg_t *> (message);
	mem_trans_t *mt = static_cast <mem_trans_t *> (message->get_trans());
	if (mt->hw_prefetch == 1) {
		STAT_INC(stat_hwprefetch);
		//cout<<" hw prefetch on address: "<<msg->address<<endl;
	}
	unsigned int cpu_id = static_cast<uint32>(message->get_trans()->cpu_id);

        VERBOSE_OUT(verb_t::mainmem,
                    "%10s @ %12llu 0x%016llx: mainmem message_arrival() %s\n",
                                get_cname(), external::get_current_cycle(),
                                msg->address, msg_t::names[msg->get_type()][0].c_str());

        ASSERT(msg->get_type() == msg_t::Read ||
               msg->get_type() == msg_t::ReadEx ||
                   msg->get_type() == msg_t::WriteBack || msg->get_type() == msg_t::DataResp);

    if (msg->get_type() == msg_t::DataResp) {
        delete msg;
        return StallNone;
    }

	if (STAT_GET(stat_commits) < mem_hier_t::ptr()->get_commits()){
		STAT_INC(stat_commits,mem_hier_t::ptr()->get_commits() - STAT_GET(stat_commits));
		//cout<<"commits: "<< mem_hier_t::ptr()->get_commits()<<endl;
	}

	//cout<<"address: "<<msg->address<<endl;

        // TODO: contention and banks
        /*tick_t lat = get_latency(message);
        if (g_conf_randomize_mm) {
                VERBOSE_OUT(verb_t::mainmem,
                    "%10s @ %12llu 0x%016llx: mainmem latency: %d\n",
                                get_cname(), external::get_current_cycle(),
                                msg->address, lat);
        }*/

        _event_t *e = new _event_t(msg, this, 0, event_handler);
        //e->enqueue();
	newTransId=transId++;
	tMap[newTransId]=e;	
	lMap[newTransId]=external::get_current_cycle();

  	Transaction tr;
	
	//if (cpu_id == 3) 
	//cout<<"address: "<<msg->address<<" stripped: "<< (msg->address & 0xEFFFFFFF) << " from cpu "<<cpu_id<<endl;
	if (msg->get_type() == msg_t::WriteBack)
	{
		//tr = new Transaction(DATA_WRITE, msg->address, NULL, newTransId);
		if (dmbr==false){
			numaMap[newTransId]=get_add_latency(cpu_id,msg->address);
  			mem->addTransaction(DATA_WRITE,msg->address,newTransId,0);
		}
		else
			TryToRemap(DATA_WRITE,msg->address,newTransId,cpu_id);
	}
	else
	{
		if (dmbr==false){
			numaMap[newTransId]=get_add_latency(cpu_id,msg->address);
  			mem->addTransaction(DATA_READ,msg->address,newTransId,0);
		}
		else
			TryToRemap(DATA_READ,msg->address,newTransId,cpu_id);
		//tr = new Transaction(DATA_READ, msg->address, NULL, newTransId);
	}
	
        num_requests++;
        STAT_INC(stat_requests);
	stat_request_distrib->inc_total(1,cpu_id);

//send this request to dramsim

	//cout<<"Processor #"<<cpu_id<<" requests address "<<hex<<msg->address<<" at rank "<<mem->getRank(msg->address)<<" with delay ";
	//cout<<dec<<get_add_latency(cpu_id,msg->address)<<endl;

        return StallNone;
}

template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t,msg_t>::advance_cycle(){

	if (g_cycles%2==0){	
		mem->update();
	}
	g_cycles++;
	//cout<<"dean"<<endl;
}

template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t, msg_t>::event_handler(_event_t *e)
{
        msg_t *msg = e->get_data();
        dram_t *dram = e->get_context();
        delete e;

        ASSERT(dram->num_requests > 0);
        dram->num_requests--;

	//cout<<"actually handled: "<<msg->address<<" on cycle "<<external::get_current_cycle()<<endl;
	//stat_dramlatency_histo->inc_total(1, external::get_current_cycle() - returnMap[msg->address] + returnLatency[msg->address]);
	
        if (msg->get_type() == msg_t::WriteBack) {
                if (g_conf_cache_data) {
                        dram->write_memory(msg->address, msg->size, msg->data);
                }


                
                // TEMP: if hooked directly to; should fix to better mechanism 
                if (msg->need_writeback_ack()) {
                        VERBOSE_OUT(verb_t::mainmem,
                                "%10s @ %12llu 0x%016llx: mainmem sending writeback ack\n",
                                                dram->get_cname(), external::get_current_cycle(),
                                                msg->address);
                        
                        msg->set_type(msg_t::WBAck);

                        uint32 link = (msg->io_type == IONone) ? cache_link : io_link;
                        stall_status_t ret = dram->links[link]->send(msg);
                        ASSERT(ret == StallNone);
                } else {
                        msg->get_trans()->clear_pending_event(MEM_EVENT_MAINMEM_WRITE_BACK);
                        msg->get_trans()->pending_messages--;

                        delete msg;
                }

        } else {

                // Main memory doesn't distinguish between read and read exclusive requests
                ASSERT(msg->data == NULL);
                if (g_conf_cache_data) {
                        msg->data = new uint8[msg->size];
                        dram->read_memory(msg->address, msg->size, msg->data);
                }

                VERBOSE_OUT(verb_t::mainmem,
                        "%10s @ %12llu 0x%016llx: mainmem sending data reponse\n",
                                        dram->get_cname(), external::get_current_cycle(),
                                        msg->address);

                msg->set_type(msg_t::DataResp);
                //msg->sender_id = 0;
                uint32 link = (msg->io_type == IONone) ? cache_link : io_link;
                stall_status_t ret = dram->links[link]->send(msg);
                ASSERT(ret == StallNone);
        }
}

template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t, msg_t>::to_file(FILE *file)
{
        // Output class name
        fprintf(file, "%s\n", typeid(this).name());
}

template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t, msg_t>::from_file(FILE *file)
{
        // Input and check class name
    bool condor_checkpoint = mem_hier_t::ptr()->is_condor_checkpoint();
    if (!condor_checkpoint)
        mem_hier_t::ptr()->cleanup_previous_stats(file, typeid(this).name());
        char classname[g_conf_max_classname_len];
        fscanf(file, "%s\n", classname);

    if (strcmp(classname, typeid(this).name()) != 0)
        DEBUG_OUT("Read %s   Require %s file_pos %d\n",classname,
            typeid(this).name(), ftell(file));
        ASSERT(strcmp(classname, typeid(this).name()) == 0);
}

template <class prot_sm_t, class msg_t>
bool
dram_t<prot_sm_t, msg_t>::is_quiet()
{
        return (num_requests == 0);
}

template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t, msg_t>::write_memory(addr_t addr, size_t size, uint8 *data)
{
        VERBOSE_OUT(verb_t::data,
                    "%10s @ %12llu 0x%016llx: dram writing %d bytes\n",
                                get_cname(), external::get_current_cycle(),
                                addr, size);
        external::write_to_memory(addr, size, data);
        VERBOSE_DATA(get_cname(), external::get_current_cycle(),
                                addr, size, data);
}

// Reads memory, stores into data array
// assumes data points to an array large enough to hold size bytes
template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t, msg_t>::read_memory(addr_t addr, size_t size, uint8 *data)
{
        VERBOSE_OUT(verb_t::data,
                    "%10s @ %12llu 0x%016llx: dram reading %d bytes\n",
                                get_cname(), external::get_current_cycle(),
                                addr, size);
        external::read_from_memory(addr, size, data);
        VERBOSE_DATA(get_cname(), external::get_current_cycle(),
                                addr, size, data);
}

template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t, msg_t>::read_complete(uint id, uint64_t address, uint64_t clock_cycle, uint32_t transId)//callback function when read completes
{
	//cout<<" read callback at address "<<hex<<address<<" cycle: ";
	//cout<<dec<<clock_cycle<<" transId: "<<transId<<endl;
	//cout<<"transId returned: "<<external::get_current_cycle()<<" with address "<<address<<endl;
	//tMap[transId]->enqueue(); //schedule event immediately
	tMap[transId]->requeue(numaMap[transId]); //schedule event with additional delay


	if ( numaMap[transId] == 0 ){
		stat_dramlatency_histo_local->inc_total(1, external::get_current_cycle() - lMap[transId]);
	}
	else if ( numaMap[transId] == HOP_COST ) {
		stat_dramlatency_histo_neighbor->inc_total(1, external::get_current_cycle() - lMap[transId] + numaMap[transId]);
	}
	else if ( numaMap[transId] == 2 * HOP_COST ) {
		stat_dramlatency_histo_remote->inc_total(1, external::get_current_cycle() - lMap[transId] + numaMap[transId]);
	}
	else
		ASSERT(1==0);//something wrong here
	
	tMap.erase(transId);//delete entry in map
	ASSERT(external::get_current_cycle() > lMap[transId]);

	if (mem_hier_t::ptr()->get_commits() > g_conf_dram_warmup){
		stat_dramlatency_histo->inc_total(1, external::get_current_cycle() - lMap[transId]);
	}

	lMap.erase(transId);//delete entry in map
	numaMap.erase(transId);

	returnMap[address]=external::get_current_cycle();
	returnLatency[address]=external::get_current_cycle()-lMap[transId]; //optimize this later
}

template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t, msg_t>::write_complete(uint id, uint64_t address, uint64_t clock_cycle, uint32_t transId)//callback function when write completes
{
	//cout<<" write callback at address "<<hex<<address<<" cycle: ";
	//cout<<clock_cycle<<" transId: "<<transId<<endl;
	//cout<<"transId returned: "<<external::get_current_cycle()<<" with address "<<address<<endl;
	//tMap[transId]->enqueue(); //schedule event immediately
	tMap[transId]->requeue(numaMap[transId]); //schedule event with additional delay

	if ( numaMap[transId] == 0 ){
		stat_dramlatency_histo_local->inc_total(1, external::get_current_cycle() - lMap[transId]);
	}
	else if ( numaMap[transId] == HOP_COST ) {
		stat_dramlatency_histo_neighbor->inc_total(1, external::get_current_cycle() - lMap[transId] + numaMap[transId]);
	}
	else if ( numaMap[transId] == 2 * HOP_COST ) {
		stat_dramlatency_histo_remote->inc_total(1, external::get_current_cycle() - lMap[transId] + numaMap[transId]);
	}
	else
		ASSERT(1==0);//something wrong here

	tMap.erase(transId);//delete entry in map
	ASSERT(external::get_current_cycle() > lMap[transId]);

	if (mem_hier_t::ptr()->get_commits() > g_conf_dram_warmup){
		stat_dramlatency_histo->inc_total(1, external::get_current_cycle() - lMap[transId]);
	}

	lMap.erase(transId);//delete entry in map
	numaMap.erase(transId);

	returnMap[address]=external::get_current_cycle();
	returnLatency[address]=external::get_current_cycle()-lMap[transId]; //optimize this later
}


template <class prot_sm_t, class msg_t>
void 
power_callback(double a, double b, double c, double d)
{
        //printf("power callback: %0.3f, %0.3f, %0.3f, %0.3f\n",a,b,c,d);
}

template <class prot_sm_t, class msg_t>
int
dram_t<prot_sm_t, msg_t>::get_add_latency(unsigned int proc, uint64_t addr)
{
	unsigned int delay = HOP_COST;
	//cout << "proc "<<proc << " accesses "<<addr<< " which is on Rank " <<mem->getRank(addr)<<endl;
	if (g_conf_num_controllers==1)
	{
		return 0;
	}
	else 
	{
		unsigned int rank, distance,procGroupMap,memGroupMap;
		rank = mem->getRank(addr);
		procGroupMap = proc % g_conf_num_controllers;
		memGroupMap = rank % g_conf_num_controllers;
		//distance = abs(proc%g_conf_num_controllers - rank%g_conf_num_controllers);		

		stat_rank_histo->inc_total(1,rank);
		
		if (procGroupMap == 0 && (memGroupMap == 1 || memGroupMap == 2))
		{
			distance = 1;
			STAT_INC(stat_neighbor);
			stat_nb_histo->inc_total(1,proc);
		}
		else if (procGroupMap == 0 && memGroupMap == 3)
		{
			distance = 2;
			STAT_INC(stat_remote);
			stat_remote_histo->inc_total(1,proc);
		}
		else if (procGroupMap == 1 && (memGroupMap == 0 || memGroupMap == 3))
		{
			distance = 1;
			STAT_INC(stat_neighbor);
			stat_nb_histo->inc_total(1,proc);
		}
		else if (procGroupMap == 1 && memGroupMap == 2)
		{
			distance = 2;
			STAT_INC(stat_remote);
			stat_remote_histo->inc_total(1,proc);
		}
		else if (procGroupMap == 2 && (memGroupMap == 0 || memGroupMap == 3))
		{
			distance = 1;
			STAT_INC(stat_neighbor);
			stat_nb_histo->inc_total(1,proc);
		}
		else if (procGroupMap == 2 && memGroupMap == 1)
		{
			distance = 2;
			STAT_INC(stat_remote);
			stat_remote_histo->inc_total(1,proc);
		}
		else if (procGroupMap == 3 && (memGroupMap == 1 || memGroupMap == 2))
		{
			distance = 1;
			STAT_INC(stat_neighbor);
			stat_nb_histo->inc_total(1,proc);
		}
		else if (procGroupMap == 3 && memGroupMap == 0)
		{
			distance = 2;
			STAT_INC(stat_remote);
			stat_remote_histo->inc_total(1,proc);
		}
		else if (procGroupMap == memGroupMap){
			distance = 0;
			STAT_INC(stat_local);
			stat_local_histo->inc_total(1,proc);
		//	cout<<"proc "<< proc << " local access"<<endl;
		}
		else 
		{
			ASSERT(0);//unknown mapping
		}
			return distance*delay;		
	}
}


template <class prot_sm_t, class msg_t>
bool
dram_t<prot_sm_t, msg_t>::isLocalReq(unsigned int proc, uint64_t addr){
	unsigned int rank, procGroupMap,memGroupMap;
	rank = mem->getRank(addr);
	procGroupMap = proc % g_conf_num_controllers;
	memGroupMap = rank % g_conf_num_controllers;
	
	if (procGroupMap == memGroupMap){
		//cout<<"p: "<<proc<<"rank: "<<rank<<endl;
		return true;
	}
	else 
		return false;
}

template <class prot_sm_t, class msg_t>
int
dram_t<prot_sm_t, msg_t>::inART(addr_t addr){
        for (unsigned int j =0; j < ART.size();j++){
                if (addr >= ART[j].beginAdd && addr <= ART[j].endAdd){
			if ( j % 2 != 0 ) {
			  STAT_INC(victim_stat);
			}
                        return j;
                }
        }
	//cout<<hex<<"cannot find: "<<addr<<endl;
	//cout<<dec;
	return -1;
}

template <class prot_sm_t, class msg_t>
bool
dram_t<prot_sm_t, msg_t>::onePageFrART(addr_t addr){
	for (unsigned int j =0; j < ART.size();j++){
		if (g_conf_DMAPSE==true)
		if (addr <= ART[j].endAdd + PAGE_SIZE && addr >= ART[j].beginAdd )
			return true; 
		else
		if (addr <= ART[j].endAdd + CACHE_LSIZE && addr >= ART[j].beginAdd )
			return true; 
	}
	return false;
}

template <class prot_sm_t, class msg_t>
bool
dram_t<prot_sm_t, msg_t>::latViolation(addr_t addr){
	//once we are here then we know its already 1 page size from an ART entry
	unsigned k;
	for ( k = 0; k < ART.size();k++ ){
		if ( addr <= ART[k].endAdd + PAGE_SIZE && addr >= ART[k].beginAdd ){
			break;
		}
	}

	int partner;

	if (k%2==0)
	   partner=k+1;
	else
	   partner=k-1;

	long long padd,eraser;	
	eraser=0xE;
	eraser=eraser<<32;	
	eraser=~eraser;
	padd=ART[partner].oldRank;
	padd=padd<<33;
	eraser=eraser&addr;	
	padd=padd ^ eraser;
	
	//cout<<"partner new rank:"<<ART[partner].oldRank<<"partner oldRank: "<<ART[partner].newRank<<endl;
	//cout<<hex<<"expanding add: "<<addr<<" corresponding: "<<padd<<endl;
	for (unsigned int j = 0; j < LAT.size();j++){
		if (addr >= LAT[j].beginAdd && addr <= LAT[j].endAdd){
			return true;
		}//address violation itself

		if ( ART[k].endAdd + PAGE_SIZE >= LAT[j].beginAdd && ART[k].endAdd + PAGE_SIZE <= LAT[j].endAdd ) {
			return true;
		}//violation at possible end address of expanded ART

		if (padd >= LAT[j].beginAdd && padd <= LAT[j].endAdd){
			return true;
		}//violation of possible translated address at partner ART entry

		if ( ART[partner].endAdd + PAGE_SIZE >= LAT[j].beginAdd && ART[partner].endAdd + PAGE_SIZE <= LAT[j].endAdd ) {
			return true;
		}//violation at partner ART entry expansion

	}

	return false;
}

template <class prot_sm_t, class msg_t>
addr_t
dram_t<prot_sm_t, msg_t>::Translate(addr_t addr, int ARTindex,int callnum){
//Scheme 4 of DRAMSim2
// chan::rank::bank::row::col::byteoffset
// 0::   3   :: 3  :: 14::10 :: 6
	//cout<<"tr address:"<<hex<<addr<<" ind: ";
	//cout<<dec<<ARTindex<<endl;
	//cout<<"call num: " <<callnum<<endl;
	ASSERT(ARTindex > -1);
	ASSERT(addr <= ART[ARTindex].endAdd);
	ASSERT(addr >= ART[ARTindex].beginAdd);
	//cout<<dec<<"index: " <<ARTindex<<endl;
	//cout<<"address: "<<hex<<addr<<" is between "<<ART[ARTindex].beginAdd<<" and " << ART[ARTindex].endAdd<<endl;
	long long newAdd,holder,addr2;
	addr2=addr;	
	newAdd = ART[ARTindex].newRank;
	newAdd = newAdd << 33; // all bits except rank total to 33;
	holder=0xE;
	holder=holder<<32;
	holder=~holder;
	//cout<<hex<<"holder: "<<holder<<" address2: "<<addr2<<" newadd: "<<newAdd<<" ";
	holder = addr2 & holder;
	//cout<<" holder: "<<holder;
	newAdd = newAdd ^ holder;

	//cout<<" oldrank: " <<ART[ARTindex].oldRank<<" newrank: "<<ART[ARTindex].newRank;
	//cout<<hex<<" old: " <<addr<<" new: " << newAdd<<endl;
	//cout<<dec;

	return newAdd;
}

template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t, msg_t>::createLAT(addr_t addr){

	for (unsigned int j =0; j < LAT.size();j++){
		if (addr >= LAT[j].beginAdd && addr <= LAT[j].endAdd){
			//cout<<"no need to add"<<endl;
			return ;
		}
	}
	if (g_conf_DMAPSE==true)
		LAT.push_back( LATEntry( addr, addr + PAGE_SIZE ));
	else
		LAT.push_back( LATEntry( addr, addr + CACHE_LSIZE ));
	STAT_INC(lat_cnt);
	//cout<<"LAT entries:"<<LAT.size()<<endl;
	//cout<<"begin: "<<LAT[LAT.size()-1].beginAdd<<" end: "<<LAT[LAT.size()-1].endAdd<<endl;

}

template <class prot_sm_t, class msg_t>
int
dram_t<prot_sm_t, msg_t>::createART(addr_t addr,unsigned int proc){
        unsigned int rank, distance,procGroupMap,memGroupMap;
        rank = mem->getRank(addr);
        procGroupMap = proc % g_conf_num_controllers;
        memGroupMap = rank % g_conf_num_controllers;
	long long addr2,eraser;

	unsigned int newRank = procGroupMap + ( toggle[procGroupMap] * g_conf_num_controllers );

	if (toggle[procGroupMap] == 1)
		toggle[procGroupMap] = 0;
	else
		toggle[procGroupMap] = 1;
	
	ART.push_back(ARTEntry(rank,newRank,addr,addr+PAGE_SIZE));
	cout<<"ART: "<<ART.size()-1<<hex<<" b: "<<ART[ART.size()-1].beginAdd<<" e: "<<ART[ART.size()-1].endAdd;
	cout<<dec<<" old: "<<rank<<" new: "<<newRank<<endl;
	STAT_INC(art_cnt);

	//also add twin address	
	addr2=newRank;
	addr2=addr2<<33;
	//cout<<hex<<addr2;
	eraser=0xE;
	eraser=eraser<<32;
	eraser=~eraser;
	addr = addr & eraser;
	//cout<<" "<<addr<<endl;;
	addr2=addr2 ^ addr;
	
	if (g_conf_DMAPSE==true)	
		ART.push_back(ARTEntry(newRank,rank,addr2,addr2+PAGE_SIZE));
	else
		ART.push_back(ARTEntry(newRank,rank,addr2,addr2+CACHE_LSIZE));

	cout<<"ART: "<<ART.size()-1<<hex<<" b: "<<ART[ART.size()-1].beginAdd<<" e: "<<ART[ART.size()-1].endAdd;
	cout<<dec<<" old: "<<newRank<<" new: "<<rank<<endl;

	STAT_INC(art_cnt);
	return (ART.size()-1);
	
}

template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t, msg_t>::expandART(addr_t addr){
	//cout<<hex<<addr<<" provisioning "<<endl;
	for (unsigned int j =0; j < ART.size();j++){
		if ( g_conf_DMAPSE == true )
		if ( addr <= ART[j].endAdd + PAGE_SIZE && addr >= ART[j].beginAdd ){
	//		cout<<"expanded entry: "<<j<<" of "<<ART.size()<<hex<<" old end: "<<ART[j].endAdd;
			ART[j].endAdd+=PAGE_SIZE;
	//		cout<<" new end: " << ART[j].endAdd<<endl;
			break;
		}
		else
		if ( addr <= ART[j].endAdd + CACHE_LSIZE && addr >= ART[j].beginAdd ){
			ART[j].endAdd+=CACHE_LSIZE;
			break;
		}
		
	}
	//cout<<"expanded ART entry"<<endl;
}


template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t, msg_t>::TryToRemap(bool isWrite, uint64_t addr, uint32_t _transId, uint32_t proc){

	if ( isLocalReq(proc,addr)==true ){
		//cout<<" local request "<<endl;
		//cout << " creating local access table entry " << endl;
		createLAT(addr);
		//sending request
		mem->addTransaction(isWrite,addr,_transId,proc);
		numaMap[newTransId]=get_add_latency(proc,addr);
		ASSERT(numaMap[newTransId]==0);
		//cout<<"creating new LAT"<<endl;
	}
	else {
		if ( inART(addr) > -1 ){
			//cout<<"here 1"<<endl;
			mem->addTransaction(isWrite,Translate(addr,inART(addr),1),_transId,proc);
			numaMap[newTransId]=get_add_latency(proc,Translate(addr,inART(addr),2));
			//cout<<"re-using ART entry"<<endl;
			//numaMap
			STAT_INC(stat_reusepages);
			//cout<<"here 1a"<<endl;
		}
		else {//see if we can expand a current ART entry w/o violating previous local access
		    if ( onePageFrART(addr) == true) {
			if (latViolation(addr)==true){ //send them normally
				mem->addTransaction(isWrite,addr,_transId,proc);
				numaMap[newTransId]=get_add_latency(proc,addr);
				STAT_INC(lat_viol);
				//cout<<"lat violation: cannot expand"<<endl;
			}
			else {//then we can expand an ART entry and continue to use remapped space
				STAT_INC(stat_expansion);
			//	cout<<"here 2"<<endl;
				expandART(addr);
				mem->addTransaction(isWrite,Translate(addr,inART(addr),3),_transId,proc);
				//numaMap[newTransId]=get_add_latency(proc,Translate(addr,inART(addr),4));
				if (g_conf_DMAPSE==true)
					if ( g_conf_no_add_latency == true )
					//	numaMap[newTransId]=get_add_latency(proc,temp);
						numaMap[newTransId]=get_add_latency(proc,Translate(addr,inART(addr),4));
					else
						numaMap[newTransId]=PSE_COST;
				else
					numaMap[newTransId]=CLE_COST;
				//cout<<"expanding"<<endl;
			//	cout<<"here 2a"<<endl;
			}
		    }	
		    else {
			int maxentry;
			if (g_conf_DMAPSE==true)
				maxentry=ART_MAX;
			else
				maxentry=ART_MAX2;
			if (ART.size() == maxentry ){ //ART is full, send normally
				mem->addTransaction(isWrite,addr,_transId,proc);
				numaMap[newTransId]=get_add_latency(proc,addr);
				//cout<<"full"<<endl;
			}
			else {
				//cout<<"new ART entry for "<<hex<<addr<<endl;
			   	createART(addr,proc);
				uint64_t temp=Translate(addr,inART(addr),5);
				mem->addTransaction(isWrite,temp,_transId,proc);
				
				if (g_conf_DMAPSE==true)
					if ( g_conf_no_add_latency == true )
					numaMap[newTransId]=get_add_latency(proc,temp);
					else
					numaMap[newTransId]=PSE_COST;
				else
					numaMap[newTransId]=CLE_COST;
				//cout<<"here 3a"<<endl;
				//cout<<hex<<addr<<" here 3"<<endl;
			}
		    }	
		}
	}
	
}
