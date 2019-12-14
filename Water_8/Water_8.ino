#include <LiquidCrystal_I2C.h>
#include <Wire.h>
//push buttons
#define pbfill 13 // Push button Fill
#define pbdrain 10 // Push button Drain
#define pbrefresh 11 // Push button Refresh
#define pbestop 12 // Push button All Stop
//Valves
#define supply 2 //Supply Valve
#define drainV 3 // Drain Valve
#define relief 4 // Relief Valve 1
#define relief2 5 // Relief Valve 2
//Sensors
#define senfull 8 // Full Tank Sensor
#define senempty 9 // Empty Tank Sensor
//Variables
int fill; // switch  for filling
int drain; // switch  for draining
int refresh; // switch for refresh
int estop; //estop switch
int statusFill; 
int statusDrain;
int statusRefresh;
int closeRelief;
//LCD variables
String newvalue;
String oldvalue;
//sensor variables
int senF;
int senE;
//timers
unsigned long emptyPush = 0;
unsigned long fillPush = 0;
unsigned long refreshPush = 0;
unsigned long closeMillis = 0;
const unsigned long supplyTime = 3000;
const unsigned long refreshTime = 3000;
const unsigned long emptyTime = 3000;

LiquidCrystal_I2C lcd(0x3F, 16, 2);

void setup() {
  pinMode(supply,OUTPUT);
  pinMode(drainV,OUTPUT);
  pinMode(relief,OUTPUT);
  pinMode(relief2,OUTPUT);
  statusFill = 0;             //used to track fill status
  statusDrain = 0;            //used to track fill status
  statusRefresh = 0;          //used to track fill status
  closeRelief = 0;            //used to track fill status
  Serial.begin(9600);
  oldvalue = ("BOOT");
  lcd.begin();
  lcd.backlight();            //Power on the back light
  Serial.print(oldvalue);
  lcd.print("BOOT");
  delay(500);
}

void loop() {
    
    senF = digitalRead(senfull);      // full sensor {low(0) = triggered} {high(1) = not triggered}
    senE = digitalRead(senempty) ;    // empty sensor {low(0) = triggered} {high(1) = not triggered}
    fill = digitalRead(pbfill);       //{low(0) = triggered} {high(1) = not triggered}
    refresh = digitalRead(pbrefresh); //{low(0) = triggered} {high(1) = not triggered}
    drain = digitalRead(pbdrain);     //{low(0) = triggered} {high(1) = not triggered}
    estop = digitalRead(pbestop);     //{low(0) = triggered} {high(1) = not triggered}
    
    //Set timers
  if (drain == 0){            //set a time on the drain push
      emptyPush = millis();
    }
  if (fill == 0){             //set a time on the fill push
      fillPush = millis();
    }
  if (refresh == 0){          //set a time on the refresh push
      refreshPush = millis();
    }     
    
   //Monitoring if statements  
  if ((senF == 0) && (senE == 0) && (statusFill == 0) && (statusDrain == 0) && (statusRefresh < 1)){ //Full sensor & Empty sensor are tiggered and no other functions are running
    newvalue = ("TANK FULL");
  }
  if ((senF == 1) && (senE == 1) && (statusDrain == 0) && (statusRefresh < 1) && (statusFill == 0)){ //Full sensor & Empty sensor are not tiggered and no other functions are running
    newvalue = ("TANK EMPTY");
  }
  if ((senF == 1) && (senE == 0) && (statusDrain == 0) && (statusRefresh < 1) && (statusFill == 0)){ //Full sensor not tigggered & Empty sensor tiggered and no other functions are running
    newvalue = ("TANK IS'NT FULL");
  }

  //tank is filling either fill or refresh and full sensor is triggered
  if ((senF == 0) && (statusFill == 2) && (closeMillis < 1) || (statusRefresh == 6)){ //tank is filling and full sensor is triggered
    digitalWrite(supply, LOW);    //close supply valve
    statusRefresh = 0;            //reset refresh function
    closeRelief = 1;              // variable to close the relief valves
    closeMillis = millis();       // time to use to close relief valves
    newvalue = ("Supply Closed");     // Displayed on LCD
  }
  //close relief valves
  if ((closeRelief == 1) && (millis() - closeMillis > supplyTime)){ //delayed time to close
    digitalWrite(relief, LOW);    //close relief valve
    digitalWrite(relief2, LOW);   //close relief valve
    closeRelief = 0;              // reset closerelief variable
    closeMillis = 0;              // reset closerelief time
    statusFill = 0;               //exit status loops
  }
  
  //Fill is pushes and full sensor is not triggered
  if ((fill == 0) && (senF !=0)){     // fill is push and tank is not full
    digitalWrite(relief, HIGH);       // open relief valve
    digitalWrite(relief2, HIGH);      // open relief valve
    statusFill = 1;                   //fill status loop to 1
    newvalue = ("Open Relief");       // Display LCD
  }
  
  //relif valves open
  if ((statusFill == 1) && (millis() - fillPush > supplyTime)){   //supply valve timing
    digitalWrite(supply, HIGH);         //open supply valve
    newvalue = ("Filling Tank..");      //LCD Display
    statusFill = 2;                     //fill status loop to 2
    fillPush = 0;                       //fillPush time to 0
  }

  //refresh button pushed and empty sensor is not triggered 
  if ((refresh == 0) && (senE != 1)){     //refresh is pushed and tank is not empty
    digitalWrite(relief, HIGH);           // open relief valve
    digitalWrite(relief2, HIGH);          // open relief valve
    newvalue = ("REFRESH TANK");          //LCD Display
    statusRefresh = 1;                    // refresh loop to 1
  }

  //refresh loop started open the supply valve
  if ((statusRefresh == 1) && (millis() - refreshPush > emptyTime)){ //refresh loop + timer
    digitalWrite(drainV, HIGH);             //open supply valve
    newvalue = ("RE:DRAINING TANK");        //LCD Display
    statusRefresh = 2;                      //Refresh loop to 2
    refreshPush = 0;                        //reset refreshPush time
  }

  //in refresh loop 2 and tank empty
  if ((statusRefresh == 2) && (senE == 1)){ //tank has drained on refresh and is empty 
    digitalWrite(drainV, LOW);              //close supply valve
    newvalue = ("RE:DRAIN CLOSING");        //LCD Display
    statusRefresh = 3;                      //Refresh loop to 3
    refreshPush = millis();                 //refreshPush to millis
  }

  //in refresh loop 3 open supply
  if ((statusRefresh == 3) && (millis() - refreshPush > refreshTime)){  //loop = 3 + time
    refreshPush = 0;                        //refreshPush time set to 0
    digitalWrite(supply, HIGH);             //open supply and fill
    statusRefresh = 4;                      // refresh loop to 4
    newvalue = ("RE:FILLING TANK");         //LCD Display
  }

  //in refresh loop 4 and full sensor triggered
  if ((statusRefresh == 4) && (senF ==0)){    //full sensor triggered
    digitalWrite(supply, LOW);                //close supply
    statusRefresh = 5;                        //refresh loop to 5
    newvalue = ("RE:SUPPLY CLOSED");          //LDC Display
    refreshPush = millis();                   //set time on refreshpush
  }
  //in refresh loop 5 and time
  if ((statusRefresh == 5) && (millis() - refreshPush > refreshTime)){    //refresh loop 5 + delay time
    statusRefresh = 6;                        //change fresh loop to 6
    digitalWrite(relief, LOW);                //close Relief valve
    digitalWrite(relief2, LOW);               //close Relief valve2
    refreshPush = 0;                          //exit refresh loop
  }
  //Drain Squence
  if ((drain == 0) && (senE != 1)){ // drain button pushed and tank not empty
    digitalWrite(relief, HIGH);     // open releif valve  
    digitalWrite(relief2, HIGH);    //open relief valve 2
    statusDrain = 1;                //advance drain squence
    newvalue = ("Open Relief");     //LCD display
    }
  //Drain squence 2  
  if ((statusDrain == 1) && (millis() - emptyPush > emptyTime)){ // drain loop + delay time
    digitalWrite(drainV, HIGH);       //open drain valve
    newvalue = ("Drain Tank..");      //LCD Display
    emptyPush = 0;                    //reset time variable
  } 
  //Drain Squence 3
  if ((statusDrain == 1) && (senE == 1)){ // drain is complete
    digitalWrite(drainV, LOW);            //close drain valve
    newvalue = ("Close Drain");           //LCD Display
    statusDrain = 2;                      // advance drain loop 
    emptyPush = (millis());               //set timer to millis
  }
  //Drain Squence 4
  if ((statusDrain == 2) && (millis()- emptyPush > emptyTime)){ //drain loop + delay time
    digitalWrite(relief, LOW);            //close relief valve
    digitalWrite(relief2, LOW);           //close relief valve2
    newvalue = ("Tank Empty");            //LCD Display
    statusDrain = 0;                      //exit drain squence
  }
  //Update the LCD information
  if (newvalue != oldvalue){        // As the status changes it is displayed on the serial monitor and LCD
    Serial.println(newvalue);       //serial communication update                                                        
    lcd.clear();                    //clear LCD
    lcd.setCursor(0,0);             //set cursor
    lcd.print ("LWSS PROTOTYPE 1"); //print message
    lcd.setCursor(0,16);            //set cursor second line
    lcd.print(newvalue);            //print new value
    oldvalue = newvalue;            //make new value old
  }
  //E-stop Loop
   if (estop == 0){           // All stop of all functions
    newvalue = ("E-STOP");    // print e-stop
    statusFill = 0;           // exit loop
    statusDrain = 0;          // exit loop
    statusRefresh = 0;        // exit loop
    digitalWrite(supply, LOW); //close supply
    digitalWrite(relief, LOW); //close relief
    digitalWrite(relief2, LOW); //close relief2
    digitalWrite(drainV, LOW);  //close drain      
  }
}
