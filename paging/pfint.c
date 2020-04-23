/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

extern int page_replace_policy;

int pageCreate() 
{
	  STATWORD ps;
	  disable(ps);
	  
	  int frameNumber;
	  

	  get_frm(&frameNumber);

	  unsigned int frameAddress;
	  frameAddress = (1024 + frameNumber) * 4096;

	  pt_t *pageTable = (pt_t *)frameAddress;
	  int index;// = 0;
	  for(index=0;index<1024;index++) 
	  {
			pageTable[index].pt_pcd   = 0;
			pageTable[index].pt_acc   = 0;
			pageTable[index].pt_dirty = 0;
			pageTable[index].pt_mbz   = 0;
			pageTable[index].pt_global= 0;
			pageTable[index].pt_avail = 0;
			pageTable[index].pt_base  = 0;
                        pageTable[index].pt_pres  = 0;
                        pageTable[index].pt_write = 0;
                        pageTable[index].pt_user  = 0;
                        pageTable[index].pt_pwt   = 0;
	  }
	  restore(ps);
	  return frameNumber;
}
/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
	  STATWORD ps;
	  disable(ps);

	  
	  unsigned long pdbr,tmp;
	  
	  unsigned int pg, pt, pd;

	  
	  
	  pt_t *pageTable;

	  unsigned long virtualAddress;
	  virtualAddress = read_cr2();   //tmp_read;

	  virt_addr_t *virt_addr;
	  virt_addr = (virt_addr_t *)&virtualAddress;
	  pg = virt_addr->pg_offset;
	  int sz = sizeof(pd_t);
	  pt = virt_addr->pt_offset;
	  pd = virt_addr->pd_offset;
	  pdbr = proctab[currpid].pdbr;

	  pd_t *pd_entry;
	  pd_entry = pdbr + (virt_addr->pd_offset * sizeof(pd_t) );

	  int new_PT; 
	  if (pd_entry->pd_pres == 0)
	  {
	         	new_PT = pageCreate();
			pd_entry->pd_acc    = 0;
			pd_entry->pd_mbz    = 0;
			pd_entry->pd_fmb    = 0;
			pd_entry->pd_global = 0;
			pd_entry->pd_avail  = 0;
			pd_entry->pd_pres   = 1;
			pd_entry->pd_write  = 1;
			pd_entry->pd_user   = 0;
			pd_entry->pd_pwt    = 0;
			pd_entry->pd_pcd    = 0;

			
		       //	int base = 1024 + new_PT;
			pd_entry->pd_base = 1024 + new_PT;

			frm_tab[new_PT].fr_status = FRM_MAPPED;
			if(frm_tab[new_PT].fr_refcnt<0)
			{
				frm_tab[new_PT].fr_refcnt=0;
			}
			frm_tab[new_PT].fr_refcnt+=1;
			//frm_tab[new_PT].fr_refcnt = 0;
			frm_tab[new_PT].fr_type   = FR_TBL;
			int base = 1024 + new_PT;
			frm_tab[new_PT].fr_pid    = currpid;
     	}
    	int addTwoTwo = ((pt * sizeof(pt_t)) + (pd_entry->pd_base * 4096));
	pt_t *pt_entry;
	pt_entry = (pt_t *)(addTwoTwo);

	int newFrame;
	int store, pageth;
    	if (pt_entry->pt_pres == 0) 
	{
			get_frm(&newFrame);
			pt_entry->pt_base = 1024 + newFrame;
			int sub = pd_entry->pd_base - 1024;
			pt_entry->pt_pres   = 1;
			pt_entry->pt_write  = 1;
	
			int passVal = (1024 + newFrame) * 4096;

                        if(frm_tab[sub].fr_refcnt<0)
                        {
                                frm_tab[sub].fr_refcnt=0;
                        }			
			frm_tab[sub].fr_refcnt+= 1;
			int ins_vpno = virtualAddress/4096;
			frm_tab[newFrame].fr_status = FRM_MAPPED;
			frm_tab[newFrame].fr_type   = FR_TBL;

			fr_pid_track[newFrame][currpid]=1;

			frm_tab[newFrame].fr_pid    = currpid;
			frm_tab[newFrame].fr_vpno   = ins_vpno;

			bsm_lookup(currpid, virtualAddress, &store, &pageth);
			
//			kprintf("=======&&&&& %d",frm_tab[sub].fr_refcnt);			
			read_bs((char *)((1024 + newFrame) * 4096), store, pageth);
    }

  write_cr3(pdbr);
  restore(ps);
  return OK;
}
