/********************************************************************

                                Auxrut.c

    Auxiliary routines to interface the operating system    

*********************************************************************/



#include "stdafx.h"

#include <rt.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "vlcport.h"
#include "errors.h"
#include "driver.h"
#include "auxrut.h"


//
// Primitives and globals for the creation and destruction of threads
//


// All tasks started by CreateThread use taskStub as an entry point.  This is so
// we can pass a parameter to the new thread.  The creator puts the address of TASK and parms
// in global variables.  The stub fires up the TASK with the parm address.  
// The creator knows global varibles are free when pTask->bBusy is set to 0.
BACKGROUND_THREAD pNewThread;
void* pNewThreadParam;
void  taskStub( void )
{
   // EROP( "taskStub\n", 0,0, 0,0 );
	pNewThread( pNewThreadParam );
}
/////////////////////////////////////////////////

void usb_attach(void * data)
{
	printf("usb_attach\n");
}

void usb_detach(void * data)
{
	printf("usb_detach\n");
}


void DetectUsbPlug(void* pParam)
{
	const LPDRIVER_INST pNet = (LPDRIVER_INST)( pParam);
	printf("DetectUsbPlug in\n");
	UsbEventsWithCallback(usb_attach, usb_detach, WAIT_FOREVER);
	printf("DetectUsbPlug out\n");
}


int CreateVLCThread(LPTASK pTask, BACKGROUND_THREAD pEntryPoint, void* pParam, UINT32 StackSize, int Priority )
{
    /* Create and lounch the background task */

	int rc = SUCCESS;
      
    if(rc == SUCCESS)
    { 

        rc = CreateSemaphore( &pTask->Semaphore );
        Wait( pTask->Semaphore);                       /* Remove the initial semaphore unit */
	////	rc = CreateSemaphore(&pTask->Semaphore);
	
    }
    
    if(rc == SUCCESS)
    {      
        /* Create task */

        pTask->bBusy = 1;
        pTask->Error = SUCCESS;
		pNewThreadParam = pParam;
		pNewThread = pEntryPoint;

//        pTask->hTask = CreateRtThread( (UINT8)Priority, taskStub, StackSize, (UINT16)0 );
		pTask->hTask = CreateRtThread((UINT8)Priority, DetectUsbPlug, StackSize, pParam);
        EROP( "R3: CreateThread() hTask=%x", pTask->hTask, 0,0,0 );

        if( pTask->hTask != BAD_RTHANDLE )
        {

            int WaitCnt = 1000;         /* 10 seconds timeout for the background task to complete its init sequence */
            while( pTask->bBusy && --WaitCnt )
                RtSleep(10);            /* sleep 10ms to let the background task to complete its init sequence */
                
            if( pTask->bBusy )
                rc = IDS_VLCRTERR_TASK_TIMEOUT;
        }
        else    
            rc = IDS_VLCRTERR_CREATE_TASK;
    }

    if( (rc == SUCCESS) && (pTask->Error != SUCCESS) )     /* any errors initializing the bkg task? */
        rc = pTask->Error;

    if( rc != SUCCESS )
        DeleteVLCThread(pTask);  /* Destroy the background task */
    
	EROP("R3: CreateThread() rc =%x", rc, 0, 0, 0);
    return rc;
}


int DeleteVLCThread(LPTASK lpTask)  /* Destroy the background task */
{
    int rc = SUCCESS; 
    
    if( lpTask->hTask )
    {
        UINT16 RqStatus = DeleteRtThread(lpTask->hTask);   /* make sure the background task is deleted anyway */
        lpTask->hTask = 0;

        if(!RqStatus)
            rc = IDS_VLCRTERR_DELETE_TASK;

    }

    if( lpTask->Semaphore )
    {
        int rc1 = DeleteSemaphore(&lpTask->Semaphore);

        if( rc == SUCCESS )
            rc = rc1;
    }

    return rc;
}


/******************* Routines for background task handling *******************************/

int DeleteSemaphore(RTHANDLE * const pSemaphore)
{
    UINT16 rc = SUCCESS;

    rc = DeleteRtSemaphore(*pSemaphore);
    *pSemaphore = 0;
    if(rc == FALSE)  
        rc = IDS_VLCRTERR_DELETE_SEMAPHORE;

    return rc;
}


int CreateSemaphore(RTHANDLE * const pSemaphore)
{
    UINT16 rc  = SUCCESS;
    *pSemaphore = CreateRtSemaphore((UINT16) 1,					 /* Initial */
                                    (UINT16) 1,					 /* Max     */
                                    (UINT16) FIFO_QUEUING);	     /* FIFO    */
    if( *pSemaphore == BAD_RTHANDLE )
    {
        rc = IDS_VLCRTERR_CREATE_SEMAPHORE;
        *pSemaphore = 0;
    }
    
    return rc;
}

void Wait (RTHANDLE semaphore)
{
	//if (semaphore == NULL) { printf("semaphore WAIT NULL \n"); return; }
	WaitForRtSemaphore(semaphore, 1, WAIT_FOREVER);
}

void Kick (RTHANDLE semaphore)
{
	//if (semaphore == NULL) { printf("semaphore KICK NULL \n"); return; }
	ReleaseRtSemaphore(semaphore, 1);

}


void InitLinkedList(const LPLINKED_LIST pList)
{
    Wait(pList->Semaphore);
    (VOID *) pList->pHead = NULL;
	(VOID *) pList->pTail = NULL;
    pList->uCounter = 0;
    Kick(pList->Semaphore);
}    


//  Links a SPECIAL_INST item to Pend or Done list 
int LinkFunction(LPLINKED_LIST pList, int bCallerOwnsSemaphore, const LPSPECIAL_INST pData)
{
    // Appends a new pData struct to the list

    UINT16 uCounter;

    assert( pData != NULL);
    pData->pNext = NULL;                    // Terminate this pData

    if( !bCallerOwnsSemaphore )
        Wait(pList->Semaphore);             // Get access to the list
        
    if( pList->uCounter )                   // See if other items are enqued here
	{
        pList->pTail->pNext = pData;        // Append to tail of the list
	}
    else 
	{
        pList->pHead = pData;               // First on the list
	}

    pList->pTail = pData;                   // And update the tail value

	pList->uCounter++;
    uCounter = pList->uCounter;

    if( !bCallerOwnsSemaphore )
        Kick(pList->Semaphore);             // Release access to the list
    
    return uCounter;                        // return the # of pDatas enqued here
}


/*  Unlinks the first SPECIAL_INST item from Pend or Done list
 */
int UnlinkFirstFunction(const LPLINKED_LIST pList, int bCallerOwnsSemaphore, LPSPECIAL_INST * ppRetData )
{
    UINT16 uCounter;
    
    if( !bCallerOwnsSemaphore )                                                      
        Wait(pList->Semaphore);              /* Get access to the list              */

    if( ppRetData != NULL )
        *ppRetData = pList->pHead;          /* our item or NULL                    */

    if( pList->uCounter )                    /* Is there any pData to be unlinked? */
    {
        pList->pHead = pList->pHead->pNext; /* Take it out from the link           */
        // Mem16Dec( &pList->uCounter );
		pList->uCounter--;
    }

    uCounter = pList->uCounter;

    if( !bCallerOwnsSemaphore )
        Kick(pList->Semaphore);                /* Release access to the list              */
    
    return uCounter;                            /* return the # of items still enqued here */
}


/*  Unlinks a specific SPECIAL_INST item from Pend or Done list
 *  If NULL specified, the first SPECIAL_INST is dequed 
 */
int UnlinkFunction(const LPLINKED_LIST pList, int bCallerOwnsSemaphore, LPSPECIAL_INST pData, LPSPECIAL_INST * ppRetData )
{
    UINT16 uCounter;
    
    if( ppRetData != NULL )
        *ppRetData = NULL;

    if( !bCallerOwnsSemaphore )
        Wait(pList->Semaphore);              /* Get access to the list                     */

    if( pList->uCounter )                    /* Is there any lpData to be unlinked?                     */
    {
        LPSPECIAL_INST plpData = pList->pHead;
        LPSPECIAL_INST pPrev = NULL;          /* Needed when unlinking the last lpData, to update pTail. */
        
        if(pData == NULL)                    /* if NULL --> unlink the first lpData                     */
            pData = plpData;
    
        for(; plpData != NULL; pPrev = plpData, plpData = plpData->pNext )   /* Walk all lpDatas */
            if(plpData == pData)                                                   /* Found it ?       */
            {
                plpData = pData->pNext;             /* Take it out from the link                       */

                if(pData->pNext == NULL)             /* Did we just remove the last lpData?             */
                    pList->pTail = pPrev;            /* If so, pTail must point to the previous lpData. */
                // Mem16Dec( &pList->uCounter );
				pList->uCounter--;
                if( ppRetData != NULL )
                    *ppRetData = pData;
                break;
            }    
            
    }

    uCounter = pList->uCounter;

    if( !bCallerOwnsSemaphore )
        Kick(pList->Semaphore);                /* Release access to the list              */
    
    return uCounter;                            /* return the # of items still enqued here */
}


// Time related functions

/*******************************************************************************
 *  GetCrtMiliSecond()     
 *  Returns the crt ms since rmx start. 
 *  This value is safe: it is independent on system clock changes.
 *  It is incrementrd in KN ticks increments and wraps around every 50 days 
 */ 
UINT32 GetCrtMiliSecond()
{
    static unsigned short int TickRatio    = 0;
    static unsigned short int TickInterval = 0;
	KNTIME    lpTime;

    if(!TickRatio)
    {
        const SYSINFO pSysInfo;
		
		CopyRtSystemInfo((LPSYSINFO) &pSysInfo);
    
        TickRatio    = pSysInfo.KernelTickRatio;
        TickInterval = pSysInfo.NucleusTickInterval;   /* Milsec / nuc tick */
    }    

    knGetKernelTime((LPKNTIME) &lpTime);
    return (((UINT32) lpTime.lo) * TickInterval) / TickRatio;
}


UINT32 StartTimeout(const int Timeout)    /* Timeout in ms, returns crt ms + Timeout. Always returns a non-zero value */
{
    const UINT32 MarkTime = GetCrtMiliSecond() + Timeout;
    return MarkTime ? MarkTime : MarkTime+1;
}

/* MarkTime must be generated by StartTimout. Returns TRUE or FALSE */
/* If MarkTime == 0 or n == 0 --> consider no timeout control       */
/* If MarkTime != 0 and timeout expired --> return TRUE.            */
int IsTimeout(const UINT32 MarkTime)
{
    int rc = FALSE;     // assume we still have to wait.
    if( MarkTime )
    {
        const SINT32 ToWait = MarkTime - GetCrtMiliSecond();
        if(ToWait < 0)  
            rc = TRUE;          /* timout expired */
    }
        
    return rc;
}


void Delay( int n )
{
    RtSleep( n );
}


#define PAGE_MASK  0x00000FFFl      /* 4k page */

//
// Primitives used to map in physical dual port memory
//


// #define FAKED_DPR

#ifdef  FAKED_DPR

void * AllocateDpr( UINT32 PhysicalAddress, UINT32 Size )
{
    return malloc( Size );   // for test purposes !!!
}


int FreeDpr(void * dpr)
{
    free( dpr );           // for test purposes !!!
	return SUCCESS;
}

#else

void * AllocateDpr( UINT32 PhysicalAddress, UINT32 Size )
{
    char*   Address = NULL;
    UINT32  PageOffset = 0;

    if( PhysicalAddress & PAGE_MASK )   // if no 4k start
    {
        PageOffset = PhysicalAddress & PAGE_MASK;
        PhysicalAddress &= ~PAGE_MASK;  // map it at a lower address;
        Size += PageOffset;             // and add the extra space
    }

    if( Size & PAGE_MASK )              // if no 4k size
    {
        UINT32 PageComplement = PAGE_MASK + 1 - (Size & PAGE_MASK);
        Size += PageComplement;         // make sure Size is an integer # of pages.
    }

    Address = MapRtPhysicalMemory( PhysicalAddress, Size );

	return Address ? (void*)(Address + PageOffset) : NULL;
}


int FreeDpr(void * dpr)
{
	int rc = SUCCESS;

    if( (UINT32)dpr & PAGE_MASK )
    {
        UINT32 PageOffset = (UINT32)dpr & PAGE_MASK;
        (char*)dpr -= PageOffset;
    }

    rc = FreeRtMemory( dpr );

	if (rc == -1)
		rc = IDS_VLCRTERR_DELETE_DESCRIPTOR;

	return(rc);
}

#endif  //FAKED_DPR

/*************************************************************************************

    Debug error messages.  Uses printf.  
    Make sure no printf (puts,...) is called when running Windows: The sytem hungs up!
*/

void Erop( char* format, const int arg1, const int arg2, const int arg3, const int arg4)
{
    EROP( format, arg1, arg2, arg3, arg4);
}





/*
//------------------------------------------------------
//
// Function    : CardCopy
//
// Description : Slow memory copy (to minimize performance hit on card)
//               Avoid memcpy that mey confuse the card.
//               Performs 2 or 4 bytes access to diminish data tearing occurances
//
// Arguments   : void far *Dest
//               void far *Source
//               unsigned int Size
//
// Returns     : void
//
//------------------------------------------------------
*/
void CardCopy(VOID *Dest, VOID *Source, UINT32 Size)
{
    for(; Size >= 4; Size -= 4) *((UINT32 volatile*)Dest)++ = *((UINT32 volatile*)Source)++;
    for(; Size >= 2; Size -= 2) *((UINT16 volatile*)Dest)++ = *((UINT16 volatile*)Source)++;
    for(; Size     ; Size--   ) *((UINT8  volatile*)Dest)++ = *((UINT8  volatile*)Source)++;
}



/***************************************************************/





