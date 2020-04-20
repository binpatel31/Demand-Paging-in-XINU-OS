#include <conf.h>
#include <kernel.h>
#include<proc.h>
void pdbr_init (int pid) 
{
  STATWORD ps;
  disable(ps);
 // kprintf("==============================");
  unsigned long pdbr = proctab[pid].pdbr;
  write_cr3(pdbr);
  restore(ps);
}

