#ifndef ZCORE_H
#define	ZCORE_H

#ifdef	__cplusplus
extern "C" {
#endif

extern unsigned char USART_flag;
extern void USART_init(unsigned long bps);
extern void USART_clr();
extern void USART_putc(unsigned char);
extern void USART_puts(unsigned char *);
extern int USART_getc();
extern int USART_gets(unsigned char *,unsigned char);
extern int USART_purge(unsigned char *);
extern int USART_grep(unsigned char *,unsigned char *);
extern void TMR0_init();
extern void TMR0_clr();
extern void TMR0_on();
extern void TMR0_off();
extern unsigned long TMR0_period;
extern unsigned char TMR0_cemaphore;
extern unsigned char TMR0_count;
extern void TMR1_init();
extern void TMR1_set(unsigned int,void (*)());

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

#ifdef	__cplusplus
}
#endif

#endif	/* ZCORE_H */
