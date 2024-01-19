/***************************************************************************
                          ni_usb/ni_usb_gpib.c
                             -------------------
 driver for National Instruments usb to gpib adapters

    begin                : 2004-05-29
    copyright            : (C) 2004 by Frank Mori Hess
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
#include "stdafx.h"

#include "nec7210_registers.h"
#include "tnt4882_registers.h"

#include <rt.h>
#include <stdio.h>

#include <usbif3.h>


//#include "vlcport.h"
//#include "dcflat.h"     // EROP()
#include "driver.h"     /* SEMAPHORE */
#include "ib.h"
#include "ni_usb_gpib.h"
#include "Auxrut.h"

#define MAX_NUM_NI_USB_INTERFACES 128

//int parse_board_ibrd_readback(const uint8_t * raw_data, ni_usb_status_block * status, uint8_t * parsed_data, int parsed_data_length, int * actual_bytes_read);


//static struct usb_interface *ni_usb_driver_interfaces[MAX_NUM_NI_USB_INTERFACES];

//static int ni_usb_parse_status_block(const uint8_t *buffer, struct ni_usb_status_block *status);
//static int ni_usb_set_interrupt_monitor(LPDRIVER_INST pNet , unsigned int monitored_bits);
//static void ni_usb_stop(ni_usb_private_t *ni_priv);

//static DEFINE_MUTEX(ni_usb_hotplug_lock);

//calculates a reasonable timeout in that can be passed to usb functions
static inline unsigned long ni_usb_timeout_msecs(unsigned int usec)
{
	if(usec == 0) return 0;
	return 2000 + usec / 500;
};
// returns timeout code byte for use in ni-usb-b instructions
static unsigned short ni_usb_timeout_code(unsigned int usec)
{
	if( usec == 0 ) return 0xf0;
	else if( usec <= 10 ) return 0xf1;
	else if( usec <= 30 ) return 0xf2;
	else if( usec <= 100 ) return 0xf3;
	else if( usec <= 300 ) return 0xf4;
	else if( usec <= 1000 ) return 0xf5;
	else if( usec <= 3000 ) return 0xf6;
	else if( usec <= 10000 ) return 0xf7;
	else if( usec <= 30000 ) return 0xf8;
	else if( usec <= 100000 ) return 0xf9;
	else if( usec <= 300000 ) return 0xfa;
	else if( usec <= 1000000 ) return 0xfb;
	else if( usec <= 3000000 ) return 0xfc;
	else if( usec <= 10000000 ) return 0xfd;
	else if( usec <= 30000000 ) return 0xfe;
	else if( usec <= 100000000 ) return 0xff;
	else if( usec <= 300000000 ) return 0x01;
	/* NI driver actually uses 0xff for timeout T1000s, which is a bug in their code.
	 * I've verified on a usb-b that a code of 0x2 is correct for a 1000 sec timeout */
	else if( usec <= 1000000000 ) return 0x02;
	else
	{
		// printf("%s: bug? usec is greater than 1e9\n", __FILE__);
		return 0xf0;
	}
};


//int ni_usb_receive_control_msg(ni_usb_private_t *ni_priv, __u8 request, __u8 requesttype, __u16 value, __u16 index,
//	void *data, __u16 size, int timeout_msecs)
//{
//	struct usb_device *usb_dev;
//	int retval;
//	unsigned int in_pipe;
//
//	mutex_lock(&ni_priv->control_transfer_lock);
//	if(ni_priv->bus_interface == NULL)
//	{
//		mutex_unlock(&ni_priv->control_transfer_lock);
//		return -ENODEV;
//	}
//	usb_dev = interface_to_usbdev(ni_priv->bus_interface);
//	in_pipe = usb_rcvctrlpipe(usb_dev, 0);
//	retval = USB_CONTROL_MSG(usb_dev, in_pipe, request, requesttype, value, index, data, size, timeout_msecs);
//	mutex_unlock(&ni_priv->control_transfer_lock);
//	return retval;
//}

void ni_usb_soft_update_status(usb_data* usbdata,  unsigned int ni_usb_ibsta, unsigned int clear_mask)
{
	//printf("%s , hang disabled first, STATUS = %x\n", __FUNCTION__, ni_usb_ibsta);
	return;
	static const unsigned int ni_usb_ibsta_mask = SRQI | ATN | CIC | REM | LACS | TACS | LOK;


	unsigned int need_monitoring_bits = ni_usb_ibsta_monitor_mask;
	unsigned long flags;

	usbdata->board->status &= ~clear_mask;
	usbdata->board->status &= ~ni_usb_ibsta_mask;
	usbdata->board->status |= ni_usb_ibsta & ni_usb_ibsta_mask;
	//FIXME should generate events on DTAS and DCAS

	//spin_lock_irqsave(&board->spinlock, flags);
	//ni_priv->monitored_ibsta_bits &= ~ni_usb_ibsta;   /* remove set status bits from monitored set why ?***/
	//need_monitoring_bits &= ~ni_priv->monitored_ibsta_bits; /* mm - monitored set */
	//spin_unlock_irqrestore(&board->spinlock, flags);

	//GPIB_Dprintf ("%s: need_monitoring_bits=0x%x\n", __FUNCTION__, need_monitoring_bits);

	//if(need_monitoring_bits & ~ni_usb_ibsta)
	//{
	//	ni_usb_set_interrupt_monitor(board, ni_usb_ibsta_monitor_mask);
	//}else if(need_monitoring_bits & ni_usb_ibsta)
	//{
	//	wake_up_interruptible( &board->wait );
	//}
 	//GPIB_Dprintf ("%s: ni_usb_ibsta=0x%x\n", __FUNCTION__, ni_usb_ibsta);
	//printf("%s: \n", __FUNCTION__);
	return;
}

static int ni_usb_parse_status_block(const uint8_t *buffer, struct ni_usb_status_block *status)
{
	uint16_t count;

	status->id = buffer[0];
	status->ibsta = (buffer[1] << 8) | buffer[2];
	status->error_code = buffer[3];
	count = buffer[4] | (buffer[5] << 8);
	count = ~count;
	count++;
	status->count = count;
	return 8;
};

static void ni_usb_dump_raw_block(const uint8_t *raw_data, int length)
{
	int i;

	// printf("%s:", __FUNCTION__);
	for(i = 0; i < length; i++)
	{
		if(i % 8 == 0)
		 printf("\n");
		 printf(" %2x", raw_data[i]);
	}
	 printf("\n");
}

int ni_usb_parse_register_read_block(const uint8_t *raw_data, unsigned int *results, int num_results)
{
	int i = 0;
	int j;
	int unexpected = 0;
	static const int results_per_chunk = 3;
	for(j = 0; j < num_results;)
	{
		int k;
		if(raw_data[i++] != NIUSB_REGISTER_READ_DATA_START_ID)
		{
			// printf("%s: %s: parse error: wrong start id\n", __FILE__, __FUNCTION__);
			unexpected = 1;
		}
		for(k = 0; k < results_per_chunk && j < num_results; ++k)
		{
			results[j++] = raw_data[i++];
		}
	}
	while(i % 4)
	{
		i++;
	}
	if(raw_data[i++] != NIUSB_REGISTER_READ_DATA_END_ID)
	{
		// printf("%s: %s: parse error: wrong end id\n", __FILE__, __FUNCTION__);
		unexpected = 1;
	}
	if(raw_data[i++] % results_per_chunk != num_results % results_per_chunk)
	{
		// printf("%s: %s: parse error: wrong count=%i for NIUSB_REGISTER_READ_DATA_END\n",
		//	__FILE__, __FUNCTION__, (int)raw_data[i - 1]);
		unexpected = 1;
	}
	while(i % 4)
	{
		if(raw_data[i++] != 0)
		{
		//	 printf("%s: %s: unexpected data: raw_data[%i]=0x%x, expected 0\n",
		//		__FILE__, __FUNCTION__, i - 1, (int)raw_data[i - 1]);
			unexpected = 1;
		}
	}
	if(unexpected)
		ni_usb_dump_raw_block(raw_data, i);
	return i;
}

int ni_usb_parse_termination_block(const uint8_t *buffer)
{

	int i = 0;

	if(buffer[i++] != NIUSB_TERM_ID ||
		buffer[i++] != 0x0 ||
		buffer[i++] != 0x0 ||
		buffer[i++] != 0x0)
	{
		// printf("%s: received unexpected termination block\n" , __FILE__);
		// printf(" expected: 0x%x 0x%x 0x%x 0x%x\n",
		//	NIUSB_TERM_ID, 0x0, 0x0, 0x0);
		// printf(" received: 0x%x 0x%x 0x%x 0x%x\n",
		//	buffer[i - 4], buffer[i - 3], buffer[i - 2], buffer[i - 1]);
	}
	return i;
};

int parse_board_ibrd_readback(const uint8_t *raw_data, struct ni_usb_status_block *status,
	uint8_t *parsed_data, int parsed_data_length, int *actual_bytes_read)
{
	static const int ibrd_data_block_length = 0xf;
	static const int ibrd_extended_data_block_length = 0x1e;
	int data_block_length = 0;
	int i = 0;
	int j = 0;
	int k;
	unsigned int adr1_bits;
	int num_data_blocks = 0;
	struct ni_usb_status_block register_write_status;
	int unexpected = 0;
	//EROP("parse_board_ibrd_readback parsed_data_length: %d , actual_bytes_read: %d\n", parsed_data_length, &actual_bytes_read,0,0);
	while(raw_data[i] == NIUSB_IBRD_DATA_ID || raw_data[i] == NIUSB_IBRD_EXTENDED_DATA_ID)
	{
		//EROP("Whike %d\n",i,0,0,0);

		if(raw_data[i] == NIUSB_IBRD_DATA_ID)
		{
			data_block_length = ibrd_data_block_length;
		}else if(raw_data[i] == NIUSB_IBRD_EXTENDED_DATA_ID)
		{
			data_block_length = ibrd_extended_data_block_length;
			if(raw_data[++i] != 0)
			{
				// printf(" %s: unexpected data: raw_data[%i]=0x%x, expected 0\n",
				//	 __FUNCTION__, i, (int)raw_data[i]);
				unexpected = 1;
			}
		}else
		{
			// printf("%s: logic bug!\n", __FILE__);
			return -EINVAL;
		}
		++i;
		for(k = 0; k < data_block_length; k++)
		{
			if(j < parsed_data_length)
				parsed_data[j++] = raw_data[i++];
			else
				++i;
		}
		++num_data_blocks;
	}
	i += ni_usb_parse_status_block(&raw_data[i], status);
	if(status->id != NIUSB_IBRD_STATUS_ID)
	{
		 //printf( "bug: status->id=%i, != ibrd_status_id\n",  status->id);
		return -EIO;
	}
	adr1_bits = raw_data[i++];
	if(num_data_blocks)
		*actual_bytes_read = (num_data_blocks - 1) * data_block_length + raw_data[i++];
	else
	{
		++i;
		*actual_bytes_read = 0;
	}
	if(*actual_bytes_read > j)
	{
		 //printf("%s: bug: discarded data. actual_bytes_read=%i, j=%i\n", __FILE__, *actual_bytes_read, j);
	}
	for(k = 0; k < 2; k++)
		if(raw_data[i++] != 0)
		{
			 //printf("%s: %s: unexpected data: raw_data[%i]=0x%x, expected 0\n",
			//	__FILE__, __FUNCTION__, i - 1, (int)raw_data[i - 1]);
			unexpected = 1;
		}
	i += ni_usb_parse_status_block(&raw_data[i], &register_write_status);
	if(register_write_status.id != NIUSB_REG_WRITE_ID)
	{
		 //printf("%s: %s: unexpected data: register write status id=0x%x, expected 0x%x\n",
		//	__FILE__, __FUNCTION__, register_write_status.id, NIUSB_REG_WRITE_ID);
		unexpected = 1;
	}
	if(raw_data[i++] != 2)
	{
		 //printf("%s: %s: unexpected data: register write count=%i, expected 2\n",
		//	__FILE__, __FUNCTION__, (int)raw_data[i - 1]);
		unexpected = 1;
	}
	for(k = 0; k < 3; k++)
		if(raw_data[i++] != 0)
		{
			// printf("%s: %s: unexpected data: raw_data[%i]=0x%x, expected 0\n",
			//	__FILE__, __FUNCTION__, i - 1, (int)raw_data[i - 1]);
			unexpected = 1;
		}
	i += ni_usb_parse_termination_block(&raw_data[i]);
	if(unexpected)
		ni_usb_dump_raw_block(raw_data, i);
	return i;
}

int ni_usb_parse_reg_write_status_block(const uint8_t *raw_data, struct ni_usb_status_block *status, int *writes_completed)
{
	int i = 0;

	i += ni_usb_parse_status_block(raw_data, status);
	*writes_completed = raw_data[i++];
	while(i % 4) i++;
	//printf("%s \n", __FUNCTION__);
	return i;
}
												//const
int ni_usb_write_registers(usb_data* usbdata, const struct ni_usb_register *writes, int num_writes,
	unsigned int *ibsta)
{
	//printf("%s: \n", __FUNCTION__);
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	int retval;
	uint8_t *out_data, *in_data;
	int out_data_length;
	static const int in_data_length = 0x20;
	int bytes_written = 0, bytes_read = 0;
	int i = 0;
	int j;
	struct ni_usb_status_block status;
	static const int bytes_per_write = 3;
	int reg_writes_completed;

	out_data_length = num_writes * bytes_per_write + 0x10;
	out_data = malloc(out_data_length);
	if(out_data == NULL)
	{
		// printf(" malloc failed\n" );
		return -ENOMEM;
	}
	i += ni_usb_bulk_register_write_header(&out_data[i], num_writes);
	for(j = 0; j < num_writes; j++)
	{
		i += ni_usb_bulk_register_write(&out_data[i], writes[j].device, writes[j].address, writes[j].value);
	}
	while(i % 4)
		out_data[i++] = 0x00;
	i += ni_usb_bulk_termination(&out_data[i]);
	if(i > out_data_length)
	{
		 //printf("%s: bug! buffer overrun\n", __FUNCTION__);
	}

	int stat = 0;
	stat = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkSnd, out_data, i, &bytes_written, USB_SHORT_XFER_OK, 100);// USB_DEFAULT_TIMEOUT);
	retval = stat;
	//EROP("1 ni_usb_write_registers UsbBulkXfer stat = %d\n", stat, 0, 0, 0);
//	retval = ni_usb_send_bulk_msg(ni_priv, out_data, i, &bytes_written, 1000);
	free(out_data);
	if (stat != USB_ERR_NORMAL_COMPLETION)
	{
		
		// printf("%s: %s: ni_usb_send_bulk_msg returned %i, bytes_written=%i, i=%i\n", __FILE__, __FUNCTION__,
		//retval, bytes_written, i);
		return retval;
	}

	in_data = malloc(in_data_length);
	if(in_data == NULL)
	{

		// printf("%s: malloc failed\n", __FILE__);
		return -ENOMEM;
	}

	stat = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkRcv, in_data, in_data_length, &bytes_read, USB_SHORT_XFER_OK, 500);
	//EROP("2 ni_usb_write_registers UsbBulkRcv stat = %d, num yte:%d\n", stat, bytes_read, 0, 0);
	retval = stat;
	//	retval = ni_usb_receive_bulk_msg(ni_priv, in_data, in_data_length, &bytes_read, 1000, 0);
	if((bytes_read != 16)|| (stat != USB_ERR_NORMAL_COMPLETION))
	{
		
		//printf("%s (bytes_read != 16\n) ", __FUNCTION__);
		// printf("%s: %s: ni_usb_receive_bulk_msg returned %i, bytes_read=%i\n", __FILE__, __FUNCTION__, retval, bytes_read);
		ni_usb_dump_raw_block(in_data, bytes_read);
		free(in_data);
		return retval;
	}

	
	ni_usb_parse_reg_write_status_block(in_data, &status, &reg_writes_completed);
	//FIXME parse extra 09 status bits and termination
	free(in_data);

	if(status.id != NIUSB_REG_WRITE_ID)
	{
		//printf("NIUSB_REG_WRITE_ID\n");
		// printf("%s: %s: parse error, id=0x%x != NIUSB_REG_WRITE_ID\n", __FILE__, __FUNCTION__, status.id);
		return -EIO;
	}
	if(status.error_code)
	{
		//printf("error_code\n");
		// printf("%s: %s: nonzero error code 0x%x\n", __FILE__, __FUNCTION__, status.error_code);
		return -EIO;
	}
	if(reg_writes_completed != num_writes)
	{
		//printf("num_writes\n");
		// printf("%s: %s: reg_writes_completed=%i, num_writes=%i\n", __FILE__, __FUNCTION__,
		//	reg_writes_completed, num_writes);
		return -EIO;
	}
	if(ibsta) *ibsta = status.ibsta;
	//printf(" SUCCESS  %s\n", __FUNCTION__);

	return SUCCESS;
}

// interface functions
int ni_usb_read(usb_data* usbdata, uint8_t *buffer, size_t length, int *end, size_t *bytes_read, uint32_t timeOut)
{
	int retval, parse_retval;

	uint8_t *out_data, *in_data;
	static const int out_data_length = 0x20;
	int in_data_length;
	int usb_bytes_written = 0;
	int usb_bytes_read = 0;
	int i = 0;
	int complement_count;
	int actual_length;
	struct ni_usb_status_block status;
	static const int max_read_length = 0xffff;
	struct ni_usb_register reg;

	*bytes_read = 0;
	if(length > max_read_length)
	{
		length = max_read_length;
		// printf("%s: read length too long\n", __FILE__);
	}
	out_data = malloc(out_data_length);
	if(out_data == NULL) return -ENOMEM;
	out_data[i++] = 0x0a; //
	out_data[i++] = usbdata->eos_mode >> 8;
	out_data[i++] = usbdata->eos_char;
	out_data[i++] = ni_usb_timeout_code(timeOut *10);
	complement_count = length - 1;
	complement_count = ~complement_count;
	out_data[i++] = complement_count & 0xff;
	out_data[i++] = (complement_count >> 8) & 0xff;
	out_data[i++] = 0x0;
	out_data[i++] = 0x0;
	i += ni_usb_bulk_register_write_header(&out_data[i], 2);
	reg.device = NIUSB_SUBDEV_TNT4882;
	reg.address = nec7210_to_tnt4882_offset(AUXMR);
	reg.value = AUX_HLDI;
	i += ni_usb_bulk_register_write(&out_data[i], reg.device, reg.address, reg.value);
	reg.value = AUX_CLEAR_END;
	i += ni_usb_bulk_register_write(&out_data[i], reg.device, reg.address, reg.value);
	while(i % 4)	// pad with zeros to 4-byte boundary
		out_data[i++] = 0x0;
	i += ni_usb_bulk_termination(&out_data[i]);

	//EROP("Check for Receive we send ", 0, 0, 0, 0);

	//for (int k = 0; k < i; k++)
	//{
	//	if (k % 16 == 0) printf("\n");
	//	printf("%02x ", out_data[k]);
	//}
	//printf("\n");
	
		int32_t stat;
	stat = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkSnd, out_data, i, &usb_bytes_written, USB_SHORT_XFER_OK, timeOut);
	//EROP("_pipeBlkSnd :  =%d (%s)\n", stat, UsbGetErrorString(stat), 0, 0);
	retval = stat;
	//	retval = ni_usb_send_bulk_msg(pNet, out_data, i, &usb_bytes_written, 1000);
	free(out_data);
	if (( usb_bytes_written != i) || (stat != USB_ERR_NORMAL_COMPLETION))
	{
		if(usb_bytes_written == 0) retval = -EIO;
		// printf("%s: %s: ni_usb_send_bulk_msg returned %i, usb_bytes_written=%i, i=%i\n", __FILE__, __FUNCTION__, retval, usb_bytes_written, i);
		//EROP("usb_bytes_written != i usb_bytes_written  =%d , i=(%d)\n", usb_bytes_written, i, 0, 0);

		return retval;
	}

	in_data_length = (length / 30 + 1) * 0x20 + 0x20;
	in_data = malloc(in_data_length);
	if(in_data == NULL)
	{

		return -ENOMEM;
	}

	stat=	UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkRcv, in_data, in_data_length, &usb_bytes_read, USB_SHORT_XFER_OK, timeOut);
	retval = stat;
	//EROP("_pipeBlkRcv : retval =%d (%s), usb_bytes_read: %d, len to read :%d\n", retval, UsbGetErrorString(retval), usb_bytes_read, in_data_length);
	
	//for (int k = 0; k < usb_bytes_read; k++)
	//{
	//	if (k % 16 == 0) printf("\n");
	//	printf("%02x ", in_data[k]);
	//}
	//printf("\n");
//	retval = ni_usb_receive_bulk_msg(pNet, in_data, in_data_length, &usb_bytes_read,
		//ni_usb_timeout_msecs(usbdata->usec_timeout), 1);


	if(stat != USB_ERR_NORMAL_COMPLETION)
	{
		//EROP("Nothing received error code\n", retval, UsbGetErrorString(retval), 0, 0);

		// printf(" %s: ni_usb_receive_bulk_msg returned %i, usb_bytes_read=%i\n",
		//	 __FUNCTION__, retval, usb_bytes_read);
		//free(in_data);
		return retval;
	}
	//EROP("parse_retval\n", 0, 0, 0, 0);
	parse_retval = parse_board_ibrd_readback(in_data, &status, buffer, length, &actual_length);
	if(parse_retval != usb_bytes_read)
	{
		//EROP(" error decode lenth parse_retval != usb_bytes_read  =(%d) (%d), str:= %s\n", parse_retval, usb_bytes_read, buffer, 0);

		ni_usb_dump_raw_block(in_data, usb_bytes_read);//test
		if(parse_retval >= 0) parse_retval = -EIO;
		// printf("%s: %s: retval=%i usb_bytes_read=%i\n", __FILE__, __FUNCTION__, parse_retval, usb_bytes_read);
		free(in_data);
		return parse_retval;
	}
	free(in_data);
	if(actual_length != length - status.count)
	{
		//EROP(" Dump datas\n", 00, 0, 0, 0);

		//ni_usb_dump_raw_block(in_data, usb_bytes_read);
	}


	switch(status.error_code)
	{
	case NIUSB_NO_ERROR:
		retval = 0;
		break;
	case NIUSB_ABORTED_ERROR:
		/*this is expected if ni_usb_receive_bulk_msg got interrupted by a signal
			and returned -ERESTARTSYS */
		break;
	case NIUSB_ADDRESSING_ERROR:
		retval = -EIO;
		break;
	case NIUSB_TIMEOUT_ERROR:
		retval = -ETIMEDOUT;
		break;
	case NIUSB_EOSMODE_ERROR:
		// printf("%s: %s: driver bug, we should have been able to avoid NIUSB_EOSMODE_ERROR.\n", __FILE__, __FUNCTION__);
		retval = -EINVAL;
		break;
	default:
		// printf("%s: %s: unknown error code=%i\n", __FILE__, __FUNCTION__, status.error_code);
		retval = -EIO;
		break;
	}
	//EROP("end\n", 0, 0, 0, 0);
	ni_usb_soft_update_status(usbdata, status.ibsta, 0);
	if(status.ibsta & END) *end = 1; //need mod
	else *end = 0;
	*bytes_read = actual_length;
	//EROP("end\n", 0, 0, 0, 0);
	return retval;
}

int ni_usb_write(usb_data* usbdata, uint8_t *buffer, size_t length, int send_eoi, size_t *bytes_written, uint32_t timeOut)
	{
	
	//EROP("%s \n", __FUNCTION__, 0, 0, 9);
	int retval;

	uint8_t *out_data, *in_data;
	int out_data_length;
	static const int in_data_length = 0x10;
	int usb_bytes_written = 0, usb_bytes_read = 0;
	int i = 0, j;
	int complement_count;
	struct ni_usb_status_block status;
	static const int max_write_length = 0xffff;

	*bytes_written = 0;
	if(length > max_write_length)
	{
		length = max_write_length;
		send_eoi = 0;

	}
	out_data_length = length + 0x10;
	out_data = malloc(out_data_length);
	if(out_data == NULL) return -ENOMEM;
	out_data[i++] = 0x0d;//1
	complement_count = length;
	complement_count = length - 1;
	complement_count = ~complement_count;
	out_data[i++] = complement_count & 0xff;	//2
	out_data[i++] = (complement_count >> 8) & 0xff; //3
	out_data[i++] = ni_usb_timeout_code(timeOut*10);// usbdata->usec_timeout);//4
	out_data[i++] = 0x00;//5 08 was 00
	out_data[i++] = 0x00;//6 0a was 00


	if(send_eoi)
		out_data[i++] = 0x8;//7
	else
		out_data[i++] = 0x0;//7

	out_data[i++] = 0x0;//8

	for(j = 0; j < length; j++)
		out_data[i++] = buffer[j];//9+ num of char last will be 0x0A \n
	while(i % 4)	// pad with zeros to 4-byte boundary
		out_data[i++] = 0x0;

	i += ni_usb_bulk_termination(&out_data[i]);

	//for (int k = 0; k < i; k++)
	//{
	//	if (k % 16 == 0) printf("\n");
	//	printf("%02x ", out_data[k]);
	//}
	//printf("\n");
	//printf("\n");

	//EROP("start snd actlen = %d, snd: %s,  pipe: %x\n", i, buffer, usbdata->_pipeBlkSnd, 0);

	retval = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkSnd, out_data, i, &usb_bytes_written, USB_SHORT_XFER_OK, timeOut);

	//EROP(" snd actlen = %d, status = %d , usb_bytes_written:%d, i:%d \n", usb_bytes_written, retval,  usb_bytes_written,i);

			if (retval != USB_ERR_NORMAL_COMPLETION)
			{
				//EROP("UsbBulkXfer: fails retval=%d (%s)\n", retval, UsbGetErrorString(retval), 0, 0);

			}

	
	//	retval = ni_usb_send_bulk_msg(pNet, out_data, i, &usb_bytes_written,
//		ni_usb_timeout_msecs(usbdata->usec_timeout));
	free(out_data);
	if(usb_bytes_written != i)
	{
		//printf("Nothing \n");
		return retval;
	}

	in_data = malloc(in_data_length);
	if(in_data == NULL) return -ENOMEM;
//	retval = ni_usb_receive_bulk_msg(pNet, in_data, in_data_length, &usb_bytes_read,
		//ni_usb_timeout_msecs(usbdata->usec_timeout), 1);
	
	retval = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkRcv, in_data, in_data_length, &usb_bytes_read, USB_SHORT_XFER_OK, timeOut);
	//EROP("Rcv actlen = %d, status = %d , snd: %s\n", usb_bytes_read, retval, in_data, 0);

	//for (int k = 0; k < usb_bytes_read; k++)
	//{
	//	if (k % 16 == 0) printf("\n");
	//	printf("%02x ", in_data[k]);
	//}
	//printf("\n");

	if(usb_bytes_read != 12)
	{
		// printf("%s: %s: ni_usb_receive_bulk_msg returned %i, usb_bytes_read=%i\n", __FILE__, __FUNCTION__, retval, usb_bytes_read);
		free(in_data);
		return retval;
	}
	ni_usb_parse_status_block(in_data, &status);
	free(in_data);
	switch(status.error_code)
	{
	case NIUSB_NO_ERROR:
		retval = 0;
		break;
	case NIUSB_ABORTED_ERROR:
		/*this is expected if ni_usb_receive_bulk_msg got interrupted by a signal
			and returned -ERESTARTSYS */
		break;
	case NIUSB_ADDRESSING_ERROR:
		retval = -ENXIO;
		break;
	case NIUSB_NO_LISTENER_ERROR:
		retval = -EIO;
		break;
	case NIUSB_TIMEOUT_ERROR:
		retval = -ETIMEDOUT;
		break;
	default:
		// printf("%s: %s: unknown error code=%i\n", __FILE__, __FUNCTION__, status.error_code);
		retval = -EPIPE;
		break;
	}
	ni_usb_soft_update_status(usbdata, status.ibsta, 0);
	*bytes_written = length - status.count;
	return retval;
}

int ni_usb_command_chunk(usb_data* usbdata, uint8_t *buffer, size_t length, size_t *command_bytes_written, uint32_t timeOut)
{
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
		int retval;

	uint8_t *out_data, *in_data;
	int out_data_length;
	static const int in_data_length =0x10;
	int bytes_written = 0, bytes_read = 0;
	int i = 0, j;
	unsigned complement_count;
	struct ni_usb_status_block status;
	// usb-b gives error 4 if you try to send more than 16 command bytes at once
	static const int max_command_length = 0x10;

	*command_bytes_written = 0;
	if(length > max_command_length) length = max_command_length;
	out_data_length = length + 0x10;
	out_data = malloc(out_data_length);
	if(out_data == NULL) return -ENOMEM;
	out_data[i++] = 0x0c;
	complement_count = length - 1;
	complement_count = ~complement_count;
	out_data[i++] = complement_count;
	out_data[i++] = 0x0;
	out_data[i++] = ni_usb_timeout_code(usbdata->usec_timeout);
	for(j = 0; j < length; j++)
		out_data[i++] = buffer[j];
	while(i % 4)	// pad with zeros to 4-byte boundary
		out_data[i++] = 0x0;
	i += ni_usb_bulk_termination(&out_data[i]);

	//EROP("start snd actlen = %d, snd: %s\n", i, out_data, 0, 0);

	retval = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkSnd, out_data, i, &bytes_written, USB_SHORT_XFER_OK, timeOut);
	//EROP("snd actlen = %d, status = %d , snd: %s\n", bytes_written,  retval , out_data, 0);

//	retval = ni_usb_send_bulk_msg(pNet, out_data, i, &bytes_written,
		//ni_usb_timeout_msecs(usbdata->usec_timeout));
	free(out_data);
	if(retval || bytes_written != i)
	{
		int k;
		// printf("%s: %s: ni_usb_send_bulk_msg returned %i, bytes_written=%i, i=%i\n", __FILE__, __FUNCTION__, retval, bytes_written, i);
		// printf("\t was attempting to write command bytes:\n");
		for(k = 0; k < length; ++k)
		{
			// printf(" 0x%2x", buffer[k]);
		}
		// printf("\n");
		return retval;
	}

	in_data = malloc(in_data_length);
	if(in_data == NULL) return -ENOMEM;
//	retval = ni_usb_receive_bulk_msg(pNet, in_data, in_data_length, &bytes_read,
		//ni_usb_timeout_msecs(usbdata->usec_timeout), 1);
	retval = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkRcv, in_data, in_data_length, &bytes_read, USB_SHORT_XFER_OK, timeOut);
	//EROP("snd actlen = %d, status = %d , snd: %s\n", bytes_read, retval, in_data, 0);
	if((retval && retval != -ERESTARTSYS) || bytes_read != 12)
	{
		// printf("%s: %s: ni_usb_receive_bulk_msg returned %i, bytes_read=%i\n", __FILE__, __FUNCTION__, retval, bytes_read);
		free(in_data);
		return retval;
	}
	ni_usb_parse_status_block(in_data, &status);
	free(in_data);
	*command_bytes_written = length - status.count;
	switch(status.error_code)
	{
	case NIUSB_NO_ERROR:
		break;
	case NIUSB_ABORTED_ERROR:
		/*this is expected if ni_usb_receive_bulk_msg got interrupted by a signal
			and returned -ERESTARTSYS */
		break;
	case NIUSB_NO_BUS_ERROR:
		return -EIO;
		break;
	case NIUSB_EOSMODE_ERROR:
		// printf("%s: %s: got eosmode error.  Driver bug?\n", __FILE__, __FUNCTION__);
		return -EIO;
		break;
	default:
		// printf("%s: %s: unknown error code=%i\n", __FILE__, __FUNCTION__, status.error_code);
		return -EIO;
		break;
	}
	ni_usb_soft_update_status(usbdata, status.ibsta, 0);
	return 0;
}

int ni_usb_command(usb_data* usbdata, uint8_t *buffer, size_t length, size_t *bytes_written, uint32_t timeOut)
{
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;

	size_t count;
	int retval;

	*bytes_written = 0;
	while(*bytes_written < length)
	{
		retval = ni_usb_command_chunk(usbdata, buffer + *bytes_written, length - *bytes_written, &count,  timeOut);
		*bytes_written += count;
		if(retval < 0) return retval;
	}
	return 0;
}

int ni_usb_take_control(usb_data* usbdata, int synchronous, uint32_t timeOut)
{
	//EROP("In %s\n", __FUNCTION__, 0, 0, 0);
	int retval;
	// usb_data* usbdata = (usb_data*)pNet->USB_Data;
	uint8_t *out_data, *in_data;
	static const int out_data_length = 0x10;
	static const int  in_data_length = 0x10;
	int bytes_written = 0, bytes_read = 0;
	int i = 0;
	struct ni_usb_status_block status;

	out_data = malloc(out_data_length);
	if(out_data == NULL)
	{
		// printf("%s: malloc failed\n", __FILE__);
		return -ENOMEM;
	}
	out_data[i++] = NIUSB_IBCAC_ID;
	if(synchronous)
		out_data[i++] = 0x1;
	else
		out_data[i++] = 0x0;
	out_data[i++] = 0x0;
	out_data[i++] = 0x0;
	i += ni_usb_bulk_termination(&out_data[i]);
	
	int32_t stat;
	stat = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkSnd, out_data, i, &bytes_written, USB_SHORT_XFER_OK, timeOut);
	//retval = ni_usb_send_bulk_msg(ni_priv, out_data, i, &bytes_written, 1000);
	free(out_data);
	if((bytes_written != i) || (stat != USB_ERR_NORMAL_COMPLETION))
	{
		// printf(" ni_usb_take_control returned %i, bytes_written=%i, i=%i\n",   retval, bytes_written, i);
		 return  -ETIMEDOUT;
	}

	in_data = malloc(in_data_length);
	if(in_data == NULL)
	{
		// printf("%s: malloc failed\n", __FILE__);
		return -ENOMEM;
	}
	retval = 0;

	stat=UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkRcv, in_data, in_data_length, &bytes_read, USB_SHORT_XFER_OK, timeOut);
	
	retval = stat;
	//retval = ni_usb_receive_bulk_msg(ni_priv, in_data, in_data_length, &bytes_read, 1000, 1);
	//if((retval && retval != -ERESTARTSYS) || bytes_read != 12)
	if (( bytes_read != 12)||(stat != USB_ERR_NORMAL_COMPLETION))
	{
		if(bytes_read == 0) retval = -EIO;
		// printf(" 2ni_usb_take_control returned %i, bytes_read=%i\n",  retval, bytes_read);
		free(in_data);
		return retval;
	}
	ni_usb_parse_status_block(in_data, &status);
	free(in_data);
	ni_usb_soft_update_status(usbdata, status.ibsta, 0);
	return retval;
}

int ni_usb_go_to_standby(usb_data* usbdata)
{
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	//printf("%s: malloc failed\n", __FUNCTION__);
	int retval;

	uint8_t *out_data, *in_data;
	static const int out_data_length = 0x10;
	static const int  in_data_length = 0x20;
	int bytes_written = 0, bytes_read = 0;
	int i = 0;
	struct ni_usb_status_block status;

	out_data = malloc(out_data_length);
	if(out_data == NULL)
	{
		// printf("%s: malloc failed\n", __FILE__);
		return -ENOMEM;
	}
	out_data[i++] = NIUSB_IBGTS_ID;
	out_data[i++] = 0x0;
	out_data[i++] = 0x0;
	out_data[i++] = 0x0;
	i += ni_usb_bulk_termination(&out_data[i]);


		
		int32_t stat;
		stat = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkSnd, out_data, i, &bytes_written, USB_SHORT_XFER_OK, 100);// USB_DEFAULT_TIMEOUT);
	retval = stat;
//	retval = ni_usb_send_bulk_msg(pNet, out_data, i, &bytes_written, 1000);
	free(out_data);
	if(( bytes_written != i) || (stat != USB_ERR_NORMAL_COMPLETION))
	{
		// printf("%s: %s: ni_usb_send_bulk_msg returned %i, bytes_written=%i, i=%i\n", __FILE__, __FUNCTION__, retval, bytes_written, i);
		return retval;
	}

	in_data = malloc(in_data_length);
	if(in_data == NULL)
	{
		// printf("%s: malloc failed\n", __FILE__);
		return -ENOMEM;
	}
	stat = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkRcv, in_data, in_data_length, &bytes_read, USB_SHORT_XFER_OK, 100);// USB_DEFAULT_TIMEOUT);
	retval = stat;
//	retval = ni_usb_receive_bulk_msg(pNet, in_data, in_data_length, &bytes_read, 1000, 0);
	if( (bytes_read != 12) || (stat != USB_ERR_NORMAL_COMPLETION))
	{
		// printf("%s: %s: ni_usb_receive_bulk_msg returned %i, bytes_read=%i\n", __FILE__, __FUNCTION__, retval, bytes_read);
		free(in_data);
		return retval;
	}
	ni_usb_parse_status_block(in_data, &status);
	free(in_data);
	if(status.id != NIUSB_IBGTS_ID)
	{
		 EROP("%s: %s: bug: status.id 0x%x != INUSB_IBGTS_ID\n", __FILE__, __FUNCTION__, status.id,0);
	}
	ni_usb_soft_update_status(usbdata, status.ibsta, 0);
	return 0;
}

void ni_usb_request_system_control( usb_data* usbdata , int request_control )
{
	int retval;
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	int i = 0;
	struct ni_usb_register writes[4];
	unsigned int ibsta;
	//printf("%s: \n", __FUNCTION__);

	if(request_control)
	{
		writes[i].device = NIUSB_SUBDEV_TNT4882;
		writes[i].address = CMDR;
		writes[i].value = SETSC;
		i++;
		writes[i].device = NIUSB_SUBDEV_TNT4882;
		writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
		writes[i].value = AUX_CIFC;
		i++;
	}else
	{
		writes[i].device = NIUSB_SUBDEV_TNT4882;
		writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
		writes[i].value = AUX_CREN;
		i++;
		writes[i].device = NIUSB_SUBDEV_TNT4882;
		writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
		writes[i].value = AUX_CIFC;
		i++;
		writes[i].device = NIUSB_SUBDEV_TNT4882;
		writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
		writes[i].value = AUX_DSC;
		i++;
		writes[i].device = NIUSB_SUBDEV_TNT4882;
		writes[i].address = CMDR;
		writes[i].value = CLRSC;
		i++;
	}

	retval = ni_usb_write_registers(usbdata, writes, i, &ibsta);
	if(retval < 0)
	{
		// printf("%s: register write failed, retval=%i\n", __FUNCTION__, retval);
		return retval;
	}
	usbdata->online = 1;
	ni_usb_soft_update_status(usbdata, ibsta, 0);
	return; // 0;
}

//FIXME maybe the interface should have a "pulse interface clear" function that can return an error?
void ni_usb_interface_clear(usb_data* usbdata, int assert)
{
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;

	int retval;

	uint8_t *out_data, *in_data;
	static const int out_data_length = 0x10;
	static const int  in_data_length = 0x10;
	int bytes_written = 0, bytes_read = 0;
	int i = 0;
	struct ni_usb_status_block status;

	// FIXME: we are going to pulse when assert is true, and ignore otherwise
	if(assert == 0) return;
	out_data = malloc(out_data_length);
	if(out_data == NULL)
	{
		// printf("%s: malloc failed\n", __FILE__);
		return;
	}
	out_data[i++] = NIUSB_IBSIC_ID;
	out_data[i++] = 0x0;
	out_data[i++] = 0x0;
	out_data[i++] = 0x0;
	i += ni_usb_bulk_termination(&out_data[i]);

	retval = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkSnd, out_data, i, &bytes_written, USB_SHORT_XFER_OK,  USB_DEFAULT_TIMEOUT);// this ok

//	retval = ni_usb_send_bulk_msg(pNet, out_data, i, &bytes_written, 1000);
	free(out_data);
	if (bytes_written != i)
	{
		// printf("%s: %s: ni_usb_send_bulk_msg returned %i, bytes_written=%i, i=%i\n", __FILE__, __FUNCTION__, retval, bytes_written, i);
		return;
	}
	in_data = malloc(in_data_length);
	if(in_data == NULL)
	{
		// printf("%s: malloc failed\n", __FILE__);
		return;
	}
//	retval = ni_usb_receive_bulk_msg(pNet, in_data, in_data_length, &bytes_read, 1000, 0);
	retval = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkRcv, in_data, in_data_length, &bytes_read, USB_SHORT_XFER_OK, USB_DEFAULT_TIMEOUT); //this ok

	if( bytes_read != 12)
	{
		// printf("%s: %s: ni_usb_receive_bulk_msg returned %i, bytes_read=%i\n", __FILE__, __FUNCTION__, retval, bytes_read);
		free(in_data);
		return;
	}
	ni_usb_parse_status_block(in_data, &status);
	free(in_data);
	ni_usb_soft_update_status(usbdata, status.ibsta, 0);
	return;
}

void ni_usb_remote_enable(usb_data* usbdata, int enable)
{
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	int retval;

	struct ni_usb_register reg;
	unsigned int ibsta;

	reg.device = NIUSB_SUBDEV_TNT4882;
	reg.address = nec7210_to_tnt4882_offset(AUXMR);
	if(enable)
	{
		reg.value = AUX_SREN;
	}else
	{
		reg.value = AUX_CREN;
	}
	retval = ni_usb_write_registers(usbdata, &reg, 1, &ibsta);
	if(retval < 0)
	{
		// printf("%s: %s: register write failed, retval=%i\n", __FILE__, __FUNCTION__, retval);
		return; //retval;
	}
	ni_usb_soft_update_status(usbdata, ibsta, 0);
	return;// 0;
}

int ni_usb_enable_eos(usb_data* usbdata, uint8_t eos_byte, int compare_8_bits)
{
	

	usbdata->eos_char = eos_byte;
	usbdata->eos_mode |= REOS;
	if(compare_8_bits)
		usbdata->eos_mode |= BIN;
	else
		usbdata->eos_mode &= ~BIN;
	return 0;
}

void ni_usb_disable_eos(usb_data* usbdata)
{
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	/* adapter gets unhappy if you don't zero all the bits
		for the eos mode and eos char (returns error 4 on reads). */
	usbdata->eos_mode = 0;
	usbdata->eos_char = 0;
}

unsigned int ni_usb_update_status(usb_data* usbdata, unsigned int clear_mask )
{
	//EROP("In %s\n", __FUNCTION__, 0, 0, 0);
	int retval=0;

	static const int bufferLength = 8;
	uint8_t *buffer;
	struct ni_usb_status_block status;

	//// printf("%s: receive control pipe is %i\n", __FILE__, pipe);
	buffer = malloc(bufferLength);
	if(buffer == NULL)
	{
	//	printf("%s: malloc failed!\n", __FILE__);
		return usbdata->ibsta;
	}
	
	int32_t stat;
	stat =	UsbControlXfer(usbdata->udev, NI_USB_WAIT_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE, 0x200, 0x0, buffer, bufferLength, &retval, USB_SHORT_XFER_OK, 5);
	//printf("NI_USB_WAIT_REQUEST bufferLength: 0x%x  , retval :0x%x\n", bufferLength, retval);
	
	//retval = ni_usb_receive_control_msg(pNet, NI_USB_WAIT_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
	//0x200, 0x0, buffer, bufferLength, 1000);
	//printf("usb_control_msg returned retval: %x, bufferLength: %x \n", retval, bufferLength);

	if(retval != bufferLength) 
	{
		//printf("===============--usb_control_msg returned retval: %x, bufferLength: %x\n", retval, bufferLength);
		//EROP("UsbBulkXfer: fails e=%x (%s)\n", stat, UsbGetErrorString(stat), 0, 0);
		free(buffer);
		return usbdata->ibsta;
	}
	ni_usb_parse_status_block(buffer, &status);
	free(buffer);
	ni_usb_soft_update_status(usbdata, status.ibsta, clear_mask);
	
	usbdata->id = status.id;
	usbdata->ibsta = status.ibsta;
	usbdata->error_code = status.error_code;
	usbdata->count = status.count;
	//EROP("======== status.ibstat :%x, error :%x \n",  status.ibsta, status.error_code,0,0);
 	return status.ibsta;
}

// tells ni-usb to immediately stop an ongoing i/o operation
void ni_usb_stop(usb_data* usbdata)
{
	int retval;
	static const int bufferLength = 8;
	uint8_t *buffer;
	struct ni_usb_status_block status;

	//// printf("%s: receive control pipe is %i\n", __FILE__, pipe);
	buffer = malloc(bufferLength);
	if(buffer == NULL)
	{
		// printf("%s: malloc failed!\n", __FILE__);
		return;
	}

	 UsbControlXfer(usbdata->udev, NI_USB_STOP_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE, 0x0, 0x0, buffer, bufferLength, &retval, USB_MULTI_SHORT_OK, 100);

	if(retval != bufferLength)
	{
		// printf("%s: usb_control_msg returned %i\n", __FILE__, retval);
		free(buffer);
		return;
	}
	ni_usb_parse_status_block(buffer, &status);
	free(buffer);
}

int ni_usb_primary_address(usb_data* usbdata, unsigned int address)
{
	int retval;

	int i = 0;
	struct ni_usb_register writes[2];
	unsigned int ibsta;

	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(ADR);
	writes[i].value = address;
	i++;
	writes[i].device = NIUSB_SUBDEV_UNKNOWN2;
	writes[i].address = 0x0;
	writes[i].value = address;
	i++;
	//EROP("Set Address = %d\n", address, 0, 0, 0);
	retval = ni_usb_write_registers(usbdata, writes, i, &ibsta);
	if(retval < 0)
	{
		// printf("register write failed, retval=%i\n", retval);
		return retval;
	}
	ni_usb_soft_update_status(usbdata, ibsta, 0);
	//EROP("SUCCESS \n", 0, 0, 0, 0);
	return   SUCCESS;
}

int ni_usb_write_sad(struct ni_usb_register *writes, int address, int enable)
{
	unsigned int adr_bits, admr_bits;
	int i = 0;

	adr_bits = HR_ARS;
	admr_bits = HR_TRM0 | HR_TRM1;
	if(enable)
	{
		adr_bits |= address;
		admr_bits |= HR_ADM1;
	}else
	{
		adr_bits |= HR_DT | HR_DL;
		admr_bits |= HR_ADM0;
	}
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(ADR);
	writes[i].value = adr_bits;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(ADMR);
	writes[i].value = admr_bits;
	i++;
	writes[i].device = NIUSB_SUBDEV_UNKNOWN2;
	writes[i].address = 0x1;
	writes[i].value = enable ? MSA(address) : 0x0;
	i++;
	return i;
}

int ni_usb_secondary_address(usb_data* usbdata, unsigned int address, int enable)
{
	int retval;
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	int i = 0;
	struct ni_usb_register writes[3];
	unsigned int ibsta;

	i += ni_usb_write_sad(writes, address, enable);
	retval = ni_usb_write_registers(usbdata, writes, i, &ibsta);
	if(retval < 0)
	{
		// printf("%s: %s: register write failed, retval=%i\n", __FILE__, __FUNCTION__, retval);
		return retval;
	}
	ni_usb_soft_update_status(usbdata, ibsta, 0);
	return 0;
}
int ni_usb_parallel_poll(usb_data* usbdata, uint8_t *result)
{
	int retval;
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	uint8_t *out_data, *in_data;
	static const int out_data_length = 0x10;
	static const int  in_data_length = 0x20;
	int bytes_written = 0, bytes_read = 0;
	int i = 0;
	int j = 0;
	struct ni_usb_status_block status;

	out_data = malloc(out_data_length);
	if(out_data == NULL)
	{
		// printf("%s: malloc failed\n", __FILE__);
		return -ENOMEM;
	}
	out_data[i++] = NIUSB_IBRPP_ID;
	out_data[i++] = 0xf0;	//FIXME: this should be the parallel poll timeout code
	out_data[i++] = 0x0;
	out_data[i++] = 0x0;
	i += ni_usb_bulk_termination(&out_data[i]);

	retval = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkSnd, out_data, i, &bytes_written, USB_SHORT_XFER_OK, USB_DEFAULT_TIMEOUT);

//	retval = ni_usb_send_bulk_msg(pNet, out_data, i, &bytes_written, 1000 /*FIXME: should use parallel poll timeout (not supported yet)*/);
	free(out_data);
	if(retval || bytes_written != i)
	{
		// printf("%s: %s: ni_usb_send_bulk_msg returned %i, bytes_written=%i, i=%i\n", __FILE__, __FUNCTION__, retval, bytes_written, i);
		return retval;
	}
	in_data = malloc(in_data_length);
	if(in_data == NULL)
	{
		// printf("%s: malloc failed\n", __FILE__);
		return -ENOMEM;
	}
	retval = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkRcv, in_data, in_data_length, &bytes_read, USB_SHORT_XFER_OK, USB_DEFAULT_TIMEOUT);

//	retval = ni_usb_receive_bulk_msg(pNet, in_data, in_data_length,
		//&bytes_read, 1000 /*FIXME: should use parallel poll timeout (not supported yet)*/, 1);
	if(retval && retval != -ERESTARTSYS)
	{
		// printf("%s: %s: ni_usb_receive_bulk_msg returned %i, bytes_read=%i\n", __FILE__, __FUNCTION__, retval, bytes_read);
		free(in_data);
		return retval;
	}
	j += ni_usb_parse_status_block(in_data, &status);
	*result = in_data[j++];
	free(in_data);
	ni_usb_soft_update_status(usbdata, status.ibsta, 0);
	return retval;
}
void ni_usb_parallel_poll_configure(usb_data* usbdata, uint8_t config)
{
	int retval;
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	int i = 0;
	struct ni_usb_register writes[1];
	unsigned int ibsta;

	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	writes[i].value = PPR | config;
	i++;
	retval = ni_usb_write_registers(usbdata, writes, i, &ibsta);
	if(retval < 0)
	{
		// printf("%s: %s: register write failed, retval=%i\n", __FILE__, __FUNCTION__, retval);
		return;// retval;
	}
	ni_usb_soft_update_status(usbdata, ibsta, 0);
	return ;
}
void ni_usb_parallel_poll_response(usb_data* usbdata, int ist)
{
	int retval;
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	int i = 0;
	struct ni_usb_register writes[1];
	unsigned int ibsta;

	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	if(ist)
		writes[i].value = AUX_SPPF;
	else
		writes[i].value = AUX_CPPF;
	i++;
	retval = ni_usb_write_registers(usbdata, writes, i, &ibsta);
	if(retval < 0)
	{
		// printf("%s: %s: register write failed, retval=%i\n", __FILE__, __FUNCTION__, retval);
		return retval;
	}
	ni_usb_soft_update_status(usbdata, ibsta, 0);
	return ;
}
void ni_usb_serial_poll_response(usb_data* usbdata, uint8_t status)
{
	int retval;
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	int i = 0;
	struct ni_usb_register writes[1];
	unsigned int ibsta;

	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(SPMR);
	writes[i].value = status;
	i++;
	retval = ni_usb_write_registers(usbdata, writes, i, &ibsta);
	if(retval < 0)
	{
		// printf("%s: %s: register write failed, retval=%i\n", __FILE__, __FUNCTION__, retval);
		return retval;
	}
	ni_usb_soft_update_status(usbdata, ibsta, 0);
	return;
}
uint8_t ni_usb_serial_poll_status(usb_data* usbdata)
{
	return 0;
}
void ni_usb_return_to_local(usb_data* usbdata)
{
	int retval;
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	int i = 0;
	struct ni_usb_register writes[1];
	unsigned int ibsta;

	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	writes[i].value = AUX_RTL;
	i++;
	retval = ni_usb_write_registers(usbdata, writes, i, &ibsta);
	if(retval < 0)
	{
		// printf("%s: %s: register write failed, retval=%i\n", __FILE__, __FUNCTION__, retval);
		return retval;
	}
	ni_usb_soft_update_status(usbdata, ibsta, 0);
	return ;
}

int ni_usb_line_status( const usb_data* usbdata)
{
	int retval;
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	uint8_t *out_data, *in_data;
	static const int out_data_length = 0x20;
	static const int  in_data_length = 0x20;
	int bytes_written = 0, bytes_read = 0;
	int i = 0;
	unsigned int bsr_bits;
	int line_status = ValidALL;
	// NI windows driver reads 0xd(HSSEL), 0xc (ARD0), 0x1f (BSR)

	out_data = malloc(out_data_length);
	if(out_data == NULL)
	{
		// printf("%s: malloc failed\n", __FILE__);
		return -ENOMEM;
	}

	//retval = mutex_trylock(&ni_priv->addressed_transfer_lock);  /* line status gets called during ibwait */

	//if(retval == 0)
	//{
 //       free(out_data);
	//	return -EBUSY;
	//}
	i += ni_usb_bulk_register_read_header(&out_data[i], 1);
	i += ni_usb_bulk_register_read(&out_data[i], NIUSB_SUBDEV_TNT4882, BSR);
	while(i % 4)
		out_data[i++] = 0x0;
	i += ni_usb_bulk_termination(&out_data[i]);

	retval = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkSnd, out_data, i, &bytes_written, USB_SHORT_XFER_OK, 500);// USB_DEFAULT_TIMEOUT);
	//retval = ni_usb_nonblocking_send_bulk_msg(pNet, out_data, i, &bytes_written, 1000);
	free(out_data);
	if(retval || bytes_written != i)
	{

		if(retval != -EAGAIN)
			// printf("%s: %s: ni_usb_send_bulk_msg returned %i, bytes_written=%i, i=%i\n", __FILE__, __FUNCTION__, retval, bytes_written, i);
		return retval;
	}

	in_data = malloc(in_data_length);
	if(in_data == NULL)
	{
		// printf("%s: malloc failed\n", __FILE__);
		return -ENOMEM;
	}

	retval = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkRcv, in_data, in_data_length, &bytes_read, USB_SHORT_XFER_OK, 100);// USB_DEFAULT_TIMEOUT);;

//	retval = ni_usb_nonblocking_receive_bulk_msg(pNet, in_data, in_data_length, &bytes_read, 1000, 0);
	if(retval)
	{
		if(retval != -EAGAIN)
			// printf("%s: %s: ni_usb_receive_bulk_msg returned %i, bytes_read=%i\n", __FILE__, __FUNCTION__, retval, bytes_read);
		free(in_data);
		return retval;
	}


	ni_usb_parse_register_read_block(in_data, &bsr_bits, 1);
	free(in_data);
	if(bsr_bits & BCSR_REN_BIT)
		line_status |= BusREN;
	if(bsr_bits & BCSR_IFC_BIT)
		line_status |= BusIFC;
	if(bsr_bits & BCSR_SRQ_BIT)
		line_status |= BusSRQ;
	if(bsr_bits & BCSR_EOI_BIT)
		line_status |= BusEOI;
	if(bsr_bits & BCSR_NRFD_BIT)
		line_status |= BusNRFD;
	if(bsr_bits & BCSR_NDAC_BIT)
		line_status |= BusNDAC;
	if(bsr_bits & BCSR_DAV_BIT)
		line_status |= BusDAV;
	if(bsr_bits & BCSR_ATN_BIT)
		line_status |= BusATN;
	return line_status;
}
unsigned int ni_usb_t1_delay(usb_data* usbdata, unsigned int nano_sec )
{
	int retval;
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;
	int i = 0;
	struct ni_usb_register writes[3];
	unsigned int ibsta;
	unsigned int actual_ns = 2000;

	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	if(nano_sec <= 1100)
	{
		writes[i].value = AUXRI | USTD | SISB;
		actual_ns = 1100;
	}else
		writes[i].value = AUXRI | SISB;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	if(nano_sec <= 500)
	{
		writes[i].value = AUXRB | HR_TRI;
		actual_ns = 500;
	}else
		writes[i].value = AUXRB;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = KEYREG;
	if(nano_sec <= 350)
	{
		writes[i].value = MSTD;
		actual_ns = 350;
	}else
		writes[i].value = 0x0;
	i++;
	retval = ni_usb_write_registers(usbdata, writes, i, &ibsta);
	if(retval < 0)
	{
		// printf("%s: %s: register write failed, retval=%i\n", __FILE__, __FUNCTION__, retval);
		return -1;	//FIXME should change return type to int for error reporting
	}
	ni_usb_soft_update_status(usbdata, ibsta, 0);
	return actual_ns;
}

//static int ni_usb_allocate_private(LPDRIVER_INST pNet )
//{
//
//	board->private_data = malloc(sizeof(ni_usb_private_t), GFP_KERNEL);
//	if(board->private_data == NULL)
//		return -ENOMEM;
//	ni_priv = board->private_data;
//	memset(ni_priv, 0, sizeof(ni_usb_private_t));
//	mutex_init(&ni_priv->bulk_transfer_lock);
//	mutex_init(&ni_priv->control_transfer_lock);
//	mutex_init(&ni_priv->interrupt_transfer_lock);
//	mutex_init(&ni_priv->addressed_transfer_lock);
//	return 0;
//}
//
//static void ni_usb_free_private(ni_usb_private_t *ni_priv)
//{
//	if(ni_priv->interrupt_urb)
//		usb_free_urb(ni_priv->interrupt_urb);
//	free(ni_priv);
//	return;
//}
//

 int ni_usb_init( usb_data* usbdata)
{
	int retval;
	//ni_usb_private_t *ni_priv = board->private_data;
	int i = 0;
	struct ni_usb_register *writes;
	unsigned int ibsta;
	static const int writes_length = 24;

	writes = malloc(sizeof(struct ni_usb_register) * writes_length);
	if(writes == NULL)
	{
		// printf("%s: %s: malloc failed\n", __FILE__, __FUNCTION__);
		return -ENOMEM;
	}
	writes[i].device = NIUSB_SUBDEV_UNKNOWN3;
	writes[i].address = 0x10;
	writes[i].value = 0x0;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = CMDR;
	writes[i].value = SOFT_RESET;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	writes[i].value = AUX_7210;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = SWAPPED_AUXCR;
	writes[i].value = AUX_7210;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = HSSEL;
	writes[i].value = TNT_ONE_CHIP_BIT;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	writes[i].value = AUX_CR;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = IMR0;
	writes[i].value = TNT_IMR0_ALWAYS_BITS;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(IMR1);
	writes[i].value = 0x0;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(IMR2);
	writes[i].value = 0x0;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = IMR3;
	writes[i].value = 0x0;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	writes[i].value = AUX_HLDI;
	i++;
	/* the following three writes should share code with set_t1_delay */
	usbdata->board->t1_nano_sec = 500;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	writes[i].value = AUXRI | SISB | USTD;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	writes[i].value = AUXRB | HR_TRI;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = KEYREG;
	writes[i].value = 0x0;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	writes[i].value = AUXRG | NTNL_BIT;
	i++;
#if 0	// request system control
	i += ni_usb_bulk_register_write(&out_data[i], NIUSB_SUBDEV_TNT4882, CMDR, SETSC);
	i += ni_usb_bulk_register_write(&out_data[i], NIUSB_SUBDEV_TNT4882, nec7210_to_tnt4882_offset(AUXMR), AUX_CIFC);
#endif
	// primary address
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(ADR);
	writes[i].value = usbdata->board->pad;
	i++;
	writes[i].device = NIUSB_SUBDEV_UNKNOWN2;
	writes[i].address = 0x0;
	writes[i].value = usbdata->board->pad;
	i++;
	// secondary address
	i += ni_usb_write_sad(&writes[i], usbdata->board->sad, usbdata->board->sad >= 0);
	// is this a timeout?
	writes[i].device = NIUSB_SUBDEV_UNKNOWN2;
	writes[i].address = 0x2;
	writes[i].value = 0xfd;
	i++;
	// what is this?  There is no documented tnt4882 register at offset 0xf
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = 0xf;
	writes[i].value = 0x11;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	writes[i].value = AUX_PON;
	i++;
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	writes[i].value = AUX_CPPF;
	i++;
	if(i > writes_length)
	{
		// printf("%s: %s: bug!, buffer overrun, i=%i\n", __FILE__, __FUNCTION__, i);
		return -EINVAL;
	}
	retval = ni_usb_write_registers(usbdata, writes, i, &ibsta);
	free(writes);
	if(retval)
	{
		// printf("%s: %s: register write failed, retval=%i\n", __FILE__, __FUNCTION__, retval);
		return retval;
	}
	ni_usb_soft_update_status(usbdata, ibsta, 0);
	return 0;
}

//void ni_usb_interrupt_complete(struct urb *urb PT_REGS_ARG)
//{
//	LPDRIVER_INST pNet  = urb->context;
//	usb_data* usbdata = (usb_data*)pNet->USB_Data;
//	int retval;
//	struct ni_usb_status_block status;
//	unsigned long flags;
//
//// 	// printf("debug: %s: %s: status=0x%x, error_count=%i, actual_length=%i\n", __FILE__, __FUNCTION__,
//// 		urb->status, urb->error_count, urb->actual_length);
//
//	switch (urb->status) {
//	/* success */
//	case 0:
//		break;
//	/* unlinked, don't resubmit */
//	case -ECONNRESET:
//	case -ENOENT:
//	case -ESHUTDOWN:
//		return;
//	default: /* other error, resubmit */
//		retval = usb_submit_urb(ni_priv->interrupt_urb, GFP_ATOMIC);
//		if(retval)
//		{
//			// printf("%s: failed to resubmit interrupt urb\n", __FUNCTION__);
//		}
//		return;
//	}
//
//	ni_usb_parse_status_block(urb->transfer_buffer, &status);
//// 	// printf("debug: ibsta=0x%x\n", status.ibsta);
//
//	spin_lock_irqsave(&board->spinlock, flags);
//	ni_priv->monitored_ibsta_bits &= ~status.ibsta;
//// 	// printf("debug: monitored_ibsta_bits=0x%x\n", ni_priv->monitored_ibsta_bits);
//	spin_unlock_irqrestore(&board->spinlock, flags);
//
//	wake_up_interruptible( &board->wait );
//
//	retval = usb_submit_urb(ni_priv->interrupt_urb, GFP_ATOMIC);
//	if(retval)
//	{
//		// printf("%s: failed to resubmit interrupt urb\n", __FUNCTION__);
//	}
//}
//
//static int ni_usb_set_interrupt_monitor(LPDRIVER_INST pNet , unsigned int monitored_bits)
//{
//	int retval;
//	usb_data* usbdata = (usb_data*)pNet->USB_Data;
//	static const int bufferLength = 8;
//	uint8_t *buffer;
//	struct ni_usb_status_block status;
//	unsigned long flags;
//	//// printf("%s: receive control pipe is %i\n", __FILE__, pipe);
//	buffer = malloc(bufferLength, GFP_KERNEL);
//	if(buffer == NULL)
//	{
//		// printf("%s: malloc failed!\n", __FILE__);
//		return -ENOMEM;
//	}
//	spin_lock_irqsave(&board->spinlock, flags);
//	ni_priv->monitored_ibsta_bits = ni_usb_ibsta_monitor_mask & monitored_bits;
//// 	// printf("debug: %s: monitored_ibsta_bits=0x%x\n", __FUNCTION__, ni_priv->monitored_ibsta_bits);
//	spin_unlock_irqrestore(&board->spinlock, flags);
//	retval = ni_usb_receive_control_msg(ni_priv, NI_USB_WAIT_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
//		0x300, ni_usb_ibsta_monitor_mask & monitored_bits, buffer, bufferLength, 1000);
//	if(retval != bufferLength)
//	{
//		// printf("%s: usb_control_msg returned %i\n", __FILE__, retval);
//		free(buffer);
//		return -1;
//	}
//	ni_usb_parse_status_block(buffer, &status);
//	free(buffer);
//	return 0;
//}
//
//static int ni_usb_setup_urbs(LPDRIVER_INST pNet )
//{
//	usb_data* usbdata = (usb_data*)pNet->USB_Data;
//	struct usb_device *usb_dev;
//	int int_pipe;
//	int retval;
//
//	if (ni_priv->interrupt_in_endpoint < 0) return 0;
//
//	mutex_lock(&ni_priv->interrupt_transfer_lock);
//	if(ni_priv->bus_interface == NULL)
//	{
//		mutex_unlock(&ni_priv->interrupt_transfer_lock);
//		return -ENODEV;
//	}
//	ni_priv->interrupt_urb = usb_alloc_urb(0, GFP_KERNEL);
//	if(ni_priv->interrupt_urb == NULL)
//	{
//		mutex_unlock(&ni_priv->interrupt_transfer_lock);
//		return -ENOMEM;
//	}
//	usb_dev = interface_to_usbdev(ni_priv->bus_interface);
//	int_pipe = usb_rcvintpipe(usb_dev, ni_priv->interrupt_in_endpoint);
//	usb_fill_int_urb(ni_priv->interrupt_urb, usb_dev, int_pipe, ni_priv->interrupt_buffer,
//		sizeof(ni_priv->interrupt_buffer), &ni_usb_interrupt_complete, board, 1);
//	retval = usb_submit_urb(ni_priv->interrupt_urb, GFP_KERNEL);
//	mutex_unlock(&ni_priv->interrupt_transfer_lock);
//	if(retval)
//	{
//		// printf("%s: failed to submit first interrupt urb, retval=%i\n", __FILE__, retval);
//		return retval;
//	}
//	return 0;
//}
//
//
//static void ni_usb_cleanup_urbs(ni_usb_private_t *ni_priv)
//{
//	if(ni_priv && ni_priv->bus_interface)
//	{
//		if(ni_priv->interrupt_urb)
//			usb_kill_urb(ni_priv->interrupt_urb);
//		if(ni_priv->bulk_urb)
//			usb_kill_urb(ni_priv->bulk_urb);
//	}
//};

int ni_usb_b_read_serial_number(usb_data* usbdata)
{
	//usb_data* usbdata = (usb_data*)pNet->USB_Data;

	int retval;
	uint8_t *out_data;
	uint8_t *in_data;
	static const int out_data_length = 0x20;
	static const int  in_data_length = 0x20;
	int bytes_written = 0, bytes_read = 0;
	int i = 0;
	static const int num_reads = 4;
	unsigned results[4];
	int j;
	unsigned serial_number;
// 	// printf("%s: %s\n", __FILE__, __FUNCTION__);
	in_data = malloc(in_data_length);
	if(in_data == NULL)
	{
	        // printf("%s: %s malloc in_data failed\n", __FILE__,__FUNCTION__);
		return -ENOMEM;
	}
	out_data = malloc(out_data_length);
	if(out_data == NULL)
	{
	        // printf("%s: %s malloc out_data failed\n", __FILE__,__FUNCTION__);
		free(in_data);
		return -ENOMEM;
	}
	i += ni_usb_bulk_register_read_header(&out_data[i], num_reads);
	i += ni_usb_bulk_register_read(&out_data[i], NIUSB_SUBDEV_UNKNOWN3, SERIAL_NUMBER_1_REG);
	i += ni_usb_bulk_register_read(&out_data[i], NIUSB_SUBDEV_UNKNOWN3, SERIAL_NUMBER_2_REG);
	i += ni_usb_bulk_register_read(&out_data[i], NIUSB_SUBDEV_UNKNOWN3, SERIAL_NUMBER_3_REG);
	i += ni_usb_bulk_register_read(&out_data[i], NIUSB_SUBDEV_UNKNOWN3, SERIAL_NUMBER_4_REG);
	while(i % 4)
		out_data[i++] = 0x0;
	i += ni_usb_bulk_termination(&out_data[i]);
	retval = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkSnd, out_data, out_data_length, &bytes_written, USB_SHORT_XFER_OK, USB_DEFAULT_TIMEOUT);// ok
//	retval = ni_usb_send_bulk_msg(pNet, out_data, out_data_length, &bytes_written, 1000);
	if(retval)
	{
		// printf("%s: %s: ni_usb_send_bulk_msg returned %i, bytes_written=%i, i=%li\n", __FILE__, __FUNCTION__,
		//       retval, bytes_written, (long) out_data_length);
		goto serial_out;
	}
	retval = UsbBulkXfer(usbdata->udev, usbdata->_pipeBlkRcv, in_data, in_data_length, &bytes_read, USB_SHORT_XFER_OK, USB_DEFAULT_TIMEOUT);// ok

//	retval = ni_usb_receive_bulk_msg(pNet, in_data, in_data_length, &bytes_read, 1000, 0);
	if(retval==0)
	{
		// printf("%s: %s: ni_usb_receive_bulk_msg returned %i, bytes_read=%i\n", __FILE__, __FUNCTION__, retval, bytes_read);
		ni_usb_dump_raw_block(in_data, bytes_read);
		//goto serial_out;
	}
//	if(sizeof(results) / sizeof(results[0]) < num_reads) BUG();
	ni_usb_parse_register_read_block(in_data, results, num_reads);
	serial_number = 0;
	for(j = 0; j < num_reads; ++j)
	{
		serial_number |= (results[j] & 0xff) << (8 * j);
	}
	// printf("%s: board serial number is 0x%x\n", __FUNCTION__, serial_number);
	retval = 0;
	serial_out:
		free(in_data);
		free(out_data);
	return retval;
}

 int ni_usb_hs_wait_for_ready(usb_data* usbdata)
{
	static const int buffer_size = 0x10;
	static const int timeout = 50;
	static const int msec_sleep_duration = 100;
	int i;	uint16_t retval;
	int j;
	int unexpected = 0;
	unsigned serial_number;
	uint8_t *buffer;
	buffer = malloc(buffer_size);
	if(buffer == NULL)
	{
		// printf("%s: %s malloc failed\n", __FILE__,__FUNCTION__);
		return -ENOMEM;
	}

	int32_t stat;
	stat = UsbControlXfer(usbdata->udev, NI_USB_SERIAL_NUMBER_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE, 0x0, 0x0, buffer, buffer_size, &retval, USB_MULTI_SHORT_OK, 500);
	//retval = ni_usb_receive_control_msg(usbdata->, NI_USB_SERIAL_NUMBER_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
	//	0x0, 0x0, buffer, buffer_size, 1000);
	if((retval < 0) || (stat != USB_ERR_NORMAL_COMPLETION) )
	{
		// printf("%s: usb_control_msg request 0x%x returned %i\n", __FILE__, NI_USB_SERIAL_NUMBER_REQUEST, retval);
		goto ready_out;
	}
	j = 0;
	if(buffer[j] != NI_USB_SERIAL_NUMBER_REQUEST)
	{
		// printf( "unexpected data: buffer[%i]=0x%x, expected 0x%x\n",
		//	  j, (int)buffer[j], NI_USB_SERIAL_NUMBER_REQUEST);
		unexpected = 1;
	}
	if(unexpected)
		ni_usb_dump_raw_block(buffer, retval);
	// NI-USB-HS+ pads the serial with 0x0 to make 16 bytes
	if(retval != 5 && retval != 16)
	{
	//	 printf("received unexpected number of bytes = %i, expected 5 or 16\n", retval);
		ni_usb_dump_raw_block(buffer, retval);
	}
	serial_number = 0;
	serial_number |= buffer[++j];
	serial_number |= (buffer[++j] << 8);
	serial_number |= (buffer[++j] << 16);
	serial_number |= (buffer[++j] << 24);
	//printf("%s: board serial number is 0x%x\n", __FUNCTION__, serial_number);
	for(i = 0; i < timeout; ++i)
	{
		int ready = 0;
		stat = UsbControlXfer(usbdata->udev, NI_USB_POLL_READY_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE, 0x0, 0x0, buffer, buffer_size, &retval, USB_MULTI_SHORT_OK, 100);
//		retval = ni_usb_receive_control_msg(ni_priv, NI_USB_POLL_READY_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
//			0x0, 0x0, buffer, buffer_size, 100);
		if ((retval < 0) || (stat != USB_ERR_NORMAL_COMPLETION))
		{
		//	 printf("%s: usb_control_msg request 0x%x returned %i\n", __FILE__, NI_USB_POLL_READY_REQUEST, retval);
			goto ready_out;
		}
		j = 0;
		unexpected = 0;
		if(buffer[j] != NI_USB_POLL_READY_REQUEST)
		{
			// printf(" unexpected data: buffer[%i]=0x%x, expected 0x%x\n",
			//	 j, (int)buffer[j], NI_USB_POLL_READY_REQUEST);
			unexpected = 1;
		}
		++j;
		if(buffer[j] != 0x1 && buffer[j] != 0x0) // HS+ sends 0x0
		{
			// printf(" unexpected data: buffer[%i]=0x%x, expected 0x1 or 0x0\n",
			//	 j, (int)buffer[j]);
			unexpected = 1;
		}
		if(buffer[++j] != 0x0)
		{
			// printf(" unexpected data: buffer[%i]=0x%x, expected 0x%x\n",
			//	 j, (int)buffer[j], 0x0);
			unexpected = 1;
		}
		++j;
		// MC usb-488 (and sometimes NI-USB-HS?) sends 0x8 here; MC usb-488A sends 0x7 here
		// NI-USB-HS+ sends 0x0
		if(buffer[j] != 0x1 && buffer[j] != 0x8 && buffer[j] != 0x7 && buffer[j] != 0x0)
		{
			// printf(" unexpected data: buffer[%i]=0x%x, expected 0x0, 0x1, 0x7 or 0x8\n",
			//	 j, (int)buffer[j]);
			unexpected = 1;
		}
		++j;
		// NI-USB-HS+ sends 0 here
		if(buffer[j] != 0x30 && buffer[j] != 0x0)
		{
			// printf(" unexpected data: buffer[%i]=0x%x, expected 0x0 or 0x30\n",
			//	 j, (int)buffer[j]);
			unexpected = 1;
		}
		++j;
		// MC usb-488 (and sometimes NI-USB-HS?) and NI-USB-HS+ sends 0x0 here
		if(buffer[j] != 0x1 && buffer[j] != 0x0)
		{
			// printf(" unexpected data: buffer[%i]=0x%x, expected 0x1 or 0x0\n",
			//	 j, (int)buffer[j]);
			unexpected = 1;
		}
		if(buffer[++j] != 0x0)
		{
			ready = 1;
			// NI-USB-HS+ sends 0xf here
			if(buffer[j] != 0x2 && buffer[j] != 0xe && buffer[j] != 0xf)
			{
				// printf(" unexpected data: buffer[%i]=0x%x, expected 0x2, 0xe or 0xf\n",
				//	 j, (int)buffer[j]);
				unexpected = 1;
			}
		}
		if(buffer[++j] != 0x0)
		{
			ready = 1;
			// MC usb-488 sends 0x5 here; MC usb-488A sends 0x6 here
			if(buffer[j] != 0x3 && buffer[j] != 0x5 && buffer[j] != 0x6)
			{
				/// printf(" unexpected data: buffer[%i]=0x%x, expected 0x3 or 0x5 or 0x6\n",
				//	 j, (int)buffer[j]);
				unexpected = 1;
			}
		}
		++j;
		if(buffer[j] != 0x0 && buffer[j] != 0x2) // MC usb-488 sends 0x2 here
		{
			 //printf(" unexpected data: buffer[%i]=0x%x, expected 0x0 ox 0x2\n",
			//	 j, (int)buffer[j]);
			unexpected = 1;
		}
		++j;
		// MC usb-488A and NI-USB-HS sends 0x3 here; NI-USB-HS+ sends 0x30 here
		if(buffer[j] != 0x0 && buffer[j] != 0x3 && buffer[j] != 0x30)
		{
			// printf(" unexpected data: buffer[%i]=0x%x, expected 0x0, 0x3 or 0x30\n",
			//	  j, (int)buffer[j]);
			unexpected = 1;
		}
		if(buffer[++j] != 0x0)
		{
			ready = 1;
			if(buffer[j] != 0x96 && buffer[j] != 0x7) // MC usb-488 sends 0x7 here
			{
				// printf(" unexpected data: buffer[%i]=0x%x, expected 0x96 or 0x07\n",
				//	  j, (int)buffer[j]);
				unexpected = 1;
			}
		}

		if(unexpected)
			ni_usb_dump_raw_block(buffer, retval);
		if(ready) break;
		//retval = msleep_interruptible(msec_sleep_duration);
		//if(retval)
		//{
			// printf("ni_usb_gpib: msleep interrupted\n");
		//	retval = -ERESTARTSYS;
		//	goto ready_out;
		//}
	}
	retval = 0;

ready_out:
	free(buffer);
	//GPIB_D// printf("%s: %s exit retval=%d\n", __FILE__,__FUNCTION__,retval);
	return retval;
}


/* This does some extra init for HS+ models, as observed on Windows.  One of the
  control requests causes the LED to stop blinking.
  I'm not sure what the other 2 requests do.  None of these requests are actually required
  for the adapter to work, maybe they do some init for the analyzer interface
  (which we don't use). */
int ni_usb_hs_plus_extra_init(usb_data* usbdata)
{
	uint16_t retval;
	uint8_t *buffer;
	static const int buffer_size = 16;
	int transfer_size;

	buffer = malloc(buffer_size);
	if(buffer == NULL)
	{
		return -ENOMEM;
	}


	do
	{
		transfer_size = 16;
		int32_t stat;
		//BUG_ON(transfer_size > buffer_size);
		//retval = ni_usb_receive_control_msg(ni_priv, NI_USB_HS_PLUS_0x48_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		//	0x0, 0x0, buffer, transfer_size, 1000);
		stat= UsbControlXfer(usbdata->udev, NI_USB_HS_PLUS_0x48_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE, 0x0, 0x0, buffer, transfer_size, &retval, USB_MULTI_SHORT_OK, 500);

		if((retval < 0) || (stat != USB_ERR_NORMAL_COMPLETION) )
		{
			retval = FAILURE;
			// printf("%s: usb_control_msg request 0x%x returned %i\n", __FILE__, NI_USB_HS_PLUS_0x48_REQUEST, retval);
			break;
		}
		// expected response data: 48 f3 30 00 00 00 00 00 00 00 00 00 00 00 00 00
		if (buffer[0] != NI_USB_HS_PLUS_0x48_REQUEST)
		{
			// printf("%s: %s: unexpected data: buffer[0]=0x%x, expected 0x%x\n",
			//	__FILE__, __FUNCTION__, (int)buffer[0], NI_USB_HS_PLUS_0x48_REQUEST);
		}

		transfer_size = 2;
		//BUG_ON(transfer_size > buffer_size);
		//retval = ni_usb_receive_control_msg(ni_priv, NI_USB_HS_PLUS_LED_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		//	0x1, 0x0, buffer, transfer_size, 1000);
		stat = UsbControlXfer(usbdata->udev, NI_USB_HS_PLUS_LED_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE, 0x1, 0x0, buffer, transfer_size, &retval, USB_MULTI_SHORT_OK, 500);
		if ((retval < 0) || (stat != USB_ERR_NORMAL_COMPLETION))
		{
			retval = FAILURE;
			// printf("%s: usb_control_msg request 0x%x returned %i\n", __FILE__, NI_USB_HS_PLUS_LED_REQUEST, retval);
			break;
		}
		// expected response data: 4b 00
		if (buffer[0] != NI_USB_HS_PLUS_LED_REQUEST)
		{
			// printf("%s: %s: unexpected data: buffer[0]=0x%x, expected 0x%x\n",
			//	__FILE__, __FUNCTION__, (int)buffer[0], NI_USB_HS_PLUS_LED_REQUEST);
		}

		transfer_size = 9;
		//BUG_ON(transfer_size > buffer_size);
		//retval = ni_usb_receive_control_msg(ni_priv, NI_USB_HS_PLUS_0xf8_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
		//	0x0, 0x1, buffer, transfer_size, 1000);
		stat = UsbControlXfer(usbdata->udev, NI_USB_HS_PLUS_0xf8_REQUEST, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE, 0x0, 0x1, buffer, transfer_size, &retval, USB_MULTI_SHORT_OK, 500);
		if ((retval < 0) || (stat != USB_ERR_NORMAL_COMPLETION))
		{
			retval = FAILURE;
			// printf("%s: usb_control_msg request 0x%x returned %i\n", __FILE__, NI_USB_HS_PLUS_0xf8_REQUEST, retval);
			break;
		}
		// expected response data: f8 01 00 00 00 01 00 00 00
		if (buffer[0] != NI_USB_HS_PLUS_0xf8_REQUEST)
		{
			// printf("%s: %s: unexpected data: buffer[0]=0x%x, expected 0x%x\n",
			//	__FILE__, __FUNCTION__, (int)buffer[0], NI_USB_HS_PLUS_0xf8_REQUEST);
		}
	} while(0);

	// cleanup
	free(buffer);
	//EROP("%s = %x \n", __FUNCTION__, retval, 0, 0);
	return retval;
}

//static inline int ni_usb_device_match(struct usb_interface *interface, const gpib_board_config_t *config)
//{
//	if(gpib_match_device_path(&interface->dev, config->device_path) == 0)
//	{
//		return 0;
//	}
//
//	return 1;
//}

//int ni_usb_attach(LPDRIVER_INST pNet , const gpib_board_config_t *config)
//{
//	int retval;
//	int i;
//	ni_usb_private_t *ni_priv;
//	int product_id;
//	struct usb_device *usb_dev;
//	
//	// printf("ni_usb_gpib: attach\n");
//	mutex_lock(&ni_usb_hotplug_lock);
//	retval = ni_usb_allocate_private(board);
//	if(retval < 0)
//	{
//		mutex_unlock(&ni_usb_hotplug_lock);
//		return retval;
//	}
//	ni_priv = board->private_data;
//	for(i = 0; i < MAX_NUM_NI_USB_INTERFACES; i++)
//	{
//		if(ni_usb_driver_interfaces[i] && usb_get_intfdata(ni_usb_driver_interfaces[i]) == NULL &&
//			ni_usb_device_match(ni_usb_driver_interfaces[i], config))
//		{
//			ni_priv->bus_interface = ni_usb_driver_interfaces[i];
//			usb_set_intfdata(ni_usb_driver_interfaces[i], board);
//			usb_dev = interface_to_usbdev(ni_priv->bus_interface);
//			dev_info(&usb_dev->dev,"bus %d dev num %d attached to gpib minor %d, NI usb interface %i\n",
//				 usb_dev->bus->busnum, usb_dev->devnum, board->minor, i);
//			break;
//		}
//	}
//	if(i == MAX_NUM_NI_USB_INTERFACES)
//	{
//		mutex_unlock(&ni_usb_hotplug_lock);
//		// printf("No supported NI usb gpib adapters found, have you loaded its firmware?\n");
//		return -ENODEV;
//	}
//	if(usb_reset_configuration(interface_to_usbdev(ni_priv->bus_interface)))
//	{
//		// printf("ni_usb_gpib: usb_reset_configuration() failed.\n");
//	}
//	product_id = USBID_TO_CPU(interface_to_usbdev(ni_priv->bus_interface)->descriptor.idProduct);
//	// printf("\tproduct id=0x%x\n", product_id);
//
//	COMPAT_TIMER_SETUP(&ni_priv->bulk_timer, ni_usb_timeout_handler, 0);
//
//	switch(product_id)
//	{
//	case USB_DEVICE_ID_NI_USB_B:
//		ni_priv->bulk_out_endpoint = NIUSB_B_BULK_OUT_ENDPOINT;
//		ni_priv->bulk_in_endpoint = NIUSB_B_BULK_IN_ENDPOINT;
//		ni_priv->interrupt_in_endpoint = NIUSB_B_INTERRUPT_IN_ENDPOINT;
//		ni_usb_b_read_serial_number(ni_priv);
//		break;
//	case USB_DEVICE_ID_NI_USB_HS:
//	case USB_DEVICE_ID_MC_USB_488:
//	case USB_DEVICE_ID_KUSB_488A:
//		ni_priv->bulk_out_endpoint = NIUSB_HS_BULK_OUT_ENDPOINT;
//		ni_priv->bulk_in_endpoint = NIUSB_HS_BULK_IN_ENDPOINT;
//		ni_priv->interrupt_in_endpoint = NIUSB_HS_INTERRUPT_IN_ENDPOINT;
//		retval = ni_usb_hs_wait_for_ready(ni_priv);
//		if(retval < 0)
//		{
//			mutex_unlock(&ni_usb_hotplug_lock);
//			return retval;
//		}
//		break;
//	case USB_DEVICE_ID_NI_USB_HS_PLUS:
//		ni_priv->bulk_out_endpoint = NIUSB_HS_PLUS_BULK_OUT_ENDPOINT;
//		ni_priv->bulk_in_endpoint = NIUSB_HS_PLUS_BULK_IN_ENDPOINT;
//		ni_priv->interrupt_in_endpoint = NIUSB_HS_PLUS_INTERRUPT_IN_ENDPOINT;
//		retval = ni_usb_hs_wait_for_ready(ni_priv);
//		if(retval < 0)
//		{
//			mutex_unlock(&ni_usb_hotplug_lock);
//			return retval;
//		}
//		retval = ni_usb_hs_plus_extra_init(ni_priv);
//		if(retval < 0)
//		{
//			mutex_unlock(&ni_usb_hotplug_lock);
//			return retval;
//		}
//		break;
//	default:
//		mutex_unlock(&ni_usb_hotplug_lock);
//		// printf("\tDriver bug: unknown endpoints for usb device id\n");
//		return -EINVAL;
//	}
//
//	retval = ni_usb_setup_urbs(board);
//	if(retval < 0)
//	{
//		mutex_unlock(&ni_usb_hotplug_lock);
//		return retval;
//	}
//	retval = ni_usb_set_interrupt_monitor(board, 0);
//	if(retval < 0)
//	{
//		mutex_unlock(&ni_usb_hotplug_lock);
//		return retval;
//	}
//
//	retval = ni_usb_init(board);
//	if(retval < 0)
//	{
//		mutex_unlock(&ni_usb_hotplug_lock);
//		return retval;
//	}
//	retval = ni_usb_set_interrupt_monitor(board, ni_usb_ibsta_monitor_mask);
//	if(retval < 0)
//	{
//		mutex_unlock(&ni_usb_hotplug_lock);
//		return retval;
//	}
//
//	mutex_unlock(&ni_usb_hotplug_lock);
//	return retval;
//}

 int ni_usb_shutdown_hardware(usb_data* usbdata)
{
	int retval;
	int i = 0;
	struct ni_usb_register writes[2];
	static const int writes_length = sizeof(writes) / sizeof(writes[0]);
	unsigned int ibsta;

 	// printf( "%s\n",  __FUNCTION__);
	writes[i].device = NIUSB_SUBDEV_TNT4882;
	writes[i].address = nec7210_to_tnt4882_offset(AUXMR);
	writes[i].value = AUX_CR;
	i++;
	writes[i].device = NIUSB_SUBDEV_UNKNOWN3;
	writes[i].address = 0x10;
	writes[i].value = 0x0;
	i++;
	if(i > writes_length)
	{
		// printf("%s: %s: bug!, buffer overrun, i=%i\n", __FILE__, __FUNCTION__, i);
		return -EINVAL;
	}
	retval = ni_usb_write_registers(usbdata, writes, i, &ibsta);
	if(retval)
	{
		// printf("%s: %s: register write failed, retval=%i\n", __FILE__, __FUNCTION__, retval);
		return retval;
	}
	return 0;
}

static const int sad_offset = 0x60;

int extractPAD(Addr4882_t address)
{
	int pad = address & 0xff;

	if (address == NOADDR) return ADDR_INVALID;

	if (pad < 0 || pad > gpib_addr_max) return ADDR_INVALID;

	return pad;
}

int extractSAD(Addr4882_t address)
{
	int sad = (address >> 8) & 0xff;

	if (address == NOADDR) return ADDR_INVALID;

	if (sad == NO_SAD) return SAD_DISABLED;

	if ((sad & 0x60) == 0) return ADDR_INVALID;

	sad &= ~0x60;

	if (sad < 0 || sad > gpib_addr_max) return ADDR_INVALID;

	return sad;
}

Addr4882_t packAddress(unsigned int pad, int sad)
{
	Addr4882_t address;

	address = 0;
	address |= pad & 0xff;
	if (sad >= 0)
		address |= ((sad | sad_offset) << 8) & 0xff00;

	return address;
}

int addressIsValid(Addr4882_t address)
{
	if (address == NOADDR) return 1;

	if (extractPAD(address) == ADDR_INVALID ||
		extractSAD(address) == ADDR_INVALID)
	{
		//setIberr(EARG);
		return 0;
	}

	return 1;
}


//
//int ibstatus(usb_data* usbdata, int error, int clear_mask, int set_mask)
//{
//	int status = 0;
//	int retval;
//
//	retval = my_wait(conf, 0, clear_mask, set_mask, &status);
//	if (retval < 0) error = 1;
//
//	if (error) status |= ERR;
//	if (conf->timed_out)
//		status |= TIMO;
//	if (conf->end)
//		status |= END;
//
//	setIbsta(status);
//
//	return status;
//}

//void ni_usb_detach(LPDRIVER_INST pNet )
//{
//	ni_usb_private_t *ni_priv;
//
//	mutex_lock(&ni_usb_hotplug_lock);
////	// printf("%s: enter\n", __FUNCTION__);
//// under windows, software unplug does chip_reset nec7210 aux command, then writes 0x0 to address 0x10 of device 3
//	ni_priv = board->private_data;
//	if(ni_priv)
//	{
//		if(ni_priv->bus_interface)
//		{
//			ni_usb_set_interrupt_monitor(board, 0);
//			ni_usb_shutdown_hardware(ni_priv);
//			usb_set_intfdata(ni_priv->bus_interface, NULL);
//		}
//		mutex_lock(&ni_priv->bulk_transfer_lock);
//		mutex_lock(&ni_priv->control_transfer_lock);
//		mutex_lock(&ni_priv->interrupt_transfer_lock);
//		ni_usb_cleanup_urbs(ni_priv);
//		ni_usb_free_private(ni_priv);
//	}
////	// printf("%s: exit\n", __FUNCTION__);
//	mutex_unlock(&ni_usb_hotplug_lock);
//}

//gpib_interface_t ni_usb_gpib_interface =
//{
//	name: "ni_usb_b",
//	attach: ni_usb_attach,
//	detach: ni_usb_detach,
//	read: ni_usb_read,
//	write: ni_usb_write,
//	command: ni_usb_command,
//	take_control: ni_usb_take_control,
//	go_to_standby: ni_usb_go_to_standby,
//	request_system_control: ni_usb_request_system_control,
//	interface_clear: ni_usb_interface_clear,
//	remote_enable: ni_usb_remote_enable,
//	enable_eos: ni_usb_enable_eos,
//	disable_eos: ni_usb_disable_eos,
//	parallel_poll: ni_usb_parallel_poll,
//	parallel_poll_configure: ni_usb_parallel_poll_configure,
//	parallel_poll_response: ni_usb_parallel_poll_response,
//	local_parallel_poll_mode: NULL, // XXX
//	line_status: ni_usb_line_status,
//	update_status: ni_usb_update_status,
//	primary_address: ni_usb_primary_address,
//	secondary_address: ni_usb_secondary_address,
//	serial_poll_response: ni_usb_serial_poll_response,
//	serial_poll_status: ni_usb_serial_poll_status,
//	t1_delay: ni_usb_t1_delay,
//	return_to_local: ni_usb_return_to_local,
//};
//
//// Table with the USB-devices: just now only testing IDs
//static struct usb_device_id ni_usb_driver_device_table [] =
//{
//	{USB_DEVICE(USB_VENDOR_ID_NI, USB_DEVICE_ID_NI_USB_B)},
//	{USB_DEVICE(USB_VENDOR_ID_NI, USB_DEVICE_ID_NI_USB_HS)},
//	// gpib-usb-hs+ has a second interface for the analyzer, which we ignore
//	{USB_DEVICE_INTERFACE_NUMBER(USB_VENDOR_ID_NI, USB_DEVICE_ID_NI_USB_HS_PLUS, 0)},
//	{USB_DEVICE(USB_VENDOR_ID_NI, USB_DEVICE_ID_KUSB_488A)},
//	{USB_DEVICE(USB_VENDOR_ID_NI, USB_DEVICE_ID_MC_USB_488)},
//	{} /* Terminating entry */
//};
//MODULE_DEVICE_TABLE(usb, ni_usb_driver_device_table);
//
//static int ni_usb_driver_probe(struct usb_interface *interface,
//	const struct usb_device_id *id)
//{
//	int i;
//	char *path;
//	static const int pathLength = 1024;
//
////	// printf("ni_usb_driver_probe\n");
//	mutex_lock(&ni_usb_hotplug_lock);
//	usb_get_dev(interface_to_usbdev(interface));
//	for(i = 0; i < MAX_NUM_NI_USB_INTERFACES; i++)
//	{
//		if(ni_usb_driver_interfaces[i] == NULL)
//		{
//			ni_usb_driver_interfaces[i] = interface;
//			usb_set_intfdata(interface, NULL);
////			// printf("set bus interface %i to address 0x%p\n", i, interface);
//			break;
//		}
//	}
//	if(i == MAX_NUM_NI_USB_INTERFACES)
//	{
//		usb_put_dev(interface_to_usbdev(interface));
//		mutex_unlock(&ni_usb_hotplug_lock);
//		// printf("ni_usb_gpib: out of space in ni_usb_driver_interfaces[]\n");
//		return -1;
//	}
//	path = malloc(pathLength, GFP_KERNEL);
//	if(path == NULL)
//	{
//		usb_put_dev(interface_to_usbdev(interface));
//		mutex_unlock(&ni_usb_hotplug_lock);
//		return -ENOMEM;
//	}
//	usb_make_path(interface_to_usbdev(interface), path, pathLength);
//	// printf("ni_usb_gpib: probe succeeded for path: %s\n", path);
//	free(path);
//	mutex_unlock(&ni_usb_hotplug_lock);
//	return 0;
//}
//
//static void ni_usb_driver_disconnect(struct usb_interface *interface)
//{
//	int i;
//
//	mutex_lock(&ni_usb_hotplug_lock);
////	// printf("%s: enter\n", __FUNCTION__);
//	for(i = 0; i < MAX_NUM_NI_USB_INTERFACES; i++)
//	{
//		if(ni_usb_driver_interfaces[i] == interface)
//		{
//			LPDRIVER_INST pNet  = usb_get_intfdata(interface);
//
//			if(board)
//			{
//				usb_data* usbdata = (usb_data*)pNet->USB_Data;
//				if(ni_priv)
//				{
//					mutex_lock(&ni_priv->bulk_transfer_lock);
//					mutex_lock(&ni_priv->control_transfer_lock);
//					mutex_lock(&ni_priv->interrupt_transfer_lock);
//					ni_usb_cleanup_urbs(ni_priv);
//					ni_priv->bus_interface = NULL;
//					mutex_unlock(&ni_priv->interrupt_transfer_lock);
//					mutex_unlock(&ni_priv->control_transfer_lock);
//					mutex_unlock(&ni_priv->bulk_transfer_lock);
//				}
//			}
////			// printf("nulled ni_usb_driver_interfaces[%i]\n", i);
//			ni_usb_driver_interfaces[i] = NULL;
//			break;
//		}
//	}
//	if(i == MAX_NUM_NI_USB_INTERFACES)
//	{
//		// printf("unable to find interface in ni_usb_driver_interfaces[]? bug?\n");
//	}
//	usb_put_dev(interface_to_usbdev(interface));
////	// printf("%s: exit\n", __FUNCTION__);
//	mutex_unlock(&ni_usb_hotplug_lock);
//}
//
//static struct usb_driver ni_usb_bus_driver =
//{
//#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
//	.owner = THIS_MODULE,
//#endif
//	.name = "ni_usb_gpib",
//	.probe = ni_usb_driver_probe,
//	.disconnect = ni_usb_driver_disconnect,
//	.id_table = ni_usb_driver_device_table,
//};
//
//static int __init ni_usb_init_module(void)
//{
//	int i;
//
//	pr_info("ni_usb_gpib driver loading\n");
//	for(i = 0; i < MAX_NUM_NI_USB_INTERFACES; i++)
//		ni_usb_driver_interfaces[i] = NULL;
//	usb_register(&ni_usb_bus_driver);
//	gpib_register_driver(&ni_usb_gpib_interface, THIS_MODULE);
//
//	return 0;
//}
//
//static void __exit ni_usb_exit_module(void)
//{
//	pr_info("ni_usb_gpib driver unloading\n");
////	// printf("%s: enter\n", __FUNCTION__);
//	gpib_unregister_driver(&ni_usb_gpib_interface);
//	usb_deregister(&ni_usb_bus_driver);
////	// printf("%s: exit\n", __FUNCTION__);
//}
//
//module_init(ni_usb_init_module);
//module_exit(ni_usb_exit_module);

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
int ibcac(usb_data *usbdata, int sync, int fallback_to_async)
{
	//EROP("In %s\n", __FUNCTION__, 0, 0, 0);
	int status = ibstatus(usbdata, 0);
		//ibstatus(usbdata);
	int retval;

	if ((status & CIC) == 0)
	{
	//	EROP("gpib: not CIC during ibcac()\n",0,0,0,0);
		return -1;
	}

	if (status & ATN)
	{
		return 0;
	}

	if (sync && (status & LACS) == 0)
	{
		/* tcs (take control synchronously) can only possibly work when
		*  controller is listener.  Error code also needs to be -ETIMEDOUT
		*  or it will giveout without doing fallback. */
		retval = -ETIMEDOUT;
	}
	else
	{
		//retval = board->interface->take_control( board, sync );
		retval =  ni_usb_take_control(usbdata, sync, 1000);
	}
	if (retval < 0 && fallback_to_async)
	{
		if (sync && retval == -ETIMEDOUT)
			retval = ni_usb_take_control(usbdata, sync, 0);
	}
	//board->interface->update_status( board, 0 );
	ibstatus(usbdata, 0);

	//EROP("%s \n", __FUNCTION__, 0, 0, 0);

	return retval;
}

/*
 * IBCMD
 * Write cnt command bytes from buf to the GPIB.  The
 * command operation terminates only on I/O complete.
 *
 * NOTE:
 *      1.  Prior to beginning the command, the interface is
 *          placed in the controller active state.
 *      2.  Before calling ibcmd for the first time, ibsic
 *          must be called to initialize the GPIB and enable
 *          the interface to leave the controller idle state.
 */
int ibcmd(usb_data *usbdata, uint8_t *buf, size_t length, size_t *bytes_written, uint32_t timeOut)
{
	size_t ret = 0;
	int status;
	//EROP("%s \n", __FUNCTION__, 0, 0, 0);

	//*bytes_written = 0;
	//EROP("status :%x\n", status, 0, 0, 0);
	status = ibstatus(usbdata, 0);
	//EROP("status :%x\n", status, 0, 0, 0);

	if ((status & CIC) == 0)
	{
	//	EROP("gpib: cannot send command when not controller-in-charge\n", 0, 0, 0, 0);
		return -EIO;
	}

	//osStartTimer(board, board->usec_timeout);

	ret = ibcac(usbdata, 1, 1);
	//EROP("sending ibcac= %xs\n", ret, 0, 0, 0);
	if (ret == 0)
	{
		ret = ni_usb_command(usbdata, buf, length, bytes_written,timeOut);
			//board->interface->command(board, buf, length, bytes_written);
	}

	//osRemoveTimer(board);

	//if (io_timed_out(board))
	//	ret = -ETIMEDOUT;

	return ret;
}
/*
 * IBGTS
 * Go to the controller standby state from the controller
 * active state, i.e., turn ATN off.
 */

int ibgts(usb_data *usbdata )
{
	int status = ibstatus(usbdata, 0);
	int retval;

	if ((status & CIC) == 0)
	{
		usbdata->master = 0;

		//EROP("gpib: not CIC during ibgts()\n", 0, 0, 0, 0);
		return -1;
	}

	retval = ni_usb_go_to_standby(usbdata);                    /* go to standby */
	//if (retval < 0)
		//EROP("gpib: error while going to standby\n", 0, 0, 0, 0);

	ibstatus(usbdata, 0);

	return retval;

}
static int autospoll_wait_should_wake_up(usb_data *usbdata)
{
	int retval;
	//printf("must imp, %s", __FUNCTION__);
//	retval = board->master && board->autospollers > 0 &&
//		!atomic_read(&board->stuck_srq) &&
//		test_and_clear_bit(SRQI_NUM, &board->status);
//	smp_mb__after_atomic();

//	mutex_unlock(&board->big_gpib_mutex);
//	return retval;
	return 0;

}


/*
 * IBLINES
 * Poll the GPIB control lines and return their status in buf.
 *
 *      LSB (bits 0-7)  -  VALID lines mask (lines that can be monitored).
 * Next LSB (bits 8-15) - STATUS lines mask (lines that are currently set).
 *
 */
int iblines(usb_data *usbdata, short *lines)
{
	int retval;

	*lines = 0;
	if (ni_usb_line_status(usbdata) == NULL)
	{
		return 0;
	}
	retval = ni_usb_line_status(usbdata);
	if (retval < 0) return retval;
	*lines = retval;
	return 0;
}


/*
 * IBRD
 * Read up to 'length' bytes of data from the GPIB into buf.  End
 * on detection of END (EOI and or EOS) and set 'end_flag'.
 *
 * NOTE:
 *      1.  The interface is placed in the controller standby
 *          state prior to beginning the read.
 *      2.  Prior to calling ibrd, the intended devices as well
 *          as the interface board itself must be addressed by
 *          calling ibcmd.
 */

int ibrd(usb_data *usbdata, uint8_t *buf, size_t length, int *end_flag, size_t *nbytes,uint32_t timeOut)
{
	size_t ret = 0;
	int retval;
	size_t bytes_read;

	*nbytes = 0;
	*end_flag = 0;
	if (length == 0)
	{
	//	printf("gpib: ibrd() called with zero length?\n");
		return 0;
	}

	if (usbdata->master)
	{
		retval = ibgts(usbdata);
		if (retval < 0) return retval;
	}
	/* XXX reseting timer here could cause timeouts take longer than they should,
	 * since read_ioctl calls this
	 * function in a loop, there is probably a similar problem with writes/commands */
	//osStartTimer(board, board->usec_timeout);

	do
	{
		ret = ni_usb_read(usbdata, buf, length - *nbytes, end_flag, &bytes_read, timeOut);
		//ret = board->interface->read(board, buf, length - *nbytes, end_flag, &bytes_read);
		if (ret < 0)
		{
			/*			printk("gpib read error\n");*/
		}
		buf += bytes_read;
		*nbytes += bytes_read;
		//if (need_resched())
		//{
		//	schedule();
		//}
	} while (ret == 0 && *nbytes > 0 && *nbytes < length && *end_flag == 0);

	//osRemoveTimer(board);

	return ret;
}

/*
 * IBRPP
 * Conduct a parallel poll and return the byte in buf.
 *
 * NOTE:
 *      1.  Prior to conducting the poll the interface is placed
 *          in the controller active state.
 */
//
//int ibrpp(usb_data *usbdata, uint8_t *result)
//{
//	int retval = 0;
//
//	//osStartTimer(usbdata, board->usec_timeout);
//	retval = ibcac(board, 1, 1);
//	if (retval) return -1;
//
//	if (board->interface->parallel_poll(board, result))
//	{
//		printk("gpib: parallel poll failed\n");
//		retval = -1;
//	}
//	osRemoveTimer(board);
//	return retval;
//}
//
//int ibppc(gpib_board_t *board, uint8_t configuration)
//{
//
//	configuration &= 0x1f;
//	board->interface->parallel_poll_configure(board, configuration);
//	board->parallel_poll_configuration = configuration;
//
//	return 0;
//
//
	//static int autospoll_thread(void *board_void)
	//{
	//	gpib_board_t *board = board_void;
	//	int retval = 0;
	//
	//	GPIB_DPRINTK("entering autospoll thread\n");
	//
	//	while (1)
	//	{
	//		wait_event_interruptible(board->wait,
	//			kthread_should_stop() ||
	//			autospoll_wait_should_wake_up(board));
	//		GPIB_DPRINTK("autospoll wait satisfied\n");
	//		if (kthread_should_stop()) break;
	//
	//		mutex_lock(&board->big_gpib_mutex);
	//		/* make sure we are still good after we have
	//		 * lock */
	//		if (board->autospollers <= 0 || board->master == 0)
	//		{
	//			mutex_unlock(&board->big_gpib_mutex);
	//			continue;
	//		}
	//		mutex_unlock(&board->big_gpib_mutex);
	//
	//		if (try_module_get(board->provider_module))
	//		{
	//			retval = autopoll_all_devices(board);
	//			module_put(board->provider_module);
	//		}
	//		else
	//			printk("gpib%i: %s: try_module_get() failed!\n", board->minor, __FUNCTION__);
	//		if (retval <= 0)
	//		{
	//			printk("gpib%i: %s: struck SRQ\n", board->minor, __FUNCTION__);
	//
	//			smp_mb__before_atomic();
	//			atomic_set(&board->stuck_srq, 1);	// XXX could be better
	//			set_bit(SRQI_NUM, &board->status);
	//			smp_mb__after_atomic();
	//		}
	//	}
	//	printk("gpib%i: exiting autospoll thread\n", board->minor);
	//	return retval;
	//}

int ibonline(usb_data *usbdata)
{
	int retval;

	if (usbdata->online) return -EBUSY;

	//retval = gpib_allocate_board(board);

	if (retval < 0) return retval;

	//board->dev = NULL;
	//board->local_ppoll_mode = 0;
	//ni_usb_atta
	//retval = board->interface->attach(board, &board->config);
	//if (retval < 0)
	//{
	//	board->interface->detach(board);
		//printk("gpib: interface attach failed\n");
	//	return retval;
	//}
	/* nios2nommu on 2.6.11 uclinux kernel has weird problems
	with autospoll thread causing huge slowdowns */
//#ifndef CONFIG_NIOS2
//	board->autospoll_task = kthread_run(&autospoll_thread, board, "gpib%d_autospoll_kthread", board->minor);
//	retval = IS_ERR(board->autospoll_task);
//	if (retval)
//	{
//		printk("gpib: failed to create autospoll thread\n");
//		board->interface->detach(board);
//		return retval;
//	}
//#endif
	usbdata->online = 1;


	return 0;
}
		/* XXX need to make sure board is generally not in use (grab board lock?) */
		int iboffline(gpib_board_t *board)
		{
			int retval;
		
			if (board->online == 0)
			{
				return 0;
			}
			//if (board->interface == NULL) return -ENODEV;
		
			//if (board->autospoll_task != NULL && !IS_ERR(board->autospoll_task))
			//{
			//	retval = kthread_stop(board->autospoll_task);
			//	if (retval)
			//		printk("gpib: kthread_stop returned %i\n", retval);
			//	board->autospoll_task = NULL;
			//}
		
			//board->interface->detach(board);
			//gpib_deallocate_board(board);
			board->online = 0;

		
			return 0;
		}

/*
 * IBRSV
 * Request service from the CIC and/or set the serial poll
 * status byte.
 */
int ibrsv2(usb_data *usbdata, uint8_t status_byte, int new_reason_for_service)
{
	int board_status = ibstatus(usbdata, 0);
	const unsigned MSS = status_byte & request_service_bit;

	if ((board_status & CIC))
	{
		//printk("gpib: interface requested service while CIC\n");
		return -EINVAL;
	}

	if (MSS == 0 && new_reason_for_service)
	{
		return -EINVAL;
	}

	//if (board->interface->serial_poll_response2)
	//{
	//	board->interface->serial_poll_response2(board, status_byte, new_reason_for_service);
	//	// fall back on simpler serial_poll_response if the behavior would be the same	
	//}
	//else if (board->interface->serial_poll_response && (mss == 0 || (mss && new_reason_for_service)))
	//{
	//	board->interface->serial_poll_response(board, status_byte);
	//}
	//else
	//{
	//	return -EOPNOTSUPP;
	//}

	ni_usb_serial_poll_response(usbdata, status_byte, new_reason_for_service);

	return 0;
}

/*
 * IBSIC
 * Send IFC for at least 100 microseconds.
 *
 * NOTE:
 *      1.  Ibsic must be called prior to the first call to
 *          ibcmd in order to initialize the bus and enable the
 *          interface to leave the controller idle state.
 */
int ibsic(usb_data *usbdata, unsigned int usec_duration)
{
	if (usbdata->master == 0)
	{
	//	printf("gpib: tried to assert IFC when not system controller\n");
		return -1;
	}
	//LPSYSINFO lpSysInfo;

	//CopyRtSystemInfo( lpSysInfo);

	if (usec_duration < 100) usec_duration = 100;
	if (usec_duration > 1000)
	{
		usec_duration = 1000;
		//printk("gpib: warning, shortening long udelay\n");
	}

	//uint32_t dtime =	US_TO_KTICKS(usec_duration, lpSysInfo->KernelTickRatio);

	//printf("dtime = %d usec_duration= %d KernelTickRatio = %d \n" , dtime, usec_duration, lpSysInfo->KernelTickRatio);
	//GPIB_DPRINTK("sending interface clear\n");
	ni_usb_interface_clear(usbdata, 1);
	RtSleep(1);
	//knRtSleep(dtime);
	ni_usb_interface_clear(usbdata, 0);

	return 0;
}

void ibrsc(usb_data *usbdata, int request_control)
{
	usbdata->master = request_control != 0;

	//ni_usb_request_control
	ni_usb_request_system_control(usbdata, request_control);
}


/*
 * IBSRE
 * Send REN true if v is non-zero or false if v is zero.
 */
int ibsre(usb_data *usbdata, int enable)
{
	if (usbdata->master == 0)
	{
		//printf("gpib: tried to set REN when not system controller\n");
		return -1;
	}

	ni_usb_remote_enable(usbdata, enable);	/* set or clear REN */
	if (!enable)
		RtSleep(1);

	return 0;
}

/*
 * IBWRT
 * Write cnt bytes of data from buf to the GPIB.  The write
 * operation terminates only on I/O complete.
 *
 * NOTE:
 *      1.  Prior to beginning the write, the interface is
 *          placed in the controller standby state.
 *      2.  Prior to calling ibwrt, the intended devices as
 *          well as the interface board itself must be
 *          addressed by calling ibcmd.
 */
int ibwrt(usb_data *usbdata, uint8_t *buf, size_t cnt, int send_eoi, size_t *bytes_written,uint32_t timeOut)
{
	int ret = 0;
	int retval;

	if (cnt == 0)
	{
		//printk("gpib: ibwrt() called with zero length?\n");
		return 0;
	}

	if (usbdata->master)
	{
		retval = ibgts(usbdata);
		if (retval < 0) return retval;
	}
	//osStartTimer(board, board->usec_timeout);
	ret = ni_usb_write(usbdata, buf, cnt, send_eoi, bytes_written,timeOut);

	//if (io_timed_out(board))
	//	ret = -ETIMEDOUT;

	//osRemoveTimer(board);

	return ret;
}
/*
 * IBPAD
 * change the GPIB address of the interface board.  The address
 * must be 0 through 30.  ibonl resets the address to PAD.
 */
int ibpad(usb_data *usbdata, unsigned int addr)
{
	if (addr > 30)
	{
		//printk("gpib: invalid primary address %u\n", addr);
		return -1;
	}
	else
	{
		//usbdata-> pad = addr;
		//if (board->online)
			ni_usb_primary_address(usbdata, addr);
		//GPIB_DPRINTK("set primary addr to %i\n", board->pad);
	}
	return 0;
}


/*
 * IBSAD
 * change the secondary GPIB address of the interface board.
 * The address must be 0 through 30, or negative disables.  ibonl resets the
 * address to SAD.
 */
int ibsad(usb_data *usbdata,  int addr)
{
	if (addr > 30)
	{
		//printk("gpib: invalid secondary address %i, must be 0-30\n", addr);
		return -1;
	}
	else
	{
		//board->sad = addr;
		//if (board->online)
		//{
			if (addr >= 1)
			{
				ni_usb_secondary_address(usbdata, addr, 1);
			}
			else
			{
				ni_usb_secondary_address(usbdata, 0, 0);
			}
		//}
		//GPIB_DPRINTK("set secondary addr to %i\n", board->sad);
	}
	return 0;
}

/*
 * IBEOS
 * Set the end-of-string modes for I/O operations to v.
 *
 */
int ibeos(usb_data *usbdata, int eos, int eosflags)
{
	int retval;
	if (eosflags & ~EOS_MASK)
	{
		//printk("bad EOS modes\n");
		return -EINVAL;
	}
	else
	{
		if (eosflags & REOS)
		{
			retval = ni_usb_enable_eos(usbdata, eos, eosflags & BIN);
		}
		else
		{
			ni_usb_disable_eos(usbdata);
			retval = 0;
		}
	}
	return retval;
}

int ibstatus(usb_data *usbdata)
{
	return general_ibstatus(usbdata, 0, 0);
}

int general_ibstatus(usb_data *usbdata, int clear_mask, int set_mask)
{
	int status = 0;
	short line_status;

		status = ni_usb_update_status(usbdata, clear_mask);
		/* XXX should probably stop having drivers use TIMO bit in
		 * board->status to avoid confusion */
		status &= ~TIMO;
		/* get real SRQI status if we can */
		if (iblines(usbdata, &line_status) == 0)
		{
			if ((line_status & ValidSRQ))
			{
				if ((line_status & BusSRQ))
				{
					status |= SRQI;
				}
				else
				{
					status &= ~SRQI;
				}
			}
		}

	//if (device)
	//	if (num_status_bytes(device)) status |= RQS;

	//if (desc)
	//{
	//	if (set_mask & CMPL)
	//		atomic_set(&desc->io_in_progress, 0);
	//	else if (clear_mask & CMPL)
	//		atomic_set(&desc->io_in_progress, 1);

	//	if (atomic_read(&desc->io_in_progress))
	//		status &= ~CMPL;
	//	else
	//		status |= CMPL;
	//}
	//if (num_gpib_events(&board->event_queue))
	//	status |= EVENT;
	//else
	//	status &= ~EVENT;

		usbdata->ibsta = status;
		EROP("IbStatus: 0x%x()\n", status, 0, 0, 0);
	return status;
}

//////////////
static const int default_ppoll_usec_timeout = 2;

unsigned int timeout_to_usec(enum gpib_timeout timeout)
{
	switch (timeout)
	{
	default:
	case TNONE:
		return 0;
		break;
	case T10us:
		return 10;
		break;
	case T30us:
		return 30;
		break;
	case T100us:
		return 100;
		break;
	case T300us:
		return 300;
		break;
	case T1ms:
		return 1000;
		break;
	case T3ms:
		return 3000;
		break;
	case T10ms:
		return 10000;
		break;
	case T30ms:
		return 30000;
		break;
	case T100ms:
		return 100000;
		break;
	case T300ms:
		return 300000;
		break;
	case T1s:
		return 1000000;
		break;
	case T3s:
		return 3000000;
		break;
	case T10s:
		return 10000000;
		break;
	case T30s:
		return 30000000;
		break;
	case T100s:
		return 100000000;
		break;
	case T300s:
		return 300000000;
		break;
	case T1000s:
		return 1000000000;
		break;
	}
	return 0;
}

unsigned int ppoll_timeout_to_usec(unsigned int timeout)
{
	if (timeout == 0)
		return default_ppoll_usec_timeout;
	else
		return timeout_to_usec(timeout);
}

unsigned int usec_to_timeout(unsigned int usec)
{
	if (usec == 0) return TNONE;
	else if (usec <= 10) return T10us;
	else if (usec <= 30) return T30us;
	else if (usec <= 100) return T100us;
	else if (usec <= 300) return T300us;
	else if (usec <= 1000) return T1ms;
	else if (usec <= 3000) return T3ms;
	else if (usec <= 10000) return T10ms;
	else if (usec <= 30000) return T30ms;
	else if (usec <= 100000) return T100ms;
	else if (usec <= 300000) return T300ms;
	else if (usec <= 1000000) return T1s;
	else if (usec <= 3000000) return T3s;
	else if (usec <= 10000000) return T10s;
	else if (usec <= 30000000) return T30s;
	else if (usec <= 100000000) return T100s;
	else if (usec <= 300000000) return T300s;
	else if (usec <= 1000000000) return T1000s;

	return TNONE;
}

unsigned int usec_to_ppoll_timeout(unsigned int usec)
{
	if (usec <= default_ppoll_usec_timeout) return 0;
	else return usec_to_timeout(usec);
}


/* FIXME: NI's version returns old status byte in iberr on success.
 * Why that is at all useful, I do not know. */
int ibrsv(usb_data *usbdata, int status_byte)
{
	return ibrsv2(usbdata, status_byte, status_byte & request_service_bit);
}


