/* 
 * File:   main.c
 * Author: th
 * Gottlieb SYS80 pinball tester
 * PIC 18F45K22
 * READ Port / Write LAT
 * 
 * v 0.1 based on bally Tester
 * v 0.3 SOL8/SOL9 fixed, sound test drives sound lines,
 *       from/to range limits, t:0 lockout and switch debounce fixed
 */

//no warnings about 
#pragma warning push
#pragma warning disable 359 //illegal conversion between pointer types
#pragma warning disable 373 //'implicite signed to unsigned conversion'

#include "def.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "lcd_hd44780.h"

#define VERSION "0.3"

//typedefs
typedef union typed {
    unsigned char byte;
    struct {    
    unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    } bitv_t;
    

//function prototypes
void refresh_display(void);
uint8_t check_OK_sw(void);

//globals
char line[30];
uint8_t cur_menu = 0; //current selected menu
uint8_t cur_line = 0; //current selected line in menu
uint8_t cur_option[3] = { 0, 0, 0 }; //current option in selection
int cur_pulse = 150; //pulsetime in msec
int cur_sound_t = 1; //time n seconds to go to next sound i  test
int cur_sol_t = 1; //time n seconds to go to next solenoid i  test
int cur_lamp_t = 1; //time n seconds to go to next device i  test
uint8_t cur_mode = 0;

/*const char * mode[] = {
        "blAll",
        "blHal",
        "serie"        
    };
  */

//menu entries
const char * main_menu[] = {
    "Test Lamps",
    "Test Sound",    
    "Test Solenoids"
    };
uint8_t n_main_menu = 3;    

    const char * lamp_option[] = {
        //"all lamps ( L2...L48)",
        "all lamps",
        "excl. all u.pl. ( L8...L16)",
        "excl. u.pl. Mars",        
		"excl. u.pl. Volcano",
        "excl. u.pl. BH",
        "excl. u.pl. HH"
		
    };

    const char * sound_option[] = {
        "small SB ",
        "big SB 80&80A",
        "big SB 80B"
    };

  const char * solenoid_option[] = {
        "SOL1 ... SOL9",
        "Tilt, GO & C.Lockout"
    };

  //number of options fior the menus lamp/sound/solenoid/
  uint8_t n_option[3] = { 6, 3, 2 };
  
  //default values from to for the 3 main menues
  int select_from[3] = { 2, 1, 1 };
  int select_to[3] = { 48, 15, 9 };
  //limits for from/to: lamps 12 flip-flops * 4, sound 4 lines, 9 solenoids
  const int select_min[3] = { 2, 1, 1 };
  const int select_max[3] = { 49, 15, 9 };
  
  
    
 char line3[21], line4[21];
                 
//globals
unsigned char switchstatus[10]={0,0,0,0,0,0,0,0,0,0}; //status of the 9 switches

//util sleep n * 100ms
//while checking OK_SW
//breaks & return 1 if OK SW was pressed
//return 0 at end of sleep time
uint8_t sleep_n_100ms( int n)
{
    int i;
    for (i=0; i<n; i++)
    {
        __delay_ms(100);
        //OK switch pressed?
        if ( check_OK_sw() == 1 ) return 1;
    }
    
    return 0;
        
}


/*
 subrotines for settings
 */

//set output on 238
set_238( unsigned char data)
{
        
    bitv_t ds_select;
    
    ds_select.byte = data;
    
    //disable 238
    DS_EN = 1;
    //set output
    DS_A0 = ds_select.bitv.b0;
    DS_A1 = ds_select.bitv.b1;
    DS_A2 = ds_select.bitv.b2;
    DS_A3 = ds_select.bitv.b3;
    //enable 238
    DS_EN = 0;
}

/*
        lisy80_coil_set
        coil -> nr of coil
 *          0..47 Lamps
 *          48..51 Sound 1,2,4,8
 *          52 .. 63 Coils
        action -> 0-off, 1-on
*/
void q_set( int q, int action) {
unsigned char flip_flop, coil;        
                
//remeber the flip_flop settings of each
static union lisy80_flip_flop {
        unsigned char byte;
struct {
        unsigned FREE:4, ld1:1, ld2:1, ld3:1, ld4:1;
  } bitv;
//} flip_flop_byte[13] = { 0,0,0,0,0,0,0,0,0,0,0,0 };
} flip_flop_byte[13];

//q1 is coil 0
coil = q-1;
                 // compute number of flip_flop which is a 74175 
                 flip_flop = ( coil / 4) + 1;
                 //only flip_flop_byte[1..12] exist
                 if ( flip_flop > 12 ) return;
                 
                 //prepare the input and think of old status
                 switch ( coil % 4)     //the rest
                    {
                        case 0: flip_flop_byte[flip_flop].bitv.ld1 = action; break;
                        case 1: flip_flop_byte[flip_flop].bitv.ld2 = action; break;
                        case 2: flip_flop_byte[flip_flop].bitv.ld3 = action; break;
                        case 3: flip_flop_byte[flip_flop].bitv.ld4 = action; break;
                    }

                //now set LD1..4 to the rigth values
                LD01 = flip_flop_byte[flip_flop].bitv.ld1;
                LD02 = flip_flop_byte[flip_flop].bitv.ld2;
                LD03 = flip_flop_byte[flip_flop].bitv.ld3;
                LD04 = flip_flop_byte[flip_flop].bitv.ld4;
                                
                 //push the data to the rigth flip_flop
                //setup time of 74175 is 20 nanoseconds
                 switch ( flip_flop )
                    {                      
                     //outputs on HW 1.x are different because of wiring
                        case 1:  set_238(1); break; 
                        case 2:  set_238(0); break;
                        case 3:  set_238(3); break;
                        case 4:  set_238(2); break;
                        case 5:  set_238(5); break;
                        case 6:  set_238(4); break;
                        case 7:  set_238(7); break;
                        case 8:  set_238(6); break;
                        
                        case 9:  set_238(9); break;
                        case 10:  set_238(8); break;
                        case 11:  set_238(10); break;
                        case 12:  set_238(11); break;
                        case 13:  set_238(12); break;
                        case 14:  set_238(13); break;
                        case 15:  set_238(14); break;                        
                        case 16:  set_238(15); break;
                    }           
                 
                 //back to rest position                 
                 set_238(15);
                 
}

//set momentary solenoids or sound raw
void solenoid_pulse(unsigned char sol)
{
    switch (sol)
                        {                         
                         case 1: SOL1 = 1; __delay_ms(cur_pulse); SOL1 = 0; break;
                         case 2: SOL2 = 1; __delay_ms(cur_pulse); SOL2 = 0; break;
                         case 3: SOL3 = 1; __delay_ms(cur_pulse); SOL3 = 0; break;
                         case 4: SOL4 = 1; __delay_ms(cur_pulse); SOL4 = 0; break;
                         case 5: SOL5 = 1; __delay_ms(cur_pulse); SOL5 = 0; break;
                         case 6: SOL6 = 1; __delay_ms(cur_pulse); SOL6 = 0; break;
                         case 7: SOL7 = 1; __delay_ms(cur_pulse); SOL7 = 0; break;
                         case 8: SOL8 = 1; __delay_ms(cur_pulse); SOL8 = 0; break;
                         case 9: SOL9 = 1; __delay_ms(cur_pulse); SOL9 = 0; break;
                         default:        break;
                        }

  }

//sound data standard soundboard
//Gottlieb: sound number binary on Sound 1,2,4,8 for pulse time, then back to 0
void sound_data_standard(unsigned char value)
{
    bitv_t my_value;

    my_value.byte = value;

    SOUND1 = my_value.bitv.b0;
    SOUND2 = my_value.bitv.b1;
    SOUND4 = my_value.bitv.b2;
    SOUND8 = my_value.bitv.b3;
    __delay_ms(cur_pulse);
    SOUND1 = SOUND2 = SOUND4 = SOUND8 = 0;
}


//debounce
//check if status changed and was stable for 100ms
//if yes assign new switchstatus
//give back 1 if it changed from 0 to 0
//give back 0 otherwise

unsigned char debounce(unsigned char sw, unsigned char status)
{
    if (status != switchstatus[sw])
    {
        __delay_ms(100);    //wait debounce time
        //still changed?
        if (status != switchstatus[sw])
        {
            switchstatus[sw] = status;
            return(status);            
        }        
    }
    
    return 0;
}

//check status of 'OK' (S1) switch
// strobe 2, Ret1
//no blocking
//Gottlieb version
// 1== closed; 0==open
uint8_t check_OK_sw(void)
{        
    
    unsigned char status;    
            
    STROBE_1 = 1; STROBE_2=0; STROBE_3=1; 
    __delay_ms(10);    
    //S1 == RET1
    status = RETURN_1;
        
    //strobeall zero
    STROBE_1 = 1; STROBE_2=1; STROBE_3=1; 

    if (status == 1) status = 0; else status = 1;
    return(status);
 }

//check all switches and give back the number
//of the switch which went from open to close
//blocking mode
// 1== closed; 0==open

//switches
#define SW_OK 1 //Str2, Ret1
#define SW_UP 3 //Str1, Ret1
#define SW_DOWN 4 //Str1, Ret3
#define SW_LEFT 2 //Str1, Ret2
#define SW_RIGHT 5 //Str2, Ret2

//GOttlieb Version
unsigned char check_sw(void)
{        
    
    unsigned char status;    
        
    while(1) //wait for switch change
    {    
    //scan all switches
    
    //initial delay
    __delay_ms(10);        
        
    //Strobe1
    STROBE_1 = 0; STROBE_2=1; STROBE_3=1; 
    __delay_ms(10);    
    //S3 == RET1
    status = RETURN_1;
    if (status == 1) status = 0; else status = 1;
    if ( debounce(SW_UP, status) ) return(SW_UP);
    //S2 == RET2
    status = RETURN_2;
    if (status == 1) status = 0; else status = 1;
    if ( debounce(SW_LEFT, status) ) return(SW_LEFT);
    //S4 == RET3
    status = RETURN_3;
    if (status == 1) status = 0; else status = 1;
    if ( debounce(SW_DOWN, status) ) return(SW_DOWN);
                        
    //Strobe2
    STROBE_1 = 1; STROBE_2=0; STROBE_3=1; 
    __delay_ms(10);    
    //S1 == RET1
    status = RETURN_1;
    if (status == 1) status = 0; else status = 1;
    if ( debounce(SW_OK, status) ) return(SW_OK);
    //S5 == RET2
    status = RETURN_2;
    if (status == 1) status = 0; else status = 1;
    if ( debounce(SW_RIGHT, status) ) return(SW_RIGHT);
                
    //strobeall zero
    STROBE_1 = 1; STROBE_2=1; STROBE_3=1; 
    }
    
    return 0; //never reached
}

//dip switch TBD
/*
     //S6/Dip1 == RET3
    status = RETURN_3;
    if ( debounce(6, status) ) return(6);        
    STROBE_1 = 0; STROBE_2=0; STROBE_3=1; 
    __delay_ms(10);        
    //S6/Dip2 == RET1
    status = RETURN_1;
    if ( debounce(7, status) ) return(7);    
    
    switchstatus[7] = RETURN_1;
    //S6/Dip3 == RET2
    switchstatus[8] = RETURN_2;
    //S6/Dip4 == RET3
    switchstatus[9] = RETURN_3;

 */    


//refresh display with menu 1..3
void refresh_display( void )
{
    char *entry1,*entry2,*entry3,*entry4;
    
    entry1 = main_menu[cur_menu];
    
    //construct line 3
    sprintf(line3,"from:0x%02x to:0x%02x",select_from[cur_menu], select_to[cur_menu]);
    entry3 = line3;
       
    //construct line 3
    entry4 = line4;
    //  char line4[] = "mode:---- t:1.0sec";
    // char line4[] = "pls:150ms t:1.0sec";

    
    switch(cur_menu)
    {
        case 0: //lamps
            entry2 = lamp_option[cur_option[cur_menu]];
            //sprintf(line4,"mode:%s t:%d sec",mode[cur_mode],cur_t);
            sprintf(line4,"t:0.%d sec",cur_lamp_t);
            entry4 = line4;
            break;
        case 1: //sound
            entry2 = sound_option[cur_option[cur_menu]];            
            sprintf(line4,"t:%d sec",cur_sound_t);
            entry4 = line4;            
            break;
        case 2: //solenoid
            entry2 = solenoid_option[cur_option[cur_menu]];                        
            //sprintf(line4,"puls:%3dms t:%d sec",cur_pulse,cur_t);
            sprintf(line4,"t:%d sec",cur_sol_t);
            entry4 = line4;                        
            break;            
    }
        
//display each line with '> ' heading    
LCD_CMD(LCD_CMD_CLR);
sprintf(line,"> %s",entry1);
LCD_GotoXY(0,0);
LCD_STRING(line);
sprintf(line,"> %s",entry2);
LCD_GotoXY(0,1);
LCD_STRING(line);
sprintf(line,"> %s",entry3);
LCD_GotoXY(0,2);
LCD_STRING(line);
sprintf(line,"> %s",entry4);
LCD_GotoXY(0,3);
LCD_STRING(line);

//cursor position
LCD_GotoXY(0,cur_line);
    
}

/*-----------------------------------
line 3 make selection
from - to
-------------------------------------*/
void select_menu(void)
{
    uint8_t my_switch;
    uint8_t pos[4] = { 9, 10, 17, 18 };
    uint8_t current_pos = 0;

    LCD_GotoXY(pos[current_pos],2);
    
  do
  {
   my_switch = check_sw();
   switch ( my_switch )
   {
       case SW_LEFT:
           if (current_pos > 0) --current_pos;
           else my_switch = SW_OK; //exit to the left
           break;
       case SW_RIGHT:
           if (current_pos < 3) ++current_pos;
           else current_pos = 0; //wrap           
           break;              
       case SW_UP:           
           switch(current_pos)
           {
               case 0:
                   if (( select_from[cur_menu] + 0x10) <= select_max[cur_menu]) select_from[cur_menu] += 0x10;
                   if (select_from[cur_menu] > select_to[cur_menu]) select_to[cur_menu] = select_from[cur_menu];
                   break;
               case 1:
                   if (( select_from[cur_menu] + 1) <= select_max[cur_menu]) select_from[cur_menu] += 1;
                   if (select_from[cur_menu] > select_to[cur_menu]) select_to[cur_menu] = select_from[cur_menu];
                   break;
               case 2:
                   if (( select_to[cur_menu] + 0x10) <= select_max[cur_menu]) select_to[cur_menu] += 0x10;
                   break;
               case 3:
                   if (( select_to[cur_menu] + 1) <= select_max[cur_menu]) select_to[cur_menu] += 1;
                   break;
           }
           break;
       case SW_DOWN:
           switch(current_pos)
           {
               case 0:
                   if (( select_from[cur_menu] - 0x10) >= select_min[cur_menu]) select_from[cur_menu] -= 0x10;
                   break;
               case 1:
                   if (( select_from[cur_menu] - 1) >= select_min[cur_menu]) select_from[cur_menu] -= 1;
                   break;
               case 2:
                   if (( select_to[cur_menu] - 0x10) >= select_min[cur_menu]) select_to[cur_menu] -= 0x10;
                   if (select_to[cur_menu] < select_from[cur_menu]) select_from[cur_menu] = select_to[cur_menu];
                   break;
               case 3:
                   if (( select_to[cur_menu] - 1) >= select_min[cur_menu]) select_to[cur_menu] -= 1;
                   if (select_to[cur_menu] < select_from[cur_menu]) select_from[cur_menu] = select_to[cur_menu];                                      
                   break;               
           }           
           break;                      
   }     
      refresh_display();
      LCD_GotoXY(pos[current_pos],2);
  } while (my_switch != SW_OK);            
    
      LCD_GotoXY(0,2);
}



/* -----------------------------------
line 4
select mode
--------------------------------------*/
void mode_lamp_menu(void)
{
    
    uint8_t my_switch;
      
    LCD_GotoXY(4,3);
          
  do
  {
   my_switch = check_sw();
   switch ( my_switch )
   {
       case SW_LEFT:           
             my_switch = SW_OK; //exit to the left           
           break;
       case SW_RIGHT:
           break;              
       case SW_UP:           
           if (cur_lamp_t <= 8) ++cur_lamp_t;
           break;           
       case SW_DOWN:           
           if (cur_lamp_t >= 1) --cur_lamp_t;           
           break;                      
   }     
      refresh_display();
  } while (my_switch != SW_OK);            
    
      LCD_GotoXY(0,3);
}

void mode_sound_menu(void)
{
    
    uint8_t my_switch;
      
    LCD_GotoXY(4,3);
          
  do
  {
   my_switch = check_sw();
   switch ( my_switch )
   {
       case SW_LEFT:           
             my_switch = SW_OK; //exit to the left           
           break;
       case SW_RIGHT:
           break;              
       case SW_UP:           
           if (cur_sound_t <= 8) ++cur_sound_t;
           break;           
       case SW_DOWN:           
           if (cur_sound_t > 1) --cur_sound_t; //0 would block cancel with OK
           break;                      
   }     
      refresh_display();
  } while (my_switch != SW_OK);            
    
      LCD_GotoXY(0,3);
}

void mode_solenoid_menu(void)
{
    
    uint8_t my_switch;
      
    LCD_GotoXY(4,3);
          
  do
  {
   my_switch = check_sw();
   switch ( my_switch )
   {
       case SW_LEFT:           
             my_switch = SW_OK; //exit to the left           
           break;
       case SW_RIGHT:
           break;              
       case SW_UP:           
           if (cur_sol_t <= 8) ++cur_sol_t;
           break;           
       case SW_DOWN:           
           if (cur_sol_t > 1) --cur_sol_t; //0 would block cancel with OK
           break;                      
   }     
      refresh_display();
  } while (my_switch != SW_OK);            
    
      LCD_GotoXY(0,3);
}
//line 4 select mode
void mode_menu(void)
{
    switch(cur_menu)
    {
        case 0:
            mode_lamp_menu();
            break;
        case 1:
            mode_sound_menu();
            break;
        case 2:
            mode_solenoid_menu();
            break;
    }
    
}

/*----- END Menu entry */


void do_lamp_test(void)
{
        int i;
    
    LCD_CMD(LCD_CMD_CLR); // Display loeschen
    LCD_CMD(LCD_CMD_CUR_BLINK);  //cursor blinkend an
    
    if (cur_lamp_t > 0 )
    {
    LCD_GotoXY(0,0);
    LCD_STRING(" Lamp test running ");   
    LCD_GotoXY(0,1);
    LCD_STRING(" press&hold OK      ");   
    LCD_GotoXY(0,2);
    LCD_STRING(" to cancel test     ");   
    }
    else
    {
    LCD_GotoXY(0,0);
    LCD_STRING(" Lamp test running ");   
    LCD_GotoXY(0,1);
    LCD_STRING(" > or < move lamp  ");   
    LCD_GotoXY(0,2);
    LCD_STRING(" OK to cancel test ");   
    }        
    //set lampdriverboard
    switch(cur_option[cur_menu])
      {     
          case 0: //standard board
//              lampboard_active = 0;
//              lampdriver_variant = NO_AUX_BOARD;
              break;          
          case 1: //extended boards
//              lampboard_active = 1;
//              lampdriver_variant = AS_2518_43_12_LAMPS;                      
              break;          
          case 2: //extended boards
//              lampboard_active = 1;
//              lampdriver_variant = AS_2518_52_28_LAMPS;                      
              break;          
          case 3: //extended boards
//              lampboard_active = 1;
//              lampdriver_variant = AS_2518_23_60_LAMPS;                      
              break;          
          case 4: //comboboard is main board
//              lampboard_active = 0;
//              lampdriver_variant = AS_2518_147_60_LAMPS;                      
              break;                        
      }      
      
    //preset
    i = select_from[cur_menu];  
    
  while(1)  //test forever
  {
    if (cur_lamp_t > 0 )
    {
   
     for(i=select_from[cur_menu]; i<=select_to[cur_menu]; i++)
     {
      LCD_GotoXY(0,3);        
      sprintf(line,"lamp  %02x ON          ",i);
      LCD_STRING(line);   
      q_set(i-1,1);   //numbering 0..59  
      //wait & check OK switch                   
      if (sleep_n_100ms(cur_lamp_t)) return;      
      
      LCD_GotoXY(0,3);        
      sprintf(line,"lamp  %02x OFF         ",i);
      LCD_STRING(line);   
      q_set(i-1,0);   //numbering 0..59  
      //wait & check OK switch                   
      if (sleep_n_100ms(cur_lamp_t)) return;           }                
   }
    else
   {      
      LCD_GotoXY(0,3);        
      sprintf(line,"lamp  %02x ON          ",i);
      LCD_STRING(line);   
      q_set(i-1,1);   //numbering 0..59  
           
       switch ( check_sw())
       {
       case SW_OK:           
           q_set(i-1,0);   //old lamp OFF
           return;
       case SW_LEFT:       
            if ( i > select_from[cur_menu]) {
                    q_set(i-1,0);   //old lamp OFF
                    i = i -1;
                    }                    
                    else
                    {
                        q_set(i-1,0);   //old lamp OFF
                        i = select_to[cur_menu]; //wrap
                    }

            break;
       case SW_RIGHT:        
            if ( i < select_to[cur_menu]) {
                    q_set(i-1,0);   //old lamp OFF
                    i = i +1;
                    }
                    else
                    {
                        q_set(i-1,0);   //old lamp OFF
                        i = select_from[cur_menu]; //wrap
                    }            
            break;            
    }
  } 
 }
}    

void do_sound_test(void)
{
    int i;
    
    LCD_CMD(LCD_CMD_CLR); // Display loeschen
    LCD_CMD(LCD_CMD_CUR_BLINK);  //cursor blinkend an
    
    LCD_GotoXY(0,0);
    LCD_STRING(" Sound test running ");   
    LCD_GotoXY(0,1);
    LCD_STRING(" press&hold OK      ");   
    LCD_GotoXY(0,2);
    LCD_STRING(" to cancel test     ");   
    
    while(1)  //test forever
    {
     for(i=select_from[cur_menu]; i<=select_to[cur_menu]; i++)
     {
      LCD_GotoXY(0,3);        
      sprintf(line," play sound %02x",i);
      LCD_STRING(line);   
      LCD_GotoXY(12,3);        
      //test S&T only
      switch(cur_option[cur_menu])
      {     
          case 0:     //small SB
          case 1:     //big SB 80&80A
          case 2:     //big SB 80B
              sound_data_standard(i);
              break;          
          case 3:   //51
//              ext_soundboard_type = EXT_SB_IS_51;
//              sound_data_extended(i);
              break;          
          case 4:   //S&T              
          case 5:
          case 6:
          case 7:
          case 8:
//              ext_soundboard_type = EXT_SB_IS_SAT;              
//              sound_data_extended(i);
              break;          
      }
      if (sleep_n_100ms(cur_sound_t*10)) return;
     }
    }
}

void do_solenoid_test(void)
{
        int i;
        int sol;
    
    LCD_CMD(LCD_CMD_CLR); // Display loeschen
    LCD_CMD(LCD_CMD_CUR_BLINK);  //cursor blinkend an
    
    LCD_GotoXY(0,0);
    LCD_STRING(" Solenoid test running ");   
    LCD_GotoXY(0,1);
    LCD_STRING(" press&hold OK      ");   
    LCD_GotoXY(0,2);
    LCD_STRING(" to cancel test     ");   
    
    while(1)  //test forever
    {
     for(i=select_from[cur_menu]; i<=select_to[cur_menu]; i++)
     {
      LCD_GotoXY(0,3);        
      sprintf(line," pulse solenoid %02x",i);
      LCD_STRING(line);   
      LCD_GotoXY(12,3);        
      
      switch(cur_option[cur_menu])
      {     
          case 0: //momentary solenoids
              solenoid_pulse(i);
              break;          
          case 1: //continous solenoids
              switch(i)
              {
                  case 1: sol = 0b1110; break;
                  case 2: sol = 0b1101; break;
                  case 3: sol = 0b1011; break;
                  case 4: sol = 0b0111; break;
                  default: sol = 0b1111; break;                  
              }
              //cont_sol_set(sol);
              __delay_ms(800);
              //cont_sol_set(sol);
              break;          
      }
      if (sleep_n_100ms(cur_sol_t*10)) return;
     }
    }

    
}



//execute the test
void start_test(void)
{
    
    LCD_CMD(LCD_CMD_CLR); // Display loeschen
    LCD_CMD(LCD_CMD_CUR_BLINK);  //cursor blinkend an
        
    LCD_GotoXY(0,0);
    LCD_STRING(" press OK to        ");   
    LCD_GotoXY(0,1);
    switch(cur_menu)
    {
        case 0:
            LCD_STRING(" start Lamp Test    ");        
            break;
        case 1:
            LCD_STRING(" start Sound Test   ");                    
            break;
        case 2:
            LCD_STRING(" start Solenoid Test");                    
            break;
        
    }            
    LCD_GotoXY(0,2);
    LCD_STRING(" any other key      ");        
    LCD_GotoXY(0,3);
    LCD_STRING(" to cancel          ");        
    LCD_GotoXY(7,0); //cursor to OK string
    
    
    //check switch
    if ( check_sw() != SW_OK ) 
    {
        refresh_display();
        return;
    }
        
    
    switch(cur_menu)
    {
        case 0:
            do_lamp_test();
            break;
        case 1:
            do_sound_test();
            break;
        case 2:
            do_solenoid_test();
            break;
        
    }            

    refresh_display();    
    
}


/*
 * Main
 */
void main(void) {

    int i;
    
//Clock Source and Value: Internal 16MHz
OSCCONbits.IRCF0 = 1; 
OSCCONbits.IRCF1 = 1;
OSCCONbits.IRCF2 = 1;
//Device is running from the internal oscillator
OSCCONbits.OSTS = 0;
//Primary clock (determined by FOSC<3:0> in CONFIG1H).
OSCCONbits.SCS = 0;
//OSCCONbits.SCS = 3; //RTH: 64MHz still need to be tested
// turn off the PLL 16*4
OSCTUNEbits.PLLEN = 0;      
//OSCTUNEbits.PLLEN = 1;      // turn on the PLL 16*4

// Wait until PLL is ready
//while(!OSCCON2bits.PLLRDY);

/* Port init */
// Port A
ANSELA = 0;   //All I/O pins are configured as digital
LATA = 0;     //clear port
TRISA = 0b00010000;    //RA4 input (Returns)

//---------------------------------------------
// Port B
ANSELB = 0;   //All I/O pins are configured as digital
LATB = 0;     //clear port (sound lines off)
TRISB = 0b00001100;    //RB2&3 input (Returns)

//---------------------------------------------
//Port C
ANSELC = 0;   //All I/O pins are configured as digital
LATC = 0;     //clear port
TRISC = 0; //all output

//---------------------------------------------
// Port D
ANSELD = 0;   //All I/O pins are configured as digital
TRISD = 0;    // All Output

//---------------------------------------------
// Port E
ANSELE = 0;   //All I/O pins are configured as digital
TRISE = 0;    //All Output



/* init */
SOL1 = SOL2 = SOL3 = SOL4 = SOL5 = SOL6 = SOL7 = SOL8 = SOL9 = 0;
//set 238 to 15 ( LED ))
set_238(15);
    
LCD_INIT();

LCD_CMD(LCD_CMD_CLR); // Display loeschen
LCD_CMD(LCD_CMD_CUR_BLINK);  //cursor blinkend an

//initial output
sprintf(line,"SYS80 Tester V %s",VERSION);
LCD_GotoXY(0,0);
LCD_STRING(line);
LCD_GotoXY(0,1);
LCD_STRING("   www.lisy.dev");
LCD_GotoXY(0,2);
LCD_STRING("    READY");
LCD_GotoXY(0,3);
LCD_STRING(" press OK to start");
LCD_GotoXY(7,3); //cursor to OK string

//wait for OK 
while(check_sw() != SW_OK) { NOP(); } 

for ( i=0; i<=10; i++) __delay_ms(10);    

//show menu 1
refresh_display();

  while(1)
  {
   switch ( check_sw())
   {
       case SW_OK:           
           start_test();
           break;
       case SW_UP:
           if (cur_line > 0) --cur_line;        
           else cur_line = 3; //wrap
           refresh_display();           
           break;
       case SW_DOWN:
           if (cur_line < 3) ++cur_line;           
           else cur_line = 0; //wrap
           refresh_display();           
           break;
       case SW_LEFT:
           switch(cur_line)
           {
               case 0: //main menu
                    if (cur_menu > 0) --cur_menu;
                    else cur_menu = 2; //wrap           
                    refresh_display();
                    break;
               case 1: //option menu                    
                   if (cur_option[cur_menu] > 0) cur_option[cur_menu] -=1 ;
                   else cur_option[cur_menu] = n_option[cur_menu] -1;//wrap
                    refresh_display();                           
                    break;                   
           }
           break;
       case SW_RIGHT:
           switch(cur_line)
           {
               case 0: //main menu           
                    if (cur_menu < 2) ++cur_menu;
                    else cur_menu = 0; //wrap                      
                    refresh_display();           
                    break;
               case 1: //option menu                    
                    if (cur_option[cur_menu] < n_option[cur_menu]-1) cur_option[cur_menu] += 1;
                    else cur_option[cur_menu] = 0; //wrap
                    refresh_display();                           
                    break;         
               case 2: //select menu 
                    select_menu();
                    break;                                                              
               case 3: //mode menu 
                    mode_menu();
                    break;                                                           
            }         
           break;       
   }
   
  }//endless loop

}//main
   

