#include <sthread.h>


#ifndef STDLIB_H
#include <stdlib.h>
#endif


static sthread_mon_t mon_delay = NULL;
static int Is_off = 1;
static int sleep_time = 0;

void io_delay_on(int disk_delay)
{
   mon_delay = sthread_monitor_init();
   Is_off = 0;
   sleep_time = disk_delay;
}

void io_delay_simulator()
{
   if (Is_off) {
      free(mon_delay);
      return;
   }
   sthread_monitor_enter(mon_delay); 
   sthread_sleep(sleep_time);

   sthread_monitor_exit(mon_delay); 
}

void io_delay_read_block()
{
      io_delay_simulator();
}

void io_delay_write_block()
{
      io_delay_simulator();
}

