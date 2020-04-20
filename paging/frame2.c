/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

//#define SETZERO 0
//#define SETONE  1
//#define TWOTEN  1024
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
	
  	int i;// = 0;
	for(i=0;i<NFRAMES;i++)
  	//while (i < 1024) 
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
    		//i = i + 1;
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

  	int i;// = 0;
  	//*avail    = -1;

	for(i=0;i<NFRAMES;i++)
  	//while (i < NFRAMES) 
	{
    		//int checkStatus = frm_tab[i].fr_status;
    
    		if (frm_tab[i].fr_status == FRM_UNMAPPED) 
		{
      			//kprintf("IN");
      			scAcc[i] = 1;
				fr_pid_track[i][currpid]=1;
				*avail = i;
      			restore(ps);
      			return OK;
    		}
    //		i = i + 1;
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
  	//int index;
  	//int frameID;
  	//int storeID;
  //	int checkType;
  	unsigned long virtualAddress,pdbr;
  	unsigned int pageTable,pageDirectory;
  	//unsigned int pageDirectory;
  	//unsigned long pdbr;
  	pd_t *pd_entry;
  	pt_t *pt_entry;

  	index = i;
  	int checkType = frm_tab[index].fr_type;
	int v_p_n_o,p_i_d,p_d_b_r;
  	if (checkType == FR_PAGE) 
	{
	    	v_p_n_o = frm_tab[index].fr_vpno;
    		p_i_d = frm_tab[index].fr_pid;
			frameID = p_i_d;
			p_d_b_r = proctab[frameID].pdbr;
			virtualAddress = v_p_n_o;
    		
    		pdbr = p_d_b_r;
    		
			int andVal = 1024 - 1;
			int inHex = 0x003ff000;
    		pageTable = virtualAddress & 1023;//andVal;
    		int shiftVal = 10;
    		pageDirectory = virtualAddress>>shiftVal;
    		int proctabStore = proctab[p_i_d].store;
    		storeID = proctabStore;

    		//int a = sizeof(pd_t);
    		//int b = pageDirectory;
    		//int mult = b * a;
    		//int c = pdbr;
    		//int add = c + mult;
			//page _direcory add
    		pd_entry =  pdbr + (pageDirectory*sizeof(pd_t));//add;
			
			int v_p_n_o_dos = frm_tab[index].fr_vpno;
    		//int d = sizeof(pt_t);
    		//int e = pageTable;
    		//int multTwo = d * e;
    		//int twoFourTen = 1024 * 4;
    		//int f = pd_entry->pd_base;
    		//int multThree = twoFourTen * f;
    		//int addTwo = multTwo + multThree;
    		pt_entry =  ((sizeof(pt_t)*pageTable)+ (4096*(pd_entry->pd_base)))   ;//addTwo;

    		int proctabVh = proctab[frameID].vhpno;
    		
    		pageNumber = frm_tab[index].fr_vpno - proctab[frameID].vhpno;

    		//int indexFrame = index + 1024;
    		//indexFrame = indexFrame * twoFourTen;
    		write_bs((index+1024)*4096, storeID, pageNumber);
		int f = pd_entry->pd_base;
    		pt_entry->pt_pres = 0;
    		int frameIndex =  f - 1024;

    		if ((frm_tab[frameIndex].fr_refcnt - 1) == 0) 
		{
      			frm_tab[frameIndex].fr_pid    = -1;
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
  	int i = scPointer; //0
  	//i = i + scPointer;

  	for(i=scPointer;;i++)
	//while(1) 
	{
    		i = i % 1024;
    		//int checkType = frm_tab[i].fr_type;
    		if (frm_tab[i].fr_type == FR_PAGE) 
			{
      			//int checkSCValue = scAcc[i];
      			//
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
				//
				//if (checkSCValue == 1) 
				//{
        		//	int updateSCVAL = 0;
        		//	scAcc[i] = updateSCVAL;
      			//} 
				//else 
				//{
        		//	scPointer = i + 1;
        		//	restore (ps);
        		//	return i;
      			//}
    		}
    		//i = i + 1;
  	}

  	restore(ps);
  	return SYSERR;
}

void frameDefine(int pid) 
{
	STATWORD ps;
	disable(ps);
	int index;// = 0;
	for(index=0;index<NFRAMES;index++)
//	while (index < 1024) 
	{
    		int checkP_i_d = frm_tab[index].fr_pid;
  		if (frm_tab[index].fr_pid == pid) 
		{
    			frm_tab[index].fr_status= 0;
				frm_tab[index].fr_pid= -1;
				frm_tab[index].fr_vpno= 1024 * 4;
				frm_tab[index].fr_refcnt= 0;
				frm_tab[index].fr_type= 0;
				frm_tab[index].fr_dirty	= 0;
  		}
  		
  		//index = index + 1;
	}
	restore(ps);
}
