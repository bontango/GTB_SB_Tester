/* 
 * File:   lcd_hd44780.c
 * Author: th
 * Display lib for Display with HD77480 controoler
 * PIC 18F45K22
 * READ Port / Write LAT
 *
 * v0.2 without BUSY bit (just delay)
 */


#include <stdint.h>
#include "def.h"
#include "lcd_hd44780.h"

//1 & 10 working but slow
#define MY_LCD_DELAY 1  
#define MY_BUSY_DELAY 10
#define MY_BUSY_CHAR_DELAY 10

void set_data(unsigned char data)
{
    my_byte_t mybyte;
    
    mybyte.byte = data;        
    LCD_DATA_4 = mybyte.bitv.DATA_4;
    LCD_DATA_5 = mybyte.bitv.DATA_5;
    LCD_DATA_6 = mybyte.bitv.DATA_6;
    LCD_DATA_7 = mybyte.bitv.DATA_7;        
    __delay_us(MY_LCD_DELAY);
}

void delay_100us(int delay)
{
    int i,j;
    
    for(i=0;i<delay; i++)
    {
        for(j=0;j<10; j++) __delay_us(10); //ten times 10us       
    }                        
}

void toggle_E(void)
{
    LCD_E = 1;
    __delay_us(MY_LCD_DELAY);    
    LCD_E = 0;        
    //__delay_us(MY_LCD_DELAY);
}

void LCD_BUSY_CHAR(void)
{
    int i;
    for(i=0;i<MY_BUSY_CHAR_DELAY; i++)
    {
        __delay_us(10); //10us       
    }                            
}

void LCD_BUSY(void)
{
    delay_100us(MY_BUSY_DELAY);
    /*
unsigned char temp1;

do
{
LCD_DATA_TRIS_BIT = 1; // bit7 (busy-bit) als Eingang definieren
LCD_RS = 0; // LCD im Befehls-Mode
LCD_RW = 1; // LCD im Lesen-Mode
toggle_E();// Befehl an LCD, get high-nibble
__delay_us(MY_LCD_DELAY);			//wait
temp1 = LCD_BUSY_BIT;
toggle_E();// Befehl an LCD, get low nibble
} while (temp1);//bit7 is Busy flag

LCD_RW = 0; // Busy = low: LCD im Schreiben-Mode
LCD_DATA_TRIS_BIT = 0; // als Ausgang definieren
//add 4us
__delay_us(MY_LCD_DELAY); //RTH
*/
     } 

    
void LCD_INIT(void)
{
LCD_RS = 0; // Alle LCD-Steuerleitungen LOW
LCD_RW = 0;
LCD_E = 0;
delay_100us(1500); //wait 150ms
set_data(0x30);
toggle_E();// Befehl an LCD
delay_100us(41); // 4.1 ms warten
toggle_E();// Befehl an LCD
delay_100us(1); // 100 us warten
toggle_E();// Befehl an LCD
LCD_BUSY(); // warten bis LCD bereit
//LCD_DATA = LCD_DATA & 0x0F; // Hoeherwertigeres Nibble loeschen
//LCD_DATA = LCD_DATA | 0x20; // Befehl fuer 4-Bit-Interface
set_data(0x20);
toggle_E();// Befehl an LCD
LCD_BUSY(); // warten bis LCD bereit
LCD_CMD(LCD_CMD_CLR); // Display loeschen und Cursor home
LCD_CMD(LCD_CMD_FUNCTION_SET); // 4 Bit, 2 Zeilem, 5x7
LCD_CMD(LCD_CMD_DISP_OFF); // Display aus, Cursor aus, Blinken aus
LCD_CMD(LCD_CMD_ENTRY_INC); // Shift aus
LCD_CMD(LCD_CMD_DISP_ON); // Display ein
}

void LCD_CMD(unsigned char befehl)
{
unsigned char temp;

//LCD_DATA = LCD_DATA & 0x0F; // Hoeherwertigeres Nibble loeschen
temp = befehl & 0xF0; // Hoeherwertiges Nibble ausmaskieren
//LCD_DATA = LCD_DATA | temp; // High-Nibble an LCD uebergeben

LCD_RW = 0; // LCD im Schreiben-Mode
LCD_RS = 0; // LCD im Befehl-Mode
LCD_BUSY(); // Warten bis LCD bereit ist

set_data(temp);

toggle_E();// Befehl an LCD
//LCD_DATA = LCD_DATA & 0x0F; // Hoeherwertigeres Nibble loeschen
//SWAPF(befehl,1);
befehl = (befehl << 4) | (befehl >> 4);
temp = befehl & 0xF0; // Niederwert. Nibble ausmaskieren
//LCD_DATA = LCD_DATA | temp; // High-Nibble an LCD uebergeben
set_data(temp);
toggle_E();// Befehl an LCD

LCD_BUSY(); // Warten bis LCD bereit ist
}

void LCD_CHAR(unsigned char zeichen)
{
unsigned char temp;

//LCD_DATA = LCD_DATA & 0x0F; // Hoeherwertigeres Nibble loeschen
temp = zeichen & 0xF0; // Hoeherwertiges Nibble ausmaskieren
//LCD_DATA = LCD_DATA | temp; // High-Nibble an LCD uebergeben

LCD_RW = 0; // LCD im Schreiben-Mode
LCD_RS = 1; // LCD im Daten-Mode
LCD_BUSY_CHAR(); // Warten bis LCD bereit ist
set_data(temp);

toggle_E();// Befehl an LCD
//LCD_DATA = LCD_DATA & 0x0F; // Hoeherwertigeres Nibble loeschen
//SWAPF(zeichen,1);
zeichen = (zeichen << 4) | (zeichen >> 4);
temp = zeichen & 0xF0; // Niederwert. Nibble ausmaskieren
//LCD_DATA = LCD_DATA | temp; // High-Nibble an LCD uebergeben
set_data(temp);
toggle_E();// Befehl an LCD
}

void LCD_STRING(const char *pString)
{
char zeichen;
zeichen = *pString;
while(zeichen > 0)
{
LCD_CHAR(zeichen); // zeichen am LC-Display ausgeben
pString++; // Zeiger um 1 erhoehen, er zeigt nun auf das
// naechste Zeichen der auszugebenden Zeichen-
// kette
zeichen = *pString; // Den Inhalt diese Zeigers in die Variable
} // zeichen kopieren.
}

void LCD_GotoXY(uint8_t x,uint8_t y)
{
    //for 4 lines 20 chars
    if(x>=20) return;
	switch(y)
	{
		case 0:
			break;
		case 1:
			x|=0b01000000;
			break;
		case 2:
			x+=0x14;
			break;
		case 3:
			x+=0x54;
			break;
	}
    
    x|=0b10000000;
  	LCD_CMD(x);        
    
}

