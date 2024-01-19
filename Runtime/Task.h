/***************************************************************

            Task.h             

    interface to background routines

*****************************************************************/


#ifndef  __TASK_H__
#define  __TASK_H__


int     Pend(const LPDRIVER_INST lpNet, LPSPECIAL_INST const lpData);
void    DeleteBackgroundTask(const LPDRIVER_INST lpNet);
void    VerifyDoneList(const LPLINKED_LIST lpDone);
int     WaitForAllFunctionCompletion(LPDRIVER_INST const lpNet);
int     CreateBackgroundTask( LPDRIVER_INST pNet );
/****************************************************************************/


#endif      /* __TASK_H__ */
