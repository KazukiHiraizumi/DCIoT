#ifndef PIN_H
#define	PIN_H

#ifdef	__cplusplus
extern "C" {
#endif

//USART
#define USART_TxTRIS TRISCbits.TRISC6
#define USART_Tx LATCbits.LC6
#define USART_RxTRIS TRISCbits.TRISC7
#define USART_Rx PORTCbits.RC7

//Virtual Serial
#define VSO_Tx LATBbits.LB6
#define VSO_TxTRIS TRISBbits.RB6


#ifdef	__cplusplus
}
#endif

#endif	/* PIN_H */

