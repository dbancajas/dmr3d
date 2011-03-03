/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    const fields of various message classes
 * initial author: Philip Wells 
 *
 */
 
//  // #include "simics/first.h"
// // RCSID("$Id: messages.cc,v 1.3.2.15 2005/11/11 21:51:31 pwells Exp $");

#include "definitions.h"
#include "protocols.h"

///////////////////////////////////////////////////////////////////////////////
// default_msg_t
const default_msg_type_t default_msg_t::ProcRead = dTypeRead;
const default_msg_type_t default_msg_t::ProcWrite = dTypeWrite;
const default_msg_type_t default_msg_t::StorePref = dTypeStorePref;
const default_msg_type_t default_msg_t::Read = dTypeRead;
const default_msg_type_t default_msg_t::ReadEx = dTypeRead;
const default_msg_type_t default_msg_t::WriteBack = dTypeWrite;
const default_msg_type_t default_msg_t::DataResp = dTypeDataResp;
const default_msg_type_t default_msg_t::ReqComplete = dTypeReqComplete;
const default_msg_type_t default_msg_t::WBAck = dTypeDataResp;

const string default_msg_t::names[][2] = { 
	{ "ProcRead", "Proceessor Read Request" },
	{ "ProcWrite", "Proceessor Read Request" },
	{ "StorePred", "Proceessor Store Prefetch Request" },
	{ "DataResp", "Data Response" },
	{ "ReqComplete", "Request is Complete" },
};

///////////////////////////////////////////////////////////////////////////////// simple_proc_msg_t
// simple_proc_msg_t
const uint32 simple_proc_msg_t::ProcRead;
const uint32 simple_proc_msg_t::ProcWrite;
const uint32 simple_proc_msg_t::StorePref;
const uint32 simple_proc_msg_t::ReadPref;
const uint32 simple_proc_msg_t::DataResp;
const uint32 simple_proc_msg_t::ReqComplete;
const uint32 simple_proc_msg_t::ProcInvalidate;   

const string simple_proc_msg_t::names[][2] = { 
	{ "ProcRead", "Proceessor Read Request" },
	{ "ProcWrite", "Proceessor Read Request" },
	{ "StorePref", "Proceessor Store Prefetch Request" },
    { "ReadPref", "Proceessor Read Prefetch Request" },
	{ "DataResp", "Data Response" },
	{ "ReqComplete", "Request is Complete" },
	{ "ProcInvalidate", "Invalidate speculative ld/st" },   
};

///////////////////////////////////////////////////////////////////////////////
// simple_mainmem_msg_t
const string simple_mainmem_msg_t::names[][2] = { 
	{ "Read", "Read Request" },
	{ "ReadEx", "Exclusive Read Request" },
	{ "WriteBack", "Write Back Request" },
	{ "DataResp", "Data Response" },
	{ "WBAck", "Write Back Ack" },
};

/*
///////////////////////////////////////////////////////////////////////////////
// mp_one_msi_msg_t
const mp_one_msi_msg_type_t mp_one_msi_msg_t::Read = mp1msi_TypeRead;
const mp_one_msi_msg_type_t mp_one_msi_msg_t::ReadEx = mp1msi_TypeReadEx;
const mp_one_msi_msg_type_t mp_one_msi_msg_t::Upgrade = mp1msi_TypeUpgrade;
const mp_one_msi_msg_type_t mp_one_msi_msg_t::WriteBack = mp1msi_TypeWriteBack;
const mp_one_msi_msg_type_t mp_one_msi_msg_t::DataResp = mp1msi_TypeDataResp;
const mp_one_msi_msg_type_t mp_one_msi_msg_t::WBAck = mp1msi_TypeWBAck;

const string mp_one_msi_msg_t::names[][2] = { 
	{ "Read", "Request for shared cpoy of data" },
	{ "ReadEx", "Request for exclusive copy of data" },
	{ "Upgrade", "Request for exclusive permission" },
	{ "WriteBack", "Write Back Request" },
	{ "DataResp", "Data Response" },
	{ "WBAck", "Write Back Ack" },
};


const mp_two_msi_msg_type_t mp_two_msi_msg_t::Read = mp2msi_TypeRead;
const mp_two_msi_msg_type_t mp_two_msi_msg_t::ReadEx = mp2msi_TypeReadEx;
const mp_two_msi_msg_type_t mp_two_msi_msg_t::Upgrade = mp2msi_TypeUpgrade;
const mp_two_msi_msg_type_t mp_two_msi_msg_t::WriteBack = mp2msi_TypeWriteBack;
const mp_two_msi_msg_type_t mp_two_msi_msg_t::WriteBackonInvalidate = mp2msi_TypeWriteBackonInvalidate;
const mp_two_msi_msg_type_t mp_two_msi_msg_t::DataResp = mp2msi_TypeDataResp;
const mp_two_msi_msg_type_t mp_two_msi_msg_t::ReadPref = mp2msi_TypeReadPref;
const mp_two_msi_msg_type_t mp_two_msi_msg_t::StorePref = mp2msi_TypeStorePref;
const mp_two_msi_msg_type_t mp_two_msi_msg_t::Invalidate = mp2msi_TypeInvalidate;
const mp_two_msi_msg_type_t mp_two_msi_msg_t::InvalidateandWriteBack = mp2msi_TypeInvalidateandWriteBack;
//const mp_two_msi_msg_type_t mp_two_msi_msg_t::ClearPendingBusAddress = mp2msi_TypeClearPendingBusAddress;

const string mp_two_msi_msg_t::names[][2] = { 
	{ "Read", "Request for shared cpoy of data" },
	{ "ReadEx", "Request for exclusive copy of data" },
	{ "Upgrade", "Request for exclusive permission" },
	{ "WriteBack", "Write Back Request" },
    { "WriteBackonInvalidate", "WriteBack on Invalidate request"},
	{ "DataResp", "Data Response" },
    { "StorePrefetch", "Store Prefetch Request" },
    { "ReadPrefetch", "Read Prefetch Request" },
	{"Invalidate", "Replacement Invalidation sent up"},
    {"InvalidateandWriteBack", "Invalidate and Writeback request"},
    {"ClearPendingBusAddress", "Clears Any Pending Address in the Bus"},
};

*/

///////////////////////////////////////////////////////////////////////////////
// unip_two_msg_t
#ifdef COMPILE_UNIP_TWO
const uint32 unip_two_protocol_t::TypeIFetch;
const uint32 unip_two_protocol_t::TypeRead;
const uint32 unip_two_protocol_t::TypeWrite;
const uint32 unip_two_protocol_t::TypeDataResp;
const uint32 unip_two_protocol_t::TypeInvalidate;
const uint32 unip_two_protocol_t::TypeReadPref;
const uint32 unip_two_protocol_t::TypeStorePref;

const string unip_two_protocol_t::names[][2] = { 
	{ "IFetch", "ICache read request" },
	{ "Read", "DCache read request" },
	{ "Write", "DCache write-through" },
	{ "DataResp", "Data Response" },
	{ "Invalidate", "Invalidate from L2" },
    { "ReadPref", "Read Prefetch Request" },
    { "StorePref", "Read Prefetch Request" },
};
#endif 

///////////////////////////////////////////////////////////////////////////////
// cmp_incl_msg_t
#ifdef COMPILE_CMP_INCL 
const uint32 cmp_incl_protocol_t::TypeRead;
const uint32 cmp_incl_protocol_t::TypeReadEx;
const uint32 cmp_incl_protocol_t::TypeUpgrade;
const uint32 cmp_incl_protocol_t::TypeReplace;
const uint32 cmp_incl_protocol_t::TypeWriteBack;
const uint32 cmp_incl_protocol_t::TypeInvAck;
const uint32 cmp_incl_protocol_t::TypeFwdAck;
const uint32 cmp_incl_protocol_t::TypeFwdNack;
const uint32 cmp_incl_protocol_t::TypeFwdAckData;
const uint32 cmp_incl_protocol_t::TypeDataRespShared;
const uint32 cmp_incl_protocol_t::TypeDataRespExclusive;
const uint32 cmp_incl_protocol_t::TypeInvalidate;
const uint32 cmp_incl_protocol_t::TypeReadFwd;
const uint32 cmp_incl_protocol_t::TypeReadExFwd;
const uint32 cmp_incl_protocol_t::TypeWriteBackAck;
const uint32 cmp_incl_protocol_t::TypeStorePref;
const uint32 cmp_incl_protocol_t::TypeReadPref;

const string cmp_incl_protocol_t::names[][2] = { 
	{ "Read", "Request for shared cpoy of data" },
	{ "ReadEx", "Request for exclusive copy of data" },
	{ "Upgrade", "Request for exclusive permission" },
	{ "Replace", "Send a replacement notification" },
	{ "WriteBackClean", "Write Back Clean (no data)" },
	{ "WriteBack", "Write Back Request" },
	{ "InvAck", "Acknowledge an Invalidate" },
	{ "FwdAck", "Acknowledge a Forward Request" },
	{ "FwdNack", "Negative-ack a forward request" },
	{ "FwdAckData", "Acknowledge a Forward Request, write back dirty line as well" },
	{ "DataRespShared", "Data Response Shared" },
	{ "DataRespExclusive", "Data Response Exclusive" },
	{ "Invalidate", "Invalidate Line" },
	{ "ReadFwd", "Read Forward Request" },
	{ "ReadExFwd", "ReadEx Forward Request" },
	{ "WriteBackAck", "Write Back Acknowledgement" },
    { "UpgradeAck",   "Upgrade Request Acknowledgement" },
    { "ReplaceAck", "Replacement Notification Acknowledgement"},
    { "StorePrefetch", "Store Prefetch Request" },
    { "ReadPrefetch", "Read Prefetch Request" },
};
#endif 


#ifdef COMPILE_CMP_INCL_WT
const uint32 cmp_incl_wt_protocol_t::TypeRead;
const uint32 cmp_incl_wt_protocol_t::TypeReadEx;
const uint32 cmp_incl_wt_protocol_t::TypeReplace;
const uint32 cmp_incl_wt_protocol_t::TypeInvAck;
const uint32 cmp_incl_wt_protocol_t::TypeDataResp;
const uint32 cmp_incl_wt_protocol_t::TypeInvalidate;
const uint32 cmp_incl_wt_protocol_t::TypeUpdate;
const uint32 cmp_incl_wt_protocol_t::TypeWriteComplete;
const uint32 cmp_incl_wt_protocol_t::TypeStorePref;
const uint32 cmp_incl_wt_protocol_t::TypeReadPref;

const string cmp_incl_wt_protocol_t::names[][2] = {
    { "Read", "Read Request"},
    { "ReadEx", "ReadEx Request"}, 
    { "Replace", "Replace Notification"},
    { "InvAck", "Invalidate Ack"},
    { "DataResp", "Data Response"},
    { "Invaldate", "Invalidate Request"},
    { "Update", "Update Data Item"},
    { "WriteComplete", "Write Completion Response"},
    { "StorePref", "Store Prefetch"}, 
    { "ReadPref", "Read Prefetch"}
};    
        
    
#endif
///////////////////////////////////////////////////////////////////////////////
// cmp_excl_msg_t
#ifdef COMPILE_CMP_EXCL 
const uint32 cmp_excl_protocol_t::TypeRead;
const uint32 cmp_excl_protocol_t::TypeReadEx;
const uint32 cmp_excl_protocol_t::TypeUpgrade;
const uint32 cmp_excl_protocol_t::TypeReplace;
const uint32 cmp_excl_protocol_t::TypeWriteBack;
const uint32 cmp_excl_protocol_t::TypeWriteBackClean;
const uint32 cmp_excl_protocol_t::TypeInvAck;
const uint32 cmp_excl_protocol_t::TypeDataRespShared;
const uint32 cmp_excl_protocol_t::TypeDataRespExclusive;
const uint32 cmp_excl_protocol_t::TypeInvalidate;
const uint32 cmp_excl_protocol_t::TypeReadFwd;
const uint32 cmp_excl_protocol_t::TypeReadExFwd;
const uint32 cmp_excl_protocol_t::TypeWriteBackAck;
const uint32 cmp_excl_protocol_t::TypeStorePref;
const uint32 cmp_excl_protocol_t::TypeReadPref;

const string cmp_excl_protocol_t::names[][2] = { 
	{ "Read", "Request for shared cpoy of data" },
	{ "ReadEx", "Request for exclusive copy of data" },
	{ "Upgrade", "Request for exclusive permission" },
	{ "Replace", "Send a replacement notification" },
	{ "WriteBackClean", "Write Back Clean (no data)" },
	{ "WriteBack", "Write Back Request" },
	{ "InvAck", "Acknowledge an Invalidate" },
	{ "DataRespShared", "Data Response Shared" },
	{ "DataRespExclusive", "Data Response Exclusive" },
	{ "Invalidate", "Invalidate Line" },
	{ "ReadFwd", "Read Forward Request" },
	{ "ReadExFwd", "ReadEx Forward Request" },
	{ "WriteBackAck", "Write Back Acknowledgement" },
    { "UpgradeAck",   "Upgrade Request Acknowledgement" },
    { "ReplaceAck", "Replacement Notification Acknowledgement"},
    { "StorePrefetch", "Store Prefetch Request" },
    { "ReadPrefetch", "Read Prefetch Request" },
};
#endif 

///////////////////////////////////////////////////////////////////////////////
// cmp_excl_3l_msg_t
#ifdef COMPILE_CMP_EX_3L
const uint32 cmp_excl_3l_protocol_t::TypeRead;
const uint32 cmp_excl_3l_protocol_t::TypeReadEx;
const uint32 cmp_excl_3l_protocol_t::TypeUpgrade;
const uint32 cmp_excl_3l_protocol_t::TypeReplace;
const uint32 cmp_excl_3l_protocol_t::TypeWriteBackClean;
const uint32 cmp_excl_3l_protocol_t::TypeWriteBack;
const uint32 cmp_excl_3l_protocol_t::TypeInvAck;
const uint32 cmp_excl_3l_protocol_t::TypeDataRespShared;
const uint32 cmp_excl_3l_protocol_t::TypeDataRespExclusive;
const uint32 cmp_excl_3l_protocol_t::TypeInvalidate;
const uint32 cmp_excl_3l_protocol_t::TypeReadFwd;
const uint32 cmp_excl_3l_protocol_t::TypeReadExFwd;
const uint32 cmp_excl_3l_protocol_t::TypeWriteBackAck;
const uint32 cmp_excl_3l_protocol_t::TypeUpgradeAck;
const uint32 cmp_excl_3l_protocol_t::TypeReplaceAck;
const uint32 cmp_excl_3l_protocol_t::TypeStorePref;
const uint32 cmp_excl_3l_protocol_t::TypeReadPref;
const uint32 cmp_excl_3l_protocol_t::TypeL1DataRespShared;
const uint32 cmp_excl_3l_protocol_t::TypeL1InvAck;

const string cmp_excl_3l_protocol_t::names[][2] = { 
	{ "Read", "Request for shared cpoy of data" },
	{ "ReadEx", "Request for exclusive copy of data" },
	{ "Upgrade", "Request for exclusive permission" },
	{ "Replace", "Send a replacement notification" },
	{ "WriteBackClean", "Write Back Clean (no data)" },
	{ "WriteBack", "Write Back Request" },
	{ "InvAck", "Acknowledge an Invalidate" },
	{ "DataRespShared", "Data Response Shared" },
	{ "DataRespExclusive", "Data Response Exclusive" },
	{ "Invalidate", "Invalidate Line" },
	{ "ReadFwd", "Read Forward Request" },
	{ "ReadExFwd", "ReadEx Forward Request" },
	{ "WriteBackAck", "Write Back Acknowledgement" },
    { "UpgradeAck",   "Upgrade Request Acknowledgement" },
    { "ReplaceAck", "Replacement Notification Acknowledgement"},
    { "ReplaceNotification", "Notification of Replacement from L2"},
    { "StorePrefetch", "Store Prefetch Request" },
    { "ReadPrefetch", "Read Prefetch Request" },
	{ "L1DataRespShared", "L1 Data Response Shared" },
	{ "L1InvAck", "L1 Acknowledge an Invalidate" },
};
#endif 


///////////////////////////////////////////////////////////////////////////////
// cmp_incl_3l_msg_t
#ifdef COMPILE_CMP_EX_3L
const uint32 cmp_incl_3l_protocol_t::TypeRead;
const uint32 cmp_incl_3l_protocol_t::TypeReadEx;
const uint32 cmp_incl_3l_protocol_t::TypeUpgrade;
const uint32 cmp_incl_3l_protocol_t::TypeReplace;
const uint32 cmp_incl_3l_protocol_t::TypeWriteBackClean;
const uint32 cmp_incl_3l_protocol_t::TypeWriteBack;
const uint32 cmp_incl_3l_protocol_t::TypeInvAck;
const uint32 cmp_incl_3l_protocol_t::TypeDataRespShared;
const uint32 cmp_incl_3l_protocol_t::TypeDataRespExclusive;
const uint32 cmp_incl_3l_protocol_t::TypeInvalidate;
const uint32 cmp_incl_3l_protocol_t::TypeReadFwd;
const uint32 cmp_incl_3l_protocol_t::TypeReadExFwd;
const uint32 cmp_incl_3l_protocol_t::TypeWriteBackAck;
const uint32 cmp_incl_3l_protocol_t::TypeUpgradeAck;
const uint32 cmp_incl_3l_protocol_t::TypeReplaceAck;
const uint32 cmp_incl_3l_protocol_t::TypeStorePref;
const uint32 cmp_incl_3l_protocol_t::TypeReadPref;
const uint32 cmp_incl_3l_protocol_t::TypeL1DataRespShared;
const uint32 cmp_incl_3l_protocol_t::TypeL1InvAck;

const string cmp_incl_3l_protocol_t::names[][2] = { 
	{ "Read", "Request for shared cpoy of data" },
	{ "ReadEx", "Request for exclusive copy of data" },
	{ "Upgrade", "Request for exclusive permission" },
	{ "Replace", "Send a replacement notification" },
	{ "WriteBackClean", "Write Back Clean (no data)" },
	{ "WriteBack", "Write Back Request" },
	{ "InvAck", "Acknowledge an Invalidate" },
	{ "DataRespShared", "Data Response Shared" },
	{ "DataRespExclusive", "Data Response Exclusive" },
	{ "Invalidate", "Invalidate Line" },
	{ "ReadFwd", "Read Forward Request" },
	{ "ReadExFwd", "ReadEx Forward Request" },
	{ "WriteBackAck", "Write Back Acknowledgement" },
    { "UpgradeAck",   "Upgrade Request Acknowledgement" },
    { "ReplaceAck", "Replacement Notification Acknowledgement"},
    { "ReplaceNotification", "Notification of Replacement from L2"},
    { "StorePrefetch", "Store Prefetch Request" },
    { "ReadPrefetch", "Read Prefetch Request" },
	{ "L1DataRespShared", "L1 Data Response Shared" },
	{ "L1InvAck", "L1 Acknowledge an Invalidate" },
};
#endif 

