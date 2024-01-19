/**************************************************************

                TASK.C

***************************************************************/

#include "stdafx.h"


#include <rt.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "vlcport.h"
#include "csflat.h"     // LowPriority
#include "dcflat.h"     // BuildUiotPointer()
#include "driver.h"
#include "errors.h"     /* IDS_RTERR_...                       */
#include "auxrut.h"     /* StartTimeout(), IsTimeout(), EROP() */
#include "card.h"       // DoCommand
#include "task.h"




/********************* Driver almost specific  *******************************/

int DoAsyncSpecialFunction( LPDRIVER_INST const pNet, SPECIAL_INST* const pData )   /* pData access is not protected by semaphore! */
{
	if (pData == NULL)
	{
		return VLCFNCSTAT_WRONGPARAM;
	}

    int rc = VLCFNCSTAT_WRONGPARAM;

    const int FunctionId = pData->Work.paramHeader.FunctionId;

    switch(FunctionId) 
    {
        case  DRVF_COMMAND:
		{
            DoCommand( pNet, pData );

            rc = VLCFNCSTAT_OK;     /* as far as the VLC is concerned, the function completed */
        }                   
        break;    

		case DRVF_READ_GPIB:
		{
			EROP(":--- DoGpibRead() rc =%x", rc, 0, 0, 0);
			DoGpibRead(pNet, pData);
			rc = VLCFNCSTAT_OK;     /* as far as the VLC is concerned, the function completed */
			EROP(":-5-- DoGpibRead() rc =", 0, 0, 0, 0);
		}
		break;
		
		case DRVF_SET_RSV:
		{
			EROP(":--- DoGpibRead() rc =%x", rc, 0, 0, 0);
			DoIbRSVWrite(pNet, pData);
			rc = VLCFNCSTAT_OK;     /* as far as the VLC is concerned, the function completed */
			EROP(":-5-- DoGpibRead() rc =", 0, 0, 0, 0);
		}


	}

    return rc;                          /* this is a non-zero value              */
}



/*
static int IsCompleted( PSPECIAL_INST pData)
{
    PDRIVER_INST pNet = (PDRIVER_INST )pData->pNet;
    DUAL_PORT far*  dp  = (DUAL_PORT far*)pNet->pDpr;    / * pointer to the dualport     * /
    int Status     = SUCCESS;                             / * or FUNC_ERR_STATUS_OK,  ... * /
    
    return Status;
}


static int IsExpired( PSPECIAL_INST pData)
{
    return (GetCrtSecond() > pData->Timeout) ? FUNC_ERR_STATUS_TIMEOUT : SUCCESS;
}
*/


/**********************************************************************
 *
 *  Pretty generic routines related to the background task handling  
 *
 **********************************************************************/ 

#define BUSY_NOTBUSY    0       /* io special block is not in use    */
#define BUSY_BUSY       1       /* io special block is in use        */
#define BUSY_ABORT      2       /* user spoiled the io block         */
    



/********************************************************************************
 *  Here is the background task. Is is designed as an endless loop.
 */
void far BackgroundTask( LPDRIVER_INST const pNet )
{
    TASK far* pTask = &pNet->BackgroundTask;
    int bEmpty = 1;
        
    /*  Initialize linked list pointers */
	
//    EROP( "R3: BackgroundTask() pNet=%p, PortAddress=%x", pNet, pNet->PortAddress, 0, 0 );

    for(;;) 
    {
        LPSPECIAL_INST pData = NULL;
		
        if( bEmpty)                                 /* If  Pend list is empty,                  */
        {
		
            pTask->bBusy = 0;                      /* Tell the world we can be shutdown        */
            Wait( pTask->Semaphore);               /* List empty, wait for go ahead            */
			pTask->bBusy = 1;                      /* Tell the world we are not to be shutdown */


        }

        bEmpty = !UnlinkFirstFunction( &pNet->Pend, FALSE, &pData);  /* first pData == current item */

        assert( pData != NULL );

        if (pData->Busy == BUSY_BUSY)                                /* if not double-posted, */
            pData->Status = DoAsyncSpecialFunction( pNet, pData );   /* process the request   */

        LinkFunction(&pNet->Done, FALSE, pData ); /* and append it to the done list */

    }

}



static char* GetCatalogName( UINT16 Port )
{
    static char TaskName[16];

    strcpy( TaskName, "NIUSBGPIB" );

    // sprintf needs to be implemented in FCoreSup
    // sprintf( TaskName, "NIUSBGPIB%03X", (int)Port );  /* max 12 characters */
    return TaskName;
}



void DeleteBackgroundTask(const LPDRIVER_INST pNet)
{
	UINT16 Port = pNet->GpibAddr;

   DeleteVLCThread(&pNet->BackgroundTask);
   DeleteSemaphore( &pNet->Pend.Semaphore );
   DeleteSemaphore( &pNet->Done.Semaphore );
}






int CreateBackgroundTask( LPDRIVER_INST pNet )
{

	//return SUCCESS;
    int    rc        = SUCCESS;
	UINT16 Port = pNet->GpibAddr;// PortAddress;
    UINT16 Priority  = LowPriority;
    UINT32 StackSize = 4096UL;

    if(rc == SUCCESS)
    {

        rc = CreateSemaphore( &pNet->Pend.Semaphore );
        if(rc == SUCCESS)
            InitLinkedList(&pNet->Pend);

    }    

    if(rc == SUCCESS)
    {
        rc = CreateSemaphore( &pNet->Done.Semaphore );
        if(rc == SUCCESS)
            InitLinkedList(&pNet->Done);

    }    

    if(rc == SUCCESS)
    {
		EROP(": BackgroundTask() rc =%x", rc, 0, 0, 0);
		// Create thread and make sure the other thread sees lpNet.
		rc = CreateVLCThread( &pNet->BackgroundTask, BackgroundTask, pNet, StackSize, Priority );
    }                                                                                                 

    if(rc != SUCCESS)
        DeleteBackgroundTask( pNet );

    return rc;
}

int Pend(const LPDRIVER_INST pNet, LPSPECIAL_INST const pData)
{
    /* Adds a new pData struct to Pending list */
    int   rc = VLCFNCSTAT_BUSY;

    if( pData->Busy == BUSY_NOTBUSY ) 
    {
        const LPLINKED_LIST pPend = &pNet->Pend;
        pData->Busy   = BUSY_BUSY;                 /* mark it busy               */
        pData->Status = VLCFNCSTAT_BUSY;
        memcpy(&pData->Work, &pData->User, sizeof(SPECIAL_INST_PARAM));

        if( LinkFunction(pPend, FALSE, pData) == 1 )  /* Append pData to Pend list */
            Kick( pNet->BackgroundTask.Semaphore);     /* if first --> kick the task */
    }
    else
    {

        rc = VLCFNCSTAT_ABORTED;
        pData->Busy = BUSY_ABORT;
    }

    return rc;
}



void SetReturnStatus( const LPSPECIAL_INST pData )
{
    /***************************************************************************
     *  This function also resynchronizes the result set by the background task,
     *  with the main thread.
     *  It is called by Input() so, at this moment the user code is idle.
     *  We assume pData points to a valid async function that just completed, 
     *  and the Work.Header.pStatus return pointer is correctly generated.
     */

	UINT16* pStatus = BuildUiotPointer( pData->Work.paramHeader.ofsStatus );
    *pStatus = pData->Status ? pData->Status : VLCFNCSTAT_OK;

	
}


void VerifyDoneList(const LPLINKED_LIST pDone)
{
    /* Pass back parameter blocks that have now completed */
    /* and empty up the Done List as fast as possible     */

    
    if( pDone->uCounter )
    {
        Wait(pDone->Semaphore);        /* It takes less time to remove all pDatas with only 1 wait */

        while( pDone->uCounter )
        {
            LPSPECIAL_INST pData = NULL;
            
            UnlinkFirstFunction( pDone, TRUE, &pData );

            assert( pData != NULL );                    /* this is the only place to remove items from the Done list */
                    
            if( pData->Busy == BUSY_BUSY )                 /* if not double-posted, */
            {                                               /* set user status       */
            
                /***************************************************************************
                 *  This function also resynchronizes the result set by the background task,
                 *  with the main thread.
                 *  It is called by Input() so, at this moment the user code is idle.
                 *  We assume pData points to a valid async function that just completed, 
                 *  and the Work.Header.pStatus return pointer is correctly generated.
                 */
        
                SetReturnStatus( pData);       /* set user status */
            }    
    
            pData->Busy = BUSY_NOTBUSY;
        }

        Kick(pDone->Semaphore);  
    }
}
    


/*******************************************
    MarkTime == 0 --> no timeout control. 
*/
static int IsValidMarkTime( UINT32 MarkTime )
{
    return MarkTime && !IsTimeout( MarkTime );
}


/* 'wait' for empty pend and calm background task */
int WaitForAllFunctionCompletion(LPDRIVER_INST const pNet)
{
    int rc  = SUCCESS;
    int Cnt = 30;   // Wait at most 30 seconds
  
    // This mechanism may be improved.  
    // We may want to have the pNet->Pend.pHead special function how much to wait.  

    for( ;; )
    {
        if( !Cnt-- )
        {
            rc = IDS_VLCRTERR_OFFLINE_TIMEOUT;
            break;
        }

        if( !pNet->Pend.uCounter && !pNet->BackgroundTask.bBusy )
        {
            break;              // We are done!
        }
        else
        {
            RtSleep( 1000 );    // Wait 1 second
        }
    }

    VerifyDoneList( &pNet->Done );

    return rc;
}              
              
              
              


/****************************************************************************/

