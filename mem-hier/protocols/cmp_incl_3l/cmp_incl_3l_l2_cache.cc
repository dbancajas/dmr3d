/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:	implementation of a two-level CMP L2 cache
 * initial author: Koushik Chakraborty 
 *
 */
 
 
//  #include "simics/first.h"
// RCSID("$Id: cmp_incl_3l_l2_cache.cc,v 1.1.2.19 2006/03/21 19:17:07 kchak Exp $");

#include "definitions.h"
#include "protocols.h"
#include "statemachine.h"
#include "device.h"
#include "message.h"
#include "cache.h"
#include "tcache.h"
#include "external.h"
#include "transaction.h"
#include "debugio.h"

#include "cmp_incl_3l_l2_cache.h"
#include "cmp_incl_3l_l3_cache.h"

const uint8 cmp_incl_3l_l2_protocol_t::interconnect_link;
const uint8 cmp_incl_3l_l2_protocol_t::icache_link;
const uint8 cmp_incl_3l_l2_protocol_t::dcache_link;
const uint8 cmp_incl_3l_l2_protocol_t::icache_id;
const uint8 cmp_incl_3l_l2_protocol_t::dcache_id;

/////////////////////////////////////////////////////////////////////////////////
// Uniprocessor one-level line functions
//
void
cmp_incl_3l_l2_line_t::init(cache_t *_cache, addr_t _idx, uint32 _lsize)
{
	line_t::init(_cache, _idx, _lsize);
	state = StateInvalid;
	dirty = false;
	icache_incl = dcache_incl = false;
	requester = (uint32) -1;
}

void
cmp_incl_3l_l2_line_t::wb_init(addr_t tag, addr_t idx, line_t *cache_line)
{
	line_t::wb_init(tag, idx, cache_line);
	state = StateUnknown;
	dirty = true;
	icache_incl = dcache_incl = false;
	requester = (uint32) -1;
	
	if (cache_line) {
		cmp_incl_3l_l2_line_t *line = static_cast <cmp_incl_3l_l2_line_t *> (cache_line);

		icache_incl = line->icache_incl;
		dcache_incl = line->dcache_incl;

		if (line->get_state() == StateShared) {
			if (dcache_incl || icache_incl)
				state = StateShToShToInv;
			else 
				state = StateShToInv;
		}
		else if (line->get_state() == StateModified || 
			line->get_state() == StateModifiedShared)
		{ 
			if (dcache_incl || icache_incl)
				state = StateModToModToInv;
			else 
				state = StateModToInv;
		}
		else
			FAIL;
	}
}

void
cmp_incl_3l_l2_line_t::reinit(addr_t _tag)
{
	line_t::reinit(_tag);
	state = StateInvalid;
	dirty = false;
	icache_incl = dcache_incl = false;
	requester = (uint32) -1;
}	
	
bool
cmp_incl_3l_l2_line_t::is_free()
{
	return (state == StateInvalid);
}	
	
bool
cmp_incl_3l_l2_line_t::can_replace()
{
	// Can replace if not in a busy state
	return (state == StateInvalid ||
			state == StateNotPresent ||
			state == StateShared ||
			state == StateModified ||
			state == StateModifiedShared);
}

bool
cmp_incl_3l_l2_line_t::is_stable()
{
	// Same as can_replace
	return (can_replace());
}

void
cmp_incl_3l_l2_line_t::set_state(uint32 _state)
{
	state = _state;
}
	
uint32
cmp_incl_3l_l2_line_t::get_state()
{
	return state;
}

void
cmp_incl_3l_l2_line_t::set_requester(uint32 _requester)
{
	requester = _requester;
}
	
uint32
cmp_incl_3l_l2_line_t::get_requester()
{
	return requester;
}

void
cmp_incl_3l_l2_line_t::set_dirty(bool _dirty)
{
	dirty = _dirty;
}
	
bool
cmp_incl_3l_l2_line_t::is_dirty()
{
	return dirty;
}

void
cmp_incl_3l_l2_line_t::set_icache_incl(bool incl)
{
	icache_incl = incl;
}
	
bool
cmp_incl_3l_l2_line_t::get_icache_incl()
{
	return icache_incl;
}
	
void
cmp_incl_3l_l2_line_t::set_dcache_incl(bool incl)
{
	dcache_incl = incl;
}
	
bool
cmp_incl_3l_l2_line_t::get_dcache_incl()
{
	return dcache_incl;
}

void
cmp_incl_3l_l2_line_t::to_file(FILE *file)
{
	fprintf(file, "%d %u %d %d %u ", dirty ? 1 : 0, state,
		icache_incl ? 1 : 0, dcache_incl ? 1 : 0, requester);
	line_t::to_file(file);
}

void
cmp_incl_3l_l2_line_t::from_file(FILE *file)
{
	int temp_dirty, temp_ii, temp_di;
	fscanf(file, "%d %u %d %d %u ", &temp_dirty, &state, &temp_ii, &temp_di, 
		&requester);
	dirty = (temp_dirty == 1);
	icache_incl = (temp_ii == 1);
	dcache_incl = (temp_di == 1);

	line_t::from_file(file);
}

/////////////////////////////////////////////////////////////////////////////////
// CMP Exclusion protocol L2 cache functions
//

cmp_incl_3l_l2_cache_t::cmp_incl_3l_l2_cache_t(string name, cache_config_t *conf,
	uint32 _id, conf_object_t *_cpu, uint64 prefetch_alg)
	: tcache_t<cmp_incl_3l_l2_protocol_t> (name, 3, conf, _id, prefetch_alg), cpu(_cpu)
	// Three links for this protocol
{ }

uint32
cmp_incl_3l_l2_cache_t::get_action(message_t *msg)
{
	// Up/down same type
	upmsg_t *inmsg = static_cast<upmsg_t *> (msg);

	switch(inmsg->get_type()) {

	// up
	case TypeRead:
		return ActionL1Read;
	case TypeReadEx:
		return ActionL1ReadEx;
	case TypeUpgrade:
		return ActionL1Upgrade;
	case TypeReplace:
		return ActionL1Replace;
	case TypeWriteBackClean:
		return ActionL1WriteBackClean;
	case TypeWriteBack:
		return ActionL1WriteBackClean;
	case TypeL1InvAck:
		return ActionL1InvAck;
	case TypeL1DataRespShared:
		return ActionL1DataRespSh;
		
	// down
	case TypeDataRespShared:	
		return ActionDataShared;
	case TypeDataRespExclusive:
		return ActionDataExclusive;
	case TypeInvalidate:
		return ActionInvalidate;
	case TypeReadFwd:
		return ActionReadFwd;
	case TypeReadExFwd:
		return ActionReadExFwd;
	case TypeWriteBackAck:
		return ActionWriteBackAck;
	case TypeReplaceAck:
		return ActionReplaceAck;
	case TypeUpgradeAck:
		return ActionUpgradeAck;
		
	default:
		FAIL_MSG("Invalid message type %s %u %u\n",
				 upmsg_t::names[(int)inmsg->get_type()][0].c_str(),
				 inmsg->get_source(), inmsg->get_dest());
	}
}

void
cmp_incl_3l_l2_cache_t::trigger(tfn_info_t *t, tfn_ret_t *ret)
{
	ret->block_message = StallNone;
	ret->next_state = StateUnknown;
	
	
	for (uint32 i = 0; i < numwb_buffers; i++) {
		if (wb_lines[i].get_tag() || wb_lines[i].get_index())
			DEBUG_TR_FN("tag %llu index %llu", t, ret, wb_lines[i].get_tag(),
				 wb_lines[i].get_index());
	}
	
	switch(t->curstate) {

	/// NotPresent
	case StateNotPresent:
		switch(t->action) {
			
		case ActionL1Read:
			ret->block_message = can_insert_new_line(t, ret);
			if (ret->block_message != StallNone)   goto block_message;
			if (!can_allocate_mshr(t, ret))		goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_other;

			insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			set_inclusion(t, ret);
			send_protocol_msg_down(t, ret, false, TypeRead, true);
			ret->next_state = StateInvToShEx;
			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
			break;
		
		case ActionL1ReadEx:
			ret->block_message = can_insert_new_line(t, ret);
			if (ret->block_message != StallNone)   goto block_message;
			if (!can_allocate_mshr(t, ret))		goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_other;
			insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			set_inclusion(t, ret);
			send_protocol_msg_down(t, ret, false, TypeReadEx, true);
			ret->next_state = StateInvToMod;
			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
			break;
		case ActionInvalidate: 
			goto donothing;
		case ActionWriteBackAck:
			////ASSERT(t->trans->occurred(MEM_EVENT_L2_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L2_WRITE_BACK);
			goto donothing;
		case ActionL1InvAck:
			goto donothing;
		default: goto error;
		}
		break;

	/// Invalid
	case StateInvalid:
		switch(t->action) {
			
		case ActionL1Read:
			if (!can_allocate_mshr(t, ret))		goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_other;

			insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			set_inclusion(t, ret);
			send_protocol_msg_down(t, ret, false, TypeRead, true);
			ret->next_state = StateInvToShEx;
			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
			break;
			
		case ActionL1ReadEx:
			if (!can_allocate_mshr(t, ret))		goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_other;

			insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			set_inclusion(t, ret);
			send_protocol_msg_down(t, ret, false, TypeReadEx, true);
			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			ret->next_state = StateInvToMod;
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
			break;
		case ActionWriteBackAck:
			////ASSERT(t->trans->occurred(MEM_EVENT_L2_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L2_WRITE_BACK);
			// fallthrough
		case ActionL1InvAck:
			goto donothing;
		default: goto error;
		}
		break;

	/// InvalidToMod
	case StateInvToMod:
		switch(t->action) {

		case ActionDataExclusive:
			store_data_to_cache(t, ret);
			perform_mshr_requests(t, ret);
			ret->next_state = StateModified;
			deallocate_mshr(t, ret);
			break;
		case ActionReadFwd:
		case ActionReadExFwd:
		case ActionInvalidate:
			ret->block_message = StallSetEvent;
			goto block_message;
		case ActionL1Read:
		case ActionL1ReadEx:
			goto block_message_set;
		case ActionWriteBackAck:
			////ASSERT(t->trans->occurred(MEM_EVENT_L2_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L2_WRITE_BACK);
			// fallthrough
		case ActionL1InvAck:
			goto donothing;
		default: goto error;
		}
		break;

	case StateInvToShEx:
	
		switch(t->action) {
		case ActionDataShared:
			store_data_to_cache(t, ret);
			perform_mshr_requests(t, ret);
			ret->next_state = StateShared;
			deallocate_mshr(t, ret);
			break;
		case ActionDataExclusive:
			store_data_to_cache(t, ret);
			perform_mshr_requests(t, ret);
			if (t->line->get_dcache_incl() && t->line->get_icache_incl())
				ret->next_state = StateModifiedShared; // both I and D have requested
			else 
				ret->next_state = StateModified;
			deallocate_mshr(t, ret);
			break;
		case ActionReadFwd:
		case ActionReadExFwd:
		case ActionInvalidate:
			ret->block_message = StallSetEvent;
			goto block_message;
		case ActionL1Read:
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_other;
			enqueue_request_mshr(t,ret);
			set_inclusion(t, ret);
			ret->next_state = t->curstate;
			t->trans->mark_pending(MEM_EVENT_L2_MSHR_PART_HIT);
			break;
		case ActionL1ReadEx:
			ret->block_message = StallSetEvent;
			goto block_message;
		case ActionWriteBackAck:
			////ASSERT(t->trans->occurred(MEM_EVENT_L2_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L2_WRITE_BACK);
			// fallthrough
		case ActionL1InvAck:
			goto donothing;
		default: goto error;
		}
		break;
		
	case StateShared:
		switch(t->action) {

		case ActionL1Read:
			set_inclusion(t, ret);
			send_protocol_msg_up(t, ret, TypeDataRespShared);
			t->trans->mark_pending(MEM_EVENT_L2_HIT);
			ret->next_state = t->curstate;
			break;
			
		case ActionL1ReadEx:
			if (!can_allocate_mshr(t, ret))		   goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_other;

			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			if (t->line->get_icache_incl())
				send_protocol_msg_up(t, ret, TypeInvalidate, true);
			set_inclusion(t, ret);
			send_protocol_msg_down(t, ret, false, TypeUpgrade, true);
			ret->next_state = StateShInvToMod;
			t->trans->mark_pending(MEM_EVENT_L2_COHER_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
 			break;
			
		case ActionL1Upgrade:
			if (!can_allocate_mshr(t, ret))		   goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_other;

			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			if (t->line->get_icache_incl())
				send_protocol_msg_up(t, ret, TypeInvalidate, true);
			set_inclusion(t, ret);
			send_protocol_msg_down(t, ret, false, TypeUpgrade, true);
			ret->next_state = StateShToMod;
			t->trans->mark_pending(MEM_EVENT_L2_COHER_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
 			break;
			
		case ActionReplace:
			if (!can_allocate_writebackbuffer()) goto block_message_other;
			allocate_writebackbuffer(t->line);
			send_invalidates_up(t, ret, true); // whether incl or not
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				t->trans->mark_pending(MEM_EVENT_L2_WRITE_BACK);
				send_protocol_msg_down(t, ret, false, TypeReplaceNote, false);
			}
			ret->next_state = StateInvalid;  // wbb line goes to approp. state
			break;
		case ActionInvalidate:
			send_invalidates_up(t, ret);
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				send_protocol_msg_down(t, ret, false, TypeInvAck, false);
				ret->next_state = StateInvalid;
			} else {
				ret->next_state = StateShToInvOnInv;
			}
			break;

		case ActionReadFwd:
			set_requester(t, ret);
			send_protocol_msg_down(t, ret, true, downmsg_t::TypeDataRespShared, false);
			ret->next_state = t->curstate;

			////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
			t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			break;
		case ActionWriteBackAck:
			////ASSERT(t->trans->occurred(MEM_EVENT_L2_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L2_WRITE_BACK);
			goto donothing;
			
		case ActionL1Replace:
			unset_inclusion(t, ret);
			ret->next_state = t->curstate;

			////ASSERT(t->trans->occurred(MEM_EVENT_L1_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L1_WRITE_BACK);
			break;
			
		default: goto error;
		}
		break;


	case StateModified:
		switch(t->action) {
		case ActionL1Read:
			send_invalidates_up(t, ret);  // before set_incl
			if (t->line->get_dcache_incl() || t->line->get_icache_incl())
				goto block_message_set; // wait for invAck
			
			set_inclusion(t, ret);
			send_protocol_msg_up(t, ret, TypeDataRespShared);
			ret->next_state = StateModifiedShared;
			t->trans->mark_pending(MEM_EVENT_L2_HIT);
			break;
		case ActionL1ReadEx:
			send_invalidates_up(t, ret);  // before set_incl
			if (t->line->get_dcache_incl() || t->line->get_icache_incl())
				goto block_message_set; // wait for invAck

			set_inclusion(t, ret);
			send_protocol_msg_up(t, ret, TypeDataRespExclusive);
			ret->next_state = t->curstate;
			t->trans->mark_pending(MEM_EVENT_L2_HIT);
			break;
		case ActionReplace:
			if (!can_allocate_writebackbuffer()) goto block_message_other;
			allocate_writebackbuffer(t->line);
			send_invalidates_up(t, ret, true); // whether incl or not
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				send_protocol_msg_down(t, ret, false, 
					t->line->is_dirty() ? TypeWriteBack : TypeWriteBackClean,
					true);
			}
			ret->next_state = StateInvalid;  // wbb line goes to approp. state
			break;
		case ActionInvalidate:
			send_invalidates_up(t, ret);
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				send_protocol_msg_down(t, ret, false, 
                    t->line->is_dirty() ? TypeWriteBack : TypeWriteBackClean, true);
				ret->next_state = StateInvalid;
			} else {
				ret->next_state = StateModToInvOnInv;
			}
			break;
		case ActionReadExFwd:
			set_requester(t, ret);
			send_invalidates_up(t, ret);
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				send_protocol_msg_down(t, ret, true, TypeDataRespExclusive, false);
				ret->next_state = StateInvalid;

				////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
				t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			} else {
				ret->next_state = StateModToInvOnFwd;
			}
			break;
		case ActionReadFwd:
			set_requester(t, ret);
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				send_protocol_msg_down(t, ret, true, TypeDataRespShared, false);
				ret->next_state = StateShared;

				////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
				t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			} else {
				// L2 might be stale
				send_protocol_msg_up(t, ret, TypeReadFwd,
					!t->line->get_dcache_incl());
				ret->next_state = StateModToSh;
			}				
			break;
				
		case ActionL1WriteBack:
			store_data_to_cache(t, ret);
			t->line->set_dirty(true);
			// fallthrough
		case ActionL1WriteBackClean:
			unset_inclusion(t, ret);
			ret->next_state = t->curstate;

			////ASSERT(t->trans->occurred(MEM_EVENT_L1_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L1_WRITE_BACK);
			break;
			
		default: goto error;	
				
		}
		break;
		
	case StateModifiedShared:
		switch(t->action) {
		case ActionL1Read:
			set_inclusion(t, ret);
			send_protocol_msg_up(t, ret, TypeDataRespShared);
			ret->next_state = t->curstate;
			t->trans->mark_pending(MEM_EVENT_L2_HIT);
			break;
		case ActionL1ReadEx:
			send_invalidates_up(t, ret);  // before set_incl
			if (t->line->get_dcache_incl() || t->line->get_icache_incl())
				goto block_message_set; // wait for invAck

			set_inclusion(t, ret);
			send_protocol_msg_up(t, ret, TypeDataRespExclusive);
			ret->next_state = StateModified;
			t->trans->mark_pending(MEM_EVENT_L2_HIT);
			break;
		case ActionL1Upgrade:
			send_protocol_msg_up(t, ret, TypeInvalidate, true); // icache
			if (t->line->get_icache_incl())
				goto block_message_set; // wait for invAck

			set_inclusion(t, ret);
			send_protocol_msg_up(t, ret, TypeUpgradeAck);
			ret->next_state = StateModified;
			t->trans->mark_pending(MEM_EVENT_L2_HIT);
			break;
		case ActionReplace:
			if (!can_allocate_writebackbuffer()) goto block_message_other;
			allocate_writebackbuffer(t->line);
			send_invalidates_up(t, ret, true); // whether incl or not
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				t->trans->mark_pending(MEM_EVENT_L2_WRITE_BACK);
				send_protocol_msg_down(t, ret, false, 
					t->line->is_dirty() ? TypeWriteBack : TypeWriteBackClean,
					true);
			}
			ret->next_state = StateInvalid;  // wbb line goes to approp. state
			break;
		case ActionInvalidate:
			send_invalidates_up(t, ret);
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				send_protocol_msg_down(t, ret, false, 
                    t->line->is_dirty() ? TypeWriteBack : TypeWriteBackClean, true);
				ret->next_state = StateInvalid;
			} else {
				ret->next_state = StateModToInvOnInv;
			}
			break;
		case ActionReadExFwd:
			set_requester(t, ret);
			send_invalidates_up(t, ret);
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				send_protocol_msg_down(t, ret, true, TypeDataRespExclusive, false);
				ret->next_state = StateInvalid;

				////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
				t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			} else {
				ret->next_state = StateModToInvOnFwd;
			}
			break;
		case ActionReadFwd:
			// L2 isn't stale
			set_requester(t, ret);
			send_protocol_msg_down(t, ret, true, TypeDataRespShared, false);
			ret->next_state = StateShared;

			////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
			t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			break;
				
		case ActionL1Replace:
			////ASSERT(t->trans->occurred(MEM_EVENT_L1_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L1_WRITE_BACK);
			// fallthrough
		case ActionL1InvAck:
			unset_inclusion(t, ret);
			ret->next_state = t->curstate;
			break;
			
		default: goto error;	
				
		}
		break;
		
	case StateShToMod:
		switch (t->action) {
		case ActionL1Read:
		case ActionL1ReadEx:
			goto block_message_set;
		case ActionUpgradeAck:
			if (t->line->get_icache_incl())
				goto block_message_set; // wait for invAck

			ret->next_state = StateModified;
			perform_mshr_requests(t, ret);
			deallocate_mshr(t, ret);
			break;
		case ActionInvalidate:
			ASSERT(t->line->get_dcache_incl());
			ret->next_state = StateShToInvToMod;
			send_invalidates_up(t, ret);
			break;
		case ActionReadFwd:
			// Should we block this message ... ??
			set_requester(t, ret);
			send_protocol_msg_down(t, ret, true, TypeDataRespShared, false);
			ret->next_state = t->curstate;

			////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
			t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			break;
		case ActionL1Replace:
			////ASSERT(t->trans->occurred(MEM_EVENT_L1_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L1_WRITE_BACK);
			// fallthrough
		case ActionL1InvAck:
			unset_inclusion(t, ret);
			ret->next_state = t->curstate;
			break;
		
		default : goto error;
		}
		break;
		
	case StateShInvToMod:
		switch (t->action) {
		case ActionL1Read:
		case ActionL1ReadEx:
			goto block_message_set;
		case ActionUpgradeAck:
			if (t->line->get_icache_incl())
				goto block_message_set; // wait for invAck

			ret->next_state = StateModified;
			perform_mshr_requests(t, ret);
			deallocate_mshr(t, ret);
			break;
		case ActionInvalidate:
			// L1 in InvToMod, don't need to invalidate it, just reissue trans
			ASSERT((g_conf_tstate_l1_bypass && t->trans->is_thread_state()) ||
                t->line->get_dcache_incl());
			if (t->line->get_icache_incl())
				goto block_message_set; // inv already sent, wait for invAck

			send_protocol_msg_down(t, ret, false, TypeInvAck, false);
			send_protocol_msg_down(t, ret, false, TypeReadEx, false, 
                get_enqueued_trans(t, ret));
			ret->next_state = StateInvToMod;
			break;
		case ActionReadFwd:
			set_requester(t, ret);
			send_protocol_msg_down(t, ret, true, TypeDataRespShared, false);
			ret->next_state = t->curstate;

			////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
			t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			break;
		case ActionL1Replace:
			////ASSERT(t->trans->occurred(MEM_EVENT_L1_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L1_WRITE_BACK);
			// fallthrough
		case ActionL1InvAck:
			unset_inclusion(t, ret);
			ret->next_state = t->curstate;
			break;
		
		default : goto error;
		}
		break;
		
	case StateModToSh:
		switch (t->action) {
		case ActionL1Read:
		case ActionL1ReadEx:
			goto block_message_set;
		case ActionInvalidate:
		case ActionReadFwd:
			goto block_message_set;

		case ActionL1DataRespSh: 
			store_data_to_cache(t, ret);
			t->line->set_dirty(true);
			send_protocol_msg_down(t, ret, true, TypeDataRespShared, false);
			ret->next_state = StateShared;

			////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
			t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			break;
			
		case ActionL1WriteBack:
			store_data_to_cache(t, ret);
			t->line->set_dirty(true);
			// fallthrough
		case ActionL1WriteBackClean:
			unset_inclusion(t, ret);
			send_protocol_msg_down(t, ret, true, TypeDataRespShared, false);
			ret->next_state = StateShared;

			////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
			t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			break;
		
		default : goto error;
		}
		break;
	
	case StateModToModToInv:	
		switch (t->action) {
		case ActionL1Read:
		case ActionL1ReadEx:
			goto block_message_set;
		case ActionL1Upgrade:
			goto donothing; // drop it; L1 will reissue
		case ActionL1WriteBack:
			store_data_to_cache(t, ret);
			t->line->set_dirty(true);
			// fallthrough
		case ActionL1WriteBackClean:
		case ActionL1Replace:
			////ASSERT(t->trans->occurred(MEM_EVENT_L1_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L1_WRITE_BACK);
			// fallthrough
		case ActionL1InvAck:
			unset_inclusion(t, ret);
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				t->trans->mark_pending(MEM_EVENT_L2_WRITE_BACK);
				send_protocol_msg_down(t, ret, false,
					t->line->is_dirty() ? TypeWriteBack : TypeWriteBackClean, 
					false);
				ret->next_state = StateModToInv;
			} else 
				ret->next_state = t->curstate;
			break;
		
		case ActionInvalidate:
		case ActionReadFwd:
		case ActionReadExFwd:
			goto block_message_set;
			
		default: goto error;
		}
		break;
		
	case StateModToInvOnInv:	
		switch (t->action) {
		case ActionL1Read:
		case ActionL1ReadEx:
			goto block_message_set;
		case ActionL1Upgrade:
			goto donothing; // drop it; L1 will reissue
		case ActionL1WriteBack:
			store_data_to_cache(t, ret);
			t->line->set_dirty(true);
			// fallthrough
		case ActionL1WriteBackClean:
		case ActionL1Replace:
			////ASSERT(t->trans->occurred(MEM_EVENT_L1_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L1_WRITE_BACK);
			// fallthrough
		case ActionL1InvAck:
			unset_inclusion(t, ret);
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				t->trans->mark_pending(MEM_EVENT_L2_WRITE_BACK);
				send_protocol_msg_down(t, ret, false,
					t->line->is_dirty() ? TypeWriteBack : TypeWriteBackClean, 
					false);
				ret->next_state = StateInvalid;
			} else 
				ret->next_state = t->curstate;
			break;
		
		case ActionInvalidate:
			goto block_message_set;
			
		default: goto error;
		}
		break;
		
	case StateModToInvOnFwd:	
		switch (t->action) {
		case ActionL1Read:
		case ActionL1ReadEx:
			goto block_message_set;
		case ActionL1Upgrade:
			goto donothing; // drop it; L1 will reissue

		case ActionL1WriteBack:
			store_data_to_cache(t, ret);
			t->line->set_dirty(true);
			// fallthrough
		case ActionL1WriteBackClean:
		case ActionL1Replace:
			////ASSERT(t->trans->occurred(MEM_EVENT_L1_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L1_WRITE_BACK);
			// fallthrough
		case ActionL1InvAck:
			unset_inclusion(t, ret);
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				send_protocol_msg_down(t, ret, true, TypeDataRespExclusive, 
					false);
				ret->next_state = StateInvalid;
			} else 
				ret->next_state = t->curstate;
			break;
		
		case ActionInvalidate:
			goto block_message_set;
			
		default: goto error;
		}
		break;
		
	case StateShToShToInv:	
		switch (t->action) {
		case ActionL1Read:
		case ActionL1ReadEx:
			goto block_message_set;
		case ActionL1Upgrade:
			goto donothing; // drop it; L1 will reissue
			
		case ActionL1Replace:
			////ASSERT(t->trans->occurred(MEM_EVENT_L1_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L1_WRITE_BACK);
			// fallthrough
		case ActionL1InvAck:
			unset_inclusion(t, ret);
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				t->trans->mark_pending(MEM_EVENT_L2_WRITE_BACK);
				send_protocol_msg_down(t, ret, false, TypeReplaceNote, false);
				ret->next_state = StateShToInv;
			} else 
				ret->next_state = t->curstate;
			break;
		
		case ActionInvalidate:
			goto block_message_set;
			
		case ActionReadFwd:
			set_requester(t, ret);
			ret->next_state = t->curstate;
			send_protocol_msg_down(t, ret, true, TypeDataRespShared, true);
			ret->next_state = t->curstate;

			////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
			t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			break;
		default: goto error;
		}
		break;
		
	case StateShToInvOnInv:	
		switch (t->action) {
		case ActionL1Read:
		case ActionL1ReadEx:
			goto block_message_set;
		case ActionL1Upgrade:
			goto donothing; // drop it; L1 will reissue
			
		case ActionL1Replace:
			////ASSERT(t->trans->occurred(MEM_EVENT_L1_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L1_WRITE_BACK);
			// fallthrough
		case ActionL1InvAck:
			unset_inclusion(t, ret);
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				send_protocol_msg_down(t, ret, false, TypeInvAck, false);
				ret->next_state = StateInvalid;
			} else 
				ret->next_state = t->curstate;
			break;
		
		case ActionInvalidate:
			goto block_message_set;
			
		default: goto error;
		}
		break;
		
	case StateModToInv:
		switch(t->action) {
		case ActionL1Read:
		case ActionL1ReadEx:
			ret->block_message = StallSetEvent;
			goto block_message;
		case ActionInvalidate:
			release_writebackbuffer(get_line_address(t->address));
			ret->next_state = StateInvalid;
			break;
		case ActionWriteBackAck:
			////ASSERT(t->trans->occurred(MEM_EVENT_L2_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L2_WRITE_BACK);
			release_writebackbuffer(get_line_address(t->address));
			ret->next_state = StateInvalid;
			break;
		case ActionReadFwd:
			ASSERT(!t->line->get_dcache_incl());
			set_requester(t, ret);
			ret->next_state = t->curstate;
			send_protocol_msg_down(t, ret, true, downmsg_t::TypeDataRespShared, true);
			ret->next_state = t->curstate;

			////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
			t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			break;
		case ActionReadExFwd:
			ASSERT(!t->line->get_dcache_incl());
			set_requester(t, ret);
			ret->next_state = t->curstate;
			send_protocol_msg_down(t, ret, true, downmsg_t::TypeDataRespExclusive, true);
			ret->next_state = t->curstate;

			////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
			t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			break;
		case ActionL1InvAck:
			goto donothing;
		default : goto error;
		}
		break;
	case StateShToInv:
		switch(t->action) {
		case ActionL1Read:
		case ActionL1ReadEx:
			ret->block_message = StallSetEvent;
			goto block_message;
		case ActionInvalidate:
		case ActionReplaceAck:
			release_writebackbuffer(get_line_address(t->address));
			ret->next_state = StateInvalid;
			break;
		case ActionReadFwd:
			ASSERT(!t->line->get_dcache_incl());
			set_requester(t, ret);
			ret->next_state = t->curstate;
			send_protocol_msg_down(t, ret, true, downmsg_t::TypeDataRespShared, true);
			ret->next_state = t->curstate;

			////ASSERT(t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD));
			t->trans->clear_pending_event(MEM_EVENT_L1DIR_REQ_FWD);
			break;
		case ActionL1InvAck:
			goto donothing;
		default : goto error;
		}
		break;
		
	case StateShToInvToMod:
	// Got an Invalidate in ShToMod, sent a Inv Up, now need to send a ReadEx
		switch(t->action) {
		case ActionL1Read:
		case ActionL1ReadEx:
			goto block_message_set;
			
		case ActionL1Replace:
			////ASSERT(t->trans->occurred(MEM_EVENT_L1_WRITE_BACK));
			t->trans->clear_pending_event(MEM_EVENT_L1_WRITE_BACK);
			// fallthrough
		case ActionL1InvAck:
			unset_inclusion(t, ret);
			if (!t->line->get_dcache_incl() && !t->line->get_icache_incl()) {
				// Drop L1s orig Upgrade -- it will reissue
				send_protocol_msg_down(t, ret, false, TypeInvAck, false);
				deallocate_mshr(t, ret);
				ret->next_state = StateInvalid;
			} else 
				ret->next_state = t->curstate;
			break;

		default : goto error;
		}
		break;

	default:
		ASSERT(0);

	}


	/// Common return case
	ASSERT(ret->next_state != StateUnknown);
	// May not have line if in NotPresent
	if (t->line) t->line->set_state(ret->next_state);
	else ASSERT(t->curstate == StateNotPresent);

    t->trans->clear_pending_event(MEM_EVENT_L2_STALL);
	profile_transition(t, ret);
	return;


/// Blocking Actions 
//block_message_poll:
//	if (ret->block_message == StallNone) ret->block_message = StallPoll;
//	// fallthrough
block_message_set:
	if (ret->block_message == StallNone) ret->block_message = StallSetEvent;
	// fallthrough
block_message_other:
	if (ret->block_message == StallNone) ret->block_message = StallOtherEvent;
	// fallthrough
block_message:
	ret->next_state = t->curstate;
	t->trans->mark_pending(MEM_EVENT_L2_STALL);
	DEBUG_TR_FN("%s Blocking Message %u", t, ret, __func__, ret->block_message);
	profile_transition(t, ret);
	return;
	

/// Other common actions	
error:
	FAIL_MSG("%10s @ %llu 0x%016llx:Invalid action: %s in state: %s\n",get_cname(), 
		external::get_current_cycle(), t->message->address, action_names[t->action][0].c_str(),
		state_names[t->curstate][0].c_str());
	
donothing:
	ret->next_state = t->curstate;
	profile_transition(t, ret);
	return;
}

////////////////////////////////////////////////////////////////////////////////
// Helper functions
void cmp_incl_3l_l2_cache_t::
send_protocol_msg_down(tfn_info_t *t, tfn_ret_t *ret, bool datafwd, 
	uint32 msgtype, bool addrmsg)
{
   send_protocol_msg_down(t, ret, datafwd, msgtype, addrmsg, t->trans);
}
        
void cmp_incl_3l_l2_cache_t::
send_protocol_msg_down(tfn_info_t *t, tfn_ret_t *ret, bool datafwd, 
	uint32 msgtype, bool addrmsg, mem_trans_t *mem_trans)
{
	addr_t addr = get_line_address(t->address);
	uint32 dest = l3_cache_ptr->get_interconnect_id(addr);;
	uint32 request_id = interconnect_id;
	
    // Serious confusion here
	//if (datafwd || msgtype == TypeInvAck || msgtype == TypeWriteBackClean ||
    //    msgtype == TypeWriteBack) {
    
    if (datafwd || msgtype == TypeInvAck) {
        
		downmsg_t *in_msg = static_cast<downmsg_t *>(t->message);
		if (datafwd)
			dest = t->line->get_requester();
		request_id = in_msg->get_requester();
	}
	
	DEBUG_TR_FN("%s sending type %s to %d trans_addr %llx", t, ret, __func__, 
		downmsg_t::names[msgtype][0].c_str(), dest, mem_trans->phys_addr);

	downmsg_t *msg = new downmsg_t(mem_trans, addr, get_lsize(), NULL,
		interconnect_id, dest, msgtype, request_id, addrmsg);
	stall_status_t retval = send_message(links[interconnect_link], msg);
	ASSERT(retval == StallNone);	
	
}

// Only for responses to L1 requests
void cmp_incl_3l_l2_cache_t::
send_protocol_msg_up(tfn_info_t *t, tfn_ret_t *ret, uint32 msgtype)
{
	send_protocol_msg_up(t, ret, msgtype, t->trans->ifetch);
}

void cmp_incl_3l_l2_cache_t::
send_protocol_msg_up(tfn_info_t *t, tfn_ret_t *ret, uint32 msgtype, bool icache)
{
	send_protocol_msg_up(t, ret, msgtype, icache, t->trans);
}

void cmp_incl_3l_l2_cache_t::
send_protocol_msg_up(tfn_info_t *t, tfn_ret_t *ret, uint32 msgtype,
	bool icache, mem_trans_t *trans)
{
	DEBUG_TR_FN("%s %s to %s", t, ret, __func__, names[msgtype][0].c_str(),
		icache ? "Icache" : "Dcache");
	upmsg_t *in_msg = static_cast<upmsg_t *>(t->message);
	
	addr_t addr = get_line_address(t->address);
	// requestor_id only matters if request fwd
	upmsg_t *msg = new upmsg_t(trans, addr, get_lsize(), NULL,
		0, 0, msgtype, in_msg->get_requester(), false);
		
	uint32 link = icache ? icache_link : dcache_link;
	stall_status_t retval = send_message(links[link], msg);
	ASSERT(retval == StallNone);
}

// Enqueue this read request to the MSHR for this address
void
cmp_incl_3l_l2_cache_t::enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("enqueue_request_mshr", t, ret);

	addr_t addr = get_line_address(t->address);
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);
    //ASSERT(addr == get_line_address(t->trans->phys_addr));
	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);

	upmsg_t *msg = static_cast <upmsg_t *> (t->message);
	
	mshr_type_t *type = new mshr_type_t;
	type->trans = t->trans;
	type->request_type = msg->get_type();
	type->icache = (msg->get_requester() == icache_id);
	mshr->enqueue_request(type);
}

// Send response back to processor using MSHR trans*
void
cmp_incl_3l_l2_cache_t::perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret)
{
	addr_t addr = get_line_address(t->address);
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);

	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);

	DEBUG_TR_FN("perform_mshr_requests", t, ret);

	upmsg_t *msg = static_cast <upmsg_t *> (t->message);
	
	mshr_type_t *request = mshr->pop_request();
	ASSERT(request);
	
	while (request) {
		// Need to change the transaction in the transfer function
		//t->trans=request->trans;

		// assert for L1 too
		ASSERT(request->trans->occurred(
			MEM_EVENT_L1_DEMAND_MISS | MEM_EVENT_L1_COHER_MISS |
			MEM_EVENT_L1_MSHR_PART_HIT));

		ASSERT(request->trans->occurred(
			MEM_EVENT_L2_DEMAND_MISS | MEM_EVENT_L2_COHER_MISS |
			MEM_EVENT_L2_MSHR_PART_HIT));
			
		request->trans->clear_pending_event(
			MEM_EVENT_L2_DEMAND_MISS | MEM_EVENT_L2_COHER_MISS | 
			MEM_EVENT_L2_HIT); 

		uint32 type;
		if (request->request_type == TypeRead) {
			if (msg->get_type() == TypeDataRespShared ||
				(t->line->get_dcache_incl() && t->line->get_icache_incl()))
			{
				// Shared response, or both I and D have requested line
				type = TypeDataRespShared;
			} else if (msg->get_type() == TypeDataRespExclusive)
				type = TypeDataRespExclusive;
			else 
				FAIL;
		}				
		else if (request->request_type == TypeReadEx)
			type = TypeDataRespExclusive;
		else if (request->request_type == TypeUpgrade)
			type = TypeUpgradeAck;
		else 
			FAIL;
		
		DEBUG_TR_FN("%s: sending %s to %s", t, ret, __func__, 
			upmsg_t::names[type][0].c_str(),
			 request->icache ? "Icache" : "DCache");
	
        ASSERT(!request->trans->is_thread_state() || 
            mem_hier_t::ptr()->is_thread_state(t->address));
		send_protocol_msg_up(t, ret, type, request->icache,
			request->trans);
		delete request;
		request = mshr->pop_request();
	}
	
}

// Send invalidates to i and or d cache
void
cmp_incl_3l_l2_cache_t::send_invalidates_up(tfn_info_t *t, tfn_ret_t *ret,
	bool always)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	if (always || t->line->get_icache_incl())
		send_protocol_msg_up(t, ret, TypeInvalidate, true);
	if (always || t->line->get_dcache_incl())
		send_protocol_msg_up(t, ret, TypeInvalidate, false);
}

// Set inclusion based on msg source
void
cmp_incl_3l_l2_cache_t::set_inclusion(tfn_info_t *t, tfn_ret_t *ret)
{
	ASSERT(is_up_message(t->message));
    if (g_conf_tstate_l1_bypass && t->trans->is_thread_state())
    {
        ASSERT(mem_hier_t::ptr()->is_thread_state(t->line->get_address()));
        return;
    }
    ASSERT(!g_conf_tstate_l1_bypass || 
        mem_hier_t::ptr()->is_thread_state(t->line->get_address()) == false);
	upmsg_t *in_msg = static_cast<upmsg_t *>(t->message);
	if (in_msg->get_source() == icache_id) {
		t->line->set_icache_incl(true);
		DEBUG_TR_FN("%s icache", t, ret, __func__);
	}
	else if (in_msg->get_source() == dcache_id) {
		t->line->set_dcache_incl(true);
		DEBUG_TR_FN("%s dcache", t, ret, __func__);
	}
	else FAIL;
}

// Clean inclusion based on msg source
void
cmp_incl_3l_l2_cache_t::unset_inclusion(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	ASSERT(is_up_message(t->message));
	upmsg_t *in_msg = static_cast<upmsg_t *>(t->message);
	if (in_msg->get_source() == icache_id)
		t->line->set_icache_incl(false);
	else if (in_msg->get_source() == dcache_id)
		t->line->set_dcache_incl(false);
	else FAIL;
}

// Clean inclusion based on msg source
void
cmp_incl_3l_l2_cache_t::set_requester(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	downmsg_t *in_msg = static_cast<downmsg_t *>(t->message);
	t->line->set_requester(in_msg->get_requester());
}

void cmp_incl_3l_l2_cache_t::set_l3_cache_ptr(cmp_incl_3l_l3_cache_t *l3_cache)
{
	l3_cache_ptr = l3_cache;
}


mem_trans_t *
cmp_incl_3l_l2_cache_t::get_enqueued_trans(tfn_info_t *t, tfn_ret_t *ret)
{
	addr_t addr = get_line_address(t->address);
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);

	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);

	DEBUG_TR_FN("%s", t, ret, __func__);

    
    mshr_type_t *request = mshr->pop_request();
	ASSERT(request);
	mshr->enqueue_request(request);
    return request->trans;
}



bool
cmp_incl_3l_l2_cache_t::is_up_message(message_t *message) {
	upmsg_t *msg = static_cast<upmsg_t *>(message);
	
	return (msg->get_type() == TypeRead ||
		msg->get_type() == TypeReadEx ||
		msg->get_type() == TypeUpgrade ||
		msg->get_type() == TypeReplace ||
		msg->get_type() == TypeWriteBack ||
		msg->get_type() == TypeWriteBackClean ||
		msg->get_type() == TypeL1InvAck ||
		msg->get_type() == TypeL1DataRespShared);
}

bool
cmp_incl_3l_l2_cache_t::is_down_message(message_t *message) {
	return (!is_up_message(message));
}

bool
cmp_incl_3l_l2_cache_t::is_request_message(message_t *message) {
	if (!is_up_message(message))
		return false;
	
	upmsg_t *msg = static_cast<upmsg_t *>(message);
	
	return (msg->get_type() == TypeRead ||
		msg->get_type() == TypeReadEx ||
		msg->get_type() == TypeUpgrade);
}


bool
cmp_incl_3l_l2_cache_t::wakeup_signal(message_t* msg, prot_line_t *line,
	uint32 action)
{
	return true;
}	
	

////////////////////////////////////////////////////////////////////////////////
// Uniprocessor onelevel cache constant definitions

const string cmp_incl_3l_l2_protocol_t::action_names[][2] = {
	{ "Replace", "" }, 
	{ "L1Read", "" }, 
	{ "L1ReadEx", "" }, 
	{ "L1Upgrade", "" }, 
	{ "L1Replace", "" }, 
	{ "L1WriteBack", "" }, 
	{ "L1WriteBackClean", "" }, 
	{ "L1InvAck", "" }, 
	{ "L1DataRespSh", "" }, 
	{ "DataShared", "" }, 
	{ "DataExclusive", "" },
	{ "UpgradeAck", "" },
	{ "Invalidate", "" },
	{ "WriteBackAck", "" },
	{ "ReplaceAck", ""},
	{ "ReadFwd", "" },
	{ "ReadExFwd", "" },
	{ "Flush", "" },
};
const string cmp_incl_3l_l2_protocol_t::state_names[][2] = {
	{ "Unknown", "Unknown" },
	{ "NotPresent", "Line is not present in cache" },
	{ "Invalid", "Data may be present but is invalid" },
	{ "InvToShEx", "Line was invalid, issued a read request, waiting for data, will go to wither shared or exclusive" },
	{ "InvToMod", "Line was invalid, got a write, issued a readex request, waiting for data" },
	{ "Shared", "Line is present and clean and shared" },
	{ "Modified", "Line is present and dirty and I am owner; One or zero L1s modified or excl" },
	{ "ModifiedShared", "Line is present and dirty and I am global owner; One or more L1s with shared" },
	{ "ShToMod", "Line was shared in L2&l1, received upgrade, send Upgrade down, waiting for ack" },
	{ "ShInvToMod", "Line was shared, inv in L1, received readEx, send Upgrade down, waiting for ack" },
	{ "ModToSh", "" },
	{ "ModToInv", "" },
	{ "ModToInvOnFwd", "Line was modified, got fwdex, waiting for L1InvAck before sending Fwd" },
	{ "ModToInvOnInv", "Line was modified, got invalidate, waiting for L1InvAck before Acking dir" },
	{ "ModToModToInv", "Line was modified, replaced, waiting for L1InvAck before writing back to dir" },
	{ "ShToShToInv", "Line was Shared, replaced, waiting for L1InvAck before sending repl to dir" },
	{ "ShToInvOnInv", "Line was Shared, got invalidate, waiting for L1InvAck before Acking dir" },
	{ "ShToInv",  "Line was Shared, replaced and went to WBB"},
	{ "ShToInvToMod", "Got invalid in ShToMod, don't reissue readex/upgrade" },
};

const string cmp_incl_3l_l2_protocol_t::prot_name[] = 
	{"cmp_incl_3l_l2", "Two-level Uniprocessor L2 Cache"};
