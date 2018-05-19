#ifndef BLECMD_H
#define	BLECMD_H

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
#define DI_STAT1    PORTBbits.RB4
#define DI_STAT2    PORTBbits.RB5

#ifdef	__cplusplus
}
#endif

#endif
