#ifndef BLECMD_H
#define	BLECMD_H

extern unsigned long hex2byte(char *);
extern int hex2str(char *src,char *dest); //convert hex string to ascii string
extern void str2hex(char *src,char *dest);
extern void d2hex(int ,char *dest);
extern void lu2hex(unsigned long ,char *dest);
extern void lx2hex(unsigned long ,char *dest);

//Comm Libs
extern int XSART_parse(char *src,unsigned int *handle,unsigned long *value);
extern void USART_2stup(char *);
extern void USART_cr();
extern void XSART_putSHW(int);
extern void XSART_puts(char *);
extern void XSART_putInt8(int);
extern void XSART_putInt16(int);

//Characteristic Handles
#define CH_ADS 0x0072
#define CH_WOUT 0x0074
//IOs
//#define DI_RESET    PORTCbits.RC2
#define DO_RESET    LATCbits.LC2
#define TR_RESET    TRISCbits.RC2
#define DO_RXIND    LATCbits.LC3
#define TR_RXIND    TRISCbits.RC3
#define DI_STAT1    PORTCbits.RC4
#define DI_STAT2    PORTCbits.RC5

#ifdef	__cplusplus
}
#endif

#endif
