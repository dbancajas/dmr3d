/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id $
 *
 * description:    Base class for spin loop detection heauristics
 * initial author: Koushik Chakraborty 
 *
 */
 
 
class spin_interface_t {
     public:
        spin_interface_t();
        virtual void observe_memop(dynamic_instr_t *) = 0;
        virtual bool check_spinloop() = 0;
        
};
