#include <conf.h>
#include <kernel.h>
#include <proc.h>

extern void pdbr_init(int pid);

void pdbr_init (int pid) 
{
  STATWORD ps;
  disable(ps);
  unsigned long pdbr = proctab[pid].pdbr;
  write_cr3(pdbr);
  restore(ps);
}

