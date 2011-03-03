General info on mem-hier:

----------------- Usage -------------------

Current "memory-topology" configuration variable options:

unip-one-default -- default setup for a single level uniprocessor cache
                    can be unified I/D

unip-two-default -- default setup for two-level, split I/D uniproc cache
                    D-cache is write-through no allocate
                    I-Cache is kept cohereny with D-cache writes
                    L2 is write-back, allocate
                    It is not necessary to use the ICache

msi-one-bus -- relatively simple single-level bused based MSI MP protocol
               can be used in an uniprocessor if desired

msi-two-bus -- Pritave MSI l1s connected to a bus and a shared L2
               can be used in an uniprocessor if desired

minimal-default     -- For testing
mp-minimal-default  -- For testing
unip-one-bus        -- For testing


----------------- Interface -------------------
Calls to the mem-hier go to:
  make_request(mem_trans_t *)

The mem-hier calls the processor's:
  complete_request(mem_trans_t *)
when the transaction has been performed

A separate module, mem-driver, can be used to translate simics's
timing-model interface to that of the mem-hier


----------------- Network -------------------

device_t   - (abstract class) a 'device' in the memory hierarchy 
             objects such as caches, main mem, or even a bus should inherit 
			 this class

link_t     - a generic, unidirectional, point-to-point 'link' between two
             devices
			 link_t is nothing more than an interface
             buffered_link_t is a link with delay and buffering

mem_trans_t - a transaction generated either by the processor module or the 
              mem-driver module, one for each memory request
			  
message_t  - send across a link between two devices 


I envision the logic of a 'network' (such as a bus or switch) as being embodied
in a device, with links to and from each device that's a member
of the bus.  Havn't written the bus yet though.

---------              ---------
- cache -    link_t    -       -
- (dev) -  --------->  -       -
---------  <---------  -       -   link_t   ----------
             link_t    -  bus  - ---------> - memory -
   ...                 - (dev) - <--------- - (dev)  -
                       -       -   link_t   ----------
---------              -       -
- cache -    link_t    -       -
- (dev) -  --------->  -       -
---------  <---------  ---------
             link_t

			 
			 
----------------- Protocols -------------------

XXX_prot_sm_t - the protocol state machine definition, which includes type
                definitions for the actions, states, etc.  

XXX_msg_t  - the definition of the message that passes between two devices,
             including the typedef of the message_type enumeration

proc_t     - a templated class representing the processor device
             it is templated with the protocol class that it uses to communicate
			 with its connected device (i.e. an L1 cache)

main_mem_t - a templated class representing main memory
             it also is templated with the protocol class that it uses to 
			 communicate with its connected device(s) (i.e. an L2 cache)

cache_t    - an abstract class that contains the basic cache interface and 
             some limited functionality.  It implements a state machine, and
             connects two links (which have different message types)

tcache_t   - a templated cache class that inherits cache_t It contains as much of
             the functionality common among protocols as reasonable
             It is templated on the prot_sm_t, as well as the msg_t types of both
             the 'up' (closer to proc) and 'down' (farther from proc) links
			 
XXX_cache_t - inherets tcache_t and provides the protocol specific functionality
              such as the statemachine definition
			 
statemachine_t - a templated (abstract) class specifying statemachine iface
			 
mem_hier_t - main class which (will) handle config, etc.  Main entry point
             connects to proc_t objects (below)


The geneology looks something like this:

device_t --> cache_t --> tcache_t<prot_t> --> XXX_cache_t
                            ^
statemachine_t<prot_t> -----|

device_t --> main_mem_t<prot_t>

device_t --> proc_t<prot_t>


It seems like a long way to get to the protocol's cache (XXX_cache_t), but I
wanted to
 a) be able to have multiple cache classes with different protocols
 b) inherit common functionality
 c) be able to refer to a generic cache object (to find info about
    its size, assoc, etc) without having to know which protocol it used 
I wasn't able to think of a better way, but am still willing to take
suggestions!

The only protocol I currently have is unip_one_protocol_t (uniprocessor
one-level).

Though I like the table-lookup statemachine in our previous modules, I decided
against that for this protocol (though another protocol could do it differently)
because I wanted:
 a) to be able to have an arbitrary number of (simple) transition functions per
 event without having an extremely sparse table
 b) its nice to encode a _small_ amount of logic in the there that is hard to
 code into the table (i.e. you get a miss and want to allocate an MSHR then send
 the request, but what if you can't alloate an MSHR?)
 c) transition functions don't have to be static

 
--------------- Open Issues ----------------------------

How to configure the network, individual caches, etc.? Probably a large
collection of global variables is not the best way. Nor is a global string that
selects from a preselected list. Maybe some kind of config 'structure' that can
be associated with each device or something?

How to include and compile in all the protocols: It might not be a good idea to
force every protocol .cc file to be compiled even if its not being used -- both
because of compile time, library size, and more importantly, every protocol must
compile all the time inorder to use any.

SW prefetch instructions are currently treated as loads. Should either a) ignore
or b) don't block them until the data is avail

----------------- Caching Data -------------------------

Requires that most things be handled in a more correct manner than do our
coherence protocols used only for timing.

  Uncacheable accesses:  Need to be handled correctly.

  Atomic accesses: Simple protocols treat first access as a write (for coherence
  purposes) and ignore the second. Correct protocols need to see both
  transactions. The cache line is aquired in a modifiable state, locked after
  the first read, then unlocked when the next transaction comes along.
  
  I/O: Currently, all accesses to IO memory space are ignored. IO devices that
  access physical memory (i.e. DMA write) have to be handled. Unfortunately,
  these accesses cannot be stalled. IO device reads are currently ignored,
  though that needs to change eventually. IO device writes need to supply the
  data to subsequent reads. Currently, an IO write causes all subsequent
  requests to block until the write is finished (all caches flush any lines that
  are part of the IO write). This is heavy handed and can be relaxed in the
  future.
  
  LDD/LDDA/Flush: Need a better mechanism to identify these instructions. Need
  to know either a) don't need to supply data for this (flush) or *do* need to
  (i.e. the second, unstallable access of ldd/ldda which are marked atomic for
  u3 and *not* marked atomic for u2). Need to look at opcode instead of current
  hacks!
  
The only protocol that supports caching data currently is unip_one_conplex

Open issues:

  IO Requirements:
  - Can't stall simics transactions
  - Check the returned value of *all* reads with simics (for development)
  - Simple as possible (not an research topic that we care about now)
  - Not pose a significant performance problem (depends on frequency of IO) 

  Architecture says that IO controller participates in cache coherence protocol
  on a line by line basis, multi-line accesses are not guranteed to be read or
  written atomicly. Simics however does them atomically. In order to check the
  values returned by the mem-hier with what simics provides, we need to do this
  as well. In the current implementation, iodev sends IOBegin to caches, caches
  flush all affected lines, wait for any WriteBackAcks, then send an IOAccessOK
  to iodev. IO then sends write to mem, waits for Ack, and sends IODone to
  caches. In the meantime, mem_hier::make_request() blocks new transactions.

  Data from IO writes need to be provided to subsequent loads immediately (i.e.
  can't wait for messages from IOdev to propogate.  Lots of ways to do this, but
  they all seem to suck.
  

