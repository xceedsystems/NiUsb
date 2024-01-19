/***************************************************************************
                               sys/ibcac.c
                             -------------------

    copyright            : (C) 2001, 2002 by Frank Mori Hess
    email                : fmhess@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//#include "..\ gpibP.h"
#include "..\ni_usb_gpib.h"
//#include "..\ib.h"
//#include "..\gpib_ioctl.h"

/*
 * IBCAC
 * Return to the controller active state from the
 * controller standby state, i.e., turn ATN on.  Note
 * that in order to enter the controller active state
 * from the controller idle state, ibsic must be called.
 * If sync is non-zero, attempt to take control synchronously.
 * If fallback_to_async is non-zero, try to take control asynchronously
 * if synchronous attempt fails.
 */
int ibcac( usb_data *usbdata, int sync , int fallback_to_async)
{
	int status;// = ibstatus(usbdata);
	int retval;

	if( ( status & CIC ) == 0 )
	{
		printk("gpib: not CIC during ibcac()\n");
		return -1;
	}

	if( status & ATN )
	{
		return 0;
	}
	
	if(sync && (status & LACS) == 0)
	{
		/* tcs (take control synchronously) can only possibly work when
		*  controller is listener.  Error code also needs to be -ETIMEDOUT
		*  or it will giveout without doing fallback. */
		retval = -ETIMEDOUT;
	}
	else
	{
		//retval = board->interface->take_control( board, sync );
		ni_usb_take_control(usbdata, sync, 1000);
	}
	if( retval < 0 && fallback_to_async)
	{
		if(sync && retval == -ETIMEDOUT)
			retval = ni_usb_take_control(usbdata, sync, 0);
	}
	//board->interface->update_status( board, 0 );
	ni_usb_update_status(usbdata, 0);
	return retval;
}




