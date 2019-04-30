
#include <xc.h>
#include <stdio.h>
#include <string.h>
#include "lcd_x8.h"

#define START_CHAR '*'   
#define END_CHAR  '$'
#define COMMAND_START 1
#define COMMAND_END 0

unsigned char Buffer[32];
unsigned char cmnd[64];
int adc_value = 0;
float voltage = 0;
float temp =0;

void refresh_lcd();
void interrupt myIsr(void)
{
// only process timer-triggered interrupts
    if(INTCONbits.TMR0IE && INTCONbits.TMR0IF) {
        
        refresh_lcd();
        INTCONbits.TMR0IF = 0; // clear this interrupt condition
    }
}
void init_adc_no_lib() {
    ADCON0 = 0;
    ADCON1 = 0b00001101; //3 analog channels, change this according to your application
    ADCON0bits.ADON = 1; // turn adc on 
    ADCON2 = 0b10001101; // right justified 10 bits
}
int read_adc_raw_no_lib(unsigned char channel) {
    int raw_value; //0--1023
    ADCON0bits.CHS = channel;
    ADCON0bits.GO = 1; // start conversion
    while (ADCON0bits.GO) {}; // wait until conversion is done
    raw_value = ADRESH << 8 | ADRESL; // 10 bit, need to shift the bits right
    return raw_value;
}
void refresh_lcd(){
    adc_value = read_adc_raw_no_lib(1); //chanel 1  AN1
    voltage = (adc_value*5.0)/1024;
    sprintf(Buffer,"\fVoltage = %3.2f V\r",voltage);
    lcd_puts(Buffer);
    int adc_value_ = read_adc_raw_no_lib(0); //chanel 0 AN0
    adc_value_ = read_adc_raw_no_lib(0);
    adc_value_ = read_adc_raw_no_lib(0);
    temp = (adc_value_/2.048+2); // parentheses used to illustrate max  and min range
    lcd_gotoxy(1,2);
    sprintf(Buffer,"Temp = %3.2f C\r",temp);
    lcd_puts(Buffer);
}
void setupSerial()
{
        unsigned char dummy;
        BAUDCONbits.BRG16 =0;
        TXSTA =0;
		SPBRG = 25;
        SPBRGH =0;
		TXSTAbits.BRGH= 1;  //baud rate high speed option
		TXSTAbits.TXEN = 1; //	;enable transmission
		RCSTA =0;           // ;SERIAL RECEPTION W/ 8 BITS,
		RCSTAbits.CREN =1;	//;enable reception
		RCSTAbits.SPEN = 1;	//enable serial port
        dummy = RCREG;      //,W ; clear the receiver buffer      
        dummy = RCREG;      //,W ; clear the receiver buffer
		return	;
}
void setupPorts()
{
    TRISB = 0xFF;
    TRISD = 0;
    TRISE = 0;
    TRISC = 0x80;
    TRISA = 0xFF;
    // for timer zero
    T0CON = 0b10000101;
    TMR0L = 0xE5;
    TMR0H = 0x48;        
    INTCONbits.TMR0IE = 1;
    
     PR2 = 255;
    T2CON = 0;
    CCP1CON = 0x0C;
    T2CONbits.TMR2ON = 1; 
    TRISCbits.RC2 =0;
    ei();
    
    setupSerial();
}
unsigned char isSerialAvailable(){
    if (!(PIR1bits.RCIF))
    {
         return 0;
    }
     return 1;
}
unsigned char read_byte()
{
    unsigned char c = RCREG; 
    return c;
}
void send_byte_no_lib(unsigned char c) {
    while (!TXSTAbits.TRMT)//fixed 
    {
        CLRWDT()  ; //if enabled  
    }
    TXREG = c;
}
void send_string_no_lib(unsigned char *p) {
    while (*p) {
        send_byte_no_lib(*p);
        p++;
    }   
}
void send_analog_zero_serial(){
    adc_value = read_adc_raw_no_lib(1);
    voltage = (adc_value*5.0)/1024;
    sprintf(Buffer,"Voltage = *%3.2f$ V\r",voltage);
    send_string_no_lib(Buffer);
    
}
void send_analog_one_serial(){
     adc_value = read_adc_raw_no_lib(0);
    float temp;
    temp = (adc_value/2.048+2); // parentheses used to illustrate max  and min range
    sprintf(Buffer,"Temp = *%3.2f$ C",temp);
    send_string_no_lib(Buffer);
}
void exec_cmnd(char * cmnd){
    if(!stricmp(cmnd,"Vol") ){
        send_analog_zero_serial();
    }
    else if(!stricmp(cmnd,"Temp")){
        send_analog_one_serial();
    }
    else if(!stricmp(cmnd,"RD0")){
        
    }
    else if(!stricmp(cmnd,"RD1")){
        
    }
    else{
         lcd_putc('i');
    }
}
void set_pwm1_raw(unsigned int raw_value)//raw value 0 -- 1023
{
    CCPR1L = (raw_value >> 2) & 0x00FF; //the other 8 bits
    CCP1CONbits.DC1B = raw_value & 0x0003; //first two bits LSB
}
void set_pwm1_percent(float value)// value 0--100%
{
    float tmp = value*1023.0/100.0;
    int raw_val = (int)(tmp +0.5);
    if ( raw_val> 1023) raw_val = 1023;
    set_pwm1_raw(raw_val);
}

void motor(float value)
{
  set_pwm1_percent(value);        
}

unsigned char cmnd_flag = COMMAND_END;
unsigned short cmnd_ptr = 0;
unsigned char c;
void main(void) {
    setupPorts();
    delay_ms(100);
    init_adc_no_lib();
    lcd_init();
    lcd_puts((char *)"\f:");
    
    while(1) { 
        int adc_value_ = read_adc_raw_no_lib(0);
       float temp_mot = (adc_value_/2.048+2); // parentheses used to illustrate max  and min range
         lcd_gotoxy(1,2);
        sprintf(Buffer,"Temp = %3.2f C\r",temp);
        lcd_puts(Buffer);
       //compare the value of the temp and give the percentage you want for the motor
         if(temp_mot>=10 && temp_mot<=29)
        {
            motor(25.0);
        }
        else if(temp_mot>=30 && temp_mot<=49)
        {
            motor(50.0);
        }
        else if(temp_mot>=50 && temp_mot <=79)
        {
            motor(75.0);
        }
        else if(temp_mot>=80 && temp_mot <=99)
        {
            motor(100.0);
        }
        
        /* the commented code below is to send a command line to the serial as this *temp$*/
      /*  if(isSerialAvailable()){
          c = read_byte();
          if(c == START_CHAR){
              cmnd_flag= COMMAND_START;
              cmnd_ptr = 0;
          }
          else if (c == END_CHAR){
              cmnd[cmnd_ptr]='\0';
              cmnd_flag = COMMAND_END;
              cmnd_ptr = 0;
              exec_cmnd(cmnd);
              memset(cmnd, 0, sizeof cmnd);
          }
          else {
              if(cmnd_flag == COMMAND_START){
                  if(cmnd_ptr == 63){
                      
                  }
                  else 
                    cmnd[cmnd_ptr++] = c;
              }
              else{
                  lcd_putc(c);
              }
          }
        }*/
        CLRWDT();
        
    }//while
    return;
}
