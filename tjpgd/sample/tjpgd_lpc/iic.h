#ifndef I2C_DEFINED
#define I2C_DEFINED
#include "LPC1100.h"


/* I2C transaction work area */
typedef struct {
	uint8_t stat;			/* I2C transaction status */
	uint8_t sla;			/* Slave address (0..127) */
	uint16_t retry;			/* Number of retries for slave selection (0..) */
	const uint8_t *txb;		/* Data to be transmitted */
	int txc;				/* Number of bytes to tansmit (if zero, no write transaction) */
	uint8_t *rxb;			/* Data buffer for received data */
	int rxc;				/* Number of bytes to receive after write transaction (if zero, no read transaction) */
	void (*eotfunc)(uint8_t);/* Callback function to notify end of transaction (0:not used) */
} I2CCTRL;

/* Status code (I2CCTRL.stat) */
enum {
	I2C_BUSY = 0,	/* An I2C transaction is in progress */
	I2C_SUCCEEDED,	/* Transaction succeeded */
	I2C_TIMEOUT,	/* Failed due to slave not responded to addressing */
	I2C_ABORTED,	/* Failed due to slave not responded to sent data */
	I2C_ERROR		/* Failed due to bus error, arbitration failed or unknown error */
};


/* I2C control module API */
void i2c0_init (void);				/* Initialize I2C module */
int i2c0_start (volatile I2CCTRL*);	/* Start an I2C transaction */
void i2c0_abort (void);				/* Abort an I2C transaction in progress */

#endif
