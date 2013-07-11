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
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

/* the current avr-libc eeprom functions do not support the ATmega168 */
/* own eeprom write/read functions are used instead */
#if !defined(__AVR_ATmega168__) || !defined(__AVR_ATmega328P__)
#include <avr/eeprom.h>
#endif

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

/* define various device id's */
/* manufacturer byte is always the same */
#define SIG1	0x1E	// Yep, Atmel is the only manufacturer of AVR micros.  Single source :(

#if defined __AVR_ATmega1280__
#define SIG2	0x97
#define SIG3	0x03
#define PAGE_SIZE	0x80U	//128 words

#elif defined __AVR_ATmega1281__
#define SIG2	0x97
#define SIG3	0x04
#define PAGE_SIZE	0x80U	//128 words

#elif defined __AVR_ATmega128__
#define SIG2	0x97
#define SIG3	0x02
#define PAGE_SIZE	0x80U	//128 words

#elif defined __AVR_ATmega64__
#define SIG2	0x96
#define SIG3	0x02
#define PAGE_SIZE	0x80U	//128 words

#elif defined __AVR_ATmega32__
#define SIG2	0x95
#define SIG3	0x02
#define PAGE_SIZE	0x40U	//64 words

#elif defined __AVR_ATmega16__
#define SIG2	0x94
#define SIG3	0x03
#define PAGE_SIZE	0x40U	//64 words

#elif defined __AVR_ATmega8__
#define SIG2	0x93
#define SIG3	0x07
#define PAGE_SIZE	0x20U	//32 words

#elif defined __AVR_ATmega88__
#define SIG2	0x93
#define SIG3	0x0a
#define PAGE_SIZE	0x20U	//32 words

#elif defined __AVR_ATmega168__
#define SIG2	0x94
#define SIG3	0x06
#define PAGE_SIZE	0x40U	//64 words

#elif defined __AVR_ATmega328P__
#define SIG2	0x95
#define SIG3	0x0F
#define PAGE_SIZE	0x40U	//64 words

#elif defined __AVR_ATmega162__
#define SIG2	0x94
#define SIG3	0x04
#define PAGE_SIZE	0x40U	//64 words

#elif defined __AVR_ATmega163__
#define SIG2	0x94
#define SIG3	0x02
#define PAGE_SIZE	0x40U	//64 words

#elif defined __AVR_ATmega169__
#define SIG2	0x94
#define SIG3	0x05
#define PAGE_SIZE	0x40U	//64 words

#elif defined __AVR_ATmega8515__
#define SIG2	0x93
#define SIG3	0x06
#define PAGE_SIZE	0x20U	//32 words

#elif defined __AVR_ATmega8535__
#define SIG2	0x93
#define SIG3	0x08
#define PAGE_SIZE	0x20U	//32 words

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

/* Response begin/end markers */
#define RESPBEG	0x14
#define STRBEG	"\x14"
#define RESPEND	0x10
#define STREND	"\x10"

#define noreturn __attribute__((noreturn))

/* function prototypes */
static void error(void);
static void putch(char);
static char echogetch(void);
static char getch(void);
static void getNch(uint8_t);
static void byte_response(uint8_t);
static void nothing_response(void);
static uint8_t gethex(void);
static void puthex(uint8_t);
static void flash_led(uint8_t);

/* putstr provides a way to output a string symbol from PROGMEM */
#if defined(RAMPZ) && !defined(__AVR_HAVE_LPMX__)
static void _putstr(uint_farptr_t);
#else
static void _putstr(uintptr_t);
#endif

#ifdef RAMPZ
# if defined (__AVR_HAVE_LPMX__)
#  define putstr(sym)						\
do {								\
	uint8_t _putstr_hh_tmp;					\
	asm volatile						\
	(							\
		"ldi %0, hh8(%2)" "\n\t"			\
		"out %1, %0" "\n\t"				\
		: "=&d" (_putstr_hh_tmp)			\
		: "I" (_SFR_IO_ADDR(RAMPZ)), "p" (&(sym))	\
	);							\
	_putstr((uintptr_t)(prog_char*)(sym));			\
} while (0)

# else	/* __AVR_HAVE_LPMX__ */
#  define putstr(sym)						\
do {								\
	uint_farptr_t _putstr_tmp;				\
	asm							\
	(							\
		"ldi %D0, lo8(%1)" "\n\t"			\
		"ldi %C0, hi8(%1)" "\n\t"			\
		"ldi %B0, hh8(%1)" "\n\t"			\
		"clr %A0" "\n\t"				\
		: "=&d" (_putstr_tmp)				\
		: "p" (&(sym))					\
	);							\
	_putstr(_putstr_tmp);					\
} while (0)

# endif	/* __AVR_HAVE_LPMX__ */

#else  /* RAMPZ */
# define putstr(s) \
	_putstr((uintptr_t)(prog_char*)(s))

#endif	/* RAMPZ */

/* data type for word/byte access */
union byte_word_union {
	uint16_t word;
	uint8_t  byte[2];
};
typedef union byte_word_union address_t;
typedef union byte_word_union length_t;

/* some variables */
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

static void noreturn (*app_start)(void) = 0x0000;

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


/* main program starts here */
int noreturn __attribute((section(".text.main"))) main(void)
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
	asm volatile("nop\n\t");
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
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)

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

	/* 20050803: by DojoCorp, this is one of the parts provoking the
		 system to stop listening, cancelled from the original */
	//putch('\0');

	/* forever loop */
	for (;;) {

	/* get character from UART */
	ch = getch();

	/* A bunch of if...else if... gives smaller code than switch...case ! */

	/* Hello is anyone home ? */ 
	if(ch=='0') {
		nothing_response();
	}


	/* Request programmer ID */
	else if(ch=='1') {
		if (getch() == ' ') {
			static prog_char id[] = STRBEG "AVR ISP" STREND;
			putstr(id);
		} else
			error();
	}


	/* AVR ISP/STK500 board commands  DON'T CARE so default nothing_response */
	else if(ch=='@') {
		ch = getch();
		if (ch>0x85) getch();
		nothing_response();
	}


	/* AVR ISP/STK500 board requests */
	else if(ch=='A') {
		ch = getch();
		if(ch==0x80) byte_response(HW_VER);		// Hardware version
		else if(ch==0x81) byte_response(SW_MAJOR);	// Software major version
		else if(ch==0x82) byte_response(SW_MINOR);	// Software minor version
		else if(ch==0x98) byte_response(0x03);		// Unknown but seems to be required by avr studio 3.56
		else byte_response(0x00);				// Covers various unnecessary responses we don't care about
	}


	/* Device Parameters  DON'T CARE, DEVICE IS FIXED  */
	else if(ch=='B') {
		getNch(20);
		nothing_response();
	}


	/* Parallel programming stuff  DON'T CARE  */
	else if(ch=='E') {
		getNch(5);
		nothing_response();
	}


	/* P: Enter programming mode  */
	/* R: Erase device, don't care as we will erase one page at a time anyway.  */
	else if(ch=='P' || ch=='R') {
		nothing_response();
	}


	/* Leave programming mode  */
	else if(ch=='Q') {
		nothing_response();
#ifdef WATCHDOG_MODS
		// autoreset via watchdog (sneaky!)
		WDTCSR = _BV(WDE);
		while (1); // 16 ms
#endif
	}


	/* Set address, little endian. EEPROM in bytes, FLASH in words  */
	/* Perhaps extra address bytes may be added in future to support > 128kB FLASH.  */
	/* This might explain why little endian was used here, big endian used everywhere else.  */
	else if(ch=='U') {
		address.byte[0] = getch();
		address.byte[1] = getch();
		nothing_response();
	}


	/* Universal SPI programming command, disabled.  Would be used for fuses and lock bits.  */
	else if(ch=='V') {
		if (getch() == 0x30) {
			getch();
			ch = getch();
			getch();
			if (ch == 0) {
				byte_response(SIG1);
			} else if (ch == 1) {
				byte_response(SIG2); 
			} else {
				byte_response(SIG3);
			} 
		} else {
			getNch(3);
			byte_response(0x00);
		}
	}


	/* Write memory, length is big endian and is in bytes  */
	else if(ch=='d') {
		struct {
			unsigned eeprom : 1;
		} flags;
		length_t length;
		length.byte[1] = getch();
		length.byte[0] = getch();
		flags.eeprom = 0;
		if (getch() == 'E') flags.eeprom = 1;
		for (w=0;w<length.word;w++) {
			buff[w] = getch();	                        // Store data in buffer, can't keep up with serial data stream whilst programming pages
		}
		if (getch() == ' ') {
			if (flags.eeprom) {		                //Write to EEPROM one byte at a time
				address.word <<= 1;
				for(w=0;w<length.word;w++) {
#if defined(__AVR_ATmega168__)  || defined(__AVR_ATmega328P__)
					while(EECR & (1<<EEPE));
					EEAR = (uint16_t)(void *)address.word;
					EEDR = buff[w];
					EECR |= (1<<EEMPE);
					EECR |= (1<<EEPE);
#else
					eeprom_write_byte((void *)address.word,buff[w]);
#endif
					address.word++;
				}			
			}
			else {					        //Write to FLASH one page at a time
				uint8_t address_high;
				if (address.byte[1]>127) address_high = 0x01;	//Only possible with m128, m256 will need 3rd address byte. FIXME
				else address_high = 0x00;
#ifdef RAMPZ
				RAMPZ = address_high;
#endif
				address.word = address.word << 1;	        //address * 2 -> byte location
				/* if ((length.byte[0] & 0x01) == 0x01) length.word++;	//Even up an odd number of bytes */
				if ((length.byte[0] & 0x01)) length.word++;	//Even up an odd number of bytes
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__)
				while(bit_is_set(EECR,EEPE));			//Wait for previous EEPROM writes to complete
#else
				while(bit_is_set(EECR,EEWE));			//Wait for previous EEPROM writes to complete
#endif

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
				uint8_t page_word_count;
				uint8_t *bufptr = buff;
				uint8_t tmp;
				asm volatile(
					 "clr	%[wcnt]		\n\t"	//page_word_count
					 "length_loop:		\n\t"	//Main loop, repeat for number of words in block							 							 
					 "cpi	%[wcnt],0x00	\n\t"	//If page_word_count=0 then erase page
					 "brne	no_page_erase	\n\t"						 
					 //Wait for previous spm to complete
					 "wait_spm1:		\n\t"
					 LOAD_SPM_CREG_TO_TMP"	\n\t"
					 "sbrc	%[tmp],%[spmen]	\n\t"
					 "rjmp	wait_spm1	\n\t"

					 "ldi	%[tmp],0x03	\n\t"	//Erase page pointed to by Z
					 STORE_TMP_TO_SPM_CREG"	\n\t"
					 "spm			\n\t"							 
#ifdef __AVR_ATmega163__
					 ".word 0xFFFF		\n\t"
					 "nop			\n\t"
#endif
					 //Wait for previous spm to complete
					 "wait_spm2:		\n\t"
					 LOAD_SPM_CREG_TO_TMP"	\n\t"
					 "sbrc	%[tmp],%[spmen]	\n\t"
					 "rjmp	wait_spm2	\n\t"

					 "ldi	%[tmp],0x11	\n\t"	//Re-enable RWW section
					 STORE_TMP_TO_SPM_CREG"	\n\t"
					 "spm			\n\t"
#ifdef __AVR_ATmega163__
					 ".word 0xFFFF		\n\t"
					 "nop			\n\t"
#endif
					 "no_page_erase:		\n\t"							 
					 "ld	r0,Y+		\n\t"	//Write 2 bytes into page buffer
					 "ld	r1,Y+		\n\t"							 
								 
					 //Wait for previous spm to complete
					 "wait_spm3:		\n\t"
					 LOAD_SPM_CREG_TO_TMP"	\n\t"
					 "sbrc	%[tmp],%[spmen]	\n\t"
					 "rjmp	wait_spm3	\n\t"

					 "ldi	%[tmp],0x01	\n\t"	//Load r0,r1 into FLASH page buffer
					 STORE_TMP_TO_SPM_CREG"	\n\t"
					 "spm			\n\t"
								 
					 "inc	%[wcnt]		\n\t"	//page_word_count++
					 "cpi   %[wcnt],%[PGSZ]	\n\t"
					 "brlo	same_page	\n\t"	//Still same page in FLASH
					 "write_page:		\n\t"
					 "clr	%[wcnt]		\n\t"	//New page, write current one first
					 //Wait for previous spm to complete
					 "wait_spm4:		\n\t"
					 LOAD_SPM_CREG_TO_TMP"	\n\t"
					 "sbrc	%[tmp],%[spmen]	\n\t"
					 "rjmp	wait_spm4	\n\t"

#ifdef __AVR_ATmega163__
					 "andi	%A[addr],0x80	\n\t"	// m163 requires Z6:Z1 to be zero during page write
#endif							 							 
					 "ldi	%[tmp],0x05	\n\t"	//Write page pointed to by Z
					 STORE_TMP_TO_SPM_CREG"	\n\t"
					 "spm			\n\t"
#ifdef __AVR_ATmega163__
					 ".word 0xFFFF		\n\t"
					 "nop			\n\t"
					 "ori	%A[addr],0x7E	\n\t"	// recover Z6:Z1 state after page write (had to be zero during write)
#endif
					 //Wait for previous spm to complete
					 "wait_spm5:		\n\t"
					 LOAD_SPM_CREG_TO_TMP"	\n\t"
					 "sbrc	%[tmp],%[spmen]	\n\t"
					 "rjmp	wait_spm5	\n\t"

					 "ldi	%[tmp],0x11	\n\t"	//Re-enable RWW section
					 STORE_TMP_TO_SPM_CREG"	\n\t"
					 "spm			\n\t"					 		 
#ifdef __AVR_ATmega163__
					 ".word 0xFFFF		\n\t"
					 "nop			\n\t"
#endif
					 "same_page:		\n\t"							 
					 "adiw	%[addr],2	\n\t"	//Next word in FLASH
					 "sbiw	%[length],2	\n\t"	//length-2
					 "breq	final_write	\n\t"	//Finished
					 "rjmp	length_loop	\n\t"
					 "final_write:		\n\t"
					 "cpi	%[wcnt],0	\n\t"
					 "breq	block_done	\n\t"
					 "adiw	%[length],2	\n\t"	//length+2, fool above check on length after short page write
					 "rjmp	write_page	\n\t"
					 "block_done:		\n\t"
					 "clr	__zero_reg__	\n\t"	//restore zero register
					 : [wcnt] "=d" (page_word_count),
					   [tmp] "=d" (tmp),
					   [buff] "+y" (bufptr),
					   [addr] "+z" (address.word),
					   [length] "+w" (length.word)
					 : [PGSZ] "M" (PAGE_SIZE),
					   [creg] "i" (SPM_CREG_ADDR),
					   [spmen] "i" (SPM_ENABLE_BIT)
					 : "r0"
					);
				/* Should really add a wait for RWW section to be enabled, don't actually need it since we never */
				/* exit the bootloader without a power cycle anyhow */
			}
			putch(RESPBEG);
			putch(RESPEND);
		} else
			error();
	}


	/* Read memory block mode, length is big endian.  */
	else if(ch=='t') {
		struct {
			unsigned eeprom : 1;
#ifdef RAMPZ
			unsigned rampz  : 1;
#endif
		} flags;
		length_t length;
		length.byte[1] = getch();
		length.byte[0] = getch();
#ifdef RAMPZ
		if (address.word>0x7FFF) flags.rampz = 1;		// No go with m256, FIXME
		else flags.rampz = 0;
#endif
		address.word = address.word << 1;	        // address * 2 -> byte location
		if (getch() == 'E') flags.eeprom = 1;
		else flags.eeprom = 0;
		if (getch() == ' ') {		                // Command terminator
			putch(RESPBEG);
			for (w=0;w < length.word;w++) {		        // Can handle odd and even lengths okay
				if (flags.eeprom) {	                        // Byte access EEPROM read
#if defined(__AVR_ATmega168__)  || defined(__AVR_ATmega328P__)
					while(EECR & (1<<EEPE));
					EEAR = (uint16_t)(void *)address.word;
					EECR |= (1<<EERE);
					putch(EEDR);
#else
					putch(eeprom_read_byte((void *)address.word));
#endif
					address.word++;
				}
				else {

#ifdef RAMPZ
					if (flags.rampz)
						putch(pgm_read_byte_far(address.word + 0x10000));
					// Hmmmm, yuck  FIXME when m256 arrvies
					else
#endif
					putch(pgm_read_byte_near(address.word));
					address.word++;
				}
			}
			putch(RESPEND);
		}
	}


	/* Get device signature bytes  */
	else if(ch=='u') {
		if (getch() == ' ') {
			static prog_char sig[] =
				{ RESPBEG, SIG1, SIG2, SIG3, RESPEND, 0};
			putstr(sig);
		} else
			error();
	}


	/* Read oscillator calibration byte */
	else if(ch=='v') {
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
	else 
		error();

	} /* end of forever loop */

}


static void error(void) {
	static uint8_t error_count;

	if (++error_count == MAX_ERROR_COUNT)
		app_start();
}

static uint8_t gethexnib(void) {
	char a;
	a = echogetch();
	if(a >= 'a') {
		return (a - 'a' + 0x0a);
	} else if(a >= '0') {
		return(a - '0');
	}
	return a;
}


static uint8_t gethex(void) {
	return (gethexnib() << 4) + gethexnib();
}


static void puthex(uint8_t ch) {
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


static void putch(char ch)
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
#elif defined(__AVR_ATmega168__)  || defined(__AVR_ATmega328P__)
	while (!(UCSR0A & _BV(UDRE0)));
	UDR0 = ch;
#else
	/* m8,16,32,169,8515,8535,163 */
	while (!(UCSRA & _BV(UDRE)));
	UDR = ch;
#endif
}


static char getch(void)
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
#elif defined(__AVR_ATmega168__)  || defined(__AVR_ATmega328P__)
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


static char echogetch(void)
{
	char ch = getch();
	putch(ch);
	return ch;
}


#if defined(RAMPZ) && !defined(__AVR_HAVE_LPMX__)
static void _putstr(uint_farptr_t s)
{
	char c;
	while ( (c = pgm_read_byte_far(s++)) )
		putch(c);
}

#else  /* RAMPZ && !__AVR_HAVE_LPMX__ */

static char read_str_inc(uintptr_t *p)
{
#if defined (__AVR_HAVE_LPMX__)
	char result;
	asm
	(
# if defined RAMPZ
		"elpm %0, Z+" "\n\t"
# else
		"lpm %0, Z+" "\n\t"
# endif
		: "=r" (result), "+z" (*p)
	);
	return result;
#else	/* __AVR_HAVE_LPMX__ */
	return pgm_read_byte((*p)++);
#endif	/* __AVR_HAVE_LPMX__ */
}


static void _putstr(uintptr_t s)
{
	char c;
	while ( (c = read_str_inc(&s)) )
		putch(c);
}
#endif	/* RAMPZ && !__AVR_HAVE_LPMX__ */


static void byte_response(uint8_t val)
{
	if (getch() == ' ') {
		putch(RESPBEG);
		putch(val);
		putch(RESPEND);
	} else
		error();
}


static void nothing_response(void)
{
	if (getch() == ' ') {
		putch(RESPBEG);
		putch(RESPEND);
	} else
		error();
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


/* end of file ATmegaBOOT.c */
