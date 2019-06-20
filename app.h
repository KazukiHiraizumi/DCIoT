#ifndef APP_H
#define	APP_H

#ifdef	__cplusplus
extern "C" {
#endif

//MPU Operation Condition
#define _XTAL_FREQ 16000000  //Clock

/** CONFIGURATION Bits **********************************************/
#pragma config FEXTOSC  = OFF
#pragma config FCMEN    = OFF
#pragma config MCLRE    = INTMCLR
#pragma config PWRTS    = PWRT_64
#pragma config BOREN    = NOSLP
#pragma config BORV     = VBOR_245
#pragma config WDTE     = OFF
#pragma config WDTCPS   = WDTCPS_31
#pragma config CCP2MX   = PORTC
#pragma config LVP      = OFF

extern int hex2byte(char *);//convert 2 character hex string to int
extern int hex2str(char *src,char *dest); //convert hex string to ascii string
extern void str2hex(char *src,char *dest);
extern void d2hex(int ,char *dest);
extern void lu2hex(unsigned long ,char *dest);
extern void lx2hex(unsigned long ,char *dest);

//Comm Libs
extern int XSART_parse(char *src,char *res);
extern void USART_2stup(char *s);
extern void USART_cr();
extern void XSART_puts(char *s);
extern void XSART_putd(int d);
extern void XSART_putlu(unsigned long d);
extern void XSART_putlX(unsigned long d);
extern int USART_flagSHW;
extern void USART_putSHW(int x);

//Characteristic Handles
#define CH_HYS 0x0072
#define CH_STAT 0x0075
#define CH_CMD 0x0078
//IOs
#define DI_RESET    PORTBbits.RB2
#define DO_RESET    LATBbits.LB2
#define TR_RESET    TRISBbits.RB2
#define DO_RXIND    LATBbits.LB1
#define TR_RXIND    TRISBbits.RB1
#define TR_LEADSW   TRISCbits.RC1
#define DI_LEADSW   PORTCbits.RC1
#define DO_LAMP     LATBbits.LB0
#define TR_LAMP     TRISBbits.RB0
#define DI_STAT1    PORTBbits.RB4
#define DI_STAT2    PORTBbits.RB5

//E2ROM
#define EE_BFLAG    0
#define EE_WFLAG    1

#ifdef	__cplusplus
}
#endif

#endif	/* APP_H */
