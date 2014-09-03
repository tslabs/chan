/*------------------------------------------------------------------------/
/  LPC1100 I2C master control module
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

#include "iic.h"
#include <string.h>


#define PCLK_I2C	36000000
#define I2C0_RATE	400000


static
volatile I2CCTRL *Ctrl0;	/* Current I2C control structure */

static
volatile uint8_t Stat0;	/* I2C status */



/*-------------------------*/
/* I2C0 Backgrownd Process */
/*-------------------------*/

void I2C_IRQHandler (void)
{
	int i;
	volatile I2CCTRL *iic = Ctrl0;
	uint8_t res = 0;
	uint16_t rct;


	if (!iic) {	/* Spurious Interrupt */
		I2C0CONCLR = 0x6C;	/* Disable I2C */
		return;
	}

	switch ((I2C0STAT >> 3) & 0x1F) {
	case 0x01:	/* Start condition has been generated */
	case 0x02:	/* Repeated start condition has been generated */
		I2C0DAT = (iic->sla << 1) | (iic->txc ? 0 : 1);		/* Set slave addres + R/W flag */
		I2C0CONCLR = 0x28;			/* Clear SI */
		break;

	case 0x03:	/* SLA+W has been transmitted, ACK received */
		I2C0DAT = *iic->txb++;	/* Set data to be sent */
		I2C0CONCLR = 0x28;		/* Clear STA+SI */
		iic->txc--;
		break;

	case 0x04:	/* SLA+W has been transmitted, ACK not received */
		rct = iic->retry;
		if (rct) {
			I2C0CONSET = 0x30;	/* Set STA+STO */
			iic->retry = rct - 1;
		} else {
			I2C0CONSET = 0x10;	/* Set STO */
			res = I2C_TIMEOUT;	/* Timeout */
		}
		I2C0CONCLR = 0x08;	/* Clear SI */
		break;

	case 0x05:	/* Data has been transmitted, ACK received */
		i = iic->txc;
		if (i) {	/* There is data to be transmitted */
			I2C0DAT = *iic->txb++;	/* Set data to be sent */
			I2C0CONCLR = 0x08;		/* Clear SI */
			i--;
		} else {	/* All data transmitted */
			if (iic->rxc) {		/* There is any data to receive */
				I2C0CONSET = 0x20;	/* Set STA to generate repeated start condition */
			} else {			/* No data to receive */
				I2C0CONSET = 0x10;	/* Set STO to generate stop condition */
				res = 1;	/* Succeeded */
			}
		}
		iic->txc = i;
		I2C0CONCLR = 0x08;		/* Clear SI */
		break;

	case 0x06:	/* Data has been transmitted, ACK not received */
		I2C0CONSET = 0x10;	/* Set STO to generate stop condition */
		I2C0CONCLR = 0x08;	/* Clear SI */
		res = I2C_ABORTED;	/* Aborted by slave */
		break;

	case 0x08:	/* SLA+R has been transmitted, ACK received */
		if (iic->rxc >= 2) {
			I2C0CONSET = 0x04;	/* Set AA */
			I2C0CONCLR = 0x08;	/* Clear SI */
		} else {
			I2C0CONCLR = 0x0C;	/* Clear SI+AA */
		}
		break;

	case 0x09:	/* SLA+R has been transmitted, ACK not received */
		I2C0CONSET = 0x10;	/* Set STO to generate stop condition */
		I2C0CONCLR = 0x08;	/* Clear SI */
		res = I2C_ABORTED;	/* Aborted by slave */
		break;

	case 0x0A:	/* Data has been received, ACK sent */
		*iic->rxb++ = I2C0DAT;
		i = iic->rxc;
		if (--i >= 2) {
			I2C0CONSET = 0x04;	/* Set AA */
			I2C0CONCLR = 0x08;	/* Clear SI */
		} else {
			I2C0CONCLR = 0x0C;	/* Clear SI+AA */
		}
		iic->rxc = i;
		break;

	case 0x0B:	/* Data has been received, ACK not sent */
		*iic->rxb = I2C0DAT;
		I2C0CONSET = 0x10;		/* Set STO to generate stop condition */
		I2C0CONCLR = 0x08;		/* Clear SI */
		res = I2C_SUCCEEDED;	/* Succeeded */
		break;

	case 0x00:	/* Bus error */
	case 0x07:	/* Arbitration lost */
	default:	/* Unknown status */
		I2C0CONCLR = 0x6C;	/* Disable I2C */
		res = I2C_ERROR;	/* Unknown error */

	}

	if (res) {	/* End of I2C transaction? */
		Ctrl0 = 0;			/* Release I2C control structure */
		iic->stat = res;	/* Set result */
		if (iic->eotfunc) iic->eotfunc(res);	/* Notify EOT if call-back function is specified */
	}
}



/*------------------------*/
/* Initialize I2C Module  */
/*------------------------*/

void i2c0_init (void)
{
	int n, m;


	IOCON_PIO0_4 = 0x00;	/* Detach I2C module from I/O pad */
	IOCON_PIO0_5 = 0x00;

	/* Put I2C slaves to idle state */
	GPIO0DIR |= (1<<4);
	GPIO0DIR &= ~(1<<5);
	for (n = 0; n < 10; n++) {
		GPIO0DATA |= (1<<4);	/* SCL=H */
		for (m = 0; m < 100; m++) GPIO0DATA;
		GPIO0DATA &= ~(1<<4);	/* SCL=L*/
		for (m = 0; m < 100; m++) GPIO0DATA;
		if (!(GPIO0DATA & (1<<5))) n = 0;	/* Check if SDA is high for 10 clock time */
	}

	__set_SYSAHBCLKCTRL(PCI2C, 1);	/* Enable I2C module */
	PRESETCTRL &= ~(1<<1);	/* Reset I2C module */
	PRESETCTRL |=  (1<<1);
	I2C0CONCLR = 0x6C;		/* Deactivate I2C */

	/* Set master bus speed */
	I2C0SCLH = PCLK_I2C / I2C0_RATE / 2;
	I2C0SCLL = PCLK_I2C / I2C0_RATE / 2;

	Stat0 = I2C_SUCCEEDED;
	__enable_irqn(I2C_IRQn);

	IOCON_PIO0_4 = 0x01;	/* Attach I2C module to I/O pad */
	IOCON_PIO0_5 = 0x01;
}



/*--------------------------*/
/* Start an I2C Transaction */
/*--------------------------*/

int i2c0_start (
	volatile I2CCTRL *ctrl	/* Pointer to the initialized I2C control structure */
)
{
	if (!ctrl->txc && !ctrl->rxc) return 0;	/* Reject if invalid parameter */
	if (Ctrl0) return 0;	/* Reject if an I2C transaction is in progress */

	Ctrl0 = ctrl;			/* Register the I2C control strucrure as current transaction */
	ctrl->stat = I2C_BUSY;	/* An I2C transaction is in progress */
	I2C0CONSET = 0x60;		/* Set I2EN+STA to generate start condition */

	return 1;
}



/*--------------------------*/
/* Abort I2C Transaction    */
/*--------------------------*/

void i2c0_abort (void)
{
	I2C0CONCLR = 0x6C;	/* Deactivate I2C */
	Ctrl0 = 0;			/* Discard I2C control structure */
}


