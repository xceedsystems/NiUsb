/*
 *  Copyright 2006 by TenAsys Corporation.  All rights reserved.
 *
 */


#ifndef __HID_H_
#define __HID_H_
#include <_noalign.h>

#define USB_DT_HID          0x21
#define USB_DT_HID_REPORT   0x22
#define USB_DT_HID_PHYSICAL 0x23
struct usbHidDescriptor
{
	BYTE bLength;
	BYTE bDescriptorType;
	WORD bcdHID;
	BYTE bCountryCode;
	BYTE bNumDescriptors;
	BYTE bHidDescriptorType;
	WORD wDescriptorLength;
};
#include <_restore.h>

BOOL get_hid_config_descriptor(struct usbDeviceInfo *dev, BYTE ifno, 
                                      BYTE desctype, BYTE idx, void * pdesc);
BYTE * get_hid_report_descriptor(struct usbDeviceInfo * dev, 
                                        BYTE ifno, BYTE descidx, WORD len);
char * get_hid_country(BYTE country_code);
#endif
