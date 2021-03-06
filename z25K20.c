#include <xc.h>
#include <stdbool.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "pin.h"
#include "zcore.h"

//externals
extern void setup();
extern void loop();

//Globals
unsigned long OSC_Clock=1000000L;
//unsigned long USART_BPS=115200L;

//USART service
static unsigned char USART_rbuf[300];
static int USART_rbufw=0,USART_rbufr=0;
unsigned char USART_flag=0;
void USART_isr(){
    char c;
    if(PIR1bits.RCIF==0) return;
    if(RCSTAbits.OERR){  // in case of overrun error
        RCSTAbits.CREN=0;  // reset the port
        c=RCREG;
        RCSTAbits.CREN=1;  // and keep going.
    }
    else c=RCREG;
    USART_rbuf[USART_rbufw++]=c;
    if(USART_rbufw>=sizeof(USART_rbuf)) USART_rbufw=0;
}
void USART_clr(){
    USART_rbufw=USART_rbufr=0;
}
void USART_putc(unsigned char c){
    while(!TXSTAbits.TRMT);
    TXREG = c;
}
void USART_puts(unsigned char *s){
    for(;*s;s++) USART_putc(*s);
}
int USART_getc(){
    if(USART_rbufw!=USART_rbufr){
        int a=USART_rbuf[USART_rbufr++];
        if(USART_rbufr>=sizeof(USART_rbuf)) USART_rbufr=0;
        return a;
    }
    else return -1;
}
int USART_gets(unsigned char *s,unsigned char eos){
    int a=USART_getc();
    int n=strlen(s);
    if(a<0){
        return -1;
    }
    else{
        if(a<0x10){
            if(n>0){
                return 0;
            }
            else return -1;
        }
        s[n]=a;
        s[n+1]=0;
        if(eos){
            if(a==eos) return 1;
        }
        return -1;
    }
}
int USART_purge(unsigned char *s){
    unsigned char wdt=TMR0_count;
    char r[50];
    r[0]=0;
    while(1){
        int a=USART_getc();
        int l=strlen(r);
        if(a<0){
            if((signed char)(TMR0_count-wdt)<3) continue;
            else if(l>0){
                if(strstr(r,s)!=NULL){
                    if(USART_flag&1){
                        VSO_puts("//purge delayed/");
                        VSO_puts(s);
                        VSO_cr();
                    }
                    return 0;
                }
            }
            if(USART_flag&1){
                VSO_puts("//purge timeout ");
                VSO_puts(s);
                VSO_puts("/");
                VSO_puts(r);
                VSO_cr();
            }
            return -1;
        }
        wdt=TMR0_count;
        if(l<sizeof(r)-1){
            r[l]=a;
            r[l+1]=0;
        }
        else{
            strcpy(r,r+1);
            r[sizeof(r)-2]=a;
            
        }
        if(strstr(r,s)!=NULL){
            if(USART_flag&1){
                VSO_puts("//purge done ");
                VSO_puts(s);
                VSO_cr();
            }
            return 0;
        }
        else if(a<0x10){
            if(USART_flag&1){
                VSO_puts("//purge skip ");
                VSO_puts(s);
                VSO_puts("/");
                VSO_puts(r);
                VSO_cr();
            }
            r[0]=0;
        }
    }
}
int USART_grep(unsigned char *r,unsigned char *s){
    *r=0;
    while(1){
        if(USART_gets(r,0)==0){
            if(strstr(r,s)!=NULL) return 0;
            else{
                *r=0;
            }
        }
    }
    return -1;
}
void USART_init(unsigned long bps){
    unsigned short brg=OSC_Clock/4/bps-1;
    TXSTAbits.BRGH=1;
    BAUDCONbits.BRG16=1;
    SPBRG=brg&0xFF;
    SPBRGH=brg>>8;
//  BAUDCONbits.DTRXP=1; //Invert Rx
//H/W USART
    USART_Tx=1;
    USART_TxTRIS=0;
    USART_RxTRIS=1;
    TXSTAbits.TXEN=1;
    RCSTAbits.CREN=0;
    RCSTAbits.SPEN=1;
    PIE1bits.RCIE=1;
    INTCONbits.PEIE=1;
    INTCONbits.GIE=1;
    PIR1bits.RCIF=0;
    USART_clr();
    char c=RCREG;//clear FERR
    RCSTAbits.CREN=1;
}
//TMR service
unsigned char TMR0_cemaphore=0;
unsigned char TMR0_count=0;
static unsigned long TMR0_clock;
static unsigned long TMR0_period;
void TMR0_isr(){ //should be called every 25usec at any run clock
    if(INTCONbits.TMR0IF==0) return;
    INTCONbits.TMR0IF = 0;
    TMR0_cemaphore++;
    TMR0_count++;
}
void TMR0_init(){
    T0CONbits.T0CS=0;  //internal clock
    T0CONbits.T08BIT=0;  //16bit counter
    T0CONbits.PSA=0;  //use prescaler
    if((TMR0_clock=OSC_Clock/4/2)<65536L) T0CONbits.T0PS=0;  //1/2 prescaler
    else if((TMR0_clock>>=1)<65536L) T0CONbits.T0PS=1;  //1/4 prescaler
    else if((TMR0_clock>>=1)<65536L) T0CONbits.T0PS=2;  //1/8 prescaler
    else if((TMR0_clock>>=1)<65536L) T0CONbits.T0PS=3;  //1/16 prescaler
    else if((TMR0_clock>>=1)<65536L) T0CONbits.T0PS=4;  //1/32 prescaler
    else if((TMR0_clock>>=1)<65536L) T0CONbits.T0PS=5;  //1/64 prescaler
    else if((TMR0_clock>>=1)<65536L) T0CONbits.T0PS=6;  //1/128 prescaler
    else if((TMR0_clock>>=1)<65536L) T0CONbits.T0PS=7;  //1/256 prescaler
    TMR0_period=0xFFFFFFFFL/TMR0_clock;
    T0CONbits.TMR0ON=1;
//Interrupt control
    INTCONbits.TMR0IE=1;
    INTCONbits.GIE=1;
    INTCONbits.TMR0IF=0;
}
void TMR0_on(){T0CONbits.TMR0ON=1;}
void TMR0_off(){T0CONbits.TMR0ON=0;}
void TMR0_clr(){
    TMR0H=0;
    TMR0L=0;
    TMR0_cemaphore=0;
}
void (*TMR1_callback)();
void TMR1_isr(){
    if(PIR1bits.TMR1IF==0) return;
    T1CONbits.TMR1ON=0;
    (*TMR1_callback)();
    PIR1bits.TMR1IF=0;
}
void TMR1_set(unsigned msec,void (*f)()){
    TMR1_callback=f;
    TMR1H=msec>>1;
    TMR1L=msec<<7;
    PIE1bits.TMR1IE=1;
    INTCONbits.PEIE=1;
    INTCONbits.GIE=1;
    PIR1bits.TMR1IF=0;
    T1CONbits.TMR1ON=1;
}
void TMR1_init(){
    TMR1_callback=NULL;
    T1CONbits.T1CKPS=3;
    T1CONbits.T1OSCEN=0;
    T1CONbits.TMR1CS=0;
}
//Virtual Serial Out service only 2400 bps
#define VSOBPS  2400
unsigned short VSO_dt[10];
unsigned short VSO_ts(){
    unsigned short tl=TMR0L;
    unsigned short th=TMR0H;
    return tl+(th<<8);
}
void VSO_putc(unsigned char c){
    unsigned short tb=VSO_ts(),tn;
    VSO_Tx=0;
    tn=tb+VSO_dt[0];
    while((short)(VSO_ts()-tn)<0);
    for(int b=1;b<9;c>>=1,b++){
        VSO_Tx=c&1? 1:0;
        tn=tb+VSO_dt[b];
        while((short)(VSO_ts()-tn)<0);
    }
    VSO_Tx=1;
    tn=tb+VSO_dt[9];
    while((short)(VSO_ts()-tn)<0);
}
void VSO_puts(unsigned char *s){
    for(;*s;s++) VSO_putc(*s);
}
void VSO_putd(int d){
    char r[6];
    sprintf(r,"%d",d);
    VSO_puts(r);
}
void VSO_putlu(unsigned long d){
    char r[11];
    sprintf(r,"%lu",d);
    VSO_puts(r);
}
void VSO_putlx(unsigned long d){
    char r[10];
    sprintf(r,"%lX",d);
    VSO_puts(r);
}
void VSO_putx(int d){
    char r[6];
    sprintf(r,"%X",d);
    VSO_puts(r);
}
void VSO_cr(){
    VSO_puts("\n");
}
void VSO_init(){
    VSO_Tx=1;
    VSO_TxTRIS=0;
    for(int i=0;i<10;i++){
        VSO_dt[i]=TMR0_clock*(i+1)/VSOBPS-1;
    }
    VSO_puts("//TMR0/");VSO_putlu(TMR0_clock);VSO_puts("/");VSO_putlu(TMR0_period);VSO_cr();
}

//Framework service
void OSC_init(unsigned long m){
    OSC_Clock=m;
    if(m==16000000L) OSCCONbits.IRCF=7;  //Switch to 16MHz HFINTOSC
    else if(m==4000000L) OSCCONbits.IRCF=5; //4MHz
    else if(m==2000000L) OSCCONbits.IRCF=4; //2MHz
    else if(m<=1000000L){
        OSC_Clock=1000000L;
        OSCCONbits.IRCF=3; //1MHz
    }
}

void AD_init(){
    ADCON2bits.ADFM=1;
    ADCON2bits.ACQT=5; //12Tad
    ADCON2bits.ADCS=7; //Frc as clock
    ADCON1=0;
}
int AD_scan(int adds){
    ADCON0bits.CHS=adds; //AN0
    ADCON0bits.ADON=1;
    ADCON0bits.GO=1;
    while(ADCON0bits.GO) ;
    return (ADRESH<<8)|ADRESL;
}
int E2ROM_read(int a){
    EEADR=a;
    EECON1bits.EEPGD=0;
    EECON1bits.CFGS=0;
    EECON1bits.RD=1;
    return EEDATA;
}
void E2ROM_write(int a,int d){
    EEADR=a;
    EEDATA=d;
    EECON1bits.EEPGD=0;
    EECON1bits.CFGS=0;
    EECON1bits.WREN=1;
    INTCONbits.GIE=0;
    EECON2=0x55;
    EECON2=0xAA;
    EECON1bits.WR=1;
    while(EECON1bits.WR);
    INTCONbits.GIE=1;
    EECON1bits.WREN=0;
}

void main(){
    int i=0;
//  ANSEL=0;
//  ANSELH=0;
    USART_clr();
    setup();
    while(1) loop();
}

void interrupt SYS_InterruptHigh(void){
    TMR0_isr();
    TMR1_isr();
    USART_isr();
}
