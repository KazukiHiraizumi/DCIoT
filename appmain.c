#include <xc.h>
#include <stdbool.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "zcore.h"
#include "ble.h"

#define DeviceName "Shimano"
#define Revision  "0.1"

#define IdCommon  "f5b0e88109ab42000ba24f83"
#define IdServicePri "10014246"
#define IdCharADS "20024246"
#define IdCharWOUT "20054246"
#define Prompt "CMD>"

//IOs
#define TR_LEADSW   TRISBbits.RB2
#define DI_LEADSW   PORTBbits.RB2
#define DO_LED      LATCbits.LC1
#define TR_LED      TRISCbits.RC1

#define WAVEREC_DIVS    3
#define WAVEREC_MAX     ((1<<WAVEREC_DIVS)*50)
#define WAVEREC_MASK    ((1<<WAVEREC_DIVS)-1)

static unsigned char r2buf[20];//comm buffer from DC
static unsigned char r1buf[50];//comm buffer to/from RN4xxx
static unsigned char deviceID[15];
static unsigned char ModQ; //0:discon,1:adv,2:connect
//diag
static unsigned short Timer[]={0,0,0,0,0};//wallet scan,adv off,discon,battery
//Castum
static unsigned short CT_reg1,CT_reg2;
static unsigned short CT_wque=0,CT_wreg1=0,CT_wreg2=0;
static unsigned short CT_rev=0;
static unsigned char *CT_ptr;
static unsigned short CT_pwm;
static unsigned long CT_dt,CT_time;
signed long CT_tens;
static char *CT_buf=work_buffer+192;
static void CT_send();

//handler
void Timer_do();//Do every 1 sec
void WV_do(char *);//Do on receiving "%WV"
void WC_do(char *);//Do on receiving "%WC"
void CONN_do(char *);//Do on receiving "%CONNECT
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
void rn487x_aon(){//restart advertize
    rn487x_prog("A,0400,\n");
}
void rn487x_aoff(){//stop advertize
    USART_puts("A,0400,0001\n");
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
    xdelay(100);
//    while(!DI_RESET){//If ResetIC impremented
//       VSO_puts("RN487x external reset\n");
//        xdelay(1000);
//        VSO_puts("RN487x reset waiting\n");
//    }
    TR_RESET=0;
    DO_RESET=0;
    xdelay(200);
    DO_RESET=1;
    TR_RESET=1;
    if(USART_purge("%REBOOT")==0){
        rn487x_cmd();
        USART_purge(Prompt);
        if(adv==0) rn487x_aoff();
        else rn487x_aon();
        return 0;
    }
    else return -1;
}
int rn487x_reboot(){ return rn487x_reset(0);}

static void power_on(){
    if(Timer[3]==0){
        LATBbits.LB4=1;
        LATBbits.LB5=1;
        TRISBbits.RB4=0;
        TRISBbits.RB5=0;
        /*LATBbits.LB2=0;
        TRISBbits.RB2=0;*/
        xdelay(100);
    }
}
static void power_off(){
    TRISBbits.RB4=1;
    TRISBbits.RB5=1;
//    TRISBbits.RB2=1;
}
void setup(){
    int i,j,k;
    OSC_init(_XTAL_FREQ);
    xdelay(500);
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
    USART_puts("NA,09,");XSART_puts(DeviceName);USART_cr();//set LocalName(09) as BF11xxxx
    USART_purge(Prompt);
//    USART_puts("NA,07,");USART_2stup(IdServicePri""IdCommon);USART_cr();//set 128bits UUID(06)
//    USART_purge(Prompt);
    rn487x_prog("PZ\n");
    rn487x_prog("ST,0050,0100,0002,0064\n");//interval,latecy,timeout
    rn487x_prog("S-,"DeviceName"\n");
    rn487x_prog("SN,"DeviceName"\n");
    rn487x_prog("SO,0\n"); //sleep mode off
//    rn487x_prog("SO,1\n"); //enable sleep mode
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
    rn487x_prog("PC,"IdCharADS""IdCommon",08,03\n");
    rn487x_prog("PC,"IdCharWOUT""IdCommon",12,0A\n");
    rn487x_prog("LS\n");
    USART_shrink();
    rn487x_prog("V\n");
//App inits
    USART2_init(230400L);
//    USART2_init(222220L);
    TMR1_init();//32kHz
}

void loop(){
    int i,j,k,h,r2len;
ADVERTISE:
    ModQ=1;
    VSO_puts("//Adv mode\n");
//    rn487x_prog("IA,Z\n");
    rn487x_reset(1);//reset and keep advertise beacon on
    DO_RXIND=1;
//    rn487x_aload();
    r1buf[0]=r2buf[0]=0;
    TMR0_cemaphore=0;
    Timer[1]=1; //LED blink
ADVERTISE_LOOP:
//    WDTCONbits.SWDTEN=1;
//    Sleep();
//    WDTCONbits.SWDTEN=0;
    if(TMR0_cemaphore){
        Timer_do();
        TMR0_cemaphore--;
    }
    if(!DI_STAT1 && !DI_STAT2){
        ModQ=2;
    }

    if((r2len=USART2_gets(r2buf))>0){
        int cmd=r2buf[6];
//        switch(cmd){
//        case 0x22:
            ModQ=201;
//        }
    }
    switch(ModQ){
    case 2:
        goto CONNECT;
    case 201:
        goto WAVREC;
    }
    goto ADVERTISE_LOOP;
CONNECT:
    ModQ=2;
    DO_RXIND=0;
    Timer[0]=0;
    Timer[2]=30000;
    TMR0_cemaphore=0;
    xdelay(10);
    VSO_puts("//Connect mode\n");
    USART_cr();
//    USART_purge(Prompt);//flush cmd buffer
    CT_wque=0;
CONNECT_LOOP:
    k=USART_gets(r1buf,'%');
    if(DI_STAT1 || DI_STAT2){//Disconnected
        VSO_puts("//Disconnected/");VSO_putd(DI_STAT1);VSO_puts(",");VSO_putd(DI_STAT2);VSO_cr();
        Timer[0]=0;
        ModQ=1;
    }
    else if(k>0){
        if(!strncmp(r1buf,"WV",2)){
            WV_do(r1buf);
        }
        else if(!strncmp(r1buf,"WC",2)){
            WC_do(r1buf);
        }
        else if(!strncmp(r1buf,"CON",3)){//connect
            CONN_do(r1buf);
        }
        r1buf[0]=0;
    }
    else if(k==0){ //End with CR
//        VSO_puts("gets=");VSO_puts(r1buf);VSO_cr();
        r1buf[0]=0;
    }
    if((r2len=USART2_gets(r2buf))>0){
        int cmd=r2buf[6];
        switch(cmd){
        case 0x01:  //echo back
            break;
        case 0x02:  //parameter receive
            ModQ=102;
            break;
        case 0x12:  //write response
            XSART_putSHW(CH_WOUT);
            if(r2buf[8]) XSART_putInt16(0xA000|r2buf[7]);
            else XSART_putInt16(0xAFFF);
            USART_cr();
            USART_purge(Prompt);
            break;
        case 0x22:  //wave data
            ModQ=202;
            break;
        }
    }
    if(TMR0_cemaphore){
        Timer_do();
        TMR0_cemaphore--;
    }
    switch(ModQ){
    case 1:
        goto ADVERTISE;
    case 102:
        goto REGDUMP;
    case 202:
        goto WAVREC;
    case 255:
        rn487x_reboot();
        goto ADVERTISE;
    }
    goto CONNECT_LOOP;
WAVREC:
    VSO_puts("//Rec mode\n");
//    rn487x_aoff();
    CT_rev=CT_pwm=CT_dt=CT_tens=0;
    *(unsigned short *)CT_buf=0;
WAVREC_LOOP:
    CT_ptr=CT_buf+(CT_rev<WAVEREC_MAX? CT_rev>>WAVEREC_DIVS:WAVEREC_MAX>>WAVEREC_DIVS)*10;
    *(unsigned short *)CT_ptr=CT_rev;
    CT_time=*(unsigned short *)(CT_ptr+2)=(((unsigned long)r2buf[2]<<16)+((unsigned short)r2buf[3]<<8)+r2buf[4])>>8;
    CT_dt+=*(unsigned short *)(CT_ptr+4)=((unsigned short)r2buf[7]<<8)+r2buf[8];
    CT_pwm+=*(unsigned short *)(CT_ptr+6)=r2buf[10];
    CT_tens+=*(signed short *)(CT_ptr+8)=((short)(signed char)r2buf[13]<<8)+r2buf[14];
    *(unsigned short *)(CT_ptr+10)=0;     //Data end marker
    if(CT_rev<WAVEREC_MAX && (CT_rev&WAVEREC_MASK)==WAVEREC_MASK){
        *(unsigned short *)(CT_ptr+4)=CT_dt>>WAVEREC_DIVS;
        *(unsigned short *)(CT_ptr+6)=CT_pwm>>WAVEREC_DIVS;
        *(signed short *)(CT_ptr+8)=CT_tens>>WAVEREC_DIVS;
        CT_pwm=CT_dt=CT_tens=0;
    }
    CT_rev++;
    TMR1_set(200);
    while(!TMR1_up){
        if((r2len=USART2_gets(r2buf))>0){
            if(r2buf[6]==0x22) goto WAVREC_LOOP;
            else TMR1_set(200);
        }
    }
    VSO_puts("CT_done:");VSO_putd(CT_rev);VSO_cr();
    if(ModQ==201) goto ADVERTISE;
    else{
        VSO_puts("Try CT_send\n");
        CT_send();
        goto CONNECT;
    }
REGDUMP:
    CT_reg1=4095;//address limit
    CT_rev=0;//no wave data
REGDUMP_LOOP:
    switch(r2buf[6]){
    case 0x02:
        CT_reg2=r2buf[7];
        if(CT_reg1>CT_reg2) CT_reg1=CT_reg2;
        CT_buf[CT_reg2]=r2buf[8];
        break;
    case 0x03:
        VSO_puts("REGDUMP ");VSO_putd(CT_reg1);VSO_puts(":");VSO_putd(CT_reg2);VSO_cr();
        goto REGDUMP_EXIT;
    }
    TMR1_set(200);
    while(!TMR1_up){
        if((r2len=USART2_gets(r2buf))>0) goto REGDUMP_LOOP;
    }
REGDUMP_EXIT:
    {
        unsigned short n=0xB000;
        for(;CT_reg1<=CT_reg2;){
            XSART_putSHW(CH_WOUT);
            XSART_putInt16(n|CT_reg1);
            int i=0;
            for(;i<8 && CT_reg1<=CT_reg2;i++,CT_reg1++){
                XSART_putInt8(CT_buf[CT_reg1]);
            }
            USART_cr();
            USART_purge(Prompt);
        }
    }
REGDUMP_ERROR:
    ModQ=2;
    goto CONNECT;
}
void Timer_do(){
    int cflag,otim,ctim;
T0UP:
    if(Timer[0]==0) goto T1UP; else if(--Timer[0]>0) goto T1UP;
    if(ModQ==2) ModQ=255; //reset
T1UP://LED blink
    if(Timer[1]==0) goto T2UP; else if(--Timer[1]>0) goto T2UP;
    if(ModQ==1){
        DO_LED=1;
        TR_LED=0;
        xdelay(2);
        TR_LED=1;
        Timer[1]=1;
    }
T2UP://disconnect timer
    if(Timer[2]==0) goto T3UP; else if(--Timer[2]>0) goto T3UP;
    if(ModQ==2){
        rn487x_prog("K,1\n");
        Timer[0]=60;//reset timer
        VSO_puts("//Connection Timeout\n");
        ModQ=1;
    }
T3UP://power off
    if(Timer[3]==0) goto T4UP; else if(--Timer[3]>0) goto T4UP;
    power_off();
    VSO_puts("Power off\n");
T4UP://auto advertise timer
    if(Timer[4]==0) goto T5UP; else if(--Timer[4]>0) goto T5UP;
T5UP:
    return;
}
//static void mkwav();
static void CT_send(){
    char *buf=CT_buf;
//  mkwav();
    __verbose=0;
    for(;CT_rev>0;buf+=10){
        unsigned short n=*(unsigned short *)buf;
        unsigned short t=*(unsigned short *)(buf+2);
        unsigned short dt=*(unsigned short *)(buf+4);
        unsigned short p=*(unsigned short *)(buf+6);
        signed short f=*(signed short *)(buf+8);
        if(n==0) break;
        XSART_putSHW(CH_WOUT);XSART_putInt16(n);XSART_putInt16(t);XSART_putInt16(dt);XSART_putInt16(p);XSART_putInt16(f);USART_cr();
        USART_purge(Prompt);
        VSO_puts("[");VSO_putd(n);VSO_puts(",");VSO_putd(dt);VSO_puts("]\n");
     }
     __verbose=1;
}
void WV_do(char *rs){
    int a,b,i;
    unsigned int han;
    unsigned long dat;
//    VSO_puts("WV:");VSO_puts(rs);VSO_cr();
    XSART_parse(rs,&han,&dat);//chara handle
    switch(han){
    case CH_ADS:
        VSO_puts("ADS=");VSO_putlx(dat);VSO_cr();
        if(dat==0xFC01){//Request to send wave data
            CT_send();
        }
        else if(dat==0xFA01){//Request to send ROM data
            unsigned char dat=0;
            power_on();
            Timer[3]=10;//Power-off timer
            USART2_cmd(1,&dat,1);
        }
        else if((dat&0xF00000)==0xA00000){
            unsigned char buf[2];
            buf[1]=dat&0xFF;
            buf[0]=(dat>>8)&0xFF;
            power_on();
            Timer[3]=5;
            USART2_cmd(0x11,buf,2);
        }
        break;
    }
}
void WC_do(char *rs){
    unsigned int han;
    unsigned long dat;
    XSART_parse(rs,&han,&dat);//chara handle
    VSO_puts("WC=");VSO_putx(han);VSO_cr();
/*    if(han==CH_WOUT+1){
        unsigned char cmd=0;
        power_on();
        Timer[3]=10;//Power-off timer
        USART2_cmd(1,&cmd,1);
    }*/
}
void CONN_do(char *rs){
    unsigned int han;
    unsigned long dat;
    unsigned char cmd=0;
    XSART_parse(rs,&han,&dat);
    VSO_puts("CONN=");VSO_putx(han);VSO_puts(",");VSO_putlx(dat);VSO_cr();
//    rn487x_prog("B\n");
}

//Comm libs b.w. RN487x
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
void XSART_putInt8(int d){
    char s[3];
    sprintf(s,"%02x",d);
    USART_puts(s);
}
void XSART_putInt16(int d){
    char s[5];
    sprintf(s,"%04x",d);
    USART_puts(s);
}
void XSART_putSHW(int d){
    char s[10];
    sprintf(s,"SHW,%04x,",d);    
    USART_puts(s);
}
unsigned long hex2byte(char *s){
    unsigned long val=0;
    for(;;val<<=4){
        int c=*s;
        int h=c-'0';
        if(h>9) h=c-'A'+10;
        val+=h;
        s++;
        c=*s;
        if(c>='0' && c<='9') continue;
        if(c>='A' && c<='F') continue;
        break;
    }
    return val;
}
int hex2str(char *src,char *dst){
    int n=strlen(src)/2;
    for(int i=0;i<n;i++,src+=2,dst++){
        *dst=hex2byte(src);
    }
    *dst=0;
    return n;
}
int XSART_parse(unsigned char *src,unsigned int *handle,unsigned long *val){
    char *d1=strchr(src,',');
    if(d1==NULL) return -1;
    char *d2=strchr(d1+1,',');
    if(d2==NULL) return -1;
    *val=hex2byte(d2+1);
    *handle=hex2byte(d1+1);
    return 0;
}


////For testing
static void mkwav(){
    CT_dt=3000L*32;
    CT_pwm=100*32;
    CT_tens=1000L*32;
    CT_time=CT_dt;
    CT_rev=31;
LOOP:
    if((CT_rev&0x1F)==0x1F){
        int tmsec=CT_time>>10;
        char *buf=work_buffer+128+(CT_rev>>5)*10;
        *(unsigned short *)buf=CT_rev;
        *(unsigned short *)(buf+2)=tmsec;
        *(unsigned short *)(buf+4)=CT_dt>>5;
        *(unsigned short *)(buf+6)=CT_pwm>>5;
        *(signed short *)(buf+8)=CT_tens>>5;
    }
    CT_time+=CT_dt;
    CT_pwm=((long)CT_pwm*225)>>8;
    CT_dt=(CT_dt*275)>>8;
    CT_tens=CT_tens-3000;
    CT_rev+=32;
    if(CT_rev<500) goto LOOP;
}
