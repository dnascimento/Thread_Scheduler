/*                                                          Emacs: -*- c -*-
 *
 * This file is just a place holder for the real assembly, which is a in a
 * cpu-specific file we conditionally include.
 */

#include <config.h>

#ifdef STHREAD_CPU_I386	
#include "sthread_switch_i386.h"
#endif

#ifdef STHREAD_CPU_POWERPC
#include "sthread_switch_powerpc.h"
#endif		
