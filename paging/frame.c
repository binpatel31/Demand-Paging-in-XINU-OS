/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

extern int page_replace_policy;

//extern int fr_pid_track[NFRAMES][NPROC];
/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
	STATWORD ps;
  	disable(ps);
	
  	int i;
	for(i=0;i<NFRAMES;i++) 
	{
    		frm_tab[i].fr_status  = 0;
    		frm_tab[i].fr_pid     = -1;
    		frm_tab[i].fr_vpno    = 0;
    		int p;
			for(p=0;p<NPROC;p++)
			{
				fr_pid_track[i][p]=-1;
			}
			frm_tab[i].fr_refcnt  = 0;
    		frm_tab[i].fr_type    = 0;
    		frm_tab[i].fr_dirty   = 0;
			
    		scAcc[i]  = 0;
    		scPointer   = 0;
  	}
  	restore(ps);
  	return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  	STATWORD ps;
  	disable(ps);

  	int i;

	for(i=0;i<NFRAMES;i++) 
	{
    		if (frm_tab[i].fr_status == FRM_UNMAPPED) 
		{
      			scAcc[i] = 1;
			fr_pid_track[i][currpid]=1;
			*avail = i;
      			restore(ps);
      			return OK;
    		}
  	}
	
  	int frameNumber;
  	if (page_replace_policy == SC) 
	{
    		frameNumber = getFrameSC();
    		free_frm(frameNumber);
		//set SC bit for that frame
    		scAcc[frameNumber] = 1;
    		*avail = frameNumber;
    		restore(ps);
    		return OK;
  	}
  	else if(page_replace_policy==LFU)
  	{
    		int fr;
    		int min=0xffffffff;
    		int min_frame=-1;
    		for(fr=0;fr<NFRAMES;++fr)
    		{
        		if(frm_tab[fr].fr_type == FR_PAGE && frm_tab[fr].fr_cnt<min)
        		{
               			min = frm_tab[fr].fr_cnt;
                		min_frame=fr;
        		}
			else if(frm_tab[fr].fr_cnt == min && (frm_tab[fr].fr_vpno > frm_tab[min_frame].fr_vpno))
			{
				min_frame=fr;
			}
    		}
   		free_frm(min_frame);
   		*avail = min_frame;
   		restore(ps);
   		return OK;     
  	}
  	restore(ps);
  	return SYSERR;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
  	STATWORD ps;
  	disable(ps);
  	int pageNumber,index,frameID,storeID;

  	unsigned long virtualAddress,pdbr;
  	unsigned int pageTable,pageDirectory;

    	pd_t *pd_entry;
	int shift;
  	pt_t *pt_entry;

  	index = i;
  	int checkType = frm_tab[index].fr_type;
	int temp_vpno,temp_pid,temp_pdbr;
  	if (checkType == FR_PAGE) 
	{
		int proctabStore;
	    	temp_vpno = frm_tab[index].fr_vpno;
    		temp_pid = frm_tab[index].fr_pid;
		frameID = temp_pid;
		temp_pdbr = proctab[frameID].pdbr;
		int get_store = proctab[temp_pid].store;
		virtualAddress = temp_vpno;
    		
    		pdbr = temp_pdbr;
    		
		int and = 1024 - 1;
		int inHex = 0x003ff000;
    		pageTable = virtualAddress & 1023;
    		shift = 10;
    		pageDirectory = virtualAddress>>10;
    		proctabStore = get_store;
    		storeID = proctabStore;

    		pd_entry =  pdbr + (pageDirectory*sizeof(pd_t));
			
		int temp_vpno_2 = frm_tab[index].fr_vpno;
   
    		pt_entry =  ((sizeof(pt_t)*pageTable)+ (4096*(pd_entry->pd_base)));
    		int virt_heap_proc;
		virt_heap_proc = proctab[frameID].vhpno;
    		
    		pageNumber = frm_tab[index].fr_vpno - proctab[frameID].vhpno;

    		write_bs((index+1024)*4096, storeID, pageNumber);
		int f = pd_entry->pd_base;
    		pt_entry->pt_pres = 0;
    		int frameIndex =  f - 1024;

    		if ((frm_tab[frameIndex].fr_refcnt - 1) == 0) 
		{
      			frm_tab[frameIndex].fr_pid    = -1;
			//untrack
			fr_pid_track[frameIndex][currpid]=0;		
      			frm_tab[frameIndex].fr_status = FRM_UNMAPPED;
      			frm_tab[frameIndex].fr_vpno   = 4096;
      			frm_tab[frameIndex].fr_type   = FR_PAGE;
    		}
   	}
   	restore(ps);
  	return OK;
}

int getFrameSC() 
{
	STATWORD ps;
  	disable(ps);
  	int i = scPointer;

  	for(i=scPointer;;i++)
	{
    		i = i % 1024;
     		if (frm_tab[i].fr_type == FR_PAGE) 
		{
			if (scAcc[i] != 1)
			{
				scPointer = i + 1;
        			restore (ps);
        			return i;
			}
			else
			{
				scAcc[i]=0;
			}
    		}
  	}

  	restore(ps);
  	return SYSERR;
}

void frameDefine(int pid) 
{
	STATWORD ps;
	disable(ps);
	int i;
	for(i=0;i<NFRAMES;i++) 
	{
  		if (frm_tab[i].fr_pid == pid) 
		{
    			frm_tab[i].fr_status= FRM_UNMAPPED;
				frm_tab[i].fr_pid= -1;
				//untrack
				fr_pid_track[i][pid]=0;
				
				frm_tab[i].fr_vpno= 4096;
				frm_tab[i].fr_refcnt= 0;
				//which pid in i is current active
				int test_1;
				int debug=0;
				for (debug=0;debug<NPROC;debug++)
				{
					if (fr_pid_track[i][pid]=1)
					{
						test_1=pid;
						break;
						
					}
				}
				//kprintf("%d",test_1);		
				frm_tab[i].fr_type= FR_PAGE;
				frm_tab[i].fr_dirty	= 0;
  		}
		else
		{
			continue;
		}
	}
	restore(ps);
}
