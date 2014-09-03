/*------------------------------------------------------------------------/
/  RTC8564/DS1388 RTC controls                                            /
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2011, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/

#include "rtc.h"
#include "iic.h"

#define RTC_DS1388	1	/* 0:RTC8564, 1:DS1388 */


/*-------------------------------------------------*/
/* RTC functions                                   */


int rtc_gettime (
	RTC *rtc
)
{
	volatile I2CCTRL ct;
	BYTE buf[8];

#if RTC_DS1388
	ct.sla = 0x68;				/* DS1388 device address */
	ct.retry = 0;				/* No retry */
	buf[0] = 0;					/* Read start register */
	ct.txb = buf; ct.txc = 1;
	ct.rxb = buf; ct.rxc = 8;	/* Read 8 bytes */
	ct.eotfunc = 0;				/* No EOT function */
	i2c0_start(&ct);			/* Start I2C transaction */
	while (!ct.stat) ;			/* Wait for end of the I2C transaction */

	if (ct.stat != I2C_SUCCEEDED || (buf[7] & 0x20)) {	/* Error or Data volatiled */
		rtc->sec = 0;
		rtc->min = 0;
		rtc->hour = 0;
		rtc->wday = 0;
		rtc->mday = 1;
		rtc->month = 1;
		rtc->year = 2011;
		return 0;
	}

	/* Data is valid */
	rtc->sec = (buf[0] & 0x0F) + ((buf[0] >> 4) & 7) * 10;
	rtc->min = (buf[1] & 0x0F) + (buf[1] >> 4) * 10;
	rtc->hour = (buf[2] & 0x0F) + ((buf[2] >> 4) & 3) * 10;
	rtc->wday = (buf[3] & 0x07);
	rtc->mday = (buf[4] & 0x0F) + ((buf[4] >> 4) & 3) * 10;
	rtc->month = (buf[5] & 0x0F) + ((buf[5] >> 4) & 1) * 10;
	rtc->year = 2000 + (buf[6] & 0x0F) + (buf[6] >> 4) * 10;
	return 1;

#else
	ct.sla = 0x51;				/* RTC8564 device address */
	ct.retry = 0;				/* No retry */
	buf[0] = 2;					/* Read start register */
	ct.txb = buf; ct.txc = 1;
	ct.rxb = buf; ct.rxc = 7;	/* Read 7 bytes */
	ct.eotfunc = 0;				/* No EOT function */
	i2c0_start(&ct);			/* Start I2C transaction */
	while (!ct.stat) ;			/* Wait for end of the I2C transaction */

	if (ct.stat != I2C_SUCCEEDED || (buf[0] & 0x80)) {	/* Error or Data volatiled */
		rtc->sec = 0;
		rtc->min = 0;
		rtc->hour = 0;
		rtc->wday = 0;
		rtc->mday = 1;
		rtc->month = 1;
		rtc->year = 2011;
		return 0;
	}

	/* Data is valid */
	rtc->sec = (buf[0] & 0x0F) + ((buf[0] >> 4) & 7) * 10;
	rtc->min = (buf[1] & 0x0F) + ((buf[1] >> 4) & 7) * 10;
	rtc->hour = (buf[2] & 0x0F) + ((buf[2] >> 4) & 3) * 10;
	rtc->mday = (buf[3] & 0x0F) + ((buf[4] >> 4) & 3) * 10;
	rtc->wday = (buf[4] & 0x07);
	rtc->month = (buf[5] & 0x0F) + ((buf[5] >> 4) & 1) * 10;
	rtc->year = 2000 + (buf[6] & 0x0F) + (buf[6] >> 4) * 10;
	return 1;
#endif
}




int rtc_settime (
	const RTC *rtc
)
{

	volatile I2CCTRL ct;
	BYTE buf[10];


#if RTC_DS1388
	buf[0] = 0;
	buf[1] = rtc->sec / 10 * 16 + rtc->sec % 10;
	buf[2] = rtc->min / 10 * 16 + rtc->min % 10;
	buf[3] = rtc->hour / 10 * 16 + rtc->hour % 10;
	buf[4] = rtc->wday & 7;
	buf[5] = rtc->mday / 10 * 16 + rtc->mday % 10;
	buf[6] = rtc->month / 10 * 16 + rtc->month % 10;
	buf[7] = (rtc->year - 2000) / 10 * 16 + (rtc->year - 2000) % 10;
	buf[8] = 0;

	ct.sla = 0x68;					/* DS1388 address */
	ct.retry = 0;					/* No retry */
	ct.txb = buf; ct.txc = 9;
	ct.rxc = 0;
	ct.eotfunc = 0;
	i2c0_start(&ct);
	return (ct.stat == I2C_SUCCEEDED) ? 1 : 0;

#else

	buf[0] = 0;
	buf[1] = 0x20;
	buf[2] = 0;
	buf[3] = rtc->sec / 10 * 16 + rtc->sec % 10;
	buf[4] = rtc->min / 10 * 16 + rtc->min % 10;
	buf[5] = rtc->hour / 10 * 16 + rtc->hour % 10;
	buf[6] = rtc->wday & 7;
	buf[7] = rtc->mday / 10 * 16 + rtc->mday % 10;
	buf[8] = rtc->month / 10 * 16 + rtc->month % 10;
	buf[9] = (rtc->year - 2000) / 10 * 16 + (rtc->year - 2000) % 10;

	ct.sla = 0x68;					/* DS1388 address */
	ct.retry = 0;					/* No retry */
	ct.txb = buf; ct.txc = 10;
	ct.rxc = 0;
	ct.eotfunc = 0;
	i2c0_start(&ct);
	return (ct.stat == I2C_SUCCEEDED) ? 1 : 0;
	buf[1] = 0;
	ct.txb = buf; ct.txc = 2;
	i2c0_start(&ct);
	return (ct.stat == I2C_SUCCEEDED) ? 1 : 0;
#endif
}


