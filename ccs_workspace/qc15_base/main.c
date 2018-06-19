#include <msp430.h> 
#include <driverlib.h>

#include <rfm75.h>


/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	return 0;
}
