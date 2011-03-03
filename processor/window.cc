/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */


template <class T>
window_t <T>::window_t (const uint32 size) {
	window_size = size;
	window = new T * [window_size];

	for (uint32 i = 0; i < window_size; i++) {
		window [i] = 0;
	}

	last_created = window_size - 1;
	last_committed = window_size - 1;
    tmps = new T *[window_size];
}

template <class T>
window_t <T>::~window_t () {
	delete [] window;
    delete [] tmps;
}

template <class T> bool
window_t <T>::slot_avail (void) { //next slot should be free or else its full
	uint32 nextslot = window_increment (last_created);
	if (window [nextslot]) return false; else return true;
}

template <class T> uint32
window_t <T>::get_window_size (void) {
	return window_size;
}

template <class T> uint32
window_t <T>::window_increment (uint32 index) {
	if (index == window_size - 1) 
		return 0;
	return (index + 1);
}

template <class T> uint32
window_t <T>::window_decrement (uint32 index) {
	if (index == 0) 
		return (window_size - 1);
	return (index - 1);
}

template <class T> uint32
window_t <T>::window_wrap (uint32 index) {
	return index % window_size;
}

template <class T> bool
window_t <T>::insert (T *d) {
	if (!slot_avail ()) return false;

	last_created = window_increment (last_created);
	ASSERT (window [last_created] == 0);
	ASSERT (d != 0);

	window [last_created] = d;
	return true;
}

template <class T> T*
window_t <T>::head (void) {
	uint32 nextslot = window_increment (last_committed);
	return window [nextslot];
}

template <class T> T*
window_t <T>::tail (void) {
	return window [last_created];
}

template <class T> bool
window_t <T>::pop_head (T *d) {
    
    bool ret = false;
    // In SMT mode we can potentially evict entry from anywhere
    last_committed = window_increment (last_committed);
    
    if (window [last_committed] != d) {
        // Need to collapse
        uint32 slot = window_increment(last_committed);
        uint32 collapse_count = 0;
        while (slot != window_increment(last_created))
        {
            if (window [slot] == d)
                break;
            slot = window_increment(slot);
            collapse_count++;
        }
    
        ASSERT(window [slot] == d);
        uint32 new_slot;
        for (uint32 i = 0; i <= collapse_count; i++)
        {
            new_slot = window_decrement (slot);
            window [slot] = window [new_slot];
            slot = new_slot;
        }
        
        ret = true;
    }
    
	window [last_committed] = 0;
    return ret;
}

template <class T> void 
window_t <T>::pop_tail (T *d) {
	ASSERT (window [last_created] == d);
	window [last_created] = 0;
	last_created = window_decrement (last_created);
}

template <class T> void
window_t <T>::erase (uint32 index) {
	window [index] = 0;
}

template <class T> uint32 
window_t <T>::get_last_created (void) {
	return last_created;
}

template <class T> uint32
window_t <T>::get_last_committed (void) {
	return last_committed;
}

template <class T> T*
window_t <T>::peek (uint32 index) {
	return window [index];
}

template <class T> bool 
window_t <T>::empty (void) {
	return ( (last_created == last_committed) && (slot_avail ()) );
}

template <class T> bool
window_t <T>::full (void) {
	return (!slot_avail ());
}
	


template <class T> void window_t<T> ::collapse(void) {
    
    
    uint32 count = 0;
    
    uint32 slot = window_increment(last_committed);
    
    do {
        if (window[slot]) tmps[count++] = window [slot];
        slot = window_increment(slot);
    } while (slot != window_increment(last_created));
    
    for (uint32 i = 0; i < window_size; i++)
    {
        if (i < count) window[i]  = tmps[i];
        else window[i] = 0;
    }
    
    last_committed = window_size - 1;
    last_created = count - 1;
    
    
    
}
