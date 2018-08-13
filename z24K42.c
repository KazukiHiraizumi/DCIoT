#include <xc.h>
#include <stdbool.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "zcore.h"

//externals
extern void setup();
extern void loop();

//Globals
unsigned long OSC_Clock=1000000L;
//unsigned long USART_BPS=115200L;
unsigned char work_buffer[1000];

//USART service
unsigned char __verbose=1;
static unsigned char *USART_rbuf=work_buffer;
static int USART_rbufw=0,USART_rbufr=0,USART_rbufsz=500;
void USART_isr(){
    char c;
    if(PIR1bits.RC1IF==0) return;
    if(RCSTA1bits.OERR){  // in case of overrun error
        RCSTA1bits.CREN=0;  // reset the port
        c=RCREG1;
        RCSTA1bits.CREN=1;  // and keep going.
    }
    else c=RCREG1;
    USART_rbuf[USART_rbufw++]=c;
    if(USART_rbufw>=USART_rbufsz) USART_rbufw=0;
}
void USART_clr(){
    USART_rbufw=USART_rbufr=0;
}
void USART_putc(unsigned char c){
    while(!TXSTA1bits.TRMT);
    TXREG1 = c;
}
void USART_puts(unsigned char *s){
    for(;*s;s++) USART_putc(*s);
}
int USART_getc(){
    if(USART_rbufw!=USART_rbufr){
        int a=USART_rbuf[USART_rbufr++];
        if(USART_rbufr>=USART_rbufsz) USART_rbufr=0;
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
int USART_purge(const unsigned char *s){
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
                    if(__verbose){
                        VSO_puts("//purge delayed/");
                        VSO_puts(s);
                        VSO_cr();
                    }
                    return 0;
                }
            }
            if(__verbose){
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
            if(__verbose){
                VSO_puts("//purge done ");
                VSO_puts(s);
                VSO_cr();
            }
            return 0;
        }
        else if(a<0x10){
            if(__verbose){
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
    TXSTA1bits.BRGH=1;
    BAUDCON1bits.BRG16=1;
    SPBRG1=brg&0xFF;
    SPBRGH1=brg>>8;
//  BAUDCONbits.DTRXP=1; //Invert Rx
//H/W USART
    USART_Tx=1;
    USART_TxTRIS=0;
    USART_RxTRIS=1;
    TXSTA1bits.TXEN=1;
    RCSTA1bits.CREN=0;
    RCSTA1bits.SPEN=1;
    PIE1bits.RC1IE=1;
    INTCONbits.PEIE=1;
    INTCONbits.GIE=1;
    PIR1bits.RC1IF=0;
    USART_clr();
    char c=RCREG1;//clear FERR
    RCSTA1bits.CREN=1;
}
void USART_shrink(){
    USART_rbufsz=64;
    USART_rbufw=USART_rbufr=0;    
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
void TMR0_on(){
	TMR0_clr();
	T0CONbits.TMR0ON=1;
}
void TMR0_off(){T0CONbits.TMR0ON=0;}
void TMR0_clr(){
    TMR0H=0;
    TMR0L=0;
    TMR0_cemaphore=0;
}
unsigned char TMR1_up,TMR1_tick;
void TMR1_isr(){
    if(PIR1bits.TMR1IF==0) return;
    if(--TMR1_tick==0){
        T1CONbits.TMR1ON=0;
        TMR1_up=1;
    }
    else PIR1bits.TMR1IF=0;
}
void TMR1_set(unsigned int c){
    TMR1_tick=c/10;
    TMR1_up=0;
    TMR1H=0xFF;
    TMR1L=0xFF;
    PIE1bits.TMR1IE=1;
    INTCONbits.PEIE=1;
    INTCONbits.GIE=1;
    PIR1bits.TMR1IF=0;
    T1CONbits.TMR1ON=1;
}
void TMR1_init(){
    T1CONbits.T1OSCEN=0;
    T1CONbits.TMR1CS=0;
    T1CONbits.T1CKPS=0;
}
//Virtual Serial Out service only 2400 bps
#define VSOBPS  9600
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

//USART-2 service
#ifdef TXREG2
static unsigned char *USART2_rbuf=work_buffer+64;
static int USART2_rbufw=0,USART2_rbufr=0,USART2_rbufsz=128;
void USART2_isr(){
    char c;
    if(PIR3bits.RC2IF==0) return;
    if(RCSTA2bits.OERR){  // in case of overrun error
        RCSTA2bits.CREN=0;  // reset the port
        c=RCREG2;
        RCSTA2bits.CREN=1;  // and keep going.
    }
    else c=RCREG2;
    USART2_rbuf[USART2_rbufw++]=c;
    if(USART2_rbufw>=USART2_rbufsz) USART2_rbufw=0;
}
void USART2_clr(){
    USART2_rbufw=USART2_rbufr=0;
}
static unsigned char USART2_sum=0;
void USART2_putc(unsigned char c){
    while(!TXSTA2bits.TRMT);
    USART2_sum+=c;
    TXREG2 = c;
}
void USART2_cmd(int cmd,int dat){
    unsigned int sum;
    VSO_puts("cmd:");VSO_putd(cmd);VSO_cr();
    USART2_sum=0;
    USART2_putc(2);//STX
    USART2_putc(0);//Timestamp
    USART2_putc(0);
    USART2_putc(0);
    USART2_putc(0);
    USART2_putc(2);//Length
    USART2_putc(cmd);
    USART2_putc(dat);
    USART2_putc(sum=(-USART2_sum)&0xFF);
}
int USART2_getc(){
    if(USART2_rbufw!=USART2_rbufr){
        int a=USART2_rbuf[USART2_rbufr++];
        if(USART2_rbufr>=USART2_rbufsz) USART2_rbufr=0;
        return a;
    }
    else return -1;
}
static int USART2_wp=0;
int USART2_gets(unsigned char *s){
    int a=USART2_getc();
    if(a>=0){
        if(USART2_wp==0){
            if(a==2){
                USART2_sum=a;
                s[USART2_wp++]=a;
            }
        }
        else{
            s[USART2_wp++]=a;
            if(USART2_wp>5 && USART2_wp>=s[5]+7){
                int wn=USART2_wp;
                USART2_wp=0;
                s[wn]=USART2_sum;
                return wn+1;
            }
            USART2_sum+=a;
        }
    }
    return -1;
}
void USART2_init(unsigned long bps){
    unsigned short brg=OSC_Clock/4/bps-1;
    TXSTA2bits.BRGH=1;
    BAUDCON2bits.BRG16=1;
    SPBRG2=brg&0xFF;
    SPBRGH2=brg>>8;
    USART2_Tx=1;
    USART2_TxTRIS=0;
    USART2_RxTRIS=1;
    TXSTA2bits.TXEN=1;
    RCSTA2bits.CREN=0;
    RCSTA2bits.SPEN=1;
    PIE3bits.RC2IE=1;
//    INTCONbits.PEIE=1;
//    INTCONbits.GIE=1;
    PIR3bits.RC2IF=0;
    USART2_clr();
    char c=RCREG2;//clear FERR
    RCSTA2bits.CREN=1;
}
#endif

void main(){
    int i=0;
    ANSELA=0;
    ANSELB=0;
    ANSELC=0;
    
    USART_clr();
#ifdef TXREG2
    USART2_clr();
#endif
    setup();
    while(1) loop();
}

void interrupt SYS_InterruptHigh(void){
    TMR0_isr();
    TMR1_isr();
    USART_isr();
#ifdef TXREG2
    USART2_isr();
#endif
}
