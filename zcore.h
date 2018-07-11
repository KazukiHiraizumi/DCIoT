#ifndef ZCORE_H
#define	ZCORE_H

#ifdef	__cplusplus
extern "C" {
#endif

//MPU Operation Condition
#define _XTAL_FREQ 16000000L  //Clock

/** CONFIGURATION Bits **********************************************/
#pragma config FOSC     = INTIO67
#pragma config PRICLKEN = ON
#pragma config FCMEN    = OFF
#pragma config IESO     = OFF
#pragma config PWRTEN   = ON
#pragma config BOREN    = SBORDIS
#pragma config BORV     = 18
#pragma config WDTEN    = SWON
#pragma config WDTPS    = 256
#pragma config CCP2MX   = PORTC1
#pragma config PBADEN   = OFF
#pragma config HFOFST   = OFF
#pragma config MCLRE    = INTMCLR
#pragma config STVREN   = ON
#pragma config LVP      = OFF

//USART
#define USART_TxTRIS TRISCbits.TRISC6
#define USART_Tx LATCbits.LC6
#define USART_RxTRIS TRISCbits.TRISC7
#define USART_Rx PORTCbits.RC7
    
//USART2
#ifdef TXREG2
#define USART2_TxTRIS TRISBbits.TRISB6
#define USART2_Tx LATBbits.LB6
#define USART2_RxTRIS TRISBbits.TRISB7
#define USART2_Rx PORTBbits.RB7
#endif

//Virtual Serial
#define VSO_Tx LATCbits.LC0
#define VSO_TxTRIS TRISCbits.RC0

//Functions
extern unsigned char __verbose;
extern unsigned char USART_flag;
extern unsigned char work_buffer[];
extern void USART_init(unsigned long bps);
extern void USART_shrink();//Shrink work area
extern void USART_clr();
extern void USART_putc(unsigned char);
extern void USART_puts(unsigned char *);
extern int USART_getc();
extern int USART_gets(unsigned char *,unsigned char);
extern int USART_purge(const unsigned char *);
extern int USART_grep(unsigned char *,unsigned char *);
extern void TMR0_init();
extern void TMR0_clr();
extern void TMR0_on();
extern void TMR0_off();
extern unsigned long TMR0_period;
extern unsigned char TMR0_cemaphore;
extern unsigned char TMR0_count;
extern void TMR1_init();
extern void TMR1_set(unsigned int);
extern unsigned char TMR1_up;
extern void VSO_init();
extern void VSO_putc(unsigned char);
extern void VSO_puts(unsigned char *);
extern void VSO_cr();
extern void VSO_putd(int);
extern void VSO_putx(int);
extern void VSO_putlu(unsigned long);
extern void VSO_putlx(unsigned long);

extern unsigned long OSC_Clock;
extern void OSC_init(unsigned long);

extern void AD_init();
extern int AD_scan(int adds);

extern int E2ROM_read(int adds);
extern void E2ROM_write(int adds,int val);

extern void USART2_init(unsigned long bps);
extern int USART2_gets(unsigned char *dat);//returns cmd
extern void USART2_cmd(int cmd,int dat);
extern void USART2_putc(unsigned char c);

#ifdef	__cplusplus
}
#endif

#endif
