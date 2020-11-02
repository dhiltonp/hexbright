/**********************************************************/
/* Serial Bootloader for Atmel megaAVR Controllers        */
/*                                                        */
/* tested with ATmega8, ATmega128 and ATmega168           */
/* should work with other mega's, see code for details    */
/*                                                        */
/* ATmegaBOOT.c                                           */
/*                                                        */
/*                                                        */
/* 20090308: integrated Mega changes into main bootloader */
/*           source by D. Mellis                          */
/* 20080930: hacked for Arduino Mega (with the 1280       */
/*           processor, backwards compatible)             */
/*           by D. Cuartielles                            */
/* 20070626: hacked for Arduino Diecimila (which auto-    */
/*           resets when a USB connection is made to it)  */
/*           by D. Mellis                                 */
/* 20060802: hacked for Arduino by D. Cuartielles         */
/*           based on a previous hack by D. Mellis        */
/*           and D. Cuartielles                           */
/*                                                        */
/* Monitor and debug functions were added to the original */
/* code by Dr. Erik Lins, chip45.com. (See below)         */
/*                                                        */
/* Thanks to Karl Pitrich for fixing a bootloader pin     */
/* problem and more informative LED blinking!             */
/*                                                        */
/* For the latest version see:                            */
/* http://www.chip45.com/                                 */
/*                                                        */
/* ------------------------------------------------------ */
/*                                                        */
/* based on stk500boot.c                                  */
/* Copyright (c) 2003, Jason P. Kyle                      */
/* All rights reserved.                                   */
/* see avr1.org for original file and information         */
/*                                                        */
/* This program is free software; you can redistribute it */
/* and/or modify it under the terms of the GNU General    */
/* Public License as published by the Free Software       */
/* Foundation; either version 2 of the License, or        */
/* (at your option) any later version.                    */
/*                                                        */
/* This program is distributed in the hope that it will   */
/* be useful, but WITHOUT ANY WARRANTY; without even the  */
/* implied warranty of MERCHANTABILITY or FITNESS FOR A   */
/* PARTICULAR PURPOSE.  See the GNU General Public        */
/* License for more details.                              */
/*                                                        */
/* You should have received a copy of the GNU General     */
/* Public License along with this program; if not, write  */
/* to the Free Software Foundation, Inc.,                 */
/* 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA */
/*                                                        */
/* Licence can be viewed at                               */
/* http://www.fsf.org/licenses/gpl.txt                    */
/*                                                        */
/* Target = Atmel AVR m128,m64,m32,m16,m8,m162,m163,m169, */
/* m8515,m8535. ATmega161 has a very small boot block so  */
/* isn't supported.                                       */
/*                                                        */
/* Tested with m168                                       */
/**********************************************************/

/* Preprocessor defines that can be tweaked from the Makefile:
 *
 * F_CPU            [Hz] CPU clock frequency. 
 *                  Default: NO DEFAULT!!!
 * MAX_ERROR_COUNT  Maximum errors before giving up.
 *                  Default: 5
 * BAUD_RATE        Serial interface baud rate.
 *                  Default: 19200
 */

#ifndef F_CPU
# error "F_CPU must be set in the Makefile!"
#endif

/* Number of tolerated errors before giving up and launching application */
#ifndef MAX_ERROR_COUNT
# define MAX_ERROR_COUNT 5
#endif

/* set the UART baud rate */
#ifndef BAUD_RATE
# define BAUD_RATE   19200
#endif

/* some includes */
#include <inttypes.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

/* STK500 commands */
#include "command.h"

/* SW_MAJOR and MINOR needs to be updated from time to time to avoid
 * warning message from AVR Studio.
 * never allow AVR Studio to do an update !!!!
 */
#define HW_VER	 0x02
#define SW_MAJOR 0x01
#define SW_MINOR 0x10


/* Adjust to suit whatever pin your hardware uses to enter the bootloader */
/* ATmega128 has two UARTS so two pins are used to enter bootloader and select UART */
/* ATmega1280 has four UARTS, but for Arduino Mega, we will only use RXD0 to get code */
/* BL0... means UART0, BL1... means UART1 */
#ifdef __AVR_ATmega128__
#define BL_DDR  DDRF
#define BL_PORT PORTF
#define BL_PIN  PINF
#define BL0     PINF7
#define BL1     PINF6
#endif


/* flashing onboard LED is used to indicate that the bootloader was entered */
/* if monitor functions are included, LED goes on after monitor was entered */
#if defined HEXBRIGHT
/* The Hexbright doesn't have an "onboard" LED, but green LED in the back
 * switch is connected to pin PD5.
 */
#define LED_DDR  DDRD
#define LED_PORT PORTD
#define LED_PIN  PIND
#define LED      PIND5
#define RLED     PIND2

#elif defined __AVR_ATmega128__ || defined __AVR_ATmega1280__
/* Onboard LED is connected to pin PB7 (e.g. Crumb128, PROBOmega128, Savvy128, Arduino Mega) */
#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED_PIN  PINB
#define LED      PINB7

#else
/* Onboard LED is connected to pin PB5 in Arduino NG, Diecimila, and Duomilanuove */ 
/* other boards like e.g. Crumb8, Crumb168 are using PB2 */
#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED_PIN  PINB
#define LED      PINB5
#endif

/* If there are any boards with the LED between VCC and pin 13, you'll have
 * to compile the bootloader with -DLED_INVERTED=1
 */
#ifndef LED_INVERTED
#define LED_INVERTED 0
#endif

/* monitor functions will only be compiled when using ATmega128, due to bootblock size constraints */
#if defined(__AVR_ATmega128__) || defined(__AVR_ATmega1280__)
#define MONITOR 1
#endif

// Some models have an SPM Control Register,
// some have an SPM Control and Status Register
#if defined SPMCSR
# define SPM_CREG  SPMCSR
#else
# define SPM_CREG  SPMCR
#endif

// Similarly, this bit is called SPMEN on some devices
// and SELFPRGEN on others
#if defined(SPMEN)
#  define SPM_ENABLE_BIT  SPMEN
#elif defined(SELFPRGEN)
#  define SPM_ENABLE_BIT  SELFPRGEN
#else
#  error Cannot find SPM Enable bit definition!
#endif

#define noreturn __attribute__((noreturn))

/* data type for word/byte access */
union byte_word_union {
	uint16_t word;
	uint8_t  byte[2];
};
union byte_word_dword_union {
	uint32_t dword;
	uint16_t word;
	uint8_t  byte[4];
};
#ifdef RAMPZ
typedef union byte_word_dword_union address_t;
#else
typedef union byte_word_union address_t;
#endif
typedef union byte_word_union length_t;

/* pgmptr_t is just enough big to serve as a pointer for reading
 * PROGMEM bytes from the bootloader area.
 */
#if defined(RAMPZ) && !defined(__AVR_HAVE_LPMX__)
typedef uint_farptr_t pgmptr_t;
# define read_pgmptr	pgm_read_byte_far

#else
typedef uintptr_t pgmptr_t;
# define read_pgmptr	pgm_read_byte

#endif

/* function prototypes */
static void load_address(address_t *address);
static void universal_command(void);
static void prog_buffer(uintptr_t *, uint8_t *, uint16_t);
static void error(void);
static char check_sync(void);
static void putch(char);
static char getch(void);
static void getNch(uint8_t);
static void _putstr(pgmptr_t);
static void byte_response(uint8_t);
static void nothing_response(void);
static void flash_led(uint8_t);
#if defined MONITOR
static char echogetch(void);
static uint8_t gethex(void);
static void puthex(uint8_t);
#endif

/* use_pgmvar provides an abstraction for reading PROGMEM. */
#ifdef RAMPZ
# if defined (__AVR_HAVE_LPMX__)
#  define use_pgmvar(sym,func)					\
do {								\
	uint8_t _use_pgmvar_hh_tmp;				\
	asm volatile (						\
		"ldi	%0, hh8(%2)"	"\n\t"			\
		"out	%1, %0"		"\n\t"			\
		: "=&d" (_use_pgmvar_hh_tmp)			\
		: "I" (_SFR_IO_ADDR(RAMPZ)), "p" (&(sym))	\
	);							\
	func((pgmptr_t)(prog_char*)&(sym));			\
} while (0)

# else	/* __AVR_HAVE_LPMX__ */
#  define use_pgmvar(sym,func)					\
do {								\
	uint_farptr_t _use_pgmvar_tmp;				\
	asm (							\
		"ldi	%A0, lo8(%1)"	"\n\t"			\
		"ldi	%B0, hi8(%1)"	"\n\t"			\
		"ldi	%C0, hh8(%1)"	"\n\t"			\
		"clr	%D0"		"\n\t"			\
		: "=d" (_use_pgmvar_tmp)			\
		: "p" (&(sym))					\
	);							\
	func(_use_pgmvar_tmp);					\
} while (0)

# endif	/* __AVR_HAVE_LPMX__ */

#else  /* RAMPZ */
# define use_pgmvar(sym,func)					\
	func((pgmptr_t)(prog_char*)&(sym))

#endif	/* RAMPZ */

/* putstr provides a way to output a string symbol from PROGMEM */
#define putstr(sym)	use_pgmvar(sym,_putstr)

// Sadly, _SFR_IO_REG_P can't be used as a pre-processor condition...
// Update the following list if compilation fails in the assembler
// with a message like "I/O address out of range 0...0x3f".
#if defined (__AVR_ATmega128__) || defined (__AVR_ATmega64__)
# define SPM_CREG_ADDR		_SFR_MEM_ADDR(SPM_CREG)
# define LOAD_SPM_CREG_TO_TMP	"lds	%[tmp],%[creg]"
# define STORE_TMP_TO_SPM_CREG	"sts	%[creg],%[tmp]"
#else
# define SPM_CREG_ADDR		_SFR_IO_ADDR(SPM_CREG)
# define LOAD_SPM_CREG_TO_TMP	"in	%[tmp],%[creg]"
# define STORE_TMP_TO_SPM_CREG	"out	%[creg],%[tmp]"
#endif

/* some variables */
static prog_char signature_response[] = {
	Resp_STK_INSYNC,
	SIGNATURE_0, SIGNATURE_1, SIGNATURE_2,
	Resp_STK_OK, 0
};

static uint8_t buff[256] __attribute__((section(".noinit")));

#if defined(__AVR_ATmega128__)
static uint8_t bootuart = 0;

#elif defined __AVR_ATmega1280__
/* the mega1280 chip has four serial ports ...
 * we could eventually use any of them, or not?
 * however, we don't wanna confuse people, to avoid making a mess,
 * we will stick to RXD0, TXD0
 */
# define bootuart 1

#else
# define bootuart 0
#endif

static uint8_t timeout_on_getch = 1;

static inline void led_on(void)
{
#if LED_INVERTED
	LED_PORT &= ~_BV(LED);
#else
	LED_PORT |= _BV(LED);
#endif
}

static inline void led_off(void)
{
#if LED_INVERTED
	LED_PORT |= _BV(LED);
#else
	LED_PORT &= ~_BV(LED);
#endif
}

static inline int is_led(void)
{
#if LED_INVERTED
	return bit_is_clear(LED_PIN,LED);
#else
	return bit_is_set(LED_PIN,LED);
#endif
}

// If there is no red LED, use the standard LED instead
#ifndef RLED
# define RLED	LED
#endif

static inline void rled_on(void)
{
	LED_PORT |= _BV(RLED);
}

static inline void rled_off(void)
{
	LED_PORT &= ~_BV(RLED);
}

static inline void __attribute__((naked)) noreturn app_start(void)
{
	asm volatile ("jmp	0");
}

/* execution starts here after RESET */
void __attribute__((naked, section(".vectors"))) init(void)
{
	asm volatile ("clr __zero_reg__");

	SREG = 0;
	SP = RAMEND;

#ifndef NO_DATA
	/* Copy initialized static data. */
	extern char __data_start[], __data_end[], __data_load_start[];
	register char *data_ptr = __data_start;
	register char *data_load_ptr = __data_load_start;
	register uint8_t data_endhi;
	asm volatile (
		".global __do_copy_data"	"\n\t"
	"__do_copy_data:"			"\n\t"
		"ldi	%[endhi],hi8(%[end])"	"\n\t"
		"rjmp	2f"			"\n\t"
	"1:"					"\n\t"
#if defined (__AVR_HAVE_LPMX__)
		"lpm	r0,%a[lptr]+"		"\n\t"
#else
		"lpm"				"\n\t"
		"adiw	%[lptr],1"		"\n\t"
#endif
		"st	%a[ptr]+,r0"		"\n\t"
	"2:"					"\n\t"
		"cpi	%A[ptr],lo8(%[end])"	"\n\t"
		"cpc	%B[ptr],%[endhi]"	"\n\t"
		"brne	1b"			"\n\t"
		: [ptr] "+e" (data_ptr),
		  [lptr] "+z" (data_load_ptr),
		  [endhi] "=d" (data_endhi)
		: [start] "p" (__data_start),
		  [end] "p" (__data_end),
		  [load_start] "p" (__data_load_start)
		: "r0", "memory"
		);
#endif	/* NO_DATA */

#ifndef NO_BSS
	/* Clear BSS.  */
	extern char __bss_start[], __bss_end[];
	register char *bss_ptr = __bss_start;
	register uint8_t bss_endhi;
	asm volatile (
		".global __do_clear_bss"	"\n\t"
	"__do_clear_bss:"			"\n\t"
		"ldi	%[endhi],hi8(%[end])"	"\n\t"
		"rjmp	2f"			"\n\t"
	"1:"					"\n\t"
		"st	%a[ptr]+,__zero_reg__"	"\n\t"
	"2:"					"\n\t"
		"cpi	%A[ptr],lo8(%[end])"	"\n\t"
		"cpc	%B[ptr],%[endhi]"	"\n\t"
		"brne	1b"			"\n\t"
		: [ptr] "+e" (bss_ptr),
		  [endhi] "=d" (bss_endhi)
		: [end] "i" (__bss_end)
		: "memory"
		);
#endif	/* NO_BSS */

	/* Call main().  */
	asm volatile ("rjmp	main"	"\n\t");
}

/* main program starts here */
int noreturn main(void)
{
	address_t address;
	uint8_t ch;
	uint16_t w;

#ifdef WATCHDOG_MODS
	ch = MCUSR;
	MCUSR = 0;

	WDTCSR |= _BV(WDCE) | _BV(WDE);
	WDTCSR = 0;

	// Check if the WDT was used to reset, in which case we dont bootload
	// and skip straight to the code. woot.
	if (! (ch &  _BV(EXTRF))) // if it's a not an external reset...
		app_start();  // skip bootloader
#else
	asm volatile("nop"	"\n\t");
#endif

	/* set pin direction for bootloader pin and enable pullup */
	/* for ATmega128, two pins need to be initialized */
#ifdef __AVR_ATmega128__
	BL_DDR &= ~_BV(BL0);
	BL_DDR &= ~_BV(BL1);
	BL_PORT |= _BV(BL0);
	BL_PORT |= _BV(BL1);
#endif

#if defined __AVR_ATmega128__
	/* check which UART should be used for booting */
	if(bit_is_clear(BL_PIN, BL0)) {
		bootuart = 1;
	}
	else if(bit_is_clear(BL_PIN, BL1)) {
		bootuart = 2;
	}
#endif

	/* check if flash is programmed already, if not start bootloader anyway */
	if(pgm_read_byte_near(0x0000) != 0xFF) {

#ifdef __AVR_ATmega128__
	/* no UART was selected, start application */
	if(!bootuart) {
		app_start();
	}
#endif
	}

#ifdef __AVR_ATmega128__
	/* no bootuart was selected, default to uart 0 */
	if(!bootuart)
		bootuart = 1;
#endif


	/* initialize UART(s) depending on CPU defined */
#if defined(__AVR_ATmega128__) || defined(__AVR_ATmega1280__)
	if(bootuart == 1) {
		UBRR0L = (uint8_t)(F_CPU/(BAUD_RATE*16L)-1);
		UBRR0H = (F_CPU/(BAUD_RATE*16L)-1) >> 8;
		UCSR0A = 0x00;
		UCSR0C = 0x06;
		UCSR0B = _BV(TXEN0)|_BV(RXEN0);
	}
	if(bootuart == 2) {
		UBRR1L = (uint8_t)(F_CPU/(BAUD_RATE*16L)-1);
		UBRR1H = (F_CPU/(BAUD_RATE*16L)-1) >> 8;
		UCSR1A = 0x00;
		UCSR1C = 0x06;
		UCSR1B = _BV(TXEN1)|_BV(RXEN1);
	}
#elif defined __AVR_ATmega163__
	UBRR = (uint8_t)(F_CPU/(BAUD_RATE*16L)-1);
	UBRRHI = (F_CPU/(BAUD_RATE*16L)-1) >> 8;
	UCSRA = 0x00;
	UCSRB = _BV(TXEN)|_BV(RXEN);	
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined(__AVR_ATmega328P__)

#ifdef DOUBLE_SPEED
	UCSR0A = (1<<U2X0); //Double speed mode USART0
	UBRR0L = (uint8_t)(F_CPU/(BAUD_RATE*8L)-1);
	UBRR0H = (F_CPU/(BAUD_RATE*8L)-1) >> 8;
#else
	UBRR0L = (uint8_t)(F_CPU/(BAUD_RATE*16L)-1);
	UBRR0H = (F_CPU/(BAUD_RATE*16L)-1) >> 8;
#endif

	UCSR0B = (1<<RXEN0) | (1<<TXEN0);
	UCSR0C = (1<<UCSZ00) | (1<<UCSZ01);

	/* Enable internal pull-up resistor on pin D0 (RX), in order
	to supress line noise that prevents the bootloader from
	timing out (DAM: 20070509) */
	DDRD &= ~_BV(PIND0);
	PORTD |= _BV(PIND0);
#elif defined __AVR_ATmega8__
	/* m8 */
	UBRRH = (((F_CPU/BAUD_RATE)/16)-1)>>8; 	// set baud rate
	UBRRL = (((F_CPU/BAUD_RATE)/16)-1);
	UCSRB = (1<<RXEN)|(1<<TXEN);  // enable Rx & Tx
	UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);  // config USART; 8N1
#else
	/* m16,m32,m169,m8515,m8535 */
	UBRRL = (uint8_t)(F_CPU/(BAUD_RATE*16L)-1);
	UBRRH = (F_CPU/(BAUD_RATE*16L)-1) >> 8;
	UCSRA = 0x00;
	UCSRC = 0x06;
	UCSRB = _BV(TXEN)|_BV(RXEN);
#endif

#if defined __AVR_ATmega1280__
	/* Enable internal pull-up resistor on pin D0 (RX), in order
	to supress line noise that prevents the bootloader from
	timing out (DAM: 20070509) */
	/* feature added to the Arduino Mega --DC: 080930 */
	DDRE &= ~_BV(PINE0);
	PORTE |= _BV(PINE0);
#endif

	/* flash onboard LED to signal entering of bootloader */
	flash_led(NUM_LED_FLASHES + bootuart);

#ifdef HEXBRIGHT
	/* set red LED pin as output */
	LED_DDR |= _BV(RLED);
#endif

	/* 20050803: by DojoCorp, this is one of the parts provoking the
		 system to stop listening, cancelled from the original */
	//putch('\0');

	/* forever loop */
	for (;;) {

	/* get character from UART */
	ch = getch();

	/* A bunch of if...else if... gives smaller code than switch...case ! */

	/* Hello is anyone home ? */ 
	if(ch == Cmnd_STK_GET_SYNC) {
		nothing_response();
	}


	/* Request programmer ID */
	else if(ch == Cmnd_STK_GET_SIGN_ON) {
		if (check_sync())
			continue;

		static prog_char id[] = {
			Resp_STK_INSYNC,
			'A', 'V', 'R', ' ', 'I', 'S', 'P',
			Resp_STK_OK, 0 };
		putstr(id);
	}


	/* AVR ISP/STK500 board commands  DON'T CARE so default nothing_response */
	else if(ch == Cmnd_STK_SET_PARAMETER) {
		ch = getch();
		if (ch>0x85) getch();
		nothing_response();
	}


	/* AVR ISP/STK500 board requests */
	else if(ch == Cmnd_STK_GET_PARAMETER) {
		ch = getch();
		if (ch == Parm_STK_HW_VER)
			byte_response(HW_VER);		// Hardware version
		else if (ch == Parm_STK_SW_MAJOR)
			byte_response(SW_MAJOR);	// Software major ver
		else if (ch == Parm_STK_SW_MINOR)
			byte_response(SW_MINOR);	// Software minor ver
		else if (ch == 0x98)
			byte_response(0x03);		// Unknown but seems to be required by avr studio 3.56
		else
			byte_response(0x00);		// Covers various unnecessary responses we don't care about
	}


	/* Device Parameters  DON'T CARE, DEVICE IS FIXED  */
	else if(ch == Cmnd_STK_SET_DEVICE) {
		getNch(20);
		nothing_response();
	}


	/* Parallel programming stuff  DON'T CARE  */
	else if(ch == Cmnd_STK_SET_DEVICE_EXT) {
		getNch(5);
		nothing_response();
	}


	/* P: Enter programming mode  */
	/* R: Erase device, don't care as we will erase one page at a time anyway.  */
	else if(ch == Cmnd_STK_ENTER_PROGMODE || ch == Cmnd_STK_CHIP_ERASE) {
		nothing_response();
	}


	/* Leave programming mode  */
	else if(ch == Cmnd_STK_LEAVE_PROGMODE) {
		nothing_response();
#ifdef WATCHDOG_MODS
		// autoreset via watchdog (sneaky!)
		WDTCSR = _BV(WDE);
		while (1); // 16 ms
#endif
	}

	/* Set address. */
	else if(ch == Cmnd_STK_LOAD_ADDRESS) {
		load_address(&address);
	}

	else if(ch == Cmnd_STK_CHECK_AUTOINC) {
		nothing_response();
	}

	/* Universal SPI programming command.  */
	else if(ch == Cmnd_STK_UNIVERSAL) {
		universal_command();
	}

	/* Write memory, length is big endian and is in bytes  */
	else if(ch == Cmnd_STK_PROG_PAGE) {
		length_t length;
		uint8_t memtype;
		length.byte[1] = getch();
		length.byte[0] = getch();
		memtype = getch();
		for (w=0;w<length.word;w++) {
			buff[w] = getch();	                        // Store data in buffer, can't keep up with serial data stream whilst programming pages
		}

		if (check_sync())
			continue;

		rled_on();
		if (memtype == 'E') {		                //Write to EEPROM one byte at a time
			while (length.word) {
				eeprom_write_byte((void *)address.word,buff[w]);
				address.word++;
				length.word--;
			}
		} else {
			//Write to FLASH one page at a time
#ifdef RAMPZ
			RAMPZ = address.byte[2];
#endif
			if ((length.byte[0] & 0x01)) length.word++;	//Even up an odd number of bytes
			eeprom_busy_wait();			//Wait for previous EEPROM writes to complete
			prog_buffer(&address.word, buff, length.word);
			/* Should really add a wait for RWW section to be enabled, don't actually need it since we never */
			/* exit the bootloader without a power cycle anyhow */
		}
		rled_off();
		putch(Resp_STK_INSYNC);
		putch(Resp_STK_OK);
	}


	/* Read memory block mode, length is big endian.  */
	else if(ch == Cmnd_STK_READ_PAGE) {
		length_t length;
		uint8_t memtype;
		length.byte[1] = getch();
		length.byte[0] = getch();
		memtype = getch();

		if (check_sync())
			continue;

		putch(Resp_STK_INSYNC);
		led_on();
		while (length.word) {
			if (memtype == 'E') {
				// Byte access EEPROM read
				putch(eeprom_read_byte((void *)address.word));
			}
			else {
#ifdef RAMPZ
				putch(pgm_read_byte_far(address.dword));
#else
				putch(pgm_read_byte_near(address.word));
#endif
			}
			address.word++;
			length.word--;
		}
		led_off();
		putch(Resp_STK_OK);
	}

	/* Get device signature bytes  */
	else if(ch == Cmnd_STK_READ_SIGN) {
		if (check_sync())
			continue;

		putstr(signature_response);
	}


	/* Read oscillator calibration byte */
	else if(ch == Cmnd_STK_READ_OSCCAL) {
		byte_response(0x00);
	}


#if defined MONITOR 

	/* here come the extended monitor commands by Erik Lins */

	/* check for three times exclamation mark pressed */
	else if(ch=='!') {
		ch = getch();
		if(ch=='!') {
		ch = getch();
		if(ch=='!') {
#if defined(__AVR_ATmega128__) || defined(__AVR_ATmega1280__)
			uint16_t extaddr;
#endif
			uint8_t addrl, addrh;

#ifdef CRUMB128
			static prog_char welcome[] = "ATmegaBOOT / Crumb128 - (C) J.P.Kyle, E.Lins - 050815\n\r";
#elif defined PROBOMEGA128
			static prog_char welcome[] = "ATmegaBOOT / PROBOmega128 - (C) J.P.Kyle, E.Lins - 050815\n\r";
#elif defined SAVVY128
			static prog_char welcome[] = "ATmegaBOOT / Savvy128 - (C) J.P.Kyle, E.Lins - 050815\n\r";
#elif defined HEXBRIGHT
			static prog_char welcome[] = "ATmegaBOOT / Hexbright - (C) ptesarik - 130617\n\r";
#elif defined __AVR_ATmega1280__ 
			static prog_char welcome[] = "ATmegaBOOT / Arduino Mega - (C) Arduino LLC - 090930\n\r";
#else
			static prog_char welcome[] = "ATmegaBOOT / UNKNOWN\n\r";
#endif

			/* No timeout - use 'j' to leave monitor */
			timeout_on_getch = 0;

			/* turn on LED */
			LED_DDR |= _BV(LED);
			led_on();

			/* print a welcome message and command overview */
			putstr(welcome);

			/* test for valid commands */
			for(;;) {
				static prog_char prompt[] = "\n\r: ";
				putstr(prompt);

				ch = echogetch();

				/* toggle LED */
				if(ch == 't') {
					if(!is_led()) {
						led_on();
						putch('1');
					} else {
						led_off();
						putch('0');
					}
				} 

				/* read byte from address */
				else if(ch == 'r') {
					ch = echogetch();
					addrh = gethex();
					addrl = gethex();
					putch('=');
					ch = *(uint8_t *)((addrh << 8) + addrl);
					puthex(ch);
				}

				/* write a byte to address  */
				else if(ch == 'w') {
					ch = echogetch();
					addrh = gethex();
					addrl = gethex();
					ch = echogetch();
					ch = gethex();
					*(uint8_t *)((addrh << 8) + addrl) = ch;
				}

				/* read from uart and echo back */
				else if(ch == 'u') {
					/* re-enable timeout */
					timeout_on_getch = 1;
					for(;;)
						echogetch();
				}
#if defined(__AVR_ATmega128__) || defined(__AVR_ATmega1280__)
				/* external bus loop  */
				else if(ch == 'b') {
					static prog_char bus[] = "bus";
					putstr(bus);
					MCUCR = 0x80;
					XMCRA = 0;
					XMCRB = 0;
					extaddr = 0x1100;
					for(;;) {
						ch = *(volatile uint8_t *)extaddr;
						if(++extaddr == 0) {
							extaddr = 0x1100;
						}
					}
				}
#endif

				else if(ch == 'j') {
					app_start();
				}

			} /* end of monitor functions */

		}
		}
	}
	/* end of monitor */
#endif
	else {
		if (!check_sync())
			putch(Resp_STK_UNKNOWN);
		error();
	}

	} /* end of forever loop */

}

// Perhaps extra address bytes may be added in future to support > 128kB FLASH.
// This might explain why little endian was used here, although big endian
// is used everywhere else.
static void load_address(address_t *address)
{
	address->byte[0] = getch();
	address->byte[1] = getch();

	/* Both memory types are word-addressable, but byte addresses
	 * are used for EEAR and the Z register in lpm/spm, so convert
	 * the word address into a byte address here.
	 */
	asm (
		"lsl	%A[addr]"	"\n\t"
		"rol	%B[addr]"	"\n\t"
#ifdef RAMPZ
		"clr	%C[addr]"	"\n\t"
		"rol	%C[addr]"	"\n\t"
		"clr	%D[addr]"	"\n\t"
		: [addr] "+r" (address->dword)
#else
		: [addr] "+r" (address->word)
#endif
	);
	nothing_response();
}

static void universal_command(void)
{
	uint8_t byte1, byte2, byte3, byte4;
	byte1 = getch();
	byte2 = getch();
	byte3 = getch();
	byte4 = getch();
	if (byte1 == 0x30 && byte2 == 0x00) {
		// Read Signature Byte
#define sig_fn(ptr)	byte_response(read_pgmptr(ptr + byte3))
		use_pgmvar(signature_response[1], sig_fn);
#undef sig_fn
	} else if ( (byte1 & ~0x08) == 0x50 &&
		    (byte2 & ~0x08) == 0x00) {
		// Read Lock/LFUSE/HFUSE/EFUSE
		uint16_t addr = 0;
		asm (
			"bst	%[byte1],3"	"\n\t"
			"bld	%A[addr],0"	"\n\t"
			"bst	%[byte2],3"	"\n\t"
			"bld	%A[addr],1"	"\n\t"
			"ldi	%[tmp],%[cval]"	"\n\t"
			STORE_TMP_TO_SPM_CREG	"\n\t"
#if defined (__AVR_HAVE_LPMX__)
			"lpm	%[tmp],Z"	"\n\t"
#else
			"lpm"			"\n\t"
			"mov	%[tmp],r0"	"\n\t"
#endif
			: [addr] "+z" (addr),
			  [tmp] "=r" (byte4)
			: [byte1] "r" (byte1),
			  [byte2] "r" (byte2),
			  [creg] "i" (SPM_CREG_ADDR),
			  [cval] "i" (_BV(BLBSET) | _BV(SPM_ENABLE_BIT))
#if !defined(__AVR_HAVE_LPMX__)
			: "r0"
#endif
		);
		byte_response(byte4);
	} else {
		byte_response(0x00);
	}
}

static void prog_buffer(uintptr_t *address, uint8_t *buffer, uint16_t length)
{
	uint8_t page_word_count = 0;
	uint8_t tmp;
	asm volatile (
		//Main loop, repeat for number of words in block
	"length_loop:"			"\n\t"

		//If page_word_count=0 then erase page
		"cpi	%[wcnt],0x00"	"\n\t"
		"brne	no_page_erase"	"\n\t"

		//Wait for previous spm to complete
	"wait_spm1:"			"\n\t"
		LOAD_SPM_CREG_TO_TMP	"\n\t"
		"sbrc	%[tmp],%[spmen]""\n\t"
		"rjmp	wait_spm1"	"\n\t"

		//Erase page pointed to by Z
		"ldi	%[tmp],0x03"	"\n\t"
		STORE_TMP_TO_SPM_CREG	"\n\t"
		"spm"			"\n\t"
#ifdef __AVR_ATmega163__
		".word 0xFFFF"		"\n\t"
		"nop"			"\n\t"
#endif
		//Wait for previous spm to complete
	"wait_spm2:"			"\n\t"
		LOAD_SPM_CREG_TO_TMP	"\n\t"
		"sbrc	%[tmp],%[spmen]""\n\t"
		"rjmp	wait_spm2"	"\n\t"

		//Re-enable RWW section
		"ldi	%[tmp],0x11"	"\n\t"
		STORE_TMP_TO_SPM_CREG	"\n\t"
		"spm"			"\n\t"
#ifdef __AVR_ATmega163__
		".word 0xFFFF"		"\n\t"
		"nop"			"\n\t"
#endif
	"no_page_erase:"		"\n\t"
		//Write 2 bytes into page buffer
		"ld	r0,X+"		"\n\t"
		"ld	r1,X+"		"\n\t"

		//Wait for previous spm to complete
	"wait_spm3:"			"\n\t"
		LOAD_SPM_CREG_TO_TMP	"\n\t"
		"sbrc	%[tmp],%[spmen]""\n\t"
		"rjmp	wait_spm3"	"\n\t"

		//Load r0,r1 into FLASH page buffer
		"ldi	%[tmp],0x01"	"\n\t"
		STORE_TMP_TO_SPM_CREG	"\n\t"
		"spm"			"\n\t"

		"inc	%[wcnt]"	"\n\t"	//page_word_count++
		"cpi   %[wcnt],%[PGSZ]"	"\n\t"
		"brlo	same_page"	"\n\t"	//Still same page in FLASH

		//New page, write current one first
	"write_page:"			"\n\t"
		"clr	%[wcnt]"	"\n\t"

		//Wait for previous spm to complete
	"wait_spm4:"			"\n\t"
		LOAD_SPM_CREG_TO_TMP	"\n\t"
		"sbrc	%[tmp],%[spmen]""\n\t"
		"rjmp	wait_spm4"	"\n\t"

#ifdef __AVR_ATmega163__
		// m163 requires Z6:Z1 to be zero during page write
		"andi	%A[addr],0x80"	"\n\t"
#endif
		//Write page pointed to by Z
		"ldi	%[tmp],0x05"	"\n\t"
		STORE_TMP_TO_SPM_CREG	"\n\t"
		"spm"			"\n\t"
#ifdef __AVR_ATmega163__
		".word 0xFFFF"		"\n\t"
		"nop"			"\n\t"
		// recover Z6:Z1 state after page write (had to be zero during write)
		"ori	%A[addr],0x7E"	"\n\t"
#endif
		//Wait for previous spm to complete
	"wait_spm5:"			"\n\t"
		LOAD_SPM_CREG_TO_TMP	"\n\t"
		"sbrc	%[tmp],%[spmen]""\n\t"
		"rjmp	wait_spm5"	"\n\t"

		//Re-enable RWW section
		"ldi	%[tmp],0x11"	"\n\t"
		STORE_TMP_TO_SPM_CREG	"\n\t"
		"spm"			"\n\t"
#ifdef __AVR_ATmega163__
		".word 0xFFFF"		"\n\t"
		"nop"			"\n\t"
#endif
	"same_page:"			"\n\t"
		"adiw	%[addr],2"	"\n\t"	//Next word in FLASH
		"sbiw	%[length],2"	"\n\t"	//length-2
		"breq	final_write"	"\n\t"	//Finished
		"rjmp	length_loop"	"\n\t"
	"final_write:"			"\n\t"
		"cpi	%[wcnt],0"	"\n\t"
		"breq	block_done"	"\n\t"
		"adiw	%[length],2"	"\n\t"	//length+2, fool above check on length after short page write
		"rjmp	write_page"	"\n\t"
	"block_done:"			"\n\t"
		"clr	__zero_reg__"	"\n\t"	//restore zero register
		: [tmp] "=d" (tmp),
		  [wcnt] "+d" (page_word_count),
		  [buff] "+x" (buffer),
		  [addr] "+z" (*address),
		  [length] "+w" (length)
		: [PGSZ] "M" (SPM_PAGESIZE / 2),
		  [creg] "i" (SPM_CREG_ADDR),
		  [spmen] "i" (SPM_ENABLE_BIT)
		: "r0"
	);
}

static void error(void) {
#ifdef GPIOR0
# define error_count (*(uint8_t*)&GPIOR0)
#else
	static uint8_t error_count;
#endif

	if (++error_count == MAX_ERROR_COUNT)
		app_start();
}

static char check_sync(void)
{
	if (getch() != Sync_CRC_EOP) {
		putch(Resp_STK_NOSYNC);
		error();
		return 1;
	}
	return 0;
}

static __attribute__((noinline)) void putch(char ch)
{
#if defined(__AVR_ATmega128__) || defined(__AVR_ATmega1280__)
	if(bootuart == 1) {
		while (!(UCSR0A & _BV(UDRE0)));
		UDR0 = ch;
	}
	else if (bootuart == 2) {
		while (!(UCSR1A & _BV(UDRE1)));
		UDR1 = ch;
	}
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined(__AVR_ATmega328P__)
	while (!(UCSR0A & _BV(UDRE0)));
	UDR0 = ch;
#else
	/* m8,16,32,169,8515,8535,163 */
	while (!(UCSRA & _BV(UDRE)));
	UDR = ch;
#endif
}


static __attribute__((noinline)) char getch(void)
{
	uint32_t count = 0;
#if defined(__AVR_ATmega128__) || defined(__AVR_ATmega1280__)
	if(bootuart == 1) {
		while(!(UCSR0A & _BV(RXC0))) {
			/* 20060803 DojoCorp:: Addon coming from the previous Bootloader*/               
			/* HACKME:: here is a good place to count times*/
			count += timeout_on_getch;
			if (count > MAX_TIME_COUNT)
				app_start();
			}

			return UDR0;
		}
	else if(bootuart == 2) {
		while(!(UCSR1A & _BV(RXC1))) {
			/* 20060803 DojoCorp:: Addon coming from the previous Bootloader*/               
			/* HACKME:: here is a good place to count times*/
			count += timeout_on_getch;
			if (count > MAX_TIME_COUNT)
				app_start();
		}

		return UDR1;
	}
	return 0;
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined(__AVR_ATmega328P__)
	while(!(UCSR0A & _BV(RXC0))){
		/* 20060803 DojoCorp:: Addon coming from the previous Bootloader*/               
		/* HACKME:: here is a good place to count times*/
		count += timeout_on_getch;
		if (count > MAX_TIME_COUNT)
			app_start();
	}
	return UDR0;
#else
	/* m8,16,32,169,8515,8535,163 */
	while(!(UCSRA & _BV(RXC))){
		/* 20060803 DojoCorp:: Addon coming from the previous Bootloader*/               
		/* HACKME:: here is a good place to count times*/
		count += timeout_on_getch;
		if (count > MAX_TIME_COUNT)
			app_start();
	}
	return UDR;
#endif
}


static void getNch(uint8_t count)
{
	while(count--)
		getch();
}


static char read_str_inc(pgmptr_t *p)
{
	char result;
#if defined (__AVR_HAVE_LPMX__)
	asm (
# if defined RAMPZ
		"elpm	%0, Z+"		"\n\t"
# else
		"lpm	%0, Z+"		"\n\t"
# endif
		: "=r" (result), "+z" (*p)
	);
#else  /* __AVR_HAVE_LPMX__ */
	result = read_pgmptr(*p);
	++*p;
#endif	/* __AVR_HAVE_LPMX__ */
	return result;
}

static void _putstr(pgmptr_t s)
{
	char c;
	while ( (c = read_str_inc(&s)) )
		putch(c);
}

static void byte_response(uint8_t val)
{
	if (check_sync())
		return;

	putch(Resp_STK_INSYNC);
	putch(val);
	putch(Resp_STK_OK);
}


static void nothing_response(void)
{
	if (check_sync())
		return;

	putch(Resp_STK_INSYNC);
	putch(Resp_STK_OK);
}

static void flash_led(uint8_t count)
{
	/* set LED pin as output */
	LED_DDR |= _BV(LED);

	while (count--) {
		led_on();
		_delay_ms(100);
		led_off();
		_delay_ms(100);
	}
}


// gethex/puthex is used only with MONITOR
// and produces a gcc warning otherwise.
#if defined MONITOR

static char echogetch(void)
{
	char ch = getch();
	putch(ch);
	return ch;
}

static uint8_t gethexnib(void)
{
	char a;
	a = echogetch();
	if(a >= 'a') {
		return (a - 'a' + 0x0a);
	} else if(a >= '0') {
		return(a - '0');
	}
	return a;
}

static uint8_t gethex(void)
{
	return (gethexnib() << 4) + gethexnib();
}

static void puthex(uint8_t ch)
{
	uint8_t ah;

	ah = ch >> 4;
	if(ah >= 0x0a) {
		ah = ah - 0x0a + 'a';
	} else {
		ah += '0';
	}

	ch &= 0x0f;
	if(ch >= 0x0a) {
		ch = ch - 0x0a + 'a';
	} else {
		ch += '0';
	}

	putch(ah);
	putch(ch);
}

#endif	/* MONITOR */

/* end of file ATmegaBOOT.c */
