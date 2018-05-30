#include <xc.h>
#include <stdbool.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "zcore.h"
#include "ble.h"

#define Revision  "0.1"

#define IdCommon  "f5b0e88109ab42000ba24f83"
#define IdServicePri "10014246"
#define IdCharLogger "20024246"
#define IdCharResponse "20034246"
#define IdCharRequest "20044246"
#define IdServiceSec "10024246"
#define Prompt "CMD>"

//IOs
#define TR_LEADSW   TRISBbits.RB2
#define DI_LEADSW   PORTBbits.RB2
#define DO_LED      LATCbits.LC1
#define TR_LED      TRISCbits.RC1

static unsigned char rnbuf[50];//comm buffer to/from RN4xxx
static unsigned char deviceID[15];
static unsigned char ModQ; //0:discon,1:adv,2:connect
//diag
static unsigned short Timer[]={0,0,0,0,0};//wallet scan,adv off,discon,battery
//handler
void Timer_do();//Do every 1 sec
void WV_do(char *);//Do on receiving "%WV"
void WC_do(char *);//Do on receiving "%WC"
int USART_flagSHW;
void xdelay(int ms){
    int tc=OSC_Clock/_XTAL_FREQ;
    int dn=ms*tc;
    do{ __delay_ms(1);}while(--dn>0);
}
//HAL for RN487x
void rn487x_prog(char *s){
    USART_puts(s);
    USART_purge(Prompt);
}
void rn487x_aload(){
    rn487x_prog("IA,01,06\n");//ad type flag
    USART_puts("IA,09,");XSART_puts(deviceID+8);USART_cr();//set LocalName(09) as BF11xxxx
    USART_purge(Prompt);
    USART_puts("IA,07,");USART_2stup(IdServicePri""IdCommon);USART_cr();//set 128bits UUID(06)
    USART_purge(Prompt);
}
void rn487x_aoff(){//stop advertize
    USART_puts("A,0100,0001\n");
    USART_purge("%ADV_TIMEOUT");
}
int rn487x_cmd(){
    DO_RXIND=0;
    TR_RXIND=0;
    xdelay(50);
    rn487x_prog("$$$");
    xdelay(100);
    rn487x_prog("+\n");//echo on
    return 0;
}
int rn487x_reset(int adv){
    DO_RXIND=1;
    TR_RXIND=0;
    xdelay(50);
//    while(!DI_RESET){//If ResetIC impremented
//       VSO_puts("RN487x external reset\n");
//        xdelay(1000);
//        VSO_puts("RN487x reset waiting\n");
//    }
    TR_RESET=0;
    DO_RESET=0;
    xdelay(100);
    DO_RESET=1;
    TR_RESET=1;
    if(USART_purge("%REBOOT")==0){
        rn487x_cmd();
        if(!adv) rn487x_aoff();
        else USART_puts("A,0100\n");
        return 0;
    }
    else return -1;
}
int rn487x_reboot(){ return rn487x_reset(0);}
void setup(){
    int i,j,k;
    OSC_init(_XTAL_FREQ);
Restart:
    USART_init(115200L);
    TMR0_init();
    VSO_init();
    VSO_puts("//Setup LV1\n");
    if(rn487x_reboot()==0) goto Reset;
    goto Restart;
Reset:
    rn487x_prog("U\n");
    rn487x_prog("NA,Z\n");
    rn487x_prog("NA,01,06\n");//ad type flag
  //  USART_puts("NA,09,");XSART_puts(deviceID+8);USART_cr();//set LocalName(09) as BF11xxxx
    USART_puts("NA,09,");XSART_puts("DCIoT");USART_cr();//set LocalName(09) as BF11xxxx
    USART_purge(Prompt);
    USART_puts("NA,07,");USART_2stup(IdServicePri""IdCommon);USART_cr();//set 128bits UUID(06)
    USART_purge(Prompt);
    rn487x_prog("PZ\n");
    rn487x_prog("ST,0050,0100,0002,0064\n");//interval,latecy,timeout
    rn487x_prog("S-,DCIoT\n");
    rn487x_prog("SN,DCIoT\n");
    rn487x_prog("SO,1\n"); //enable sleep mode
//  rn487x_prog("SB,03\n");//default 115200
    rn487x_prog("SW,0B,07\n");//P13 as Status-1
    rn487x_prog("SW,0A,08\n");//P12 as Status-2
    rn487x_reboot();
    xdelay(30);
 
    TMR0_init();
    VSO_init();
    VSO_puts("//Setup LV2\n");
    xdelay(100);
    USART_puts("D\n");
    USART_purge("BTA=");
    deviceID[0]=0; while(USART_gets(deviceID,0)<0); deviceID[12]=0;
    USART_purge(Prompt);
    VSO_puts("//deviceID/");VSO_puts(deviceID);VSO_cr();
//Private service
    rn487x_prog("PS,"IdServicePri""IdCommon"\n");
    rn487x_prog("PC,"IdCharLogger""IdCommon",1A,10\n");
    rn487x_prog("PC,"IdCharResponse""IdCommon",1A,10\n");//Read,Write,Notify
    rn487x_prog("PC,"IdCharRequest""IdCommon",18,10\n");//Write
    rn487x_prog("LS\n");
    USART_shrink();
    rn487x_prog("V\n");
//App inits
}

void loop(){
    int i,j,k;
ADVERTISE:
    ModQ=1;
    VSO_puts("//Adv mode\n");
//    rn487x_prog("IA,Z\n");
    rn487x_reset(1);//reset and keep advertise beacon on
    DO_RXIND=1;
//    rn487x_aload();
    rnbuf[0]=0;
    TMR0_cemaphore=0;
ADVERTISE_LOOP:
    WDTCONbits.SWDTEN=1;
    Sleep();
    WDTCONbits.SWDTEN=0;
    DO_LED=1;
    TR_LED=0;
    xdelay(2);
    Timer_do();
    if(!DI_STAT1 && !DI_STAT2){
        ModQ=2;
    }
//    else if(TMR0_cemaphore){
//        Timer_do();
//        TMR0_cemaphore--;
//    }
    TR_LED=1;
    switch(ModQ){
    case 2:
        goto CONNECT;
    }
    goto ADVERTISE_LOOP;
CONNECT:
    ModQ=2;
    DO_RXIND=0;
    Timer[0]=0;
    Timer[2]=60;
    TMR0_cemaphore=0;
    xdelay(10);
    VSO_puts("//Connect mode\n");
CONNECT_LOOP:
    k=USART_gets(rnbuf,'%');
    if(DI_STAT1 || DI_STAT2){//Disconnected
        VSO_puts("//Disconnected/");VSO_putd(DI_STAT1);VSO_puts(",");VSO_putd(DI_STAT2);VSO_cr();
        Timer[0]=Timer[2]=0;
        ModQ=1;
    }
    else if(k>0){
        if(!strncmp(rnbuf,"WV",2)){
            Timer[2]=30;//disconnect time
            WV_do(rnbuf);
        }
        else if(!strncmp(rnbuf,"WC",2)){
            Timer[2]=30;//disconnect timer
            WC_do(rnbuf);
        }
        rnbuf[0]=0;
    }
    else if(k==0){ //End with CR
//        VSO_puts("gets=");VSO_puts(rnbuf);VSO_cr();
        rnbuf[0]=0;
    }
    else if(TMR0_cemaphore){
        Timer_do();
        TMR0_cemaphore--;
    }
    switch(ModQ){
    case 1:
        goto ADVERTISE;
    case 99:
        rn487x_reboot();
        goto ADVERTISE;
    }
    goto CONNECT_LOOP;
}
void Timer_do(){
    int cflag,otim,ctim;
T0UP:
    if(Timer[0]==0) goto T1UP; else if(--Timer[0]>0) goto T1UP;
    if(ModQ==2) ModQ=99; //reset
T1UP://adv off timer
    if(Timer[1]==0) goto T2UP; else if(--Timer[1]>0) goto T2UP;
T2UP://disconnect timer
    if(Timer[2]==0) goto T3UP; else if(--Timer[2]>0) goto T3UP;
    if(ModQ==2){
        rn487x_prog("K,1\n");
        Timer[0]=60;//reset timer
        VSO_puts("//Connection Timeout\n");
        ModQ=1;
    }
T3UP://cover scan
    if(Timer[3]==0) goto T4UP; else if(--Timer[3]>0) goto T4UP;
    if(ModQ==0){
        if(DI_LEADSW) ModQ=1;
    }
    Timer[3]=1;
T4UP://auto advertise timer
    if(Timer[4]==0) goto T5UP; else if(--Timer[4]>0) goto T5UP;
T5UP:
    return;
}
void WV_do(char *rs){
    int a,b,i;
    char s[15];
    int h=XSART_parse(rs,s);//chara handle
    DO_RXIND=0;
    xdelay(5);
    USART_cr();
    USART_purge(Prompt);//flush cmd buffer
    switch(h){
    case CH_CMD:
        switch(*s){
            case 'L':
            VSO_puts("cmdL");VSO_cr();
            __verbose=0;
            for(i=0;i<10;i++){
                USART_puts("SHW,0075,");XSART_putd(i);XSART_putlu(0x12341234);XSART_putlu(0x12341234);USART_cr();
                USART_purge(Prompt);
//                xdelay(10);
            }
            __verbose=1;
            break;
        case 'S':
            switch(atoi(s+1)){
            case 10007:
            case 10009:
                break;
            case 10037:
                __delay_ms(100);
                USART_putSHW(CH_STAT);
                XSART_puts("Done");
                break;
            case 10038:
                __delay_ms(100);
                break;
            case 101:
                __delay_ms(100);
                break;
            case 10:
                __delay_ms(100);
                break;
            case 121:
                USART_putSHW(CH_STAT);
                XSART_putd(10);
                break;
            }
				break;
        case 'W':
            h=s[1]-'0';
            USART_putSHW(CH_STAT);
            XSART_putd(h);
            break;
        }
        break;
    }
    if(USART_flagSHW>0){
        USART_cr();
        USART_purge(Prompt);
    }
}
void WC_do(char *rs){
}
//Comm libs b.w. RN487x
int XSART_parse(char *src,char *res);
void USART_2stup(char *s){//reverse put string
    int i=strlen(s)-2;
    char *p=s+i;
    char b[3];
    b[2]=0;
    for(;i>=0;i-=2,p-=2){
        strncpy(b,p,2);
        USART_puts(b);
    }
}
void USART_cr(){
    USART_puts("\n");
}
void XSART_puts(char *s){//put string as hex dump
    int n=strlen(s);
    for(int i=0;i<n;i++,s++){
        int h=(unsigned char)*s>>4;
        if(h<10) USART_putc(h+'0');
        else USART_putc(h-10+'A');
        h=*s&0x0F;
        if(h<10) USART_putc(h+'0');
        else USART_putc(h-10+'A');
    }
}
void XSART_putd(int d){
    char s[12];
    sprintf(s,"%d",d);
    XSART_puts(s);
}
void XSART_putlu(unsigned long d){
    char s[12];
    sprintf(s,"%lu",d);
    XSART_puts(s);
}
void XSART_putlX(unsigned long d){
    char s[10];
    sprintf(s,"%lX",d);
    XSART_puts(s);
}
void USART_putSHW(int x){
    char s[10];
    sprintf(s,"%04X",x);
    USART_puts("SHW,");
    USART_puts(s);
    USART_puts(",");
    USART_flagSHW++;
}
int hex2byte(char *s){
    int h=s[0]-'0';
    if(h>9) h=s[0]-'A'+10;
    int l=s[1]-'0';
    if(l>9) l=s[1]-'A'+10;
    return (h<<4)|l;
}
int hex2str(char *src,char *dst){
    int n=strlen(src)/2;
    for(int i=0;i<n;i++,src+=2,dst++){
        *dst=hex2byte(src);
    }
    *dst=0;
    return n;
}
int XSART_parse(unsigned char *src,unsigned char *val){
    char *d1=strchr(src,',');
    if(d1==NULL) return -1;
    char *d2=strchr(d1+1,',');
    if(d2==NULL) return -1;
    hex2str(d2+1,val);
    return hex2byte(d1+1)*256+hex2byte(d1+3);
}
