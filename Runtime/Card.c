/********************************************************************

                                Card.c

    Interface specific code. 
    This file only should touch the hardware.

*********************************************************************/


#include "stdafx.h"

#include <rt.h>
#include <string.h>     // strlen()
#include <stdio.h>      // sprintf()
//#include <ioctl.h>

#include "vlcport.h"
#include "dcflat.h"     // EROP()
#include "driver.h"     /* SEMAPHORE */
#include "errors.h"     /* IDS_RT_DP_RW_TEST                     */
#include "auxrut.h"     /* StartTimeout(), IsTimeout(), EROP     */
#include "NiUsbGpib.h"   /* DUAL_PORT                             */
#include "card.h"       /* Init()                                */
#include "ni_usb_gpib.h"



/******************* Card specific  Functions  *******************************/


/******************* Initialization  *******************************/


static int TestAndFill(UINT8* pc, const int Size, const int test, const int fill)   /* test == 1  --> no test */
{
    int i  = 0;
    for(; i < Size;  *pc++ = fill, i++)
    {
        int c = *pc & 255;
        if(test != 1  &&  test != c)
        {
            EROP("Ram Error.  Address %p, is 0x%02x, and should be 0x%02x", pc, c, test, 0);
            return IDS_NIUSBGPIB_HW_TEST;
        }
    }

	int x;
	int y;

	
    return SUCCESS;
}


int  Init( DUAL_PORT* const dp, const UINT16 DprHWTests, P_ERR_PARAM const lpErrors)
{
    int rc = SUCCESS;

    if(DprHWTests == HWTEST_RW)
    {
        UINT8* const pc = (UINT8*)dp;

        TestAndFill(pc, DPR_TOTAL_SIZE, 1, 0xff);                     /* write 0xff overall       */
        rc = TestAndFill(pc, DPR_TOTAL_SIZE, 0xff, 0x55);             /* test 0xff and write 0x55 */
        if(rc == SUCCESS) rc = TestAndFill(pc, DPR_TOTAL_SIZE, 0x55, 0xaa);
        if(rc == SUCCESS) rc = TestAndFill(pc, DPR_TOTAL_SIZE, 0xaa, 0x00);
    }

    return rc;
}



/****************************************************************************************
    IN:     pName   --> pointer to the device user name
            Address --> device's network address
            pBuf    --> pointer to the destination buffer
            szBuf   --> size of the destination buffer

    OUT:    *pBuf   <-- "Address xx (usr_name)".  
    Note:   The device user name may be truncated!
*/
static void LoadDeviceName( char* pName, UINT16 Address, char* pBuf, size_t szBuf )
{
    if( szBuf && (pBuf != NULL) )
    {
        char* format = "Address %d";

        *pBuf = '\0';

        if( szBuf > (strlen(format)+3) )    /* Address may need more digits */
        {
            size_t  len;

            sprintf(pBuf, format, Address & 0xffff);

            len = strlen( pBuf ); 

            if( pName && ((szBuf - len) > 10) )     /* if we still have 10 free bytes  */
            {
                strcat(pBuf, " (");
                len += 2;
                strncat( pBuf, pName, szBuf-len-2 );
                *(pBuf + szBuf - 2) = '\0';
                strcat( pBuf, ")" );
            }
        }
    }
}



int  TestConfig( LPDRIVER_INST const pNet, P_ERR_PARAM const lpErrors )
{
    int rc = SUCCESS;

    LPDEVICE_INST pDevice = (LPDEVICE_INST)pNet->pDeviceList;
        
    for( ; pDevice->Type && (rc == SUCCESS); pDevice++ )
    {
        DUAL_PORT* const dp = (DUAL_PORT*)pNet->pDpr;     /* pointer to the dualport */

        pDevice->bPresent = 1;

        /*
        Check pDevice. 
        if( the device is not on the network )
            pDevice->bPresent = 0;
        */
        
        if( pDevice->Address == 5 )     /* let's say for example, device 5 is offline */
            pDevice->bPresent = 0;
        
        if( !pDevice->bPresent && pDevice->bCritical)
        {
            LoadDeviceName( pDevice->pName, pDevice->Address, lpErrors->Param3, sizeof(lpErrors->Param3) );
            rc = IDS_NIUSBGPIB_DEVICE_OFFLINE;
        }
    }

    return rc;
}


/********************* runtime specific card access functions ********************/



void PortInput( SPECIAL_INST_PORT* const pData)
{
    int   rc  = SUCCESS;

    UINT16 const Addr = pData->Address;
    UINT16* pResult   = BuildUiotPointer( pData->Header.ofsResult );
    
    if( Addr >= 0x100 && Addr < 0x1000 )
    {
        UINT16* pInValue = BuildUiotPointer( pData->ofsInValue );

        switch(pData->Length)
        {
            case 1: 
					*pInValue = inbyte( Addr );
					break;

            case 2: 
					*pInValue = inhword( Addr );
					break;

            case 4: 
					// *pInValue = inword( Addr );
					break;
        }
    }
    else 
        rc = IDS_NIUSBGPIB_INVALID_ADDERSS;

    *pResult = rc;
}


void PortOutput( SPECIAL_INST_PORT* const pData)
{
    int   rc  = SUCCESS;

    UINT16 const Addr = pData->Address;
    UINT16* pResult   = BuildUiotPointer( pData->Header.ofsResult );
    
    if( Addr >= 0x100 && Addr < 0x1000 )
    {
        const UINT16 OutValue = pData->OutValue;
    
        switch(pData->Length)
        {
            case 1: 
					outbyte( Addr, OutValue );
					break;

            case 2: 
                    outhword( Addr, OutValue );
					break;

            case 4: 
                    // outword( Addr, OutValue );
					break;
        }
    }
    else 
        rc = IDS_NIUSBGPIB_INVALID_ADDERSS;

    *pResult = rc;
}





void DoPeekCommand( const LPDRIVER_INST pNet, SPECIAL_INST_COMMAND* const pData )
{
    int     rc       = SUCCESS;
    UINT8*  const dp = (UINT8*)pNet->pDpr; 
    const UINT32     dpSize   = DPR_TOTAL_SIZE; 
    const UINT32     dpOffset = pData->Address; 
    const LPPTBUFFER pRBuffer = &pData->RBuffer;
    const UINT32     Length   = pData->WLength;
    UINT16* pResult = BuildUiotPointer( pData->Header.ofsResult );

    if( !Length )
    {
        rc = IDS_NIUSBGPIB_RW_ZERO;
    } 
    else if( Length > pRBuffer->Size )
    {
        rc = IDS_NIUSBGPIB_READ_SIZE;
    }
    else if( dpOffset + Length > dpSize )
    {
        rc = IDS_NIUSBGPIB_DPR_OUT;
    }
    else
    {
        UINT8* p = BuildUiotPointer( pRBuffer->Offset );
        CardCopy( p, dp+dpOffset, Length);
    }
    
    *pResult = rc;

    {
        /*
        char buffer[100];
        char Param[68];
        LoadDeviceName( "Bambi55", 10, Param, sizeof( Param ) );
        printf( "Param=%s\n", Param );
        printf( "Param=%s\n", Param );
        printf( "Param=%s. Len=%d\n", Param, strlen(Param) );
        sprintf( buffer, "Test1. %s", Param );
        printf( "%s\n", buffer );
        */
    }

}

void DoPokeCommand( const LPDRIVER_INST pNet, SPECIAL_INST_COMMAND* const pData )
{
    int          rc  = SUCCESS;
    UINT8* const dp  = (UINT8*)pNet->pDpr; 
    const UINT32     dpSize   = DPR_TOTAL_SIZE; 
    const UINT32     dpOffset = pData->Address; 
    const LPPTBUFFER pWBuffer = &pData->WBuffer;
    const UINT32     Length   = pData->WLength;
    UINT16* pResult = BuildUiotPointer( pData->Header.ofsResult );


    if( !Length )
    {
        rc = IDS_NIUSBGPIB_RW_ZERO;
    } 
    else if( Length > pWBuffer->Size )
    {
        rc = IDS_NIUSBGPIB_WRITE_SIZE;
    }
    else if( dpOffset + Length > dpSize )
    {
        rc = IDS_NIUSBGPIB_DPR_OUT;
    }
    else
    {
        UINT8* p = BuildUiotPointer( pWBuffer->Offset );
        CardCopy( dp+dpOffset, p, Length);
    }
    
    *pResult = rc;
}



void GetDriverStatus( const LPDRIVER_INST pNet, SPECIAL_INST_COMMAND* const pData )
{
    const DUAL_PORT* const dp = (DUAL_PORT*)pNet->pDpr;
    UINT16* pDriverStatus = BuildUiotPointer( pData->ofsDDevStatus );
    *pDriverStatus = dp->NetStatus;
}


void GetDeviceStatus( const LPDRIVER_INST pNet, SPECIAL_INST_COMMAND* const pData )
{
    int    rc       = SUCCESS;
    UINT16 Address  = pData->Address;
    UINT16* pResult = BuildUiotPointer( pData->Header.ofsResult );
    
    if( Address < MAX_DEVICES )
    {
        UINT16* pDeviceStatus = BuildUiotPointer( pData->ofsDDevStatus );
        const DUAL_PORT* const dp = (DUAL_PORT*)pNet->pDpr;
        *pDeviceStatus = dp->DevStatus[Address];
    }
    else 
        rc = IDS_NIUSBGPIB_INVALID_ADDERSS;
        
    *pResult = rc;
}


/*
 *   Long lasting function, asynchronusely processed, called by BackgroundTask().
 *   Copies WriteBuffer into ReadBuffer, one character per second. 
 */
void DoCommand( const LPDRIVER_INST pNet, SPECIAL_INST* const pData )
{
    int rc = SUCCESS;

    SPECIAL_INST_COMMAND* const pWork = &pData->Work.paramCommand;

    UINT16* pResult = BuildUiotPointer( pWork->Header.ofsResult );

    const LPPTBUFFER pWBuffer = &pWork->WBuffer;
    const LPPTBUFFER pRBuffer = &pWork->RBuffer;

    UINT8* Src = BuildUiotPointer( pWBuffer->Offset );
    UINT8* Dst = BuildUiotPointer( pRBuffer->Offset );
    UINT16 Length  = pWork->WLength;
    UINT16 Address = pWork->Address;
    
    if( Address >= MAX_DEVICES )
    {
        rc = IDS_NIUSBGPIB_INVALID_ADDERSS;
    }
    else if( Address == 5 )
    {
        rc = IDS_NIUSBGPIB_DEVICE_OFFLINE;
    }
    else if( !Length )
    {
        rc = IDS_NIUSBGPIB_RW_ZERO;
    } 
    else if( Length > pWBuffer->Size )
    {
        rc = IDS_NIUSBGPIB_WRITE_SIZE;
    }
    else if( Length > pRBuffer->Size )
    {
        rc = IDS_NIUSBGPIB_READ_SIZE;
    }
    else
    {
        UINT16* pRLength = BuildUiotPointer( pWork->ofsRLength );

        int    Timeout  = pWork->Timeout * 1000;        /* how much time we can afford to wait */
        UINT16 n = 0;

        pData->MarkTime = StartTimeout(Timeout);        /* milisecond when it should complete  */

        while( (rc == SUCCESS) && (n < Length) && !pNet->bGoOffLine  )
        {
            *Dst++    = *Src++;
            *pRLength = ++n;
    
            if( IsTimeout( pData->MarkTime ) )          /* if not timeout yet, sleeps 1 tick */
                rc = IDS_NIUSBGPIB_TIMEOUT;
            else
            {
                //Delay(100);      // sleep 100 ms
                Delay(1000);     // sleep 1 second 
            }
        }
    }
    
    *pResult = rc;
}

void DoGpibWrite(const LPDRIVER_INST pNet, GPIB_WRITE_COMMAND* const pData)
{
	usb_data* usbdata = ((usb_data*)(pNet->USB_Data)); //OK

	const LPPTBUFFER pWBuffer = &pData->WBuffer;
	uint8_t* Src = BuildUiotPointer(pWBuffer->Offset);

	UINT16* ofsCallResult = BuildUiotPointer(pData->Header.ofsResult);
	UINT16* ofsCallStatus = BuildUiotPointer(pData->Header.ofsStatus);

	UINT16 res=0;
	uint32_t stat=0;
	//EROP("Endpoint used Snd=0x%x, Rcv=0x%x \n", usbdata->_pipeBlkSnd, usbdata->_pipeBlkRcv, 0, 0);

	//printf("pData->StrLen %x\n", pData->StrLen);
	//printf("pData->bEndOfLine %x\n", pData->bEndOfLine);
	//printf("pData->Timeout %d\n", pData->Timeout);
	//printf("pData->Src %s\n", Src);


	stat=ibwrt(usbdata, Src, pData->StrLen, pData->bEndOfLine, &res, pData->Timeout);
	//stat = ibcmd(usbdata, Src, pData->StrLen, pData->bEndOfLine, &res, pData->Timeout);
	//printf("res : %d stat :%d\n", res);
	//stat = ni_usb_write(usbdata, Src, pData->StrLen, pData->bEndOfLine, &res, pData->Timeout);
	*ofsCallResult = 0;
	*ofsCallStatus = res;
	EROP("ni_usb_write out \n", 0, 0, 0, 0);
	

}

void    DoGpibRead(const LPDRIVER_INST pNet, SPECIAL_INST* const _pData)// GPIB_READ_COMMAND* const pData)
{
	EROP(":-1-- DoGpibRead() rc =", 0, 0, 0, 0);
	if (pNet == NULL) return;
	if (_pData == NULL) return;
	EROP(":-2-- DoGpibRead() rc =", 0, 0, 0, 0);

	//printf("DoGpibRead \n");
	usb_data* usbdata = ((usb_data*)(pNet->USB_Data)); //OK
	//printf("DoGpibRead2 \n");
	GPIB_READ_COMMAND* const pData = &_pData->Work.paramGpibRead;

	UINT16* ofsCallResult = BuildUiotPointer(pData->Header.ofsResult);
	UINT16* ofsCallStatus = BuildUiotPointer(pData->Header.ofsStatus);

	const LPPTBUFFER pWBuffer = &pData->RBuffer;
	UINT8* Dst = BuildUiotPointer(pWBuffer->Offset);
	int End = -1;
	UINT32* ofsEnd = BuildUiotPointer(pData->bEndOfLine);

	//EROP("Endpoint READ Snd=0x%x, Rcv=0x%x \n", usbdata->_pipeBlkSnd, usbdata->_pipeBlkRcv, 0, 0);

	ni_usb_read(usbdata, Dst, pData->StrLen, &End, ofsCallResult, pData->Timeout);


	EROP(":-1-- DoGpibRead %s ", Dst, 0, 0, 0);

	ofsCallStatus = End;
	EROP(":-2-- DoGpibRead %d ", End, 0, 0, 0);
	ofsEnd = End;
	EROP(":-3-- DoGpibRead %x ", ofsCallResult, 0, 0, 0);
	ofsCallResult = 0;

	EROP(":-4-- DoGpibRead() rc =", 0, 0, 0, 0);
}

void    DoPadWrite(const LPDRIVER_INST pNet, SET_PAD_COMMAND* const pData)
{
	usb_data* usbdata = ((usb_data*)(pNet->USB_Data)); //OK

	UINT16* ofsCallResult = BuildUiotPointer(pData->Header.ofsResult);
	//UINT16* ofsCallStatus = BuildUiotPointer(pData->Header.ofsStatus);

	UINT16 pad = pData->padAddr;
	
	 
	ofsCallResult = ibpad(usbdata, pad);

}


void DoSadWrite(const LPDRIVER_INST pNet, SET_SAD_COMMAND* const pData)
{
	usb_data* usbdata = ((usb_data*)(pNet->USB_Data)); //OK
	UINT16* ofsCallResult = BuildUiotPointer(pData->Header.ofsResult);
	//UINT16* ofsCallStatus = BuildUiotPointer(pData->Header.ofsStatus);

	UINT16 sad = pData->sadAddr;
	
	
	 ofsCallResult = ibsad(usbdata, sad);
}

void DoIbRSVWrite(const LPDRIVER_INST pNet, SPECIAL_INST* const _pData) //SET_IBRSV_COMMAND* const pData)
{
	usb_data* usbdata = ((usb_data*)(pNet->USB_Data)); //OK

	SET_IBRSV_COMMAND* const pData = &_pData->Work.paramGpibRead;

	UINT16* ofsCallResult = BuildUiotPointer(pData->Header.ofsResult);
	//UINT16* ofsCallStatus = BuildUiotPointer(pData->Header.ofsStatus);

	UINT8 code = (UINT8)pData->Code;
	ofsCallResult = ibrsv(usbdata, code);
	ofsCallResult = 0;
}

void DoClearTxRxBuffer(const LPDRIVER_INST pNet, SPECIAL_INST* const _pData) //SET_IBRSV_COMMAND* const pData)
{
	usb_data* usbdata = ((usb_data*)(pNet->USB_Data)); //OK

	CLEAR_TXRX_COMMAND* const pData = &_pData->Work.paramGpibRead;

	UINT16* ofsCallResult = BuildUiotPointer(pData->Header.ofsResult);
	//UINT16* ofsCallStatus = BuildUiotPointer(pData->Header.ofsStatus);

	UINT8 code = (UINT8)pData->Code;

	if (code == 0)
		UsbClearStall(usbdata->udev, usbdata->_pipeBlkSnd);
	if (code == 1)
		UsbClearStall(usbdata->udev, usbdata->_pipeBlkRcv);
	ofsCallResult = 0;
}




void GpibsStatusUpdate(const LPDRIVER_INST pNet,  const LPDEVICE_INST pDevice, VOID *Dest )
{
	usb_data* usbdata = ((usb_data*)(pNet->USB_Data)); //OK
	UINT32 Stat;
	// cannot read every time error

	return;
	Stat = ibstatus(usbdata);
	
	if (Dest == NULL) return;

	//EROP("GpibsStatusUpdate READ Stat=0x%x  0x%x\n", Stat, Dest, 0, 0);
	if (pNet->bFirstCycle)
	{
		UINT32 Stat;
		Stat = ibstatus(usbdata);
		*(UINT32  volatile*)Dest = (UINT32)Stat;
		return;
	}

		*(UINT32  volatile*)Dest = (UINT32)usbdata->ibsta;

		usbdata->ibstaOn = 0;

}



//
//interface {
//	minor = 0	/* board index, minor = 0 uses /dev/gpib0, minor = 1 uses /dev/gpib1, etc. */
//board_type = "ni_pci"	/* type of interface board being used */
//name = "violet"	/* optional name, allows you to get a board descriptor using ibfind() */
//pad = 0	/* primary address of interface             */
//sad = 0	/* secondary address of interface           */
//timeout = T3s	/* timeout for commands */
//
//eos = 0x0a	/* EOS Byte, 0xa is newline and 0xd is carriage return */
//set - reos = yes	/* Terminate read if EOS */
//set - bin = no	/* Compare EOS 8-bit */
//set - xeos = no	/* Assert EOI whenever EOS byte is sent */
//set - eot = yes	/* Assert EOI with last byte on writes */
//
///* settings for boards that lack plug-n-play capability */
//base = 0	/* Base io ADDRESS                  */
//irq = 0	/* Interrupt request level */
//dma = 0	/* DMA channel (zero disables)      */
//
///* pci_bus and pci_slot can be used to distinguish two pci boards supported by the same driver */
///*	pci_bus = 0 */
///*	pci_slot = 7 */
//
//master = yes	/* interface board is system controller */
//}
//*/
//


