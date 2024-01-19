/*
*  Copyright 2006 by TenAsys Corporation.  All rights reserved.
*  
*/


#include <rt.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <usbif3.h>
#include "hid.h"

char * country_list [] = 
{ 
	"Not Supported",
	"Arabic",
	"Belgian",
	"Canadian-Bilingual",
	"Canadian-French",
	"Czech Republic",
	"Danish",
	"Finnish",
	"French",
	"German",
	"Greek",
	"Hebrew",
	"Hungary",
	"Internal(ISO)",
	"Italian",
	"Japan(Katakana)",
	"Korean",
	"Latin America",
	"Netherlands/Dutch",
	"Norwegian",
	"Persian(Farsi)",
	"Poland",
	"Portugese",
	"Russia",
	"Slovakia",
	"Spanish",
	"Swedish",
	"Swiss/French",
	"Swiss/German",
	"Switzerland",
	"Taiwan",
	"Turkish-Q",
	"UK",
	"US",
	"Yugoslavia",
	"Turkish-F",
	NULL
};

//*******************************************************************
//
//*******************************************************************
char * get_hid_country(BYTE country_code)
{
	if (country_code >= 36)
		return "Reserved";
	else
		return country_list[country_code];
}

//*******************************************************************
//
//*******************************************************************
BOOL get_hid_config_descriptor(struct usbDeviceInfo *dev, BYTE ifno,  
							   BYTE desctype, BYTE idx, void * pdesc)
{
	struct usbDescriptorHeader * hdr;
	struct usbConfigDescriptor cdesc;
	int sts;
	BYTE * hid_desc;
	BYTE ifidx = 0, epidx, hididx;

	if (sts = UsbGetDescriptor(dev->iHandle, USB_DT_CONFIG, 0, 0, &cdesc,sizeof(cdesc)))
	{
		printf("usbls: could not retrieve HID Descriptor e=%d\n", sts);
		return FALSE;
	}
	hid_desc = malloc(cdesc.wTotalLength);
	memset(hid_desc,0xa5,cdesc.wTotalLength);
	if (sts = UsbGetDescriptor(dev->iHandle, USB_DT_CONFIG, 0, 0, hid_desc,cdesc.wTotalLength))
	{
		printf("usbls: could not retrieve HID Descriptor e=%d\n", sts);
		return FALSE;
	}

	hdr = (struct usbDescriptorHeader *)hid_desc;

	while ((BYTE*)hdr < hid_desc + cdesc.wTotalLength)
	{
		if (ifidx > ifno+1)
			goto bail;

		if (hdr->bDescriptorType == USB_DT_CONFIG)
			// ヘッダをスキップします
			;
		else
			if (hdr->bDescriptorType == USB_DT_INTERFACE)
			{
				hididx = 0;
				epidx = 0;
				if (desctype == hdr->bDescriptorType && ifidx == ifno+1)
				{
					memcpy(pdesc,hdr,sizeof(struct usbInterfaceDescriptor));
					goto bail;
				}
				++ifidx;
			}
			else
				if (hdr->bDescriptorType == USB_DT_HID)
				{
					if (desctype == hdr->bDescriptorType && ifidx == ifno+1 && hididx == idx)
					{
						memcpy(pdesc,hdr,sizeof(struct usbHidDescriptor));
						goto bail;
					}     
					++hididx;
				}
				else
					if (hdr->bDescriptorType == USB_DT_ENDPOINT)
					{
						if (desctype == hdr->bDescriptorType && ifidx == ifno+1 && epidx == idx)
						{
							memcpy(pdesc,hdr,sizeof(struct usbEndpointDescriptor));
							goto bail;
						}
					}
					hdr = (struct usbDescriptorHeader *)(((BYTE*)hdr) + hdr->bLength);
	}
	free(hid_desc);
	return FALSE;

bail:
	free(hid_desc);
	return TRUE;
}



//*******************************************************************
//
//*******************************************************************
BYTE * get_hid_report_descriptor(struct usbDeviceInfo * dev, 
								 BYTE ifno, BYTE descidx, WORD len)
{
	int sts;
	BYTE * buf;
	WORD act_len;

	buf = malloc(len);
	if (buf == NULL)
		return NULL;

	memset(buf,0,len);
	sts = UsbControlXfer(dev->iHandle, USB_REQ_GET_DESCRIPTOR,
		USB_RT_READ_INTERFACE,  USB_DT_HID_REPORT << 8 | descidx,
		ifno, buf, len, &act_len, 0, USB_DEFAULT_TIMEOUT);
	if (sts)
	{
		free(buf);
		return NULL;
	}
	return buf;
}
