/******************************************************************************************************************* 
GMR Switch Smart Chess Board
Reads an 8 x 8 array of AD024 GMR magnetic switches and displays the time of moves and moves in algebraic notation.
A0-A3, P9-P10, and P12-P13 are sensor selects (ranks/rows) connected to sensor VDDs; P0-P2/P4-P8 
read the sensor outputs (files/columns). SDA and SCL connect to a 20 x 4 LCD Module. P3 is audio; P11 is mute. 
Disconnect P0-P2 to allow USB programming.                     NVE Corporation (sensor-apps@nve.com), rev. 8/31/23
*******************************************************************************************************************/  
#include <Wire.h> //I2C Library
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4); //20 x 4 LCD Module with 0x27 I2C address
#include "Talkie.h"
#include "Vocab_US_Large.h"
extern const uint8_t spt_BACK[] PROGMEM; //Sounds better than "BLACK"
#include "Vocab_Special.h" //Pauses
Talkie voice(true, false); //Free up pin 11 for mute
byte fileMask[9], fileMaskOld[9], vacatedRank, rank, file, vacatedFile, i;
bool vacated=false, landed=false;
String minStr, secStr; //Time of last move in minutes and seconds
String Move[5]; //Stores moves: 0 = most recent move; 4 = four moves ago
const String fString[]={"","a","b","c","d","e","f","g","h"}; //Array to convert file numbers to letters
long time0, timen; //Times of first and last moves
int8_t board[8][8] = { //Initial board array of pieces 
//Pawn=1; Rook=2; Knight=3; Bishop=4; Queen=5; King=6; positive=white; negative=black.
  {2,3,4,5,6,4,3,2},
  {1,1,1,1,1,1,1,1},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {-1,-1,-1,-1,-1,-1,-1,-1},
  {-2,-3,-4,-5,-6,-4,-3,-2}};

void setup() {
lcd.init();
lcd.backlight();
lcd.print("   NVE GMR Switch     Smart Chessboard");
pinMode(11, OUTPUT); //P11 is the amplifier mute output
digitalWrite(11, LOW); //Mute amplifier
DDRD  &= B00001000; //Set PD0-PD7/P0-P8 as inputs, except P3 is audio out
PORTD |= B11110111; //Enable PD pullups
pinMode(8, INPUT_PULLUP); //P8 is the column 8 input
}
void loop() {
for(rank=1; rank < 9; rank++) { //Scan through sensor ranks (rows)
DDRC &= B11110000; //Set PC0-PC3 (A0-A3; for ranks 1-4) as inputs to tri-state
DDRB &= B11001001; //Set PB1-PB5/P9-P13 as inputs to tri-state (except PB3/P11 is audio mute output)
if(rank<5){
  DDRC  |= 1<<(rank-1); //Turn on selected sensor row 
  PORTC |= 1<<(rank-1);
}
if((rank>4) && (rank<9)){
  DDRB  |= 1<<(rank+(rank-5)/2-4); 
  PORTB |= 1<<(rank+(rank-5)/2-4); 
}
delay(1); //Allow sensors to stabilize before reading outputs

fileMaskOld[rank] = fileMask[rank];
fileMask[rank] = ~((PIND & B0000111)|((PIND & B11110000)>>1)|((PINB & 1)<<7)); //Read sensor files (columns)
if((~fileMask[rank] & fileMaskOld[rank])>0) { //Square vacated
vacated=true; //Set flag to indicate a square was vacated
vacatedFile = log(float(~fileMask[rank] & fileMaskOld[rank]))/log(2.0)+1.5; //Calculate vacated file (1-8)
vacatedRank=rank;
delay(100); //Lockout additional moves
}
if(vacated&&((fileMask[rank] & ~fileMaskOld[rank])>0)){ //Piece landed
  file=log(float(fileMask[rank] & ~fileMaskOld[rank]))/log(2.0)+1.5;
  landed=true;
}
if(vacated&&landed&&(file==vacatedFile)&&(rank==vacatedRank)) { //Not a move
vacated=false; //Clear flags
landed=false;  
}
if(vacated&&landed) { //Legal move 
if (Move[0]=="") time0=micros(); //Store time of first move
timen=(micros()-time0)/1000000; //Time of this move (wraps after ~1 hour)
minStr=String(timen/60);
secStr=String(timen-timen/60*60); 
if(minStr.length()<2) minStr="0"+minStr;
if(secStr.length()<2) secStr="0"+secStr;

//Last move:
Move[0]=minStr+":"+secStr+" "+String(fString[vacatedFile]+String(vacatedRank)+"-"+fString[file]+String(rank));

vacated=false; //Clear flags
landed=false;
board[rank-1][file-1] = board[vacatedRank-1][vacatedFile-1]; //Update board
board[vacatedRank-1][vacatedFile-1]=0;

//Display move
lcd.clear();
for(i=4; i>0; i--){
Move[i] = Move[i-1]; //Shift recent-move array
lcd.setCursor(0,4-i);
lcd.print(Move[i]);} //Update LCD display row-by-row

//Speak move
digitalWrite(11, HIGH); //Unmute amplifier
if(board[rank-1][file-1]>0) voice.say(sp3_WHITE);
if(board[rank-1][file-1]<0) voice.say(spt_BACK); //"BLACK" not in vocaulary
voice.say(sp2_MOVE); voice.say(sp2_FROM);
sayLetter(vacatedFile); sayNumber(vacatedRank);
voice.say(spPAUSE2); voice.say(sp2_TWO); voice.say(spPAUSE2);
sayLetter(file); sayNumber(rank);
digitalWrite(11, LOW); //Mute amplifier
}}
if (Move[0]!="") { //After first move, display running time
timen=(micros()-time0)/1000000; //Update running time 
minStr="0"+String(timen/60);
secStr="0"+String(timen-timen/60*60); 
lcd.setCursor(15,3);
lcd.print(minStr.substring(minStr.length()-2)+":"+secStr.substring(secStr.length()-2));} //Update running time 
}
void sayLetter(byte L) {
        switch (L) {
        case 1:
            voice.say(sp2_A);
            break;
        case 2:
            voice.say(sp2_B);
            break;
        case 3:
            voice.say(sp2_C);
            break;
        case 4:
            voice.say(sp2_D);
            break;
        case 5:
            voice.say(sp2_E);
            break;
        case 6:
            voice.say(sp2_F);
            break;
        case 7:
            voice.say(sp2_G);
            break;
        case 8:
            voice.say(sp2_H);
            break;
        }}
void sayNumber(byte N) {
        switch (N) {
        case 1:
            voice.say(sp2_ONE);
            break;
        case 2:
            voice.say(sp2_TWO);
            break;
        case 3:
            voice.say(sp2_THREE);
            break;
        case 4:
            voice.say(sp2_FOUR);
            break;
        case 5:
            voice.say(sp2_FIVE);
            break;
        case 6:
            voice.say(sp2_SIX);
            break;
        case 7:
            voice.say(sp2_SEVEN);
            break;
        case 8:
            voice.say(sp2_EIGHT);
            break;
        }}
