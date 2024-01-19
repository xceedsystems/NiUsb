/****************************************************************************

                            Driver.h

    NIUSBGPIB Driver specfic UIOT structures and adjunct to the network node

*****************************************************************************/



#ifndef __DRIVER_H__
#define __DRIVER_H__


// "NIUSBGPIB" id num. Make sure this id is unique across all VLC C-modules and drivers.
#define DriverNIUSBGPIB       0x9e7f8d42L


/*
    Version 01.0002
    Bump this version # every time DRIVER_INST, DEVICE_INST or
    DEVICE_IO structs have changed. 
    This will force old projects to be recompiled before execution. 
*/
#define	OUR_CHANGE_VER 0x0001003AL

#define NIUSBGPIBVERS       OUR_CHANGE_VER



//  Network config struct id
#define NETCONFIG_ID        DriverNIUSBGPIB


/* 
    Network config struct version
    Bump this version # every time NETCONFIG got new fields. 
    NetPass1() will force drivers in old projects to be reconfigured. 
    If old fields and NETCONFIG size are preserved, 
    configuration in old fields will be kept. 
*/
#define NETCONFIG_VERS      OUR_CHANGE_VER //0x0001000AL



// Device  config struct id
#define DEVCONFIG_ID        0x12345678L    



/*
    Device  config struct version
    Bump this version # every time DEVCONFIG got new fields. 
    NetPass1() will force devices in old projects to be reconfigured. 
    If old fields and DEVCONFIG size are preserved, 
    configuration in old fields will be kept. 
*/
#define DEVCONFIG_VERS      OUR_CHANGE_VER //0x0001000AL     


// load this value in DRIVER_INST for rt checking
#define RT3_SENTINEL        0x55667788L


// max 4 networks can be controlled by 1 PC
#define  MAX_DRV_INSTANCES          4       



#include  "errors.h"


#ifndef APSTUDIO_READONLY_SYMBOLS



#ifndef VLCPORT_H__
#include "vlcport.h"
#pragma warning( disable: 4244 )
#endif

#ifndef DATASEG_H__
#include "dataseg.h"
#endif



/*****************************************************************************************

    This file keeps all necessary definitions needed for our NIUSBGPIB driver.
    This is a driver build for didactical purposes. It can be used as a starting point 
    when we need to create a new real driver.
    The NIUSBGPIB driver assumes we have an IO network. 
    The network is controlled by a card and can have up to MAX_DEVICES devices. 
    The card is accesible by a dual port ram and a port address.
    The DRP is DPR_TOTAL_SIZE bytes large and contains a control space, an input space and
    an output space.
    To keep things simple, our devices have all the same size: 8 bytes. 
    They are mapped in the DPR IO spaces based on their network address: device 0's input
    area can be found at the beginning of the DPR input space, device 1's input area is 
    8 bytes farther...  The same mechanism works for devices' output points. 
    In order to see input tags changing, we have to use an external application 
    that writes in the DPR input space.  We can also use the POKE special function to write there. 
    When generating the driver we can change DPR_OUTPUT_OFF to match DPR_INPUT_OFF. 
    Input space will then overlap the output space, so in the VLC project all input tags 
    will be animated by their correspondent output tags.    

    Configuring the driver:
    1.  Choose a DPR address.
    2.  Choose a port address (didactic purpose only: will not be used)
    3.  Choose an interrupt level (didactic purpose only: the interrupt routine does nothing)
    4.  Skip HW tests. We may want to control the thoroughness of initial hw tests.
    5.  Simulate:  if on, there will be no attempt to touch the hardware.
    6.  Watchdog:  if on, the card's watchdog must be cyclicly kicked at run time.
    7.  Cyclic Input Read:  if on,  we update UIOT input image every Input() call.
                            if off, we have to rely on some hardware features telling us if 
                                    any input changed.
    
    Configuring devices:
    1.  Choose a link address (0 ... 127). This determines allocation in the DPR IO space
    2.  Critical:  if on, this device must be active at Online()    
    

    There are 5 different driver models we are studying:
    Model1:     No special functions at all.    (Simulate)
    Model2:     Only sync s.f. No background task.  (PID, Utility, ...)
    Model3:     Sequential async s.f. processing:  Pend & Done lists. (ex. ABKTX, MTL, ...)
                    DRIVER_INST needs  MarkTime.
    Model4:     Simoultaneous async s.f. processing: Pend, Run, Done lists (ex. DHPLUS, ...)
                    The hw supports commands with reply. 
                    New commands can be launched while others are waiting for their replies.
    Model5:     Paralel sequential s.f. processing. 
                    The hw supports a fixed # of channels that can accept commands.
                    Commands executed on different channels can run in paralel.
                    Commands executed on a channel are performed on a FIFO basis.
                    Pend[], Done lists    (DATALOG)
                    DRIVER_INST needs  MarkTime[].
    

    Here is an example for model 3:    
    
*****************************************************************************************/



//************   1 byte alignment typedef structures !!!   ************

#pragma BYTE_ALIGN(_SPECIAL_INST)
typedef  struct _SPECIAL_INST*  LPSPECIAL_INST_;
typedef                 UINT8*  LPUINT8;

typedef  selector   SEMAPHORE;

typedef  UINT32     UIOTREF2CHAR;
typedef  UINT32     UIOTREF2SINT8;
typedef  UINT32     UIOTREF2UINT8;
typedef  UINT32     UIOTREF2SINT16;
typedef  UINT32     UIOTREF2UINT16;
typedef  UINT32     UIOTREF2SINT32;
typedef  UINT32     UIOTREF2UINT32;
typedef  UINT32     UIOTREF2DOUBLE;
typedef  UINT32     UIOTREF2VOID;


#pragma BYTE_ALIGN(_LINKED_LIST)
typedef struct _LINKED_LIST 
{
    LPSPECIAL_INST_     pHead;      // Pointer to the first element in the linked list
    LPSPECIAL_INST_     pTail;      // Pointer to the last element in the linked list
    SEMAPHORE           Semaphore;  // Semaphore that locks the SPECIAL_INST list
    UINT16              uCounter;   // How many items are enqueued here
} LINKED_LIST, *LPLINKED_LIST; 
#pragma BYTE_NORMAL()


#pragma BYTE_ALIGN(_PTBUFFER)
typedef struct _PTBUFFER 
{
    UIOTREF2VOID    Offset;  
    UINT32          Size;           // Use PTBUFFER type for PT_BUFFERs
} PTBUFFER, * LPPTBUFFER;           // Its size is 8 bytes
#pragma BYTE_NORMAL()


#pragma BYTE_ALIGN(_TASK)
typedef struct _TASK 
{
    UINT16      hTask;          // background/interrupt task handle
    SEMAPHORE   Semaphore;      // Where the background task waits
    void*       pStack;         // Pointer to the stack allocated to the task
    UINT16      bBusy;          // True if Special I/O Task is working on packet, used during shutdown
    UINT16      Error;          // error code for the task's init sequence
    void*       IrqThunk;       // pointer to the interrupt routine
    UINT16      level;          // irmx encoded IRQ
    UINT16      align;
} TASK, * LPTASK; 
#pragma BYTE_NORMAL()

#pragma BYTE_ALIGN(_DEVICE_IO)  // 1 byte alignment
typedef struct _DEVICE_IO       // Specifies the UIOT offset and the size for each device
{
	void*        pSrc;          // DPR  offset/pointer for input devices || UIOT offset/pointer for output devices
	void*        pDst;          // UIOT offset/pointer for input devices || DPR  offset/pointer for output devices
	UINT16       Size;          // device input or output size.  Never 0 !!!
	UINT16       Address;       // device's network address
	UIOTREF2VOID ofsUiot;       // ofsDst for input devices or ofsSrc for output devices
	UINT8        bPresent;      // If the device missed at OnLine(), skip it
	UINT8        bUsed;         // If no I/O tags defined in the UIOT, skip it
	UINT16       align;
} DEVICE_IO, *LPDEVICE_IO;
#pragma BYTE_NORMAL()


#pragma BYTE_ALIGN(_DEVICE_INST)         // 1 byte alignment
typedef struct _DEVICE_INST
{
    UIOTREF2VOID ofsName;       // UIOT offset to the device name generated at compile time
    UINT16       Address;       // device's network address
    UINT16       Type;          // DEVICE_4W_INPUT, ... Never 0 !!!
    UINT16       ProductCode;     
    UINT8        bCritical;     // if 1 --> device must be online when load and go
    UINT8        bPresent;      // if 1 --> device was online when load and go
    char*        pName;         // Usable UIOT pointer to the device name generated at runtime based on ofsName.
} DEVICE_INST, *LPDEVICE_INST;
#pragma BYTE_NORMAL()

#pragma BYTE_ALIGN(_DRIVER_INST) 
typedef struct _DRIVER_INST 
{
    NETWORK Net;

        // Compile-time Static variables.  This structure maps to UIOT

    UIOTREF2VOID    ofsDeviceList;      // Where the DEVICE_INST list starts.
    UIOTREF2VOID    ofsInputList;       // Where in the DEVICE_IO  Input list starts
    UIOTREF2VOID    ofsOutputList;      // Where in the DEVICE_IO Output list starts
    UINT16          GpibAddr;		    // 0xd0000, 0xd1000, ...
	UINT16          Vendor_ID;           // 
    UINT16          Product_ID;          // 
    UINT16          StopState;          // SS_HOLD_LAST_STATE,  SS_ZERO
    UINT16          bSimulate;          // =0 --> interface card must be present
    UINT16          bWatchdog;          // =1 --> kick the watchdog
    UINT16          DprHWTests;         // HWTEST_RW, HWTEST_OFF
    UINT16          InputRead;          // INPUT_READ_CYCLIC, INPUT_READ_COS,
	UINT16          usbIndex;
	UINT16          usbCount;
	UINT16          busNum;
	UINT16          addr;
	UINT16          PortNum;
	UINT16          depth;
	UINT16          ifNum;
	UINT16          align1;				// must align to 32bits

        // Run-time Dynamic Variables
    LPDEVICE_INST   pDeviceList;        // Where the DEVICE_INST list starts.
    LPDEVICE_IO     pInputList;         // Where in the DEVICE_IO  Input list starts
    LPDEVICE_IO     pOutputList;        // Where in the DEVICE_IO Output list starts

    void*           pDpr;               // (DUAL_PORT*) - iRmx ptr to where the board is in physical memory
    UINT16          bFirstCycle;        // Set by OnLine(), reset by Output(). Read by Input() and Output()
    UINT16          bGoOffLine;         // Tell all the bkg functions to shutdown

    LINKED_LIST     Pend;               // Pointer to the linked list of pending functions
    LINKED_LIST     Done;               // Pointer to the linked list of done  functions

    TASK            BackgroundTask;     // controls for the background task
    TASK            InterruptTask;      // controls for the interrupt task

	//points to our gpib
	void* 		USB_Data;

    UIOTREF2UINT32  ofsSentinel;        // 0x55667788 - display this value using Soft Scope to check corrct map
    UINT32          Sentinel;           // 0x55667788 - display this value using Soft Scope to check corrct map


} DRIVER_INST, *LPDRIVER_INST;    
#pragma BYTE_NORMAL() 


#pragma BYTE_ALIGN( _SPECIAL_INST_HEADER )      // Must be first block in all paremeter blocks
typedef struct _SPECIAL_INST_HEADER
{       // Compile-time Static variables.  This structure maps to .rcd descrition
                                    // off, sz, ob.sz
    UINT16          FunctionId;     //  0    2   2L   PT_CONST  --> UINT16, _SIZE 2L
    UINT16          align;          //  2    2
    UIOTREF2UINT16  ofsStatus;      //  4    4   2L   PT_REF    --> tag's offset in the UIOT
    UIOTREF2UINT16  ofsResult;      //  8    4   2L   PT_REF    --> tag's offset in the UIOT
} SPECIAL_INST_HEADER;              //      12 == sizeof( SPECIAL_INST_HEADER )
#pragma BYTE_NORMAL()
/*
    Note: beacuse all functions have an Id field and a return status, we can standardize them 
    at offsets 0 and 4. This is especially helpful when using customized parameter structures 
    to better match function particularities and to save memory. 
*/
#pragma BYTE_ALIGN( _SET_PAD_COMMAND ) 
typedef struct _SET_PAD_COMMAND
{       // Compile-time Static variables.  This structure maps to .rcd descrition
										// off, sz, ob.sz
	SPECIAL_INST_HEADER Header;         //  0   12        the header must always be first
	UIOTREF2SINT16      ofsCallResult;	// 12	 4	 2L
	UINT16              padAddr;        // 16    2  2L   PT_VALUE  --> UINT32, _SIZE 4L
	UINT16              align;          // 18    2	2L
} SET_PAD_COMMAND;                 //    16 == sizeof( SPECIAL_INST_COMMAND )
#pragma BYTE_NORMAL()

#pragma BYTE_ALIGN( _SET_IBRSV_COMMAND ) 
typedef struct _SET_IBRSV_COMMAND
{       // Compile-time Static variables.  This structure maps to .rcd descrition
										// off, sz, ob.sz
	SPECIAL_INST_HEADER Header;         //  0   12        the header must always be first
	UIOTREF2SINT16      ofsCallResult;	// 12	 4	 2L
	UINT16              Code;	        // 16    2  2L   PT_VALUE  --> UINT32, _SIZE 4L
	UINT16              align;          // 18    2	2L
} SET_IBRSV_COMMAND;                 //    16 == sizeof( SPECIAL_INST_COMMAND )
#pragma BYTE_NORMAL()

#pragma BYTE_ALIGN( _CLEAR_TXRX_COMMAND ) 
typedef struct _CLEAR_TXRX_COMMAND
{       // Compile-time Static variables.  This structure maps to .rcd descrition
										// off, sz, ob.sz
	SPECIAL_INST_HEADER Header;         //  0   12        the header must always be first
	UIOTREF2SINT16      ofsCallResult;	// 12	 4	 2L
	UINT16              Code;	        // 16    2  2L   PT_VALUE  --> UINT32, _SIZE 4L
	UINT16              align;          // 18    2	2L
} CLEAR_TXRX_COMMAND;                 //    16 == sizeof( SPECIAL_INST_COMMAND )
#pragma BYTE_NORMAL()



#pragma BYTE_ALIGN( _SET_SAD_COMMAND ) 
typedef struct _SET_SAD_COMMAND
{       // Compile-time Static variables.  This structure maps to .rcd descrition
										// off, sz, ob.sz
	SPECIAL_INST_HEADER Header;         //  0   12        the header must always be first
	UIOTREF2SINT16      ofsCallResult;	// 12	 4	 2L
	UINT16              sadAddr;        // 16    2  2L   PT_VALUE  --> UINT32, _SIZE 4L
	UINT16              align;          // 18    2	2L
} SET_SAD_COMMAND;                 //    16 == sizeof( SPECIAL_INST_COMMAND )
#pragma BYTE_NORMAL()


#pragma BYTE_ALIGN( _GPIB_READ_COMMAND ) 
typedef struct _GPIB_READ_COMMAND
{       // Compile-time Static variables.  This structure maps to .rcd descrition
										// off, sz, ob.sz
	SPECIAL_INST_HEADER Header;         //  0   12        the header must always be first
	UIOTREF2SINT16      ofsCallStatus;  // 12    4   2L   PT_REF    --> tag's offset in the UIOT
	UIOTREF2SINT16      ofsCallResult;	// 16	 4	 2L
	UIOTREF2SINT16		bEndOfLine;     // 20	 2	 2L 		 
	PTBUFFER            RBuffer;        // 22    8   8L   PT_BUFFER --> tag's offset in the UIOT
	UINT16              Timeout;        // 30    2   2L   PT_VALUE  --> UINT32, _SIZE 4L
	UINT16			    StrLen;         // 32    2	 2L 
	UINT16              align;          // 34    2	2L
} GPIB_READ_COMMAND;                 //    40 == sizeof( SPECIAL_INST_COMMAND )
#pragma BYTE_NORMAL()

#pragma BYTE_ALIGN( _GPIB_WRITE_COMMAND ) 
typedef struct _GPIB_WRITE_COMMAND
{       // Compile-time Static variables.  This structure maps to .rcd descrition
										// off, sz, ob.sz
	SPECIAL_INST_HEADER Header;         //  0   12        the header must always be first
	UIOTREF2SINT16      ofsCallStatus;  // 12    4   2L   PT_REF    --> tag's offset in the UIOT
	UIOTREF2SINT16      ofsCallResult;	// 16	 4	 2L
	UINT16				bEndOfLine;     // 20	 2	 2L 		 
	PTBUFFER            WBuffer;        // 22    8   8L   PT_BUFFER --> tag's offset in the UIOT
	UINT16              Timeout;        // 30    4   4L   PT_VALUE  --> UINT32, _SIZE 4L
	UINT16			    StrLen;         // 32    2	 2L 
	UINT16              align;          // 34    2  2L
} GPIB_WRITE_COMMAND;                 //   40 == sizeof( SPECIAL_INST_COMMAND )
#pragma BYTE_NORMAL()

#pragma BYTE_ALIGN( _SPECIAL_INST_COMMAND ) 
typedef struct _SPECIAL_INST_COMMAND
{       // Compile-time Static variables.  This structure maps to .rcd descrition
                                        // off, sz, ob.sz
    SPECIAL_INST_HEADER Header;         //  0   12        the header must always be first
    UINT32              Address;        // 12    4   4L   PT_VALUE, PT_DEVICE --> UINT32
    UIOTREF2UINT16      ofsDDevStatus;  // 16    4   2L   PT_REF    --> tag's offset in the UIOT
    PTBUFFER            RBuffer;        // 20    8   8L   PT_BUFFER --> tag's offset in the UIOT
    PTBUFFER            WBuffer;        // 28    8   8L   PT_BUFFER --> tag's offset in the UIOT
    UIOTREF2UINT16      ofsRLength;     // 36    4   2L   PT_REF    --> tag's offset in the UIOT
    UINT16              WLength;        // 40    2   2L   PT_VALUE  --> UINT16, _SIZE 2L
    UINT16              align;          // 42    2
    UINT32              Timeout;        // 44    4   4L   PT_VALUE  --> UINT32, _SIZE 4L
} SPECIAL_INST_COMMAND;                 //      48 == sizeof( SPECIAL_INST_COMMAND )
#pragma BYTE_NORMAL()

#pragma BYTE_ALIGN(_SPECIAL_INST_PORT)  // we may have substitutes for SPECIAL_INST_PARAM
typedef struct _SPECIAL_INST_PORT
{       // Compile-time Static variables.  This structure maps to .rcd descrition
                                        // off, sz, ob.sz
    SPECIAL_INST_HEADER Header;         //  0   12        the header must always be first
    UINT16              Address;        // 12    2   2L   PT_VALUE  --> UINT16, _SIZE 2L
    UINT16              Length;         // 14    2   2L   PT_VALUE  --> UINT16, _SIZE 2L
    UIOTREF2UINT16      ofsInValue;     // 16    4   2L   PT_REF    --> tag's offset in the UIOT 
    UINT16              OutValue;       // 20    2   2L   PT_VALUE  --> UINT16, _SIZE 2L
    UINT16              align;          // 22    2   2L   PT_VALUE  --> UINT16, _SIZE 2L
} SPECIAL_INST_PORT;                    //      24 == sizeof( SPECIAL_INST_PORT )
#pragma BYTE_NORMAL()

typedef union _SPECIAL_INST_PARAM
{       // Compile-time Static variables.  This structure maps to .rcd descrition
                                        // off, sz
    SPECIAL_INST_HEADER  paramHeader;   //  0   12
    SPECIAL_INST_COMMAND paramCommand;  //  0   48
    SPECIAL_INST_PORT   paramPort;     //  0   24
	GPIB_READ_COMMAND 	paramGpibRead;
	GPIB_WRITE_COMMAND	paramGpibWrite;
	SET_PAD_COMMAND		paramPadWrite;
	SET_SAD_COMMAND		paramSadWrite;
	SET_IBRSV_COMMAND	paramIbRSVWrite;
	CLEAR_TXRX_COMMAND	paramClearTxRx;
} SPECIAL_INST_PARAM;                   //      48 == sizeof(SPECIAL_INST_PARAM)


typedef struct _SPECIAL_INST
{       // Compile-time Static variables.  This structure maps to .rcd descrition
                                        // off,  sz
    SPECIAL_INST_PARAM  User;           //   0   48
    SPECIAL_INST_PARAM  Work;           //  48   48

        // generic, same for all drivers having asyncronous special functions
    UINT32                MarkTime;     //  96    4  when this s.f. must be complete
    SINT16                Status;       // 100    2
    UINT16                Busy;         // 102    2    
    struct _SPECIAL_INST* pNext;        // 104    4

} SPECIAL_INST, *LPSPECIAL_INST;        //      108 == sizeof( SPECIAL_INST )

/*
Note1: This struct is declared 1 byte aligned on top of file. The struct description is 
       evaluated by the "Runtime" sub-project only.  
       The 'Gui' subproject evaluates the SPECIAL_INST parameter block as presented 
       by the FNC_... definitions. 

Note2: For a very simple function module,  SPECIAL_INST is sufficient.  
       Parameter fields can be described directly in SPECIAL_INST.  
       SPECIAL_INST_PARAM, SPECIAL_INST_PORT, SPECIAL_INST_COMMAND and SPECIAL_INST_HEADER 
       are optional.  They have been defined here only to show a more complex example. 

Note3: In order to save memory SPECIAL_INST can be used only for asynchronous special functions. 
       SPECIAL_INST_COMMAND, SPECIAL_INST_PORT, or even SPECIAL_INST_HEADER 
       will do the job for synchronous special functions. 
       Make sure the correct param block size is declared NET(DEV)_FUNC_TYPE paragraph (p#2).

Note4: Because asynchronous functions are executed concurenlty with the flowchart code, 
       it is safer to provide a copy of the parameter block, to be used by the background thread. 
       This is why we have introduced the 'User' and 'Work' areas. 
       'User' is the area marked by the compiler to be filled in every time a function 
       is called. When the function is posted for execution, 'User' is copied into 'Work' 
       and 'Work' is what the background sees.
       Make sure the fields in 'User' and 'Header' match the FNC_... definitions. 
       It is a good idea to have them both mapped at offset 0.

Note5: The Runtime Special() entry point offers a pointer to the associated SPECIAL_INST. 
       Depending on the FunctionId, the right parameter layout will be selected. 
       This can be implemented in 3 ways: 
       a. Define 1 layout only large enough to encompass all parameters needed by any function. 
       b. Define 1 layout for every function, and cast to the right one based on the FunctionId. 
       c. Define 1 layout for every function, store them into a union and select the right 
          union branch based on the FunctionId. 
       Our current implementation is a mixture of a. and c. and should be optimal 
       for consumed memory and code complexity. 
*/


#ifdef WINVER          // This is for MSVC compiler


#ifndef DRVRUTIL_H__
#include "drvrutil.h"   // SS_ZERO
#endif

// What we put into the database for network config

#pragma BYTE_ALIGN(_NETCONFIG)     // 1 byte alignment
typedef struct _NETCONFIG
{
    UINT32           DriverId;          //  0  NETCONFIG_ID
    UINT16           DriverVersMinor;   //  4  LOW(  NETCONFIG_VERS )
    UINT16           DriverVersMajor;   //  6  HIGH( NETCONFIG_VERS )
    UINT16           GpibAddr;			//  8  0xd0000, 0xd1000, ...

	UINT16           Vendor_ID	;       // 10  0x250,  0x254,  ...
    UINT16           Product_ID ;       // 12  0 ... 15

    STOP_STATE_TYPES StopState;         // 14  SS_HOLD_LAST_STATE,  SS_ZERO
    //UINT16           StopState;       //   0 --> keep scanning, 1 --> stop scanning

    UINT16           bSimulate;         // 16  =0 --> interface card must be present
    UINT16           bWatchdog;         // 18  =1 --> kick the watchdog
    UINT16           DprHWTests;        // 20  HWTEST_RW, HWTEST_OFF
    UINT16           InputRead;         // 22  INPUT_READ_CYCLIC, INPUT_READ_COS,
    
    UINT16           usbIndex;       // 24  add new fields without changing NETCONFIG size
    UINT16           usbCount;      // 26 
    UINT16           busNum;        // 28  
    UINT16           Addr;         // 30  
    UINT16           PortNum;      // 32  
    UINT16           depth;        // 34  
    UINT16           ifNum;		 // 36  
    UINT16           reserved1;   // 38
	UINT16           reserved2;   // 40
	UINT16           reserved3;   // 42
//	UINT16           reserved4;   // 44

} NETCONFIG;                     // 44  == NET_CONFIG_SIZE == sizeof(NETCONFIG)
#pragma BYTE_NORMAL()


#pragma BYTE_ALIGN(_DEVCONFIG)  // 1 byte alignment
typedef struct _DEVCONFIG
{                               // Byte Offset
    UINT32  DriverId;           //  0 NETCONFIG_ID
    UINT32  DeviceId;           //  4 DEVCONFIG_ID
    UINT16  DeviceVersMinor;    //  6 LOW(  DEVCONFIG_VERS )
    UINT16  DeviceVersMajor;    //  8 HIGH( DEVCONFIG_VERS )
    UINT16  Address;            // 12 device's address on the link
    UINT16  bCritical;          // 14 =1 --> this device must be present on the link
    UINT8   ProductCode[8];     // 16 edit field. 16#ffff
    UINT16  reserved1;          // 24 add new fields without changing DEVCONFIG size
    UINT16  reserved2;          // 26 
    UINT16  reserved3;          // 28 
    UINT16  reserved4;          // 30 
} DEVCONFIG;                    // 32 == DEVCONFIG_SIZE == sizeof(DEVCONFIG)
#pragma BYTE_NORMAL()
/*
    Note: The reserved fields will be used for future developpment. 
    They ensure compatibility with projects generated by older versions of this driver.
*/


#endif      // WINVER


#endif      // ! APSTUDIO_READONLY_SYMBOLS

/* 
    Defines for .rcd file 
    Arithmetic expressions are allowed to define RC and RCD constants, 
    when  ONLY using + and -.  
    It is a good idea to have them encapsulated in ( ).
    Never use * and /.  The RC compiler silently ignores them.
*/


// SPECIAL_INST offsets & sizes
#define FNC_HD_FUNCTIONID           0L 
#define FNC_HD_FUNCTIONID_SIZE          2L      // PT_CONST  --> size 2L    
#define FNC_HD_STATUS               4L 
#define FNC_HD_STATUS_SIZE              2L      // PT_REF --> size of the object pointed to
#define FNC_HD_RESULT               8L 
#define FNC_HD_RESULT_SIZE              2L      // PT_REF --> size of the object pointed to

#define FNC_CM_ADDRESS              12L    
#define FNC_CM_ADDRESS_SIZE             4L      // PT_VALUE, PT_DEVICE  --> size 4L    
#define FNC_CM_DDSTATUS             16L    
#define FNC_CM_DDSTATUS_SIZE            2L      // PT_REF --> size of the object pointed to
#define FNC_CM_RBUFFER              20L 
#define FNC_CM_RBUFFER_SIZE             8L      // PT_BUFFER --> size 8L
#define FNC_CM_WBUFFER              28L 
#define FNC_CM_WBUFFER_SIZE             8L      // PT_BUFFER --> size 8L
#define FNC_CM_RLENGTH              36L 
#define FNC_CM_RLENGTH_SIZE             2L      // PT_REF --> size of the object pointed to
#define FNC_CM_WLENGTH              40L 
#define FNC_CM_WLENGTH_SIZE             2L      // PT_VALUE --> 2L
#define FNC_CM_TIMEOUT              44L    
#define FNC_CM_TIMEOUT_SIZE             4L      // PT_VALUE  --> size 4L    

#define FNC_PO_ADDRESS              12L    
#define FNC_PO_ADDRESS_SIZE             2L      // PT_VALUE --> 2L
#define FNC_PO_LENGTH               14L    
#define FNC_PO_LENGTH_SIZE              2L      // PT_VALUE --> 2L
#define FNC_PO_IN_VALUE             16L 
#define FNC_PO_IN_VALUE_SIZE            2L      // PT_REF --> size of the object pointed to
#define FNC_PO_OUT_VALUE            20L    
#define FNC_PO_OUT_VALUE_SIZE           2L      // PT_VALUE --> 2L


#define FNC_RG_STATUS				12L
#define FNC_RG_STATUS_SIZE				4L
#define FNC_RG_DDSTATUS             16L    
#define FNC_RG_DDSTATUS_SIZE            4L      // PT_REF --> size of the object pointed to
#define FNC_RG_EOI					20L
#define FNC_RG_EOI_SIZE					2L
#define FNC_RG_RBUFFER              24L 
#define FNC_RG_RBUFFER_SIZE             8L      // PT_BUFFER --> size 8L
#define FNC_RG_TIMEOUT				32L
#define FNC_RG_TIMEOUT_SIZE				2L
#define FNC_RG_STRLEN				34L
#define FNC_RG_STRLEN_SIZE				2L


#define FNC_WG_STATUS				12L
#define FNC_WG_STATUS_SIZE				4L
#define FNC_WG_DDSTATUS             16L    
#define FNC_WG_DDSTATUS_SIZE            4L      // PT_REF --> size of the object pointed to
#define FNC_WG_EOI					20L
#define FNC_WG_EOI_SIZE					2L
#define FNC_WG_RBUFFER              22L 
#define FNC_WG_RBUFFER_SIZE             8L      // PT_BUFFER --> size 8L
#define FNC_WG_TIMEOUT				30L
#define FNC_WG_TIMEOUT_SIZE				2L
#define FNC_WG_STRLEN				32L
#define FNC_WG_STRLEN_SIZE				2L

#define FNC_AD_RESULT				12L
#define FNC_AD_RESULT_SIZE				2L
#define FNC_AD_ADDRESS				16L
#define FNC_AD_ADDRESS_SIZE				2L

#define FNC_RSV_RESULT				12L
#define FNC_RSV_RESULT_SIZE				2L
#define FNC_RSV_CODE				16L
#define FNC_RSV_CODE_SIZE				2L

#define FNC_TXRX_CLEAR_RESULT		12L
#define FNC_TXRX_CLEAR_RESULT_SIZE		2L
#define FNC_TXRX_CLEAR_CODE			16L
#define FNC_TXRX_CLEAR_CODE_SIZE		2L

#define FNC_HD_SPECIAL_INST_SIZE        12
#define FNC_CM_SPECIAL_INST_SIZE        48
#define FNC_PO_SPECIAL_INST_SIZE        24

#define FNC_RG_SPECIAL_INST_SIZE		40
#define FNC_WG_SPECIAL_INST_SIZE		40
#define FNC_AD_SPECIAL_INST_SIZE		20
#define FNC_RSV_SPECIAL_INST_SIZE		28

#define FNC_SPECIAL_INST_SIZE           256 //108


// NETCONFIG offsets (bytes) & sizes (bits)
#define NET_ID                      0 
#define NET_ID_SIZE                     32 
#define NET_VERS                    4 
#define NET_VERS_SIZE                   32 
#define NET_GPIBADR                 8 
#define NET_GPIBADR_SIZE                16 
#define NET_VEN_ID                  10
#define NET_VEND_ID_SIZE                16 
#define NET_PRO_ID                  12
#define NET_PRO_ID_SIZE			    16 
#define NET_STOPSTATE               14 
#define NET_STOPSTATE_SIZE              16 
#define NET_SIMULATE                16 
#define NET_SIMULATE_SIZE               16 
#define NET_WATCHDOG                18 
#define NET_WATCHDOG_SIZE               16 
#define NET_HWTEST                  20 
#define NET_HWTEST_SIZE                 16 
#define NET_INPUTREAD               22 
#define NET_INPUTREAD_SIZE              16 

#define NET_USB_IDX               24
#define NET_USB_IDX_SIZE              16 
#define NET_USB_CNT               26
#define NET_USB_CNT_SIZE              16 
#define NET_BUS_NUM               28
#define NET_BUS_NUM_SIZE              16 
#define NET_ADDR				  30
#define NET_ADDR_SIZE				  16 
#define NET_PORT_NUM              32
#define NET_PORT_NUM_SIZE             16 
#define NET_DEPTH				  34
#define NET_DEPTH_SIZE				  16 
#define NET_IFNUM				  36
#define NET_IFNUM_SIZE				  16 
//
#define NETCONFIG_SIZE              44 //must not exceed this, else expand above



// DEVICECONFIG offsets & sizes
#define DEV_DRVID                   0
#define DEV_DRVID_SIZE                  32
#define DEV_ID                      4
#define DEV_ID_SIZE                     32
#define DEV_VERS                    8
#define DEV_VERS_SIZE                   32
#define DEV_ADDRESS                 12 
#define DEV_ADDRESS_SIZE                16 
#define DEV_CRITICAL                14 
#define DEV_CRITICAL_SIZE               16 
#define DEV_PCODE                   16 
#define DEV_PCODE_SIZE                  64 
#define DEVCONFIG_SIZE              32 

// Dual port ram layout
#define  DPR_CONTROL_OFF        0
#define  DPR_CONTROL_SIZE       2048
#define  DPR_INPUT_OFF          2048    // where the input image can be found in the dpr
#define  DPR_INPUT_SIZE         1024    // 1kbyte =  MAX_DEVICES * 8bytes input devices
#define  DPR_OUTPUT_OFF         3072    // where the output image can be found in the dpr
//#define  DPR_OUTPUT_OFF       2048    // for didactic purposes use 2048 --> outputs will be looped back in inputs
#define  DPR_OUTPUT_SIZE        1024    // 1kbyte =  MAX_DEVICES * 8bytes input devices
#define  DPR_TOTAL_SIZE         4096    // 4 kbytes
#define  MAX_DEVICES            128     // max 128 devices allowed by our didactical network


#define  DPADR_MIN              0x80000L
#define  DPADR_MAX              0xef000L
#define  DPADR_STEP             0x01000L    // 4 kbytes increments
#define  DPADR_DEFAULT          0xd0000L

#define  PORT_MIN               0x00
#define  PORT_MAX               0xff
#define  PORT_STEP              1           // 4 bytes increments
#define  PORT_DEFAULT           0x0

#define  NO_IRQ                 0L
#define  HWTEST_RW              1
#define  HWTEST_OFF             0
#define  INPUT_READ_COS         1
#define  INPUT_READ_CYCLIC      0

#define  BAUDRATE_125           1
#define  BAUDRATE_250           2
#define  BAUDRATE_500           3

//#define  DEVICE_4W_INPUT            10      // 64bits input
#define  DEVICE_4W_OUTPUT           11      // 64bits output
#define  DEVICE_4W_IORO             13      // 64bits input or  64bits output
#define  DEVICE_4W_IANDO            12      // 64bits input and 64bits output

#define DEVICE_GPIB_STAT			20		//gpib status io

#define  DRVF_GET_DRVSTAT           2100    // functions at driver level
#define  DRVF_GET_DEVSTAT           2101    
#define  DRVF_COMMAND               2102    
#define  DRVF_PORT_INPUT            2103    
#define  DRVF_PORT_OUTPUT           2104    
#define  DRVF_PEEK                  2105    
#define  DRVF_POKE                  2106    
#define	 DRVF_READ_GPIB				2107
#define	 DRVF_WRITE_GPIB			2108
#define	 DRVF_SET_PAD				2109
#define	 DRVF_SET_SAD				2110
#define	 DRVF_SET_RSV				2111
#define  DRVF_CLEAR_TXRX			2112

#define  DEVICE_FUNC                2010    // special device functions ids
#define  DEVF_GET_DEVSTAT           2200    // functions at device level

#define  MAX_LENGTH                 400

#define CLEAR_TXRX_LIST				300

#endif       // __DRIVER_H__ 




