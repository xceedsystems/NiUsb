/*************************************************************

            Auxrut.h             

    auxiliary routines

**************************************************************/


#ifndef  __AUXRUT_H__
#define  __AUXRUT_H__



#include "csflat.h"     // csPrintf(),  and other call back functions implemented in coresup


typedef void (* BACKGROUND_THREAD)(void *);


int     DeleteSemaphore(SEMAPHORE * const pSemaphore);
int     CreateSemaphore(SEMAPHORE * const pSemaphore);
int     CreateVLCThread( LPTASK pTask, BACKGROUND_THREAD pEntryPoint, void* pParam, UINT32 StackSize, int Priority );
int     DeleteVLCThread( LPTASK lpTask);
void    Wait (SEMAPHORE semaphore);
void    Kick (SEMAPHORE semaphore);
int     LinkFunction(LPLINKED_LIST pList, int bCallerOwnsSemaphore, const LPSPECIAL_INST pData);
int     UnlinkFirstFunction(const LPLINKED_LIST pList, int bCallerOwnsSemaphore, LPSPECIAL_INST * plpRetData );
int     UnlinkFunction(const LPLINKED_LIST pList, int bCallerOwnsSemaphore, LPSPECIAL_INST pData, LPSPECIAL_INST * plpRetData );
void    InitLinkedList(const LPLINKED_LIST pList);
UINT32  StartTimeout(const int Timeout);
UINT32  GetCrtMiliSecond();
int     IsTimeout(const UINT32 MarkTime);
void    Delay( int n );


int     FreeDpr(void * dpr);
void*   AllocateDpr( UINT32 PhysicalAddress, UINT32 Size );
void    CardCopy(PVOID pDest, PVOID pSource, UINT32 Size);
void    Erop( char* format, ... );

/*************************************************************************************

    Debug error messages uses printf.  
    Make sure no printf (puts,...) is called when running Windows: The sytem hungs up!
*/



//#define  PRINT	/* define this to use termserv printed messages */

#if defined( PRINT ) /*&& defined( _DEBUG )*/
    #define EROP(Format, Arg1, Arg2, Arg3, Arg4)                    \
            {                                                       \
                printf(Format, Arg1, Arg2, Arg3, Arg4);             \
                printf("\n");                                       \
            }
#else
    #define EROP(Format, Arg1, Arg2, Arg3, Arg4){}
#endif

//************************************************************

#endif    /*  __AUXRUT_H__  */

