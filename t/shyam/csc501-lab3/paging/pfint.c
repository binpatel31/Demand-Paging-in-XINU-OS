/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
  	STATWORD ps;
	disable(ps);

	int store =-1, pageth=-1, new_pt_frame_number, new_pt_page_number=0;
	unsigned long fault_address = read_cr2();
	virt_addr_t *virtual_address;
	int pd_offset=(fault_address>>22)&(0x000003ff);
	int pt_offset = (fault_address>>12)&(0x000003ff);
  
  
  if(bsm_lookup(currpid, fault_address, &store, &pageth) == SYSERR){		/* check that fault address is a legal address i.e mapped to any physical address */
	  kill(currpid);
	  restore(ps);
	  return SYSERR;
  }
  pd_t *pd;
  pd = (pd_t *)(proctab[currpid].pdbr + ((pd_offset)*4));
  if(pd->pd_pres ==0){		/* page is not present in main memory, get it from backing store */
	new_pt_frame_number = create_page_table();

	if(new_pt_frame_number==-1){
	  restore(ps);
	  return SYSERR;
	}
	frm_tab[new_pt_frame_number].fr_type = FR_TBL;
	frm_tab[new_pt_frame_number].fr_status = FRM_MAPPED;
	frm_tab[new_pt_frame_number].fr_refcnt = 0;
	frm_tab[new_pt_frame_number].fr_pid[currpid] = 1;
	pd->pd_pres =1;
	pd->pd_write = 1;
	pd->pd_base = FRAME0+new_pt_frame_number;	/* set the base frame of new page in page table */
  }
  pt_t *pt;
  pt= ((pd->pd_base*NBPG)+(pt_offset*sizeof(pt_t)));
 
  /*if(pt->pt_pres==0)	 check if the frame requested is already in memory, if present just share the frame*/
	  int share_frame = check_if_in_memory(currpid, store, pageth);

	if(share_frame !=-1){
		pt->pt_base = share_frame;
		pt->pt_pres = 1;
		frm_tab[share_frame-FRAME0].fr_refcnt++;
		frm_tab[share_frame-FRAME0].fr_pid[currpid]=1;
		frm_tab[share_frame-FRAME0].fr_vpno[currpid] = fault_address/NBPG;
		restore(ps);
		return OK;
	}
	
	get_frm(&new_pt_page_number);
	if(get_frm(&new_pt_page_number)==-1){
		restore(ps);
		return SYSERR;
	}
	if(sc_flag==1){
		sc_flag = 0;
		insert_sc_queue(new_pt_page_number);			/* Insert into SC policy queue only if the returned frame is new, not shared	*/
	}
	
	frm_tab[(pd->pd_base)-FRAME0].fr_refcnt += 1;
	frm_tab[new_pt_page_number].fr_status = FRM_MAPPED;
	frm_tab[new_pt_page_number].fr_pid[currpid] = 1;//currpid;
	frm_tab[new_pt_page_number].fr_vpno[currpid] = fault_address/NBPG;
	frm_tab[new_pt_page_number].fr_refcnt = 1;
	frm_tab[new_pt_page_number].fr_type = FR_PAGE;
	frm_tab[new_pt_page_number].fr_dirty = 0;
	pt->pt_pres =1;
	pt->pt_write =1;
	pt->pt_user=1;
	pt->pt_dirty=1;
	pt->pt_global=0;
	pt->pt_base = FRAME0+new_pt_page_number;
	read_bs((char *)(pt->pt_base * NBPG),store, pageth);	// called from bsm_lookup
	write_cr3(proctab[currpid].pdbr);
	restore(ps);
	return OK;
}

int check_if_in_memory(int pid,int store,int pageth){
	int ret =-1,k=0;
	unsigned long virtual_pno;
	pd_t *pd_offset ;
	pt_t *pt_offset;
	if(bsm_tab[store].bs_private==1){
		return ret;
	}
	/* implies bs is shared, check if the requested page is in memory */
	
	for(k=0; k<50; k++){
		// iterate over all process and see if any process is mapped to the backing store
		// each virtual address is a page number
		if(bsm_tab[store].bs_pid[k]==1 && k!=pid){

			// get the virtual address of pageth of this process
			virtual_pno = bsm_tab[store].bs_vpno[k] + pageth;
			
			pd_offset = proctab[k].pdbr + ((virtual_pno/1024)*4);
			if(pd_offset->pd_pres==1){
				pt_offset = (pd_offset->pd_base*NBPG)+((virtual_pno&0x000003ff)*sizeof(pt_t));
				if(pt_offset->pt_pres==1){
					ret = pt_offset->pt_base;
					return ret;
				}
			}
		}
	}
	return ret;
}
