/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: interconnect.h,v 1.1.2.5 2006/03/02 23:58:42 pwells Exp $
 *
 * description:    declaration of a interconnect for cmp_incl
 * initial author: Koushik Chakraborty 
 *
*/
 
 
class interconnect_t : public device_t {
     
     public:
        interconnect_t(uint32 _num_devices);
        virtual ~interconnect_t() {}
        virtual uint32 get_interconnect_id(device_t *);
        virtual uint32 get_latency(uint32 source, uint32 dest);
        virtual uint32 get_clustered_id(device_t *, uint32);
        virtual void setup_interconnect();
        virtual stall_status_t message_arrival(message_t *);
        virtual void from_file(FILE *file);
        virtual void to_file(FILE *file);
        virtual bool is_quiet() { return true;}
        
        virtual void cycle_end();
        
        
     protected:
        device_t **devices;
        link_t **internal_links;
        uint32 interconnect_devices;
        uint32   private_devices;
        
        bool    *link_active;
        uint32  *last_active;
     
		base_counter_t *stat_num_data_messages;
		base_counter_t *stat_num_addr_messages;
        base_counter_t *stat_migration_addr_msg;
        base_counter_t *stat_migration_data_msg;
        double_counter_t *stat_inactive_link_cycles;
};

