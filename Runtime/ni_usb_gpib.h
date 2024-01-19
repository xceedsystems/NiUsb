/***************************************************************************
                              ni_usb_gpib.h
                             -------------------

    begin                : May 2004
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

#ifndef _NI_USB_GPIB_H
#define _NI_USB_GPIB_H

#include <rt.h>
#include <stdio.h>
#include <stdlib.h>
#include <usbif3.h>
#include <strings.h>


#include "driver.h"
#include "gpib_user.h"

#include "gpib_ioctl.h"
#include "ib.h"


//typedef struct gpib_interface_struct gpib_interface_t;


/* config parameters that are only used by driver attach functions */
typedef struct
{
	/* firmware blob */
	void *init_data;
	int init_data_length;
	/* IO base address to use for non-pnp cards (set by core, driver should make local copy) */
	void *ibbase;
	/* IRQ to use for non-pnp cards (set by core, driver should make local copy) */
	unsigned int ibirq;
	/* dma channel to use for non-pnp cards (set by core, driver should make local copy) */
	unsigned int ibdma;
	/* pci bus of card, useful for distinguishing multiple identical pci cards
	 * (negative means don't care) */
	int pci_bus;
	/* pci slot of card, useful for distinguishing multiple identical pci cards
	 * (negative means don't care) */
	int pci_slot;
	/* sysfs device path of hardware to attach */
	char *device_path;
	/* serial number of hardware to attach */
	char *serial_number;
} gpib_board_config_t;

struct gpib_board_struct
{


	uint8_t *buffer;
	/* length of buffer */
	unsigned int buffer_length;
	/* Used to hold the board's current status (see update_status() above)
	 */
	volatile unsigned long status;
	/* Driver should only sleep on this wait queue.  It is special in that the
	 * core will wake this queue and set the TIMO bit in 'status' when the
	 * watchdog timer times out.
	 */

	struct device *dev;
	/* 'private_data' can be used as seen fit by the driver to
	 * store additional variables for this board */

	/* primary address */
	unsigned int pad;
	/* secondary address */
	int sad;
	/* timeout for io operations, in microseconds */
	unsigned int usec_timeout;

	unsigned int t1_nano_sec;
	/* Count that keeps track of whether board is up and running or not */
	unsigned int online;
	/* number of processes trying to autopoll */
	int autospollers;

	/* minor number for this board's device file */
	int minor;
	
	/* Flag that indicates whether board is system controller of the bus */
	unsigned master : 1;
	/* individual status bit */
	unsigned ist : 1;
	/* one means local parallel poll mode ieee 488.1 PP2 (or no parallel poll PP0),
	 * zero means remote parallel poll configuration mode ieee 488.1 PP1 */
	unsigned local_ppoll_mode : 1;
};


enum
{
	USB_VENDOR_ID_NI = 0x3923
};

enum
{
	USB_DEVICE_ID_NI_USB_B = 0x702a,
	USB_DEVICE_ID_NI_USB_B_PREINIT = 0x702b,	// device id before firmware is loaded
	USB_DEVICE_ID_NI_USB_HS = 0x709b,
	USB_DEVICE_ID_NI_USB_HS_PLUS = 0x7618,
	USB_DEVICE_ID_KUSB_488A = 0x725c,
	USB_DEVICE_ID_MC_USB_488 = 0x725d
};

enum ni_usb_device
{
	NIUSB_SUBDEV_TNT4882 = 1,
	NIUSB_SUBDEV_UNKNOWN2 = 2,
	NIUSB_SUBDEV_UNKNOWN3 = 3,
};

enum endpoint_addresses
{
	NIUSB_B_BULK_OUT_ENDPOINT = 0x2,
	NIUSB_B_BULK_IN_ENDPOINT = 0x2,
	NIUSB_B_BULK_IN_ALT_ENDPOINT = 0x6,
	NIUSB_B_INTERRUPT_IN_ENDPOINT = 0x4,
};

enum hs_enpoint_addresses
{
	NIUSB_HS_BULK_OUT_ENDPOINT = 0x2,
	NIUSB_HS_BULK_OUT_ALT_ENDPOINT = 0x6,
	NIUSB_HS_BULK_IN_ENDPOINT = 0x4,
	NIUSB_HS_BULK_IN_ALT_ENDPOINT = 0x8,
	NIUSB_HS_INTERRUPT_IN_ENDPOINT = 0x1,
};

enum hs_plus_endpoint_addresses
{
  	NIUSB_HS_PLUS_BULK_OUT_ENDPOINT = 0x1,
	NIUSB_HS_PLUS_BULK_OUT_ALT_ENDPOINT = 0x4,
	NIUSB_HS_PLUS_BULK_IN_ENDPOINT = 0x2,
	NIUSB_HS_PLUS_BULK_IN_ALT_ENDPOINT = 0x5,
	NIUSB_HS_PLUS_INTERRUPT_IN_ENDPOINT = 0x3,
};

struct ni_usb_status_block
{
	short id;
	unsigned short ibsta;
	short error_code;
	unsigned short count;
};
//
struct ni_usb_register
{
	enum ni_usb_device device;
	short address;
	unsigned short value;
};

enum ni_usb_bulk_ids
{
	NIUSB_IBCAC_ID = 0x1,
	NIUSB_UNKNOWN3_ID = 0x3, // device level function id?
	NIUSB_TERM_ID = 0x4,
	NIUSB_IBGTS_ID = 0x6,
	NIUSB_IBRPP_ID = 0x7,
	NIUSB_REG_READ_ID = 0x8,
	NIUSB_REG_WRITE_ID = 0x9,
	NIUSB_IBSIC_ID = 0xf,
	NIUSB_REGISTER_READ_DATA_START_ID = 0x34,
	NIUSB_REGISTER_READ_DATA_END_ID = 0x35,
	NIUSB_IBRD_DATA_ID = 0x36,
	NIUSB_IBRD_EXTENDED_DATA_ID = 0x37,
	NIUSB_IBRD_STATUS_ID = 0x38
};

enum ni_usb_error_codes
{
	NIUSB_NO_ERROR = 0,
	/* NIUSB_ABORTED_ERROR occurs when I/O is interrupted early by doing a NI_USB_STOP_REQUEST
		on the control endpoint. */
	NIUSB_ABORTED_ERROR = 1,
	// NIUSB_ADDRESSING_ERROR occurs when you do a board read/write as CIC but are not in LACS/TACS
	NIUSB_ADDRESSING_ERROR = 3,
	// NIUSB_EOSMODE_ERROR occurs on reads if any eos mode or char bits are set when REOS is not set.
	/* Have also seen error 4 if you try to send more than 16 command bytes at once on
	a usb-b. */
	NIUSB_EOSMODE_ERROR = 4,
	// NIUSB_NO_BUS_ERROR occurs when you try to write a command byte but there are no devices connected to the gpib bus
	NIUSB_NO_BUS_ERROR = 5,
	// NIUSB_NO_LISTENER_ERROR occurs when you do a board write as CIC with no listener
	NIUSB_NO_LISTENER_ERROR = 8,
	// get NIUSB_TIMEOUT_ERROR on board read/write timeout
	NIUSB_TIMEOUT_ERROR = 10,
};

enum ni_usb_control_requests
{
	NI_USB_STOP_REQUEST = 0x20,
	NI_USB_WAIT_REQUEST = 0x21,
	NI_USB_POLL_READY_REQUEST = 0x40,
	NI_USB_SERIAL_NUMBER_REQUEST = 0x41,
	NI_USB_HS_PLUS_0x48_REQUEST = 0x48,
	NI_USB_HS_PLUS_LED_REQUEST = 0x4b,
	NI_USB_HS_PLUS_0xf8_REQUEST = 0xf8
};

static const unsigned int ni_usb_ibsta_monitor_mask = SRQI | LOK | REM | CIC | ATN | TACS | LACS | DTAS | DCAS;

static inline int nec7210_to_tnt4882_offset(int offset)
{
	return 2 * offset;
};
static inline int ni_usb_bulk_termination(uint8_t *buffer)
{
	int i = 0;

	buffer[i++] = NIUSB_TERM_ID;
	buffer[i++] = 0x0;
	buffer[i++] = 0x0;
	buffer[i++] = 0x0;
	return i;
}

enum ni_usb_unknown3_register
{
	SERIAL_NUMBER_4_REG = 0x8,
	SERIAL_NUMBER_3_REG = 0x9,
	SERIAL_NUMBER_2_REG = 0xa,
	SERIAL_NUMBER_1_REG = 0xb,
};

static inline int ni_usb_bulk_register_write_header(uint8_t *buffer, int num_writes)
{
	int i = 0;

	buffer[i++] = NIUSB_REG_WRITE_ID;
	buffer[i++] = num_writes;
	buffer[i++] = 0x0;
	return i;
}

static inline int ni_usb_bulk_register_write(uint8_t *buffer, uint8_t device, uint8_t address, uint8_t value)
{
	int i = 0;

	buffer[i++] = device;
	buffer[i++] = address;
	buffer[i++] = value;
	return i;
}

static inline int ni_usb_bulk_register_read_header(uint8_t *buffer, int num_reads)
{
	int i = 0;

	buffer[i++] = NIUSB_REG_READ_ID;
	buffer[i++] = num_reads;
	return i;
}

static inline int ni_usb_bulk_register_read(uint8_t *buffer, int device, int address)
{
	int i = 0;

	buffer[i++] = device;
	buffer[i++] = address;
	return i;
}



int ni_usb_b_read_serial_number(usb_data* usbdata);

int ni_usb_hs_wait_for_ready(usb_data * usbdata);

int ni_usb_hs_plus_extra_init(usb_data * usbdata);

 int ni_usb_shutdown_hardware(usb_data * usbdata);

//int parse_board_ibrd_readback(const uint8_t * raw_data, ni_usb_status_block * status, uint8_t * parsed_data, int parsed_data_length, int * actual_bytes_read);

//int ni_usb_write_registers(usb_data* usbdata, const ni_usb_register * writes, int num_writes, unsigned int * ibsta);

int ni_usb_read(usb_data * usbdata, uint8_t * buffer, size_t length, int * end, size_t * bytes_read, uint32_t timeOunt);

int ni_usb_write(usb_data* usbdata, uint8_t * buffer, size_t length, int send_eoi, size_t * bytes_written, uint32_t timeOunt);

int ni_usb_command_chunk(usb_data* usbdata, uint8_t * buffer, size_t length, size_t * command_bytes_written, uint32_t timOut);

int ni_usb_command(usb_data * usbdata, uint8_t * buffer, size_t length, size_t * bytes_written, uint32_t timeOut);

int ni_usb_take_control(usb_data * usbdata, int synchronous, uint32_t timeOut);

int ni_usb_go_to_standby(usb_data * usbdata);

void ni_usb_request_system_control(usb_data * usbdata, int request_control);

void ni_usb_interface_clear(usb_data * usbdata, int assert);

int ni_usb_enable_eos(usb_data* usbdata, uint8_t eos_byte, int compare_8_bits);

void ni_usb_disable_eos(usb_data * usbdata);


unsigned int ni_usb_update_status(usb_data * usbdata, unsigned int clear_mask);

void ni_usb_stop(usb_data * usbdata);

int ni_usb_primary_address(usb_data * usbdata, unsigned int address);

int ni_usb_secondary_address(usb_data * usbdata, unsigned int address, int enable);

static unsigned long ni_usb_timeout_msecs(unsigned int usec);

static unsigned short ni_usb_timeout_code(unsigned int usec);

static void ni_usb_dump_raw_block(const uint8_t * raw_data, int length);

void ni_usb_serial_poll_response(usb_data* usbdata, uint8_t status);

uint8_t ni_usb_serial_poll_status(usb_data* usbdata);

void ni_usb_return_to_local(usb_data* usbdata);

unsigned int ni_usb_t1_delay(usb_data * usbdata, unsigned int nano_sec);

int ni_usb_init( usb_data * usbdata);

unsigned int ni_usb_t1_delay(usb_data* usbdata, unsigned int nano_sec);

//int ni_usb_write_sad(ni_usb_register* writes, int address, int enable);

int ni_usb_secondary_address(usb_data* usbdata, unsigned int address, int enable);


//typedef struct
//{
//	struct semaphore complete;
//	unsigned timed_out : 1;
//} ni_usb_urb_context_t;

// struct which defines private_data for ni_usb devices
//typedef struct
//{
//	struct usb_interface *bus_interface;
//	int bulk_out_endpoint;
//	int bulk_in_endpoint;
//	int interrupt_in_endpoint;
//	uint8_t eos_char;
//	unsigned short eos_mode;
//	unsigned int monitored_ibsta_bits;
//	struct urb *bulk_urb;
//	struct urb *interrupt_urb;
//	uint8_t interrupt_buffer[0x11];
//	struct mutex addressed_transfer_lock;
//	struct mutex bulk_transfer_lock;
//	struct mutex control_transfer_lock;
//	struct mutex interrupt_transfer_lock;
//	struct timer_list bulk_timer;
//	ni_usb_urb_context_t context;
//} ni_usb_private_t;
//

#endif	// _NI_USB_GPIB_H

int ibcac(usb_data * usbdata, int sync, int fallback_to_async);

int ibcmd(usb_data * usbdata, uint8_t * buf, size_t length, size_t * bytes_written, uint32_t timeOut);

int ibgts(usb_data * usbdata);

int iblines(usb_data * usbdata, short * lines);

int ibrd(usb_data * usbdata, uint8_t * buf, size_t length, int * end_flag, size_t * nbytes, uint32_t timeOut);

int ibrsv2(usb_data * usbdata, uint8_t status_byte, int new_reason_for_service);

int ibsic(usb_data * usbdata, unsigned int usec_duration);

void ibrsc(usb_data * usbdata, int request_control);

int ibsre(usb_data * usbdata, int enable);

int ibwrt(usb_data * usbdata, uint8_t * buf, size_t cnt, int send_eoi, size_t * bytes_written, uint32_t timeOut);

int ibpad(usb_data * usbdata, unsigned int addr);

int ibsad(usb_data * usbdata, int addr);

int ibeos(usb_data * usbdata, int eos, int eosflags);

int ibstatus(usb_data * usbdata);

unsigned int timeout_to_usec(enum gpib_timeout timeout);

unsigned int ppoll_timeout_to_usec(unsigned int timeout);

unsigned int usec_to_timeout(unsigned int usec);

unsigned int usec_to_ppoll_timeout(unsigned int usec);

int ibrsv(usb_data * usbdata, int status_byte);
