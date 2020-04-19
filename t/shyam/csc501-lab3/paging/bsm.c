/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
 bs_map_t bsm_tab[NBSM];
SYSCALL init_bsm()
{
	STATWORD ps;
	disable(ps);
	int i;
	// assign 2048 address to bsm_tab[0]
	// since we have 16 backing stores
	for(i=0; i<NBSM ; i++){
		free_bsm(i);
	}
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	STATWORD ps;
	disable(ps);
	*avail = 0;
	int i;
	for(i=0; i<NBSM; i++){
		//kprintf("current bs is %d, status = %d\n",i, bsm_tab[i].bs_status);
		if(bsm_tab[i].bs_status == BSM_UNMAPPED){
			*avail = i;
			restore(ps);
			return OK;
		}
	}
	restore(ps);
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	STATWORD ps;
	disable(ps);
	if(isbad_bsid(i)){
		restore(ps);
		return SYSERR;
	}
	bsm_tab[i].bs_status = BSM_UNMAPPED;
	//bsm_tab[i].bs_pid = -1; // do not assign any process to BS initially
	int k;
	for(k=0; k<50; k++){
		bsm_tab[i].bs_pid[k] = 0;
		bsm_tab[i].bs_vpno[k] = -1; 
		bsm_tab[i].bs_npages[k] = 0;
	}	
	bsm_tab[i].bs_sem = 0;
	bsm_tab[i].bs_private =0;
	bsm_tab[i].ref_count = 0;
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, unsigned long vaddr, int* store, int* pageth)
{
	STATWORD ps;
	disable(ps);
	int k;
	unsigned long vpno = (vaddr&0xfffff000)>>12;
	for(k=0; k<NBSM; k++){
		if((bsm_tab[k].bs_pid[pid]==1) && (vpno>=bsm_tab[k].bs_vpno[pid]) && (vpno<(bsm_tab[k].bs_vpno[pid]+bsm_tab[k].bs_npages[pid]))){
			
			*store = k;
			*pageth = (vaddr/NBPG)-bsm_tab[k].bs_vpno[pid];
			restore(ps);
			return OK;
		}
	}
	kprintf("bsm failed for address %lu,  process %d", vaddr, pid);
	restore(ps);
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	STATWORD ps;
	disable(ps);
	// just mapping for single process, need to implement the same for shared BS, multiple process
	if(isbad_npage(npages) || isbad_bsid(source)){
		restore(ps);
		return SYSERR;
	}
	proctab[pid].store = source;
	proctab[pid].backing_store_map[source]=1;
	bsm_tab[source].bs_status = BSM_MAPPED;
	bsm_tab[source].bs_pid[pid] = 1;
	bsm_tab[source].bs_npages[pid] = npages;
	bsm_tab[source].bs_vpno[pid] = vpno;
	bsm_tab[source].ref_count =1;
	restore(ps);
	return OK;
}


/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab and free relateed frames
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	/* will unmap given process from current backing store, free when all the process are unmapped unmap should save the page in backing store by calling write function before unmapping	*/
	STATWORD ps;
	disable(ps);
	if(isbadpid(pid)){
		restore(ps);
		return SYSERR;
	}
	/*flag==1 => the bsm_unmap is called while killing the process.
	if flag==0 then throw an error if the unmapping is for private heap */
	int store, pageth;
	if(bsm_lookup(pid, vpno*4096, &store,&pageth)==SYSERR){
		restore(ps);
		return SYSERR;
	}
	if(bsm_tab[store].bs_status==BSM_UNMAPPED){
		restore(ps);
		return SYSERR;
	}
	
	int vpno_base = bsm_tab[store].bs_vpno[pid];
	unsigned int k, temp;
	pd_t *pd_offset;
	pt_t *pt_offset;
	unsigned long vir_addr;
	/* unmapp all the frames of corresponding backing store for the current process */
	k = vpno_base;
	int dum;
	for(dum = 0; dum<bsm_tab[store].bs_npages[pid] ; dum++){
		vir_addr = k*4096;
		
		pd_offset = (proctab[currpid].pdbr)+((vir_addr>>22)*4);
		temp = (vir_addr&(0x0003ff000))>>12;
		/*  fetch the middle 10 bits of virtual page address */
		if (pd_offset->pd_pres==1){
			pt_offset = (pd_offset->pd_base*4096)+(temp*sizeof(pd_t));
			if (pt_offset->pt_pres ==1){	/* mention that the frame is not present */
				
				if(frm_tab[pt_offset->pt_base - FRAME0].fr_status==FRM_MAPPED){
					/*  write only if the frame is mapped */
					write_bs(pt_offset->pt_base * 4096, store, pageth );
					frm_tab[(pt_offset->pt_base) - FRAME0].fr_refcnt--;

					if(frm_tab[(pt_offset->pt_base) - FRAME0].fr_refcnt<=0){
						delete_from_sc_q(pt_offset->pt_base - FRAME0);
						free_frm(pt_offset->pt_base - FRAME0);
					}
				}
			}
		}
		k++;
	}
	

	/*  error if invalid npages or source, no process is mapped to bs, current process in not mapped to bs */
	
	if(proctab[pid].backing_store_map[store]==1){
			proctab[pid].backing_store_map[store]=0;
			bsm_tab[store].ref_count--;
		}
	bsm_tab[store].bs_pid[pid] = 0;
	bsm_tab[store].bs_npages[pid] = 0;
	bsm_tab[store].bs_vpno[pid] = 4096;
	
	if(bsm_tab[store].ref_count==0){
		free_bsm(store);
	}
	restore(ps);
	return OK;
}


