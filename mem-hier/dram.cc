
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
	mem = new MemorySystem(0, "ini/DDR2_micron_16M_8b_x8_sg3E.ini","system.ini","/users/dean/Dropbox/ms2sim_smt/mem-hier/DRAMSim2/","/users/dean/Dropbox/ms2sim_smt/mem-hier/resultsfile",1024,4);
       	Callback_t *read_cb = new Callback<dram_t<prot_sm_t,msg_t>, void, uint, uint64_t, uint64_t,uint32_t>(this, &dram_t<prot_sm_t,msg_t>::read_complete);
        Callback_t *write_cb = new Callback<dram_t<prot_sm_t,msg_t>, void, uint, uint64_t, uint64_t,uint32_t>(this, &dram_t<prot_sm_t,msg_t>::write_complete);
	mem->RegisterCallbacks(read_cb, write_cb);

	g_cycles=0;	
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
        stat_requests = stats->COUNTER_BASIC("d_mem requests", "# of requests made to DRAMSim 2");
        stat_local = stats->COUNTER_BASIC("d_mem local requests", "# of requests made to local controller");
        stat_neighbor = stats->COUNTER_BASIC("d_mem neighbor(1-hop) requests", "# of requests made to neighboring controller");
        stat_remote = stats->COUNTER_BASIC("d_mem remote(2-hops) requests", "# of requests made to a remote controller");
	stat_dramlatency_histo = stats->HISTO_EXP2("dram latency", "distribution of dram latencies",10,1,1);
	//stat_dramlatency_histo = stats->HISTO_UNIFORM("dram latency", "distribution of dram latencies",28,2,8);
	stat_request_distrib = stats->HISTO_UNIFORM("processor request distribution latency", "distribution of request from CPUs",8,1,0);//parametrize this
	cout<<"I am here!!!!\n";
	num_processors = SIM_number_processors();
}

template <class prot_sm_t, class msg_t>
void
dram_t<prot_sm_t,msg_t>::printStats(){
	mem->printStats(true);
}


template <class prot_sm_t, class msg_t>
stall_status_t
dram_t<prot_sm_t,msg_t>::message_arrival(message_t * message)
{
	msg_t *msg = static_cast <msg_t *> (message);
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

	if (msg->get_type() == msg_t::WriteBack)
	{
		//tr = new Transaction(DATA_WRITE, msg->address, NULL, newTransId);
		numaMap[newTransId]=get_add_latency(cpu_id,mem->getRank(msg->address));
  		mem->addTransaction(DATA_WRITE,msg->address,newTransId);
	}
	else
	{
		//numaMap[newTransId]=get_add_latency(cpu_id);
		numaMap[newTransId]=get_add_latency(cpu_id,mem->getRank(msg->address));
  		mem->addTransaction(DATA_READ,msg->address,newTransId);
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
	mem->update();
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
	
	tMap.erase(transId);//delete entry in map
	ASSERT(external::get_current_cycle() > lMap[transId]);
	stat_dramlatency_histo->inc_total(1, external::get_current_cycle() - lMap[transId]);
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
	tMap.erase(transId);//delete entry in map
	ASSERT(external::get_current_cycle() > lMap[transId]);
	stat_dramlatency_histo->inc_total(1, external::get_current_cycle() - lMap[transId]);
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
	unsigned int delay = 4;

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
		
		if (procGroupMap == 0 && (memGroupMap == 1 || memGroupMap == 2))
		{
			distance = 1;
			STAT_INC(stat_neighbor);
		}
		else if (procGroupMap == 0 && memGroupMap == 3)
		{
			distance = 2;
			STAT_INC(stat_remote);
		}
		else if (procGroupMap == 1 && (memGroupMap == 0 || memGroupMap == 3))
		{
			distance = 1;
			STAT_INC(stat_neighbor);
		}
		else if (procGroupMap == 1 && memGroupMap == 2)
		{
			distance = 2;
			STAT_INC(stat_remote);
		}
		else if (procGroupMap == 2 && (memGroupMap == 0 || memGroupMap == 3))
		{
			distance = 1;
			STAT_INC(stat_neighbor);
		}
		else if (procGroupMap == 2 && memGroupMap == 1)
		{
			distance = 2;
			STAT_INC(stat_remote);
		}
		else if (procGroupMap == 3 && (memGroupMap == 1 || memGroupMap == 2))
		{
			distance = 1;
			STAT_INC(stat_neighbor);
		}
		else if (procGroupMap == 3 && memGroupMap == 0)
		{
			distance = 2;
			STAT_INC(stat_remote);
		}
		else if (procGroupMap == memGroupMap){
			distance = 0;
			STAT_INC(stat_local);
		}
		else 
		{
			ASSERT(0);//unknown mapping
		}
			return distance*delay;		
	}
}
