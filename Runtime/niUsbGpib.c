/************************************************************

				Flatcode.c


This file implements all the module entry points:

int rtIdentify( P_IDENTITY_BLOCK* ppIdentityBlock );
int rtLoad(   UINT32 ScanRate, UINT32* rDirectCalls );
int rtOpen(   LPDRIVER_INST lpNet, P_ERR_PARAM lpErrors);
int rtReload( LPDRIVER_INST lpNet, P_ERR_PARAM lpErrors);
int rtOnLine( LPDRIVER_INST lpNet, P_ERR_PARAM lpErrors);
int rtInput(  LPDRIVER_INST lpNet);
int rtOutput( LPDRIVER_INST lpNet);
int rtSpecial(LPDRIVER_INST lpNet, LPSPECIAL_INST lpData);
int rtOffLine(LPDRIVER_INST lpNet, P_ERR_PARAM  lpErrors);
int rtClose(  LPDRIVER_INST lpNet, P_ERR_PARAM  lpErrors);
int rtUnload( );

**************************************************************/


#include "stdafx.h"


/*********************************************************/
/*                                                       */
/*                Flatcode Sample Program                */
/*                                                       */
/*********************************************************/

#include <rt.h>
//#include <rtdef.h>
#include <stdio.h>
#include <stdlib.h>
#include <usbif3.h>
#include <strings.h>


#include "vlcport.h"
#include "CSFlat.h"     // FCoreSup
#include "DCFlat.h"     // FDriverCore
#include "driver.h"

#include "version.h"
#include "auxrut.h"
#include "niUsbGpib.h"   // DUAL_PORT
#include "task.h"
#include "card.h"

//usb
#include "hid.h"
#include "ib.h"
#include "ni_usb_gpib.h"
/************************************************************/

/*
	This code is provided here as an example on how to use
	call back functions implemented in the csFlat.lib

*/
/*

#include <stdlib.h>     // _MAX_PATH
#include <string.h>     // strcat()

int test()
{
	int   rc;
	int   fh;
	FILE* pFile;
	char  Fname[ _MAX_PATH ];
	char  buf[ 256 ];
	char* argv[5];

	rc = GetPathInfo( ptJob, pcDir, Fname, sizeof(Fname) );

	strcat( Fname, "t.txt" );

	fh = open( Fname, O_BINARY | O_CREAT );
	if( fh >= 0 )
	{
		strcpy( buf, "Line1\r\n" );
		rc = write( fh, buf, strlen(buf) );
		strcpy( buf, "Line2\r\n" );
		rc = write( fh, buf, strlen(buf) );
		rc = lseek( fh, -7, SEEK_CUR );
		strcpy( buf, "Line3\r\n" );
		rc = write( fh, buf, strlen(buf) );

		rc = close( fh );
	}


	pFile = fopen( Fname, "wb" );
	if( pFile )
	{
		rc = ftell( pFile );
		fseek( pFile, 0, SEEK_END );
		strcpy( buf, "Line4\r\n" );
		rc = fwrite( buf, 1, strlen(buf), pFile );
		rc = fclose( pFile );
	}

	argv[0] = "notepad.exe";
	argv[1] = Fname;
	argv[2] = NULL;

	rc = spawnv( P_WAIT, argv[0], argv );

	return 0;
}
*/



int32_t attach_cb(struct usbDeviceInfo * udi);
//




struct usbDeviceId match_id_table[] = {
	{ USB_INTERFACE_INFO(USB_CLASS_HID, USB_SUBCLASS_BOOT, USB_CLASS_VENDOR_SPEC) },
	{ 0 }                                           // Terminating entry
};

//struct usbDeviceId match_id_table[] = {
//	{ USB_INTERFACE_INFO(USB_CLASS_HID, USB_SUBCLASS_BOOT, USB_PROTO_BOOT_KEYBOARD) },
//	{ 0 }
//};


void usb_thread(void * data)
{
	EROP("usb_thread in \n", 0, 0, 0, 0);


	usb_data *usbdata = ((usb_data*)(data));
		LPSYSINFO lpSysInfo;
		CopyRtSystemInfo( lpSysInfo);

		uint32_t dtime =(500 * lpSysInfo->KernelTickRatio)/10000;

		printf("dtime = %d usec_duration= %d KernelTickRatio = %d \n" , dtime, 500, lpSysInfo->KernelTickRatio);
	
		uint8_t ep;
		uint16_t epsts;

	for (;;)
	{
		knRtSleep(dtime);
		uint32_t stat;
		stat= UsbGetEndpointStatus(usbdata->udev, ep, &epsts);

		if (stat == USB_ERR_NORMAL_COMPLETION)
		{
			if (ep == usbdata->_pipeIntRcv)
			{
				EROP("_pipeIntRcv : ...\n", 0, 0, 0, 0);

				UINT32 Stat;
				// cannot read every time error
				Stat = ibstatus(usbdata);
				usbdata->ibstaOn = 1;
			}
		}

		if (usbdata->online == FALSE)
		break;
	}
	//UsbCloseDevice(kbd->kbd_ufd);

	EROP("usb_thread : exiting ...\n", 0, 0, 0, 0);
}
int32_t attach(struct usbDeviceInfo * udi)
{

	// connect to usb
	struct usbDeviceId* dev;
	struct usbDeviceId* id;
	struct usbDeviceInfo * info;

//	EROP("attach: idVendor = 0x%x : 0x%x\n", udi->idVendor, udi->idProduct, 0, 0);

	match_id_table->idVendor = 0x3923;
	match_id_table->idProduct = 0x7618;// 1e;
	match_id_table->match_flags = USB_DEVICE_ID_MATCH_DEVICE;

	id = UsbMatchId(udi, match_id_table);

	if (id == NULL)
	{
		EROP("UsbMatchId: , Null\n", 0, 0, 0, 0);

	}
	else
	{
		//found
		EROP("UsbMatchId: , idVendor=0x%x : 0x%x, busNum : 0x%x, 0x%x \n", id->idVendor, id->idProduct, udi->bBusNum, udi->bPortNum);

		return 1;

	}


	return 0;

	//	kbd = malloc(sizeof(kbd_data));
	//	if (kbd == NULL)
	//		return 0;
	//	memset(kbd, 0, sizeof(kbd_data));
	//	rval = UsbOpenDevice(udi, "Keyboard-sample", &ufd, kbd);
	//	if (rval == USB_ERR_NORMAL_COMPLETION) {
			//kbd->dev = dev;
			//kbd->ufd = ufd;
			//kbd->udi = udi;
			//return 1;
	//	}

}


void detach(void * data)
{
	/* データはUsbOpenDevice()にパスされた値   */
	//assert(data != NULL);
	//printf("usbkbd: DETACHED\n");
	EROP("detach:\n", 0, 0, 0, 0);
}


int rtIdentify(P_IDENTITY_BLOCK* ppIdentityBlock)
{
	static IDENTITY_BLOCK IdentityBlock;
	IdentityBlock.DriverId = DriverNIUSBGPIB;
	IdentityBlock.DriverVers = NIUSBGPIBVERS;
	IdentityBlock.pName = PRODUCT_NAME;// ", " PRODUCT_VERSION;
	*ppIdentityBlock = &IdentityBlock;
	return 0;
}

int rtLoad(UINT32 ScanRate, UINT32* rDirectCalls)
{
	// Executing the LOAD PROJECT command

#if defined( _DEBUG )
	SetDebuggingFlag(1);  // Disable the VLC watchdog, so we can step through our code. 
#endif  // _DEBUG


// Use direct calls for very fast applications.  
// With the appropriate bit set, Input(), Output() and/or Special()
//  can be directly called from the engine thread, 
//  saving the delay introduced by a task switch. 
// Note:  Functions exectuted in the engine thread cannot call 
//  some C stdlib APIs, like sprintf(), malloc(), ...

// *rDirectCalls = ( DIRECT_INPUT | DIRECT_OUTPUT | DIRECT_SPECIAL );

	//EROP("rtLoad() ScanRate=%d, rDirectCalls=%x", ScanRate, *rDirectCalls, 0, 0);

	return 0;
}

int rtOpen(LPDRIVER_INST pNet, P_ERR_PARAM pErrors)
{
	// Executing the LOAD PROJECT command

//	pNet->Vendor_ID = 0x0403;//
//	pNet->Product_ID = 0x6001;//keyboard

	pNet->Vendor_ID =  0x3923;
	pNet->Product_ID = 0x7618;//HS+, // HS 0x761e;

	int rc = SUCCESS;

	// rc = test();  // Example on how to use the call back functions implemented in the csFlat.lib

	if (pNet->Sentinel != RT3_SENTINEL)
		rc = IDS_VLCRTERR_ALIGNMENT;

	if (rc == SUCCESS)
	{
	
		//EROP("Vid: %x,pid: %x , cnt: %d ,idx: %d ", pNet->Vendor_ID, pNet->Product_ID, pNet->usbCount, pNet->usbIndex);
		//EROP("BusNum,Addr,PortNum,Depth= %d , %d , %d , %d", pNet->busNum, pNet->addr, pNet->PortNum, pNet->depth);
		//EROP("if: %x  gpib :%x", pNet->ifNum, pNet->GpibAddr, 0, 0);

		// start usb test
		int32_t sts;
		//EROP("Start UsbSynchronize :\n\n", 0, 0, 0, 0);
		//printf("I am in USB events:\n\n");
		sts = UsbSynchronize(2000); //was 1000USB_WAIT_FOREVER

		//EROP("UsbSynchronize: err=0x%x : %s\n", sts, UsbGetErrorString(sts), 0, 0);

		if (sts != USB_ERR_NORMAL_COMPLETION)
		{
			//EROP("UsbSynchronize: err=0x%x : %s\n", sts, UsbGetErrorString(sts), 0, 0);
			rc = FAILURE;// IDS_VLCRTERR_USB_STACK_ERROR;
		}
		//if (rc == SUCCESS)
		//UsbEventsWithCallback(attach_cb, NULL, WAIT_FOREVER);

	}

	if (rc == SUCCESS)
	{
		// connect to usb callback
		// EROP("UsbEventsWithCallback\n", 0, 0, 0, 0);
		// UsbEventsWithCallback(attach, detach, WAIT_FOREVER);

		//new_conf.settings.eos = eosmode & 0xff;                           /* local eos modes                  */
		//new_conf.settings.eos_flags = eosmode & 0xff00;
		//new_conf.settings.usec_timeout = timeout_to_usec(timo);
		//if (eot)
		//	new_conf.settings.send_eoi = 1;
		//else
		//	new_conf.settings.send_eoi = 0;



		gpib_board_t board;

		board.master = 1;
		board.buffer = NULL;
		board.buffer_length = 0;
		board.pad = 0;
		board.sad = 0;
		board.t1_nano_sec = 10000;
		
		struct usbDeviceId* id;
		id = malloc(sizeof(struct usbDeviceId));

		struct usbDeviceInfo*  udi;
		udi = malloc(sizeof(struct usbDeviceInfo));

		match_id_table->idVendor = pNet->Vendor_ID;
		match_id_table->idProduct = pNet->Product_ID;

		match_id_table->match_flags = USB_DEVICE_ID_MATCH_DEVICE;

		int32_t sts = USB_ERR_NORMAL_COMPLETION; //start find
		BOOL found = FALSE;
		//EROP("UsbFind\n", 0, 0, 0, 0);
		//EROP("Info: idVendor = 0x%x : 0x%x ,%d ", udi->idVendor, udi->idProduct, 0, 0);
		//EROP("Info BusNum,Addr,PortNum,Depth= %d , %d , %d , %d", udi->bBusNum, udi->bDevNum, udi->bPortNum, udi->bDepth);


		for (int i = 0; (i < 8); i++)
		{
			if (i == 0)
				sts = UsbFindFirstDevice(udi, match_id_table, &id);
			else
				sts = UsbFindNextDevice(udi, match_id_table, &id);

			//EROP("udi : idVendor = 0x%x : 0x%x , bInterfaceClass:%d, bDeviceClass:%d\n", id->idVendor, id->idProduct, id->bInterfaceClass, id->bDeviceClass);
			//EROP("Info: idVendor = 0x%x : 0x%x , %d, %d ", udi->idVendor, udi->idProduct, udi->bIfaceNum, udi->bIfaceCount);
			//EROP("Info BusNum,Addr,PortNum,Depth= %d , %d , %d , %d", udi->bBusNum, udi->bDevNum, udi->bPortNum, udi->bDepth);

			if (sts == USB_ERR_NORMAL_COMPLETION)
			{
				//EROP("UsbFind found\n", 0, 0, 0, 0);
				//found  check if is correct one using poer,bus,addr
				// for later
				// may need to compare info->bIfaceNum

					//if ((pNet->busNum == udi->bBusNum) && (pNet->addr == udi->bDevNum) &&
					//(pNet->PortNum == udi->bPortNum) && (pNet->depth == udi->bDepth))
					//{
					//EROP("UsbFind found\n", 0, 0, 0, 0);
					found = TRUE;
					rc = SUCCESS;
					break;
					//}
			}


			if (sts != USB_ERR_NORMAL_COMPLETION)
			{
				rc = FAILURE;
				//EROP("UsbFind eroor device: err=0x%x : %s\n", sts, UsbGetErrorString(sts), 0, 0);
				break;
			}
		}

		if (!found)
		{
			//EROP("UsbFind not found\n", 0, 0, 0, 0);
			rc = FAILURE;
		}

		if (rc == SUCCESS)	// connect usb
		{
			struct usbInterfaceDescriptor ifdesc = { 0 };
			struct usbEndpointDescriptor endpoint = { 0 };
			struct usbDeviceDescriptor descriptor = { 0 };
			//EROP("Link usb found\n", 0, 0, 0, 0);
			int32_t udev;
			usb_data * usbdata;
			uint8_t ep_cnt;

			usbdata = malloc(sizeof(usb_data));
			if (usbdata == NULL)
			{
				rc = FAILURE;
				return rc;
			}
			memset(usbdata, 0, sizeof(usb_data));
			usbdata->online = 0;
			usbdata->ibsta = 0;
			int32_t rval = UsbOpenDevice(udi, "NiUsbGpib", &udev, usbdata);
			//EROP("UsbOpenDevice error = %d , udev = %x,  %x\n", rval, udev, usbdata->udev, 0);

			if (rval != USB_ERR_NORMAL_COMPLETION)
			{
				//EROP("UsbOpenDevice: fails e=%d (%s)\n", rval, UsbGetErrorString(rval), 0, 0);
				rc = FAILURE;
			}
			// start link
			usbdata->udev = udev;
			usbdata->board = &board;

			// get end point count

			if ((rval = UsbGetInterfaceDescriptor(udev, &ifdesc)))
			{
				//EROP("could not retrieve Interface Descriptor =%d\n", rval, 0, 0, 0);
				usbdata->error_code = rval;
				rc = FAILURE;
			}
				

			if ((rval = UsbGetEndpointCount(udev, &ep_cnt)))// should be 2
			{
				//EROP("_open: could not retrieve Endpoint Count =%d\n", rval, 0, 0, 0);
				usbdata->error_code = rval;
				rc = FAILURE;
			}

			//EROP("UsbGetEndpointCount ufd =0x%x ep_cnt = %d\n", udev, ep_cnt, 0, 0);
			//extract the link to transfer
			BOOL efoundRcv = FALSE;
			BOOL efoundSnd = FALSE;

/*			- NIUSB_HS_PLUS_BULK_OUT_ENDPOINT = 0x1,
				NIUSB_HS_PLUS_BULK_OUT_ALT_ENDPOINT = 0x4,
			- 	NIUSB_HS_PLUS_BULK_IN_ENDPOINT = 0x2,
				NIUSB_HS_PLUS_BULK_IN_ALT_ENDPOINT = 0x5,
				NIUSB_HS_PLUS_INTERRUPT_IN_ENDPOINT = 0x3,

		case USB_DEVICE_ID_NI_USB_HS_PLUS:
		ni_priv->bulk_out_endpoint = NIUSB_HS_PLUS_BULK_OUT_ENDPOINT;
		ni_priv->bulk_in_endpoint = NIUSB_HS_PLUS_BULK_IN_ENDPOINT;
		ni_priv->interrupt_in_endpoint = NIUSB_HS_PLUS_INTERRUPT_IN_ENDPOINT;
		retval = ni_usb_hs_wait_for_ready(ni_priv);
*/

/*			for (int i = 0; i < ep_cnt; i++)
			{
				rval = UsbGetEndpointDescriptor(udev, (BYTE)i, &endpoint);
				//rval = UsbGetDeviceDescriptor(ufd, &descriptor);

				EROP("bEndpointAddress =%0x%x, 0x%x, %d,%d \n", endpoint.bEndpointAddress, i, endpoint.bDescriptorType, endpoint.bmAttributes);

				usbdata->_interval = endpoint.bInterval;
				usbdata->_maxpkt = endpoint.wMaxPacketSize;

				if ((endpoint.bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
				{//in
					if (!efoundRcv)
					usbdata->_pipeBlkRcv = USB_MKRCVBULKPIPE(info, endpoint.bEndpointAddress);
					EROP("found Endpoint Rcv =0x%x, 0x%x, Bulk =0x%x  ,%x \n", usbdata->_pipeBlkRcv, endpoint.bEndpointAddress, (endpoint.bEndpointAddress & USB_ENDPOINT_XFER_BULK), endpoint.bEndpointAddress);
					efoundRcv = TRUE;
				}
				else
				{//out
					if (!efoundSnd)
						usbdata->_pipeBlkSnd = USB_MKSNDBULKPIPE(info, endpoint.bEndpointAddress);
					EROP("found Endpoint Snd =0x%x, 0x%x, Bulk =0x%x, %x \n", usbdata->_pipeBlkSnd, endpoint.bEndpointAddress, (endpoint.bEndpointAddress & USB_ENDPOINT_XFER_BULK), endpoint.bEndpointAddress);
					efoundSnd = TRUE;
				}

				// if (efoundSnd && efoundRcv) break;
			}
*/
			// this part nobody teaches, lucky have linux sourcecode to refer

			usbdata->_pipeBlkSnd = USB_MKSNDBULKPIPE(udi,  0x01); //0x02);//
			usbdata->_pipeBlkRcv = USB_MKRCVBULKPIPE(udi,  0x82); //0x81);//
			usbdata->_pipeIntRcv = USB_MKRCVINTPIPE(udi, 0x85);
			usbdata->udev = udev;
			usbdata->_dev = udi;

			UsbGetDeviceDescriptor(udev, &descriptor);
			
			ni_usb_hs_wait_for_ready(usbdata);
			ni_usb_hs_plus_extra_init(usbdata);
			ni_usb_init(usbdata);
			pNet->USB_Data = usbdata;
			
			int32_t snd;

			// Xlear end points
			snd=UsbClearStall(usbdata->udev, usbdata->_pipeBlkRcv);
			//EROP("UsbClearStall : snd =%d (%s)\n", snd, UsbGetErrorString(snd), 0, 0);
			snd=UsbClearStall(usbdata->udev, usbdata->_pipeBlkSnd);
			//EROP("UsbClearStall : snd =%d (%s)\n", snd, UsbGetErrorString(snd), 0, 0);
			//EROP("Clear stall end points  \n", 0, 0, 0, 0);

				//pad = 0	/* primary address of interface             */
				//sad = 0	/* secondary address of interface           */
				//timeout = T3s	/* timeout for commands T10s = 13,*/

				//eos = 0x0a	/* EOS Byte, 0xa is newline and 0xd is carriage return */
				//set - reos = yes	/* Terminate read if EOS */
				//set - bin = no	/* Compare EOS 8-bit */
				//set - xeos = no	/* Assert EOI whenever EOS byte is sent */
				//set - eot = yes	/* Assert EOI with last byte on writes */


			ibpad(usbdata, pNet->GpibAddr); //must be first 
			//ibsad(usbdata, 0); //must be first 
			ni_usb_secondary_address(usbdata, 0, 0);
			ni_usb_enable_eos(usbdata, 0x0a, 0);
			ni_usb_return_to_local(usbdata);
			ni_usb_t1_delay(usbdata, timeout_to_usec(T10s));
		//	ibeos(usbdata,0, 0);
			
			ibrsc(usbdata, 0); // request_system_control
			//EROP("after ibrsc:  %x \n", status, 0, 0, 0);
			//ibsic(usbdata, 100);// go active
			//EROP("after ibsic:  %x \n", status, 0, 0, 0);
			 ibgts(usbdata);// active to idle

			int lines;
			iblines(usbdata, &lines);
			usbdata->_usb_thread = CreateRtThread(128, usb_thread, 8192, usbdata);// our thread

			if (usbdata->_usb_thread != BAD_RTHANDLE)
			{
				EROP("usbdata->_usb_thread \n", 0, 0, 0, 0);
				
					rc = IDS_VLCRTERR_TASK_TIMEOUT;
			}

			//EROP("IBWRT: %s, %x \n", IBWRT, IBWRT, 0, 0);
			
			//printf(" test %x \n", _IOW(GPIB_CODE, 12, 100));
			//pad_ioctl_t testoad;
			//testoad.pad = 12;
			//testoad.handle = 0;
			//printf(" test %x \n", _IOW(GPIB_CODE, 15, pad_ioctl_t));
//			int actlen;
//			ni_usb_write(usbdata, &testoad, sizeof(pad_ioctl_t), 0, &actlen, 5000);

			//printf(" test %s \n", _IOW(GPIB_CODE, 15, pad_ioctl_t));

			//ioctl(x, IBPAD, y);
			//
			//ni_usb_hs_wait_for_ready(usbdata);

/*
			

						//char Str[] = "*TST ?\n";
			char Str[] = "*IDN ?\n";
			int actlen;
			ni_usb_write(usbdata, Str, sizeof(Str) ,1, &actlen,2000);

			char RcvChar[512] = {0};
			//rtSleep(1000);
			int end;
			//ni_usb_read(usbdata, RcvChar, sizeof(RcvChar)-1, &end, &actlen);
			

			//ni_usb_b_read_serial_number(usbdata);
			
			// https://www.ni.com/en-sg/support/documentation/supplemental/18/gpib-error-codes-and-common-solutions--part-1-.html
			//char test[] = "caddr 5\r";
			*CLS - Clear status
				*ESE <enable_value> -Event status enable
				*ESR ? -Event status register query
				*IDN ? -Instrument identification
				*OPC - Set operation complete bit
				*OPC ? -Wait for current operation to complete
				*OPT ? -Show installed options
				*PSC{ 0 | 1 } -Power - on status clear
				*RCL{ 0 | 1 | 2 | 3 | 4 } -Recall instrument state
				*RST - Reset instrument to factory defaults
				*SAV{ 0 | 1 | 2 | 3 | 4 } -Save instrument state
				*SRE <enable_value> -Service request enable(enable bits in enable register of Status Byte Register group
					*STB ? -Read status byte
					*TRG - Trigger command
					*TST ? -Self - test
					* WAI - Wait for all pending operations to complete
*/
			//8,a
			//char test[] = { 0x0D,0xFA,0xFF,0xFC,0x08,0x0A,0x08,0x00,0x2A,
			//	0x54,0x53,0x54,0x3F,0x0A,0x00,0x00,0x04,0x00,0x00,0x00 };
			//= "*TST ?"
/*			char test[] = { 0x0B,0x00,0x0A,0xFC,0x00,0xFC,0xFF,0xFF,
				0x09,0x01,0x00,0x01,0x0A,0x55,0x00,0x00,0x04,0x00,0x0,0x00 };
			char* ourTx = &test[0];
			char rcv[512] = { 0 };
			char* ourRx = &rcv[0];
			int actlen = 0;
			
			snd = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkSnd, ourTx, 20 , &actlen, USB_MULTI_SHORT_OK, USB_DEFAULT_TIMEOUT);
			char test2[] = "*IDN ?\n";

			//UsbControlXfer(udev, bRequestType, bRequest, 0, wIndex, NULL, 0, NULL, 0, to_ms);
			//snd = UsbBulkXfer(usbdata->_ufd, usbdata->_pipeBlkSnd, &test2[0], sizeof(test2) - 1, &actlen, USB_SHORT_XFER_OK, USB_DEFAULT_TIMEOUT);

			EROP("actlen = %d, status = %d , snd: %s\n", actlen, snd, test, 0);
			actlen = 0;
			if (snd != USB_ERR_NORMAL_COMPLETION)
			{
				EROP("UsbBulkXfer: fails e=%d (%s)\n", snd, UsbGetErrorString(snd), 0, 0);
				rc = FAILURE;
			}

			snd = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkRcv, rcv, usbdata->_maxpkt, &actlen, USB_SHORT_XFER_OK, 2000);
			EROP("  actlen = %d, status = %d ,rcv : %s\n", actlen, snd, rcv, 0);
			
			if (snd != USB_ERR_NORMAL_COMPLETION)
			{
				EROP("UsbBulkXfer: fails e=%d (%s)\n", snd, UsbGetErrorString(snd), 0, 0);
				rc = FAILURE;
			}

			snd = UsbBulkXfer(usbdata->_ufd, usbdata->_pipeBlkRcv, ourRx, sizeof(rcv) - 1, &actlen, USB_SHORT_XFER_OK, 2000);
			EROP("  actlen = %d, status = %d ,rcv : %s, %d\n", actlen, snd, rcv, sizeof(rcv));

			if (snd != USB_ERR_NORMAL_COMPLETION)
			{
				EROP("UsbBulkXfer: fails e=%d (%s)\n", snd, UsbGetErrorString(snd), 0, 0);
				rc = FAILURE;
			}
*/

		//	EROP("SUCCESS rt open \n", 0, 0, 0, 0);

//			EROP("Close usb", 0, 0, 0, 0);
//			UsbCloseDevice(usbdata->udev);

			/*			National Instruments(3923) GPIB - USB - HS + (7618) 01ED4D17
							device : class 0 / 0 (Per Interface / -), rev 2.00 / 1.00
							speed high, power 500, bus 0, addr 5, port 7, depth 2
							iface : 0 of 2, class 255 / 0 / 0 (Vendor Specific / -/ -)
							driver = not attached
							Ep[1] Addr 01 (OUT)Attribs 02 (BULK)PktSize 512 Interval 100
							Ep[2] Addr 82 (IN)Attribs 02 (BULK)PktSize 512 Interval 100
							Ep[3] Addr 83 (IN)Attribs 03 (INT)PktSize 16 Interval 4
							Ep[4] Addr 04 (OUT)Attribs 02 (BULK)PktSize 512 Interval 100
							Ep[5] Addr 85 (IN)Attribs 02 (BULK)PktSize 512 Interval 100

							National Instruments(3923) GPIB - USB - HS + (7618) 01ED4D17
							device : class 0 / 0 (Per Interface / -), rev 2.00 / 1.00
							speed high, power 500, bus 0, addr 5, port 7, depth 2
							iface : 1 of 2, class 255 / 0 / 0 (Vendor Specific / -/ -)
							driver = not attached
							Ep[1] Addr 86 (IN)Attribs 02 (BULK)PktSize 512 Interval 100
							Ep[2] Addr 87 (IN)Attribs 03 (INT)PktSize 32 Interval 4

							////
							National Instruments(3923) GPIB - USB - HS + (761e) 01ED4D17
							device : class 0 / 0 (Per Interface / -), rev 2.00 / 1.00
							speed high, power 500, bus 0, addr 5, port 8, depth 2
							iface : 0 of 1, class 255 / 0 / 0 (Vendor Specific / -/ -)
							driver = not attached
							Ep[1] Addr 81 (IN)Attribs 02 (BULK)PktSize 512 Interval 100
							Ep[2] Addr 03 (OUT)Attribs 02 (BULK)PktSize 512 Interval 100
			*/

		}
	}

	if (rc == SUCCESS)
	{
		UINT32* pSentinel = BuildUiotPointer(pNet->ofsSentinel);
		if (*pSentinel != RT3_SENTINEL)
			rc = IDS_VLCRTERR_ALIGNMENT;
	}

	//EROP("rtOpen() pNet=%p, pErrors=%p, PortAddress=%x", pNet, pErrors, pNet->PortAddress, 0);

	if (rc == SUCCESS)
	{
		pNet->pDeviceList = BuildUiotPointer(pNet->ofsDeviceList);
		pNet->pInputList = BuildUiotPointer(pNet->ofsInputList);
		pNet->pOutputList = BuildUiotPointer(pNet->ofsOutputList);
	}

	if (!pNet->bSimulate)
	{
		if (rc == SUCCESS)
		{
		//	pNet->pDpr = AllocateDpr(pNet->DualPortAddress, DPR_TOTAL_SIZE);
		//	if (pNet->pDpr == NULL)
		//		rc = IDS_VLCRTERR_CREATE_DESCRIPTOR;
		}

		if (rc == SUCCESS)
		{
			LPDEVICE_INST pDevice = pNet->pDeviceList;
			LPDEVICE_IO   pInput = pNet->pInputList;
			LPDEVICE_IO   pOutput = pNet->pOutputList;


			for (; pDevice->Type; pDevice++)
				pDevice->pName = BuildUiotPointer(pDevice->ofsName);

			for (; pInput->Size; pInput++)
			{
				// TO DO: Generate here a pointer to the dpr input area for this device
				// In our example all devices have 8 input bytes
			//	pInput->pSrc = (void*)&pDpr->Input[pInput->Address * 8];
				pInput->pDst = BuildUiotPointer(pInput->ofsUiot);
			}

			for (; pOutput->Size; pOutput++)
			{
				// TO DO: Generate here a pointer to the dpr output area for this device
				// In our example all devices have 8 output bytes
			//	pOutput->pDst = (void*)&pDpr->Output[pOutput->Address * 8];
				pOutput->pSrc = BuildUiotPointer(pOutput->ofsUiot);
			}
		}

	

		if (rc == SUCCESS)
			rc = Init(pNet->pDpr, pNet->DprHWTests, pErrors);

		if (rc == SUCCESS)
		{
			rc =CreateBackgroundTask(pNet);
		}
	}

return rc;

}
int rtReload(LPDRIVER_INST pNet, P_ERR_PARAM pErrors)
{
	// Executing the LOAD PROJECT command
	//EROP("rtReload() pNet=%p, pErrors=%p", pNet, pErrors, 0, 0);
	if (!pNet->bSimulate)
	{
		InitLinkedList(&pNet->Pend);
		InitLinkedList(&pNet->Done);
	}

	// make sure pNet is in the same state as after rtOpen(). 
	return 0;
}

int rtOnLine(LPDRIVER_INST pNet, P_ERR_PARAM pErrors)
{
	// Executing the START PROJECT command
	int rc = SUCCESS;

	//EROP("rtOnLine() pNet=%p, pErrors=%p", pNet, pErrors, 0, 0);
	pNet->bFirstCycle = 1;
	pNet->bGoOffLine = 0;

	if (!pNet->bSimulate)
	{
		/* Check all devices. If critical devices are offline,  rc = IDS_FLATCODE_DEVICE_OFFLINE */

		//rc = TestConfig(pNet, pErrors);

		/*
			If we have a watchdog with a time consuming start procedure, start it here.
			If it takes much shorter than a scan cycle to start it, start it in the first input cycle
		 */
	}

	return rc;

}


int rtInput(LPDRIVER_INST pNet)
{
	// This is the beginning of the VLC scan cycle
	if (!pNet->bSimulate)
	{
		// Copy new input data from the hw to the driver input image in the UIOT. 
		LPDEVICE_INST pDevice = pNet->pDeviceList;

		//pNet->ofsInputList
		//bn = (pNet->PciIndex) - 1;

		for (; pDevice->Type; pDevice++)
		{
		//	pDevice.

		//	if (pDevice. Input.bUsed)
		//	{
				
		//bn = (pNet.) - 1;
				// Start Read IO,INT,POS from adlink card
				// bn crd number 0~3
				//rc = ADlinkReadIO(pDevice, bn, pDevice->Input.pDst);

		//	}
		}

		// Copy new input data from the hw to the driver input image in the UIOT. 
		LPDEVICE_IO pInput = pNet->pInputList;
		
		for (; pInput->Size; pInput++)
			if (pInput->bUsed)
			{

				GpibsStatusUpdate(pNet ,pDevice,  pInput->pDst);
				//printf("pInput Address: %d  \n", pInput->Address);
				// CardCopy(pInput->pDst, pInput->pSrc, pInput->Size);
			}
		/*
		if( pNet->bFirstCycle )
			Start the watchdog.
		else
			KickWD(dp);     kick watchdog, if any
		*/

		VerifyDoneList(&pNet->Done);    // Flush the completed background functions
	}

	// EROP( "rtInput() pNet=%p", pNet, 0, 0, 0 );

	return SUCCESS;
}


int rtOutput(LPDRIVER_INST pNet)
{
	// This is the end of the VLC scan cycle
	if (!pNet->bSimulate)
	{
		// Copy new output data from the UIOT driver output image to the hw.
		//LPDEVICE_IO pOutput = pNet->pOutputList;

		//for (; pOutput->Size; pOutput++)
		//	if (pOutput->bUsed)
		//		CardCopy(pOutput->pDst, pOutput->pSrc, pOutput->Size);

		if (pNet->bFirstCycle)     /* first Output() ? */
		{
			/*  Only now we have a valid output image in the DPR.
				EnableOutputs(dp);  enable outputs (if our hardware lets us)
			 */
			pNet->bFirstCycle = 0;
		}
	}

	// EROP( "rtOutput() pNet=%p", pNet, 0, 0, 0 );

	return SUCCESS;
}

int rtSpecial(LPDRIVER_INST pNet, LPSPECIAL_INST pData)
{
	// A trapeziodal block has been hit.
	usb_data* usbdata = ((usb_data*)(pNet->USB_Data)); //OK
	UINT16  Result = 0;
	UINT16  Status = VLCFNCSTAT_OK;

	// EROP( "rtSpecial() pNet=%p, pData=%p", pNet, pData, 0, 0 );

	if (!pNet->bSimulate)
	{
		int  FunctionId = pData->User.paramHeader.FunctionId;
		switch (FunctionId)
		{
		case DRVF_COMMAND:
			Status = Pend(pNet, pData);   /* *pResult is set by the bkg task */
			break;

		case DRVF_GET_DRVSTAT:
			GetDriverStatus(pNet, &pData->User.paramCommand);
			break;

		case DRVF_GET_DEVSTAT:
			GetDeviceStatus(pNet, &pData->User.paramCommand);
			break;

		case DEVF_GET_DEVSTAT:
			GetDeviceStatus(pNet, &pData->User.paramCommand);
			break;

		case DRVF_PORT_INPUT:
			PortInput(&pData->User.paramPort);
			break;

		case DRVF_PORT_OUTPUT:
			PortOutput(&pData->User.paramPort);
			break;

		case DRVF_PEEK:
			DoPeekCommand(pNet, &pData->User.paramCommand);
			break;

		case DRVF_POKE:
			DoPokeCommand(pNet, &pData->User.paramCommand);
			break;

		case DRVF_READ_GPIB: // kick this to thread;
			Status = Pend(pNet, pData);   /* *pResult is set by the bkg task */

		//	DoGpibRead(pNet, &pData->User.paramGpibRead);
			
			break;
			//paramGpibRead;
		case DRVF_WRITE_GPIB:
			DoGpibWrite(pNet, &pData->User.paramGpibWrite);
			break;

		case DRVF_SET_PAD:
			DoPadWrite(pNet, &pData->User.paramPadWrite);
			break;
		case DRVF_SET_SAD:
			DoSadWrite(pNet, &pData->User.paramSadWrite);
			break;

		case DRVF_SET_RSV:
			Status = Pend(pNet, pData);
			//DoIbRSVWrite(pNet, &pData->User.paramIbRSVWrite);
			break;

		case DRVF_CLEAR_TXRX: 
			Status = Pend(pNet, pData);
			break;


		default:
			Status = VLCFNCSTAT_WRONGPARAM;
			break;
		}

		EROP("Special();  FunId= %d, Status= %d, pData= %p", FunctionId, Status, pData, 0);
	}
	else
	{
		UINT16* pResult = BuildUiotPointer(pData->User.paramHeader.ofsResult);
		if (pResult)   // some functions may not have the Result param implemented
			*pResult = (UINT32)SUCCESS;

		Status = VLCFNCSTAT_SIMULATE;
	}

	if (pData->User.paramHeader.ofsStatus)   // some functions may not have the status param implemented
	{
		
		UINT16* pStatus = BuildUiotPointer(pData->User.paramHeader.ofsStatus);
		*pStatus = Status;
	}

	return SUCCESS;
}

int rtOffLine(LPDRIVER_INST pNet, P_ERR_PARAM pErrors)
{
	usb_data* usbdata = ((usb_data*)(pNet->USB_Data)); //OK

	// Executing the STOP PROJECT command
	int rc = SUCCESS;
	ni_usb_stop(usbdata);
	ibgts(usbdata);// active to idle
	DeleteRtThread(usbdata->_usb_thread);
//	ni_usb_shutdown_hardware(usbdata);
	EROP("rtOffLine() pNet=%p, pErrors=%p", pNet, pErrors, 0, 0);

	pNet->bGoOffLine = 1;
	if (!pNet->bSimulate)
	{
		rc = WaitForAllFunctionCompletion(pNet);  /* wait for the backgroung task to calm down */

		if (rc == SUCCESS)
		{
			/*
			DUAL_PORT far *  dp  = (DUAL_PORT far *)pNet->pDpr;
			if( pNet->StopState == 1 )
				rc = stop scanning;

			DisableOutputs(dp, &pNet->trans_count);
			DisableWD(dp);
			*/
		}

	}

	EROP("rtOffLine(). exit  rc= %d", rc, 0, 0, 0);

	return rc;
}

/*   if Open() fails, Close() is not automatically called for this instance.
	 if lpErrors == NULL, do not report any error, keep the Open()'s error code and params.
 */
int rtClose(LPDRIVER_INST pNet, P_ERR_PARAM pErrors)
{
	usb_data* usbdata = ((usb_data*)(pNet->USB_Data)); //OK
	ni_usb_stop(usbdata);
	ibgts(usbdata);// active to idle
	EROP("Close usb", 0, 0, 0, 0);
	UsbCloseDevice(usbdata->udev);
	DeleteRtThread(usbdata->_usb_thread);
	// Executing the UNLOAD PROJECT command
	if (!pNet->bSimulate)
	{
		EROP("rtClose(). start. pNet= %p", pNet, 0, 0, 0);
		/*
		{
			DUAL_PORT far* const dp = (DUAL_PORT far *)pNet->pDpr;     / * pointer to the dualport * /
			Reset the board;
		}
		*/

		//DeleteInterruptTask( pNet );
		DeleteBackgroundTask(pNet);

		if (pNet->pDpr)
		{
			FreeDpr(pNet->pDpr);
			pNet->pDpr = NULL;
		}
	}

	EROP("rtClose() pNet=%p, pErrors=%p", pNet, pErrors, 0, 0);
	return SUCCESS;
}

int rtUnload()
{

	// Executing the UNLOAD PROJECT command
	EROP("rtUnload()", 0, 0, 0, 0);
	return 0;
}


char * device_speed(int speed)
{

	switch (speed)
	{
	case USB_SPEED_VARIABLE: return "variable";
	case USB_SPEED_LOW: return "low";
	case USB_SPEED_FULL: return "full";
	case USB_SPEED_HIGH: return "high";
	case USB_SPEED_SUPER: return "super";
	default:
		return "unknown";
	}
}


static void dump_hid_report_descriptor(struct usbDeviceInfo * dev, BYTE ifno)
{
	struct usbHidDescriptor hdesc;
	BYTE * report_desc;
	int i;
	int linecnt = 0;

	if (get_hid_config_descriptor(dev, ifno, USB_DT_HID, 0, &hdesc) == FALSE)
		return;

	printf("        [HID Report Description / Country='%s']\n",
		get_hid_country(hdesc.bCountryCode)
	);

	report_desc = get_hid_report_descriptor(dev, ifno, 0, hdesc.wDescriptorLength);
	if (report_desc)
	{

		for (i = 0; i < hdesc.wDescriptorLength; i++)
		{
			if (i % 16 == 0)
			{
				if (!linecnt && !i)
					printf("          > ");
				else
					if (i + 1 < hdesc.wDescriptorLength)
						printf("\n          > ", ++linecnt);
			}
			else
				printf("%02x ", report_desc[i]);
		}
		printf("\n");
		free(report_desc);
	}
}

typedef struct {
	char * cinfo;
	char * scinfo;
	char * prinfo;
} class_info;

char * ifclass(int cl, int scl, int pr, class_info * ci)
{
	ci->cinfo = "-";
	ci->scinfo = "-";
	ci->prinfo = "-";

	switch (cl)
	{
	case USB_CLASS_PER_INTERFACE:
		ci->cinfo = "Per Interface";
		break;
	case USB_CLASS_AUDIO:
		ci->cinfo = "Audio";
		switch (scl)
		{
		case USB_SUBCLASS_AUDIOCONTROL:
			ci->scinfo = "Audio Control"; break;
		case USB_SUBCLASS_AUDIOSTREAM:
			ci->scinfo = "Audio Stream"; break;
		case USB_SUBCLASS_MIDISTREAM:
			ci->scinfo = "MIDI Stream"; break;
		default:
			return "";
		}
		break;
	case USB_CLASS_COMM:
		ci->cinfo = "COMM";
		switch (scl)
		{
		case USB_SUBCLASS_DIRECT_LINE_CONTROL_MODEL:
			ci->scinfo = "Audio Control"; break;
		case USB_SUBCLASS_ABSTRACT_CONTROL_MODEL:
			ci->scinfo = "Audio Stream"; break;
		case USB_SUBCLASS_TELEPHONE_CONTROL_MODEL:
			ci->scinfo = "MIDI Stream"; break;
		case USB_SUBCLASS_MULTICHANNEL_CONTROL_MODEL:
			ci->scinfo = "Multichannel Control"; break;
		case USB_SUBCLASS_CAPI_CONTROL_MODEL:
			ci->scinfo = "CAPI Control"; break;
		case USB_SUBCLASS_ETHERNET_NETWORKING_CONTROL_MODEL:
			ci->scinfo = "Ethernet Control"; break;
		case USB_SUBCLASS_ATM_NETWORKING_CONTROL_MODEL:
			ci->scinfo = "ATM Control"; break;
		case USB_SUBCLASS_WIRELESS_HANDSET_CM:
			ci->scinfo = "Wireless Handset CM"; break;
		case USB_SUBCLASS_DEVICE_MGMT:
			ci->scinfo = "Device Mgmt"; break;
		case USB_SUBCLASS_MOBILE_DIRECT_LINE_MODEL:
			ci->scinfo = "Mobile Direct Line"; break;
		case USB_SUBCLASS_OBEX:
			ci->scinfo = "OBEX"; break;
		case USB_SUBCLASS_ETHERNET_EMULATION_MODEL:
			ci->scinfo = "Ethernet Emulation"; break;
		case USB_SUBCLASS_NETWORK_CONTROL_MODEL:
			ci->scinfo = "Network Control"; break;
		default:
			return "";
		}
		break;
	case USB_CLASS_HID:
		ci->cinfo = "HID";
		switch (scl)
		{
		case USB_SUBCLASS_BOOT:
			ci->scinfo = "Boot"; break;
		}
		switch (pr)
		{
		case USB_PROTO_BOOT_KEYBOARD:
			ci->prinfo = "Keyboard"; break;
		case USB_PROTO_MOUSE:
			ci->prinfo = "Mouse"; break;
		}

		break;
	case USB_CLASS_PHYSICAL:
		ci->cinfo = "Physical";
		break;
	case USB_CLASS_STILL_IMAGE:
		ci->cinfo = "Still Image";
		switch (scl)
		{
		case USB_SUBCLASS_SIC:
			ci->scinfo = "Still Image"; break;
		}
		break;
	case USB_CLASS_PRINTER:
		ci->cinfo = "Printer";
		switch (scl)
		{
		case USB_SUBCLASS_PRINTER:
			ci->scinfo = "Printer"; break;
		}
		switch (pr)
		{
		case USB_PROTO_PRINTER_UNI:
			ci->prinfo = "Unidirectional"; break;
		case USB_PROTO_PRINTER_BI:
			ci->prinfo = "Bidirectional"; break;
		case USB_PROTO_PRINTER_1284:
			ci->prinfo = "1284"; break;
		}
		break;
	case USB_CLASS_MASS_STORAGE:
		ci->cinfo = "Mass Storage";
		switch (scl)
		{
		case USB_SUBCLASS_RBC:
			ci->scinfo = "RBC"; break;
		case USB_SUBCLASS_SFF8020I:
			ci->scinfo = "SFF8020I"; break;
		case USB_SUBCLASS_QIC157:
			ci->scinfo = "QIC157"; break;
		case USB_SUBCLASS_UFI:
			ci->scinfo = "UFI"; break;
		case USB_SUBCLASS_SFF8070I:
			ci->scinfo = "SFF8070I"; break;
		case USB_SUBCLASS_SCSI:
			ci->scinfo = "SCSI"; break;
		}
		switch (pr)
		{
		case USB_PROTO_MASS_CBI_I:
			ci->prinfo = "Mass CBI_I"; break;
		case USB_PROTO_MASS_CBI:
			ci->prinfo = "Mass CBI"; break;
		case USB_PROTO_MASS_BBB_OLD:
			ci->prinfo = "Mass BBB Old"; break;
		case  USB_PROTO_MASS_BBB:
			ci->prinfo = "Mass BBB"; break;
		}
		break;
	case USB_CLASS_HUB:
		ci->cinfo = "Hub";
		switch (scl)
		{
		case USB_SUBCLASS_HUB:
			ci->scinfo = "Hub"; break;
		}
		switch (pr)
		{
		case	USB_PROTO_FSHUB:
			ci->prinfo = "Full Speed"; break;
			// case	USB_PROTO_HSHUBSTT:
			//     ci->prinfo = "Full Speed";break;
		case	USB_PROTO_HSHUBMTT:
			ci->prinfo = "High Speed"; break;
		}
		break;
	case USB_CLASS_CDC_DATA:
		ci->cinfo = "CDC Data";
		switch (scl)
		{
		case	USB_SUBCLASS_DATA:
			ci->scinfo = "Data"; break;
		}
		switch (pr)
		{
		case USB_PROTO_DATA_ISDNBRI:
			ci->prinfo = "ISDN Physical Interface"; break;
		case	USB_PROTO_DATA_HDLC:
			ci->prinfo = "HDLC"; break;
		case	USB_PROTO_DATA_TRANSPARENT:
			ci->prinfo = "Transparent"; break;
		case	USB_PROTO_DATA_Q921M:
			ci->prinfo = " Management for Q921"; break;
		case	USB_PROTO_DATA_Q921:
			ci->prinfo = " Data for Q921"; break;
		case	USB_PROTO_DATA_Q921TM:
			ci->prinfo = " TEI multiplexer for Q921"; break;
		case	USB_PROTO_DATA_V42BIS:
			ci->prinfo = "V42BIS Data Compression"; break;
		case	USB_PROTO_DATA_Q931:
			ci->prinfo = "Q031  Euro-ISDN "; break;
		case	USB_PROTO_DATA_V120:
			ci->prinfo = "V120  V.24 rate adaption"; break;
		case	USB_PROTO_DATA_CAPI:
			ci->prinfo = "CAPI 2.0 commands"; break;
		case	USB_PROTO_DATA_HOST_BASED:
			ci->prinfo = " Host based driver"; break;
		case	USB_PROTO_DATA_PUF:
			ci->prinfo = "DATA PUF"; break;
		case	USB_PROTO_DATA_VENDOR:
			ci->prinfo = "DATA Vendor Specific"; break;
		case	USB_PROTO_DATA_NCM:
			ci->prinfo = " Network Control Model "; break;
		}
		break;
	case USB_CLASS_CSCID:
		ci->cinfo = "CSCID";
		break;
	case USB_CLASS_FIRM_UPD:
		ci->cinfo = "Firm Upd";
		break;
	case USB_CLASS_CONTENT_SEC:
		ci->cinfo = "Content Sec";
		break;
	case USB_CLASS_DIAGNOSTIC:
		ci->cinfo = "Diagnostic";
		break;
	case USB_CLASS_WIRELESS:
		ci->cinfo = "Wireless";
		break;
	case USB_CLASS_APPL_SPEC:
		ci->cinfo = "Application Specific";
		break;
	case USB_CLASS_VENDOR_SPEC:
		ci->cinfo = "Vendor Specific";
		break;
	}
	return "";
}




int32_t attach_cb(struct usbDeviceInfo * udi)
{
	BYTE end_pt_cnt;
	DWORD i;
	struct usbEndpointDescriptor epdesc;
	struct usbInterfaceDescriptor ifdesc;
	class_info if_ci;
	class_info dev_ci;

	memset(&ifdesc, 0xa5, sizeof(ifdesc));

	UsbGetInterfaceDescriptor(udi->iHandle, &ifdesc);
	ifclass(udi->bDeviceClass, udi->bDeviceSubClass, 0, &dev_ci);
	ifclass(ifdesc.bInterfaceClass, ifdesc.bInterfaceSubClass, ifdesc.bInterfaceProtocol, &if_ci);

	printf("%s (%04x) %s (%04x) %s\n  device: class %d/%d (%s/%s), rev %x.%02x/%x.%02x\n"
		"  speed %s, power %d, bus %d, addr %d, port %d, depth %d\n  ",
		udi->szVendor, udi->idVendor,
		udi->szProduct, udi->idProduct,
		udi->szSerial,
		udi->bDeviceClass, udi->bDeviceSubClass,
		dev_ci.cinfo, dev_ci.scinfo,
		(udi->bcdUSB >> 8), udi->bcdUSB & 0xFF,
		(udi->bcdDevice >> 8), udi->bcdDevice & 0xFF,
		device_speed(udi->bSpeed),
		udi->wPower,
		udi->bBusNum,
		udi->bDevNum,
		udi->bPortNum,
		udi->bDepth);

	printf("  iface: %d of %d, class %d/%d/%d (%s/%s/%s)\n\tdriver=%s\n",
		udi->bIfaceNum,
		udi->bIfaceCount,
		ifdesc.bInterfaceClass, ifdesc.bInterfaceSubClass, ifdesc.bInterfaceProtocol,
		if_ci.cinfo, if_ci.scinfo, if_ci.prinfo,
		udi->bAttached ? udi->szDriver : "not attached"
	);

	fflush(stdout);
	//UsbOpenDevice(&udev, "usbls", &ufd);
	//UsbCloseDevice(ufd);

	UsbGetEndpointCount(udi->iHandle, &end_pt_cnt);
	for (i = 0; i < end_pt_cnt; i++)
	{
		char addr[16] = { 0 };
		char attribs[32] = { 0 };

		UsbGetEndpointDescriptor(udi->iHandle, (BYTE)i, &epdesc);
		if ((epdesc.bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
			strcpy(addr, "IN");
		else
			strcpy(addr, "OUT");
		switch (epdesc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
		{
		case USB_ENDPOINT_XFER_CONTROL:
			strcpy(attribs, "CNTRL");
			break;
		case USB_ENDPOINT_XFER_ISOC:
			strcpy(attribs, "ISOC");
			break;
		case USB_ENDPOINT_XFER_BULK:
			strcpy(attribs, "BULK");
			break;
		case USB_ENDPOINT_XFER_INT:
			strcpy(attribs, "INT");
			break;
		default:
			strcpy(attribs, "??");
		}
		printf("    Ep[%d] Addr %02x (%s) Attribs %02x (%s) PktSize %d Interval %d\n",
			i + 1, epdesc.bEndpointAddress, addr, epdesc.bmAttributes, attribs, epdesc.wMaxPacketSize, epdesc.bInterval);

	}

	if (udi->bInterfaceClass == USB_CLASS_HID)
		dump_hid_report_descriptor(udi, udi->bIfaceNum);

	printf("\n");

	return 0;
}



