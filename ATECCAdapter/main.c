/* Name: main.c
 * Project: hid-data, example how to use HID for data transfer
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-11
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

/*
This example should run on most AVRs with only little changes. No special
hardware resources except INT0 are used. You may have to change usbconfig.h for
different I/O pins for USB. Please note that USB D+ must be the INT0 pin, or
at least be connected to INT0 as well.
*/

#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "vusb/usbdrv.h"

#include <string.h>

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

uint8_t lastTimer0Value;

#define KIT_BUF_SIZE 64

PROGMEM const char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x06, 0x00, 0xff,              // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    //   COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Vendor Usage 1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x40,                    //   REPORT_COUNT (64)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x82, 0x02, 0x01,              //   INPUT (Data,Var,Abs,Buf)
    0x09, 0x01,                    //   USAGE (Vendor Usage 1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x40,                    //   REPORT_COUNT (64)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0xc0                           // END_COLLECTION
};
/* Since we define only one feature report, we don't use report-IDs (which
 * would be the first byte of the report). The entire report consists of 128
 * opaque data bytes.
 */

/* The following variables store the status of the current data transfer */

static char kitRequest[KIT_BUF_SIZE];
static char kitReply[KIT_BUF_SIZE];
static uint8_t kitPtr;

static uint8_t idleRate;

/* ------------------------------------------------------------------------- */

/* usbFunctionWrite() is called when the host sends a chunk of data to the
 * device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar usbFunctionWrite(uchar *data, uchar len) {
    if (kitPtr + len > KIT_BUF_SIZE) {
	    len = KIT_BUF_SIZE - kitPtr;
    }
    
    memcpy(kitRequest + kitPtr, data, len);
    kitPtr += len;

    return kitPtr == KIT_BUF_SIZE;
}

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
	usbRequest_t* rq = (void*) data;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_CLASS) {
		return 0;
	}

	switch (rq->bRequest) {
		case USBRQ_HID_SET_REPORT:
			if (rq->wLength.word != KIT_BUF_SIZE) {
				return 0;
			}

			kitPtr = 0;
			return USB_NO_MSG;  /* use usbFunctionWrite() to receive data from host */

		case USBRQ_HID_GET_REPORT:
			PORTB ^= 0x02;
			usbMsgPtr = kitRequest;
			return KIT_BUF_SIZE;

		case USBRQ_HID_GET_IDLE:
			usbMsgPtr = &idleRate;
			return 1;

        case USBRQ_HID_SET_IDLE:
			idleRate = rq->wValue.bytes[1];
			return 0;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

int main(void)
{
	uchar   i;
	DDRB |= 0x02;
	PORTB |= 0x02;
    wdt_enable(WDTO_1S);
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    sei();
    for(;;){                /* main event loop */
        wdt_reset();
        usbPoll();
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
