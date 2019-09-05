// Hacked to put lots of strings in PROGMEM 5/7/16 jw  Thanks for some W88 contributions!
// updated 11/6/18 with some more strings and a correction
// temporarily added piezo beeper between pin 4 (-) and pin 7(+) for troubleshooting
// did some cleanup
// Tried to include watchdog timer 5/17/16, but at frame delay
// of 25 ms, many messages were longer than the max 8 sec wdt time
// Added QUICKPLAY test mode to zip thru all the messages 5/18/16
// Use the Parola library to scroll text on the display
//
// Demonstrates the use of the scrolling function to display text received 
// from the serial interface
//
// User can enter text on the serial monitor and this will display as a
// scrolling message on the display.
// Speed for the display is controlled by a pot on SPEED_IN analog in.
// Scrolling direction is controlled by a switch on DIRECTION_SET digital in.
// Invert ON/OFF is set by a switch on INVERT_SET digital in.
//
// Keyswitch library can be found at http://arduinocode.codeplex.com
//
// Updated to V4 8/30/19 to support Ray's sorta-scrolling RDU350.  That device
// only scrolls its 20 char buffer; we rewrite displayed line to appear to scroll
// longer lines.  rdubuf has blanks at beginning and end to allow this.  
// I _think_ the RDU stuff is all properly #ifdef'd in.  DATADIRPIN is for a
// demo Arduino with just an ADM483 for the RDU's RS485 input.

#include <avr/pgmspace.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#define RDU 

// set to 1 if we are implementing the user interface pot, switch, etc
#define	USE_UI_CONTROL	0

#if USE_UI_CONTROL
#include <MD_KeySwitch.h>
#endif

// Turn on debug statements to the serial output
#define  DEBUG  1

#if  DEBUG
#define	PRINT(s, x)	{ Serial.print(F(s)); Serial.print(x); }
#define	PRINTS(x)	Serial.print(F(x))
#define	PRINTX(x)	Serial.println(x, HEX)
#else
#define	PRINT(s, x)
#define PRINTS(x)
#define PRINTX(x)
#endif

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may 
// need to be adapted
#define	MAX_DEVICES	12
#define	CLK_PIN		13
#define	DATA_PIN	11
#define	CS_PIN		10

#define BEEPPIN 7
#define BEEPGND 4
void dobeep(){
  digitalWrite(BEEPPIN,HIGH);
  delay(10);
  digitalWrite(BEEPPIN,LOW);
}//end dobeep

#ifndef RDU
// HARDWARE SPI
MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);
// SOFTWARE SPI
//MD_Parola P = MD_Parola(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
#endif

#define	PAUSE_TIME		1000
#define	SPEED_DEADBAND	5
// comment next line out for normal display
// include it to run thru all msgs quickly for testing and no W88 msgs
//#define QUICKPLAY
#ifdef QUICKPLAY
#define PLAYTIME 100
#else
#define PLAYTIME 10000
#endif

// Scrolling parameters
#if USE_UI_CONTROL
#define	SPEED_IN		A5
#define	DIRECTION_SET	8	// change the effect
#define	INVERT_SET		9	// change the invert

#endif // USE_UI_CONTROL

uint8_t	frameDelay = 25;	// default frame delay value
textEffect_t	scrollEffect = SCROLL_LEFT;


// Global message buffers shared by Serial and Scrolling functions
#define  BUF_SIZE  105
#ifndef RDU
char curMessage[BUF_SIZE];
#else
#define RDUSIZE 20
#define DATADIRPIN 2
char rduBuf[BUF_SIZE+RDUSIZE*2];
char * curMessage=rduBuf+RDUSIZE-1; // leave space for <19> leading blanks 
#endif
char newMessage[BUF_SIZE];  // only works if not RDU!
bool newMessageAvailable = false;

#if USE_UI_CONTROL

MD_KeySwitch uiDirection(DIRECTION_SET);
MD_KeySwitch uiInvert(INVERT_SET);

void doUI(void)
{
  // set the speed if it has changed
  {
    int16_t	speed = map(analogRead(SPEED_IN), 0, 1023, 10, 150);

    if ((speed >= ((int16_t)P.getSpeed() + SPEED_DEADBAND)) || 
      (speed <= ((int16_t)P.getSpeed() - SPEED_DEADBAND)))
    {
      P.setSpeed(speed);
      P.setPause(speed);
      frameDelay = speed;
      PRINT("\nChanged speed to ", P.getSpeed());
    }
  }

  if (uiDirection.read() == MD_KeySwitch::KS_PRESS)	// SCROLL DIRECTION
  {
    PRINTS("\nChanging scroll direction");
    scrollEffect = (scrollEffect == SCROLL_LEFT ? SCROLL_RIGHT : SCROLL_LEFT);
    P.setTextEffect(scrollEffect, scrollEffect);
    P.displayReset();
  }

  if (uiInvert.read() == MD_KeySwitch::KS_PRESS)	// INVERT MODE
  {
    PRINTS("\nChanging invert mode");
    P.setInvert(!P.getInvert());
  }
}

void readSerial(void)
{
  static uint8_t	putIndex = 0;

  while (Serial.available())
  {
    newMessage[putIndex] = (char)Serial.read();
    if ((newMessage[putIndex] == '\n') || (putIndex >= BUF_SIZE -2))	// end of message character or full buffer
    {
      // put in a message separator and end the string
      newMessage[putIndex] = '\0';
      // restart the index for next filling spree and flag we have a message waiting
      putIndex = 0;
      newMessageAvailable = true;
    }
      else
      // Just save the next char in next location
      newMessage[putIndex++];
  }
}// end readserial()
#endif // USE_UI_CONTROL

void setup()
{
  Serial.begin(9600);

#if USE_UI_CONTROL
  uiDirection.begin();
  uiInvert.begin();
  pinMode(SPEED_IN, INPUT);

  doUI();
#endif // USE_UI_CONTROL

#ifdef QUICKPLAY
  frameDelay = 8;
#endif

  // debug piezo between pin4 (gnd) and ping 7(sig)
  pinMode(BEEPGND,OUTPUT);digitalWrite(BEEPGND,LOW);
  pinMode(BEEPPIN,OUTPUT);digitalWrite(BEEPPIN,HIGH);
  delay(20);digitalWrite(BEEPPIN,LOW);

#ifndef RDU
  P.begin();
  P.displayClear();
  P.displaySuspend(false);
  P.setZoneEffect(0, 1, FLIP_UD);

  P.displayScroll(curMessage, LEFT, scrollEffect, frameDelay);

  //strcpy(curMessage, "Hello! Enter new message?");
  //strcpy(curMessage, "Workshop 88 is great!");
  strcpy(curMessage, "Hello, World!");
  newMessage[0] = '\0';
  while(!P.displayAnimate());
  P.displayReset();
#endif

  Serial.print("\n[Parola Scrolling Display]\nType a message for the scrolling display\nEnd message line with a newline");

#ifdef RDU
// put blanks at beginning and end of buffer for RDU "scrolling"
#define RDUBLANK ' '
for(int i=0;i<RDUSIZE-1;i++){
  rduBuf[i]=RDUBLANK;
}// end for i
pinMode(DATADIRPIN,OUTPUT); //RS485 driver tx,*rx enable
digitalWrite(DATADIRPIN,HIGH);
#endif

}//end setup()


//--- WARNING!  Messages can't be longer than BUF_SIZE!
const char s0[] PROGMEM = "Workshop 88 is great!";
const char s1[] PROGMEM = "Do witches run spell checkers?";
const char s2[] PROGMEM = "Time is what keeps things from happening all at once.";
const char s3[] PROGMEM = "A day without sunshine is like, night.";
const char s4[] PROGMEM = "Atheism is a non-prophet organization.";
const char s5[] PROGMEM = "Chocolate: the OTHER major food group.";
const char s6[] PROGMEM = "How do you tell when you run out of invisible ink?";
const char s7[] PROGMEM = "Keep an open mind, but not so open your brains fall out.";
const char s8[] PROGMEM = "The early bird may get the worm, but the second mouse gets the cheese.";
const char s9[] PROGMEM = "Are part-time band leaders semi-conductors?";
const char s10[] PROGMEM = "2 monograms = 1 diagram";
const char s11[] PROGMEM = "Who is General Failure and why is he reading my hard disk?";
const char s12[] PROGMEM = "Shin: a device for finding furniture in the dark.";
const char s13[] PROGMEM = "Why do psychics have to ask you for your name?";
const char s14[] PROGMEM = "On the other hand, you have different fingers.";
const char s15[] PROGMEM = "Nothing is foolproof to a talented fool.";
const char s16[] PROGMEM = "I intend to live forever - so far so good.";
const char s17[] PROGMEM = "The sooner you fall behind the more time you'll have to catch up.";
const char s18[] PROGMEM = "Change is inevitable - except from vending machines.";
const char s19[] PROGMEM = "A lawyer is a person who writes a 10,000 word document and calls it a 'brief'.";
const char s20[] PROGMEM = "When a clock is hungry, it goes back four seconds.";
const char s21[] PROGMEM = "A closed mouth gathers no feet.";
const char s22[] PROGMEM = "Those who live by the sword get shot by those who don't.";
const char s23[] PROGMEM = "Hard work has a future payoff.  Laziness pays off now.";
const char s24[] PROGMEM = "The older I get, the older old is.";
const char s25[] PROGMEM = "What's the view like from the top of the bell curve?";
const char s26[] PROGMEM = "Sooner or later, EVERYONE stops smoking.";
const char s27[] PROGMEM = "Funny, I don't remember being absent-minded...";
const char s28[] PROGMEM = "Growing old is mandatory; growing up is optional.";
const char s29[] PROGMEM = "It's not a bug - it's a feature.";
const char s30[] PROGMEM = "Virus? I thought a Trojan was supposed to protect me!";
const char s31[] PROGMEM = "This space intentionally left blank";
const char s32[] PROGMEM = "Chemists do it with moles";
const char s33[] PROGMEM = ":w saves!";
const char s34[] PROGMEM = "Apostrophes don't make plurals.";
const char s35[] PROGMEM = "MBA: When your BS just can't take you any further.";
const char s36[] PROGMEM = "A thesaurus is NOT a giant lizard.";
const char s37[] PROGMEM = "Dyslexia makes reading NUF!";
const char s38[] PROGMEM = "I know you don't believe in ESP.";
const char s39[] PROGMEM = "6 out of 7 dwarfs are not Happy.";
const char s40[] PROGMEM = "If you're really, really careful, you can lead a really boring life.";
const char s41[] PROGMEM = "Ah, I see the screw-up fairy was here again.";
const char s42[] PROGMEM = "Some people have a way with words, others not have way.";
const char s43[] PROGMEM = "iSleepy.  There's a nap for that.";
const char s44[] PROGMEM = "Irony:  The opposite of wrinkly.";
const char s45[] PROGMEM = "If you say 'GULLIBLE' slowly, it sounds like 'ORANGES'.";
const char s46[] PROGMEM = "Slow and steady wins the race, except in a real race.";
const char s47[] PROGMEM = "Sliced bread:  An invention surpassed by nearly everything since 1928.";
const char s48[] PROGMEM = "No sense being pessimistic - it wouldn't work anyway.";
const char s49[] PROGMEM = "Be the person your dog thinks you are.";
const char s50[] PROGMEM = "Oh no - not another learning experience!";
const char s51[] PROGMEM = "What do we want? Time travel!  When do we want it?  It's irrelevant.";
const char s52[] PROGMEM = "Dragons don't believe in you, either.";
const char s53[] PROGMEM = "Similes are like metaphors."; // spelling fixed: was similies :(
const char s54[] PROGMEM = "No matter how far you push the envelope, it's still stationery.";
const char s55[] PROGMEM = "Time flies like an arrow.  Fruit flies like a banana.";
const char s56[] PROGMEM = "We have become the planet of the apps.";
const char s57[] PROGMEM = "Any sufficiently advanced technology is indistinguishable from magic.";
const char s58[] PROGMEM = "Don't complain the rose bush is full of thorns, be happy the thorn bush has roses.";
const char s59[] PROGMEM = "When you're bit on the heel by a saltwater eel, that's a moray.";
const char s60[] PROGMEM = "The speed of light exceeds the speed of sound - that's why people appear brighter before they speak.";
const char s61[] PROGMEM = "Heisenberg may have been here.";
const char s62[] PROGMEM = "If we had some ham, we could have ham and eggs, if we had some eggs.";
const char s63[] PROGMEM = "Anger is one letter short of danger.";
const char s64[] PROGMEM = "I tell bad chemical jokes 'cuz all the good ones Ar.";
const char s65[] PROGMEM = "Never apollogize for your bad jokes about greek gods.";
const char s66[] PROGMEM = "Don't have any vegetable jokes yet, so if you do, lettuce know.";
const char s67[] PROGMEM = "If you're an archeologist, does that mean your life is in ruins?";
const char s68[] PROGMEM = "Why didn't the photon have a suitcase?  Because he was traveling light.";
const char s69[] PROGMEM = "I see you have graph paper.  You must be plotting something!";
const char s70[] PROGMEM = "Dinosaur puns are pteroble.";
const char s71[] PROGMEM = "Two red blood cells fell in love, but alas, it was in vein.";
const char s72[] PROGMEM = "Whenever I'm in an airport, I start coughing and sneezing.  It's a terminal illness.";
const char s73[] PROGMEM = "What do you call a pod of musical whales?  An Orcastra!";
const char s74[] PROGMEM = "What if there were no hypothetical questions?";
const char s75[] PROGMEM = "One good turn gets all the blankets.";
// addition 11/6/18
const char s76[] PROGMEM = "Why are things always in the last place you look?";
const char s77[] PROGMEM = "If at first you don't succeed - then skydiving's not right for you.";
const char s78[] PROGMEM = "If it weren't for Mondays, we'd all hate Tuesdays.";
const char s79[] PROGMEM = "Don't raise your voice, improve your argument.";
const char s80[] PROGMEM = "A balanced diet means a cookie in both hands.";
const char s81[] PROGMEM = "An eye for an eye will ultimately, leave the whole world blind.";
const char s82[] PROGMEM = "When your memory goes, forget it.";
const char s83[] PROGMEM = "The road to success is always under construction.";
const char s84[] PROGMEM = "Whoever invented knock knock jokes should get a No Bell prize.";
const char s85[] PROGMEM = "SCIENCE:  When witchcraft, trickery and guessing don't work.";
const char s86[] PROGMEM = "SCIENCE:  It's like magic, but real.";
const char s87[] PROGMEM = "Home is where the wi-fi connects automatically.";
const char s88[] PROGMEM = "I thought I was pretty cool until I realized plants can eat sun and poop out air.";
const char s89[] PROGMEM = "Don't trust an atom.  They make up everything.";
const char s90[] PROGMEM = "If life gives you mold, make penicillin.";
const char s91[] PROGMEM = "The name's Bond. Ionic Bond. Taken, not shared.";
const char s92[] PROGMEM = "Why did the bear dissolve in water?  Because it was polar.";
const char s93[] PROGMEM = "What is the name of the first electricity detective?  Sherlock Ohms.";
const char s94[] PROGMEM = "What does a subatomic duck say?  'Quark'.";
const char s95[] PROGMEM = "Why is the pH of Youtube so stable?  Because it constantly buffers.";
const char s96[] PROGMEM = "What's a sleeping brain's favorite band?  R.E.M.";
const char s97[] PROGMEM = "One tectonic plate bumped into another and said 'My fault'.";
const char s98[] PROGMEM = "What do baby parabolas drink?  Quadratic formula.";
const char s99[] PROGMEM = "I'm not lazy.  I'm overflowing with potential energy!";
const char s100[] PROGMEM = "Magma mia - Here I flow again.";
const char s101[] PROGMEM = "My math teacher called me average. How mean!";
const char s102[] PROGMEM = "Worrying works! 90% of the things I worry about never happen.";
const char s103[] PROGMEM = "Keep the dream alive: Hit the snooze button.";
const char s104[] PROGMEM = "A SQL query goes into a bar, walks up to two tables and asks, 'Can I join you?'";
const char s105[] PROGMEM = "CAPS LOCK – Preventing Logins Since 1980.";
const char s106[] PROGMEM = "Geology rocks, but Geography is where it's at.";
const char s107[] PROGMEM = "Oxygen and Potassium went on a date.  It went OK.";
const char s108[] PROGMEM = "If you're not part of the solution, you're part of the precipitate.";

const char* const string_table[] PROGMEM = 
//BE SURE TO UPDATE string_table[] BELOW IF YOU ADD STRINGS!
  // Sorry - gotta update this with new msg vars.  :(
  {s0,s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,s12,s13,s14,
  s15,s16,s17,s18,s19,s20,s21,s22,s23,s24,s25,s26,s27,
  s28,s29,s30,s31,s32,s33,s34,s35,s36,s37,s38,s39,s40,
  s41,s42,s43,s44,s45,s46,s47,s48,s49,s50,s51,s52,s53,
  s54,s55,s56,s57,s58,s59,s60,s61,s62,s63,s64,s65,s66,
  s67,s68,s69,s70,s71,s72,s73,s74,s75,s76,s77,s78,s79,
  s80,s81,s82,s83,s84,s85,s86,s87,s88,s89,s90,s91,s92,
  s93,s94,s95,s96,s97,s98,s99,s100,s101,s102,s103,s104,
  s105,s106,s107,s108};

 
void loop() 
{
int msgindex;
int numMsgs=(sizeof string_table)/sizeof string_table[0];
int doW88IsGreat=numMsgs/5; // insert W88 msg 5 times during whole cycle

for (msgindex = 0; msgindex <numMsgs; msgindex++){
#ifndef QUICKPLAY
  if((msgindex%doW88IsGreat)==0 && msgindex != 0){dobeep();playOneMsg(0);} // not 3 times to start
  playOneMsg(msgindex);
#endif
  playOneMsg(msgindex);

}//end for
  } //end loop()


// Message player - either display device
void playOneMsg(int msgindex){
  int i;
  char *rduptr;
  strcpy_P(curMessage, (char*)pgm_read_word(&(string_table[msgindex]))); // Necessary casts and dereferencing, just copy.
#ifndef RDU
  Serial.print("msgindex= ");Serial.println(msgindex);
  while(!P.displayAnimate());
  P.displayReset();
  Serial.println(curMessage);
#else

// RDU SUPPORT
// time per char to scroll R->L
char printBuf[RDUSIZE+1];  // we print to RDU from here
int msglen=strlen(curMessage);
// put blanks after the message so it can scroll all the way off to the left
strcpy_P(curMessage+msglen,PSTR("                    ")); //20 blanks
// this is the loop that prints a sequence of strings to appear to scroll
for(i=0;i<msglen+(RDUSIZE-1);i++){
  strncpy(printBuf,rduBuf+i,RDUSIZE);
  printBuf[RDUSIZE]=(char)0; // terminating null
  Serial.print(printBuf);Serial.print("\n");
  delay(frameDelay*6);
}// end for
#endif
} //end playOneMsg
