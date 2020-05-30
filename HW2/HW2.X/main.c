#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro

////// SPI communication  ///////
#include"spi.h"
#include<math.h>

// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF // disable secondary oscillator
#pragma config IESO = OFF // disable switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // disable clock output
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // disable clock switch and FSCM
#pragma config WDTPS = PS1048576 // use largest wdt
#pragma config WINDIS = OFF // use non-window mode wdt
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 00000001 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations


// start of hw2 //
#define ONE_SEC 24000000 // this is 1 seconds

// write DAC to SPI
void write_DAC(unsigned char channel, unsigned int n){
    // bit 15 = channel (channel = 0 for VrefA, channel = 1 for VrefB)
    // bit 14 = 1 (Buffer Control)
    // bit 13 = 1 (Gain of 1)
    // bit 12 = 1 (no shutdown)
    // bits 11-0 = n (n is integer between 0-4095)
    
    // P1 = first half of SPI message, P2 = second half of SPI message
    unsigned char P1, P2;
    
    // set channel
    P1 = channel << 7;
    
    // set configuration
    P1 |= (0b111 << 4);
    
    // set n
    P1 |= (n >> 8) & 0b1111;
    
    // set P2 (casting n from int to char will truncate 8 bits)
    P2 = (unsigned char) n;
    
    // set CS low
    LATAbits.LATA0 = 0;
    
    // send messages
    spi_io(P1);
    spi_io(P2);
    
    // set CS high
    LATAbits.LATA0 = 1;
   
}

// wait function (wait 1/256 of a second)
void wait(){
    _CP0_SET_COUNT(0);
    while (_CP0_GET_COUNT() < ONE_SEC/256);
}

int main() {

    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    TRISAbits.TRISA4 = 0;
    TRISBbits.TRISB4 = 1;
    
    
    // init SPI
    initSPI();
    
    __builtin_enable_interrupts();
    
    // divide signals into 256 for sine (2Hz) and 256 for triangle (1Hz)
    unsigned int sine[256];
    unsigned int triangle[256];
    
    // 1Hz triangle wave in 256 pieces
    int t = 0;
    while (t < 256){
        // increasing wave
        if (t < 128){
            triangle[t] = 32*t;
        }
        
        // peak value (set to 4095 so it shows)
        else if (t == 128){  
            triangle[t] = 4095;
        }
        
        // decreasing wave
        else{
            triangle[t] = 8192-(32*t);
        }
        t++;
    }
    
    // 2Hz singe wave in 256 pieces
    t = 0;
    const float Pi = 3.1415;
    while (t < 256){
        // fill in with sine wave
        sine[t] = 2048*sin(2.0 * Pi * t / 128) + 2048;
        // fix peak value
        if (sine[t] == 4096) sine[t] = 4095;
        t++;
    }
    
    unsigned int i = 0;
    unsigned int j = 0;

    while (1) {
        // use _CP0_SET_COUNT(0) and _CP0_GET_COUNT() to test the PIC timing
        // remember the core timer runs at half the sysclk
        
        // test
        // write_DAC(0, 2048);
        
        // sine wave (2Hz)
        write_DAC(0, sine[i]);
        
        // triangle wave (1Hz))
        write_DAC(1, triangle[j]);
        
        // increment i and j
        i++;
        j++;
        
        // reset i and j if past
        if (i == 127) i = 0;
        if (j == 255) j = 0;
        
        // wait for 1/256 seconds
        wait();

    }
    
    return -1;
}