/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

#define SETZERO 0
#define SETONE  1
#define TWOTEN  1024

#define ANDVAL  0xfffff000
/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm(){

  STATWORD ps;
  disable(ps);
  int index = 0;
  int twoFourTen = TWOTEN * 4;
  while (index< NBSM) 
  { 
    bsm_tab[index].bs_status = BSM_UNMAPPED;
    int proc = 0;
    while (proc < NPROC) 
    {    
      bsm_tab[index].bs_pid[proc] = 0;
      bsm_tab[index].bs_npages = 0;
      bsm_tab[index].bs_vpno[proc] = 4096;  // do -1 if not work
      proc+=1;
    }
    //bsm_tab[index].bs_npages  = SETZERO;
    bsm_tab[index].bs_sem     = 0;
    bsm_tab[index].bs_private = 0;
    bsm_tab[index].bs_reference_cnt = 0;
    bsm_tab[index].bs_mapping = 0;
    index += 1;
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

  int index = 0;
  while (index < NBSM) 
  { 
    //int checkStatus = bsm_tab[index].bs_status;
    if (bsm_tab[index].bs_status == 0) 
    {
      *avail = index;
      restore(ps);
      return OK;
    }
    index += 1;
  }
  *avail=0;
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
	if (i<0 || i >=NBSM)
	{
		restore(ps);
		return SYSERR;
	}
	bsm_tab[i].bs_status=BSM_UNMAPPED;
   	bsm_tab[i].bs_status = BSM_UNMAPPED;
    	int proc = 0;
    	while (proc < NPROC)
    	{
      		bsm_tab[i].bs_pid[proc] = 0;
      		bsm_tab[i].bs_npages  = 0;
     	 	bsm_tab[i].bs_vpno[proc] = 4096;  // do -1 if not work
      		proc+=1;
    	}
    	//bsm_tab[index].bs_npages  = SETZERO;
        bsm_tab[i].bs_sem     = 0;
        bsm_tab[i].bs_private = 0;
        bsm_tab[i].bs_reference_cnt = 0;
        bsm_tab[i].bs_mapping = 0;
  
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	STATWORD ps;
	disable(ps);
 
  	int index = SETZERO;
 	unsigned long taddr = (vaddr&ANDVAL);
  	taddr = (taddr)>>12;
  	while (index < NBSM) 
  	{
    		int valid = bsm_tab[index].bs_pid[pid];
    		if (valid==1) 
		{
                	int val1 = bsm_tab[index].bs_vpno[pid]+bsm_tab[index].bs_npages;
			int val2 = bsm_tab[index].bs_vpno[pid];
			if(taddr < val1 && taddr >= val2)
			{
      				*store = index;
      				//int vpn = bsm_tab[index].bs_vpno[pid];
      				*pageth = taddr - val2; // here change taddr to (vaddr/NBPG)
      				restore(ps);
      				return OK;
			}
    		}
    		index = index + SETONE;
  	}
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
	if (npages<=0  || npages>128)
	{
		restore(ps);
		return SYSERR;
		
	}
        if (source<0  || source>=16)
        {
                restore(ps);
                return SYSERR;

        }

  //	if (bsm_tab[source].bs_status==BSM_UNMAPPED) 
//	{
      		bsm_tab[source].bs_status = BSM_MAPPED;
      		bsm_tab[source].bs_npages = npages;
  //	}
  bsm_tab[source].bs_pid[pid] = 1;
  bsm_tab[source].bs_sem      = 0;
  bsm_tab[source].bs_vpno[pid]= vpno;
  // int setVPN = vpno;
  //int setSource = source;
  bsm_tab[source].bs_reference_cnt=1;
  bsm_tab[source].bs_mapping = 1;
  proctab[currpid].vhpno = vpno;
  proctab[currpid].store = source;
  proctab[currpid].bs[source]=1;
  restore(ps);
  return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
  	STATWORD ps;
	disable(ps);
  	if (pid<0 || pid>=NPROC)
	{
		restore(ps);
		return SYSERR;
	}

 // 	int index = 0;
       	int bs_no = proctab[pid].store;
  	
  	int pageth;
  	//unsigned long virtualAddress = vpno * 4096;
  	bsm_tab[proctab[pid].store].bs_mapping = bsm_tab[proctab[pid].store].bs_mapping - 1;
  	int i;
	unsigned long virt_addr = vpno*4096;
	for(i=0;i<1024;++i)
	//while (index < TWOTEN) 
	{
     	//	int checkPid = frm_tab[i].fr_pid;
    	//	int checkTyp = frm_tab[i].fr_type;
		
		if(frm_tab[i].fr_pid == pid)
	    	//if (checkPid == pid && checkTyp == 0) 
		{
			if(frm_tab[i].fr_type == FR_PAGE)
			{
       	 			if (bsm_lookup(pid, virt_addr, &bs_no, &pageth) != SYSERR) 
				{
          				//continue;
          				write_bs(((1024+i) * 4096), bs_no, pageth);
        			}
        			//int twotenI = TWOTEN+i;
        			//int mult = (1024+i) * 4096;
        			//write_bs(((1024+i) * 4096), bs_no, pageth);
			}
    		}
    		//index = index + SETONE;
  	}
  	restore(ps);
  	return OK;
}
