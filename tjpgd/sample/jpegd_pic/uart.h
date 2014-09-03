#ifndef _COMMFUNC
#define _COMMFUNC

#include "integer.h"

void uart_init (void);
int uart_test (void);
void uart_putc (BYTE);
BYTE uart_getc (void);

#define __kbhit()	uart_test()
#define __getch()	uart_getc()

#endif

