/***************************************************************

                            errors.h

 ***************************************************************/


#ifndef __ERRORS_H__
#define __ERRORS_H__



/*********************************************************************************************************
     Driver specific errors 
     Returned by Open(), Reload(), OnLine(), OffLine(), Close() and SpecialFunction.Result.
     Resource strings should be provided.
     These codes are in the same space therefore must not colide "Vlc runtime errors"!
*/

#define IDS_NIUSBGPIB_BASE               0                           /* We may want the base set to 0, 
                                                                                  to preserve the manufacturer's error codes */
                                                              
#define IDS_NIUSBGPIB_HW_TEST            ( IDS_NIUSBGPIB_BASE + 1 )   /* Hardware test failed                                  */
#define IDS_NIUSBGPIB_INVALID_ADDERSS    ( IDS_NIUSBGPIB_BASE + 2 )   /* Address is out of range                               */
#define IDS_NIUSBGPIB_DEVICE_OFFLINE     ( IDS_NIUSBGPIB_BASE + 3 )   /* Address points to an offline device                   */
#define IDS_NIUSBGPIB_TIMEOUT            ( IDS_NIUSBGPIB_BASE + 4 )   /* Timeout error for this function                       */
#define IDS_NIUSBGPIB_READ_SIZE          ( IDS_NIUSBGPIB_BASE + 5 )   /* Read  buffer too short                                */
#define IDS_NIUSBGPIB_WRITE_SIZE         ( IDS_NIUSBGPIB_BASE + 6 )   /* Write buffer too short                                */
#define IDS_NIUSBGPIB_RW_ZERO            ( IDS_NIUSBGPIB_BASE + 7 )   /* Zero bytes to be transferred                          */
#define IDS_NIUSBGPIB_DPR_OUT            ( IDS_NIUSBGPIB_BASE + 8 )   /* Out of DPR image                                      */



/*********************************************************************************************************
     VLC runtime errors.  Operating system (iRMX) errors. 
     Returned by Open(), Reload(), OnLine(), OffLine() and Close().
     Resource strings should be provided.
     These codes are in the same space therefore must not colide "Driver specific errors"
*/

#define IDS_VLCRTERR_BASE               400                         /* Make sure it does not colide "Driver specific errors" */

#define IDS_VLCRTERR_CREATE_SEMAPHORE   ( IDS_VLCRTERR_BASE +  1 )  /* Cannot create semaphore       */
#define IDS_VLCRTERR_DELETE_SEMAPHORE   ( IDS_VLCRTERR_BASE +  2 )  /* Cannot delete semaphore       */
#define IDS_VLCRTERR_CREATE_DESCRIPTOR  ( IDS_VLCRTERR_BASE +  3 )  /* Cannot create descriptor      */
#define IDS_VLCRTERR_DELETE_DESCRIPTOR  ( IDS_VLCRTERR_BASE +  4 )  /* Cannot delete descriptor      */
#define IDS_VLCRTERR_CREATE_SEGMENT     ( IDS_VLCRTERR_BASE +  5 )  /* Cannot create segment         */
#define IDS_VLCRTERR_DELETE_SEGMENT     ( IDS_VLCRTERR_BASE +  6 )  /* Cannot delete segment         */
#define IDS_VLCRTERR_CREATE_TASK        ( IDS_VLCRTERR_BASE +  7 )  /* Cannot create background task */
#define IDS_VLCRTERR_DELETE_TASK        ( IDS_VLCRTERR_BASE +  8 )  /* Cannot delete background task */
#define IDS_VLCRTERR_CATALOG_OBJECT     ( IDS_VLCRTERR_BASE +  9 )  /* Cannot catalog object         */
#define IDS_VLCRTERR_UNCATALOG_OBJECT   ( IDS_VLCRTERR_BASE + 10 )  /* Cannot uncatalog object       */  
#define IDS_VLCRTERR_INVALID_INTERRUPT  ( IDS_VLCRTERR_BASE + 11 )  /* Interrupt other than 0,1,2,9,3,4,5,6,7,10,11,12,14,15 */  
#define IDS_VLCRTERR_SET_INTERRUPT      ( IDS_VLCRTERR_BASE + 12 )  /* Cannot set interrupt          */  
#define IDS_VLCRTERR_RESET_INTERRUPT    ( IDS_VLCRTERR_BASE + 13 )  /* Cannot reset interrupt        */  
#define IDS_VLCRTERR_ENABLE_INTERRUPT   ( IDS_VLCRTERR_BASE + 14 )  /* Cannot enable interrupt       */  
#define IDS_VLCRTERR_DISABLE_INTERRUPT  ( IDS_VLCRTERR_BASE + 15 )  /* Cannot disable interrupt      */  
#define IDS_VLCRTERR_TASK_TIMEOUT       ( IDS_VLCRTERR_BASE + 16 )  /* Background task initialization sequence timeout */  
#define IDS_VLCRTERR_IRMX_ERROR         ( IDS_VLCRTERR_BASE + 20 )  /* Other irmx error                                */
#define IDS_VLCRTERR_ALIGNMENT          ( IDS_VLCRTERR_BASE + 21 )  /* The runtime DRIVER_INST mismatches the compiled DRIVER_INST    */
#define IDS_VLCRTERR_OFFLINE_TIMEOUT    ( IDS_VLCRTERR_BASE + 22 )  /* OffLine() timeout waiting for the background task to calm down */
#define IDS_VLCRTERR_USB_STACK_ERROR    ( IDS_VLCRTERR_BASE + 23 )  /* OffLine() timeout waiting for the background task to calm down */


/*********************************************************************************************************
     VLC Special Function status.
     Returned by SpecialFunction.Status
     Resource strings are not necessary.
     These codes are in a VLC separate space.
*/
#define VLCFNCSTAT_BUSY             0       /* Asynchronous function running. Must be zero!             */
#define VLCFNCSTAT_OK               1       /* Function (successfully) completed. See the result field. */
#define VLCFNCSTAT_SIMULATE         2       /* Driver running in simulate mode. No action was performed */
#define VLCFNCSTAT_ABORTED          3       /* user re-posted io-block before complete                  */
#define VLCFNCSTAT_WRONGPARAM       4       /* software error: wrong function id.                       */


/*********************************************************************************************************
     General purpose error codes.
     Mainly useful at compile time. 
*/
#define  SUCCESS          0     /* Must be zero! */
#define  REMARK           1
#define  WARNING          2
#define  FAILURE          3


#endif          /*  __ERRORS_H__   */


