/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: trans_rec.h,v 1.1 2005/02/02 18:15:37 kchak Exp $
 *
 * description:    Transaction record for forward progress error detection
 * initial author: Koushik Chakraborty
 *
 */
 

#ifndef _TRANS_REC_H_
#define _TRANS_REC_H_


class trans_rec_info {
    public:
        mem_trans_t *trans;
        tick_t request_cycle;
        
        trans_rec_info(mem_trans_t *, tick_t);
};

class trans_rec_t {
    private:
        uint32 num_procs;
        map<mem_trans_t *, trans_rec_info *> *records;
        
    public:
        trans_rec_t(uint32 num_procs);
        void insert_record(uint32 _proc, mem_trans_t *tr,tick_t req_time);
        
        void delete_record(uint32 _proc, mem_trans_t *);
        
        void detect_error(tick_t);
        
};


#endif
 
