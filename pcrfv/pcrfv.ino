/*
 * DIY Thermal Cycler for PCR
 * PCR code for denaturing, annealing and extending DNA cycles.
 * iDP (duilio.peroni@gmail.com) may 2018
 * Derived from: Stacey Kuznetsov (stace@cmu.edu) and Matt Mancuso (mcmancuso@gmail.com) July 2012
 * V 1.0 01/06/2018
 * Arduino based harware
 * - K-type thermocouple sensor with MAX6675 A/D converter
 * - 2x 50W resistor heater
 * - 12V fan cooler
 * - Start/stop button (internal pullup -> active-low signal)
 * - RGB LED & I2C LCD local diagn
 * - CSV remote log
 * 
 * PCR Cycle
 *            |              N x Cycles          |
 *      |  H  |   D   | C |    A    |H|   eX  |H |C|
 * Td _ _ _ _ _________ _ _ _ _ _ _ _ _ _ _ _ _ _ _|_
 *      |    /|       |\  |         | |       | /| |
 * Te _ _ _ /_ _ _ _ _|_\_|_ _ _ _ _  _________/_|_|_
 *      |  /  |       |  \|         |/|       |\ | | 
 * Ta _ _ /_ _|_ _ _ _|_ _\_________/_|_ _ _ _|_\|_|_
 *      |/    |       |   |         | |       |  \ |
 * Tr __/ _ _ |_ _ _ _| _ | _ _ _ _ |_|_ _ _ _|_ |\__
 *      |     |       |   |         | |       |  | |
 *      
 * CSV log format:
 * <total_time>;<cycle_number>;<phase>;<phase_time>;<temperature>;<status><cr><lf>
 * <total_time>   :: HH:MM:SS
 * <cycle_number> :: 1..NUM_CYCLES
 * <phase>        :: I=Idle, H=heating, C=cooling, D=denaturation, A=annealing, E=extension, F= fault 
 * <phase_time>   :: HH:MM:SS
 * <temperature>  :: tt.tt (°C)
 * <status>       :: 0=ok, 1=operator stop, 2=sensor not working, 3=temp already over setpoint, 4=temp not stable
 * 
 * RGB diagn colors:
 * Idle=green, Heating=red, Cooling=blue, Denaturation=fuchsia, Annealing=cyan, eXtension=yellow, fault=white.
 * 
 */
//libraries 
#include <max6675.h>    //include library for K-junction thermocouple A/D converter
#include <Wire.h>       //include I2C Library
#include "rgb_lcd.h"    //include LCD application lib

//DEBUG switches
//#define DEBUG_PARAM           //DEBUG MODE!!! if defined reduced cycles, times and/or temperature are used for debug purpose; comment for real parameters
//#define DEBUG_SENS            //DEBUG MODE!!! if defined temperature values from potentiometer on A0 pin; comment for thermocouple reading     

//hardware definition (based on Arduino UNO/Leonardo/Micro/Nano or compatibles)
//input
#define START_STOP_PIN  8   //start/stop cycle pushbutton
#define THERMO_DO       4   //thermocouple data pin
#define THERMO_CS       5   //thermocouple select pin
#define THERMO_CLK      6   //thermocouple clock pin
//output
#define HEAT_PIN        7   //heater resistor relay output
#define FAN_PIN         9   //fan driver output
#define RED_PIN         10  //status RGB LED red pin
#define GREEN_PIN       11  //status RGB LED green pin
#define BLUE_PIN        12  //status RGB LED blue pin
//I2C LCD
//UNO/Nano:       A4(SDA),A5(SCL)
//Leonardo/Micro: 2 (SDA),3 (SCL)

//PCR parameters
#ifndef DEBUG_PARAM     //OPERATIVE MODE

//cycles numbers
#define NUM_CYCLES      32  //number of cycles of PCR sequence

//temperatures
#define DENATURE_TEMP             94.0    //denaturation phase temperature
#define ANNEALING_TEMP            60.0    //annealing phase temperature
#define EXTENSION_TEMP            72.0    //extension phase temperature
#define FAN_THRESHOLD_TEMP        0.75    //fan threshold temperature (in order to avoid underruning temperature in cooling)
#define SETPOINT_THRESHOLD_TEMP   0.5     //setpoint threshold temperature (in order stabilize setpoit holding)
#define STABILITY_THRESHOLD_TEMP  1.25    //max deviation from setpoint
#define APPROACH_THRESHOLD_TEMP   1.0     //approaching temp threshold in heating

//other heating parameters
#define MAX_PULSE_DURATION      650       //max duration of an heating pulse (mS)
#define PULSE_DURATION_MULT     600       //multiplication coefficient to get pulse duration proportional to temperature gap (adimensional)
#define MIN_PULSE_DURATION      80        //min duration of an heating pulse when approaching to setpoit (mS)
#define OVERHEAT_TEST           30        //number of iteration before each overheat test (adimensional)
#define OVERHEAT_TIME           250       //overheat wait pulse (no heating, mS)


//times
#define DENATURE_TIME         33000   //denaturation phase duration
#define ANNEALING_TIME        33000   //annealing phase duration
#define EXTENSION_TIME        35000   //extension phase duration
#define INITIAL_DENATURE_TIME 300000  //first denaturation phase duration
#define FINAL_EXTENSION_TIME  600000  //last extension phase duration
#define COOL_SAMPLE_TIME      300     //temperature sample time during cooling phase
#define HOLD_PULSE_DURATION   90      //hold temperature pulse duration
#define HOLD_PAUSE_DURATION   210     //hold temperature pulse duration

//safety params (°C)
#define ROOM_TEMP               18    //room temperature setpoit (to check for sensor failure)
#define MAX_ALLOWED_TEMP        100   //max allowed temperature (for safety purpose)
#define MAX_HEAT_INCREASE       2.5   //max slope of rising temperature (for safety purpose)
#define MAX_ALLOWED_TEMP_TIME  1000   //max allowed temp sample time
#define MAX_HEAT_INCREASE_TIME 1000   //max heat increase sample time

#else                   //DEBUG MODE

//cycles numbers
#define NUM_CYCLES      2  //number of cycles of PCR sequence

//temperatures
#ifndef DEBUG_SENS    //DEBUG PARAMS WITH THERMOCOUPLE SENSOR

#define DENATURE_TEMP             31.0    //denaturation phase temperature
#define ANNEALING_TEMP            30.0    //annealing phase temperature
#define EXTENSION_TEMP            29.0    //extension phase temperature
#define FAN_THRESHOLD_TEMP        0.0     //fan threshold temperature (in order to avoid underruning temperature in cooling)
#define SETPOINT_THRESHOLD_TEMP   0.0     //setpoint threshold temperature (in order stabilize setpoit holding)
#define STABILITY_THRESHOLD_TEMP  1.25    //max deviation from setpoint
#define APPROACH_THRESHOLD_TEMP   0.5     //approaching temp threshold in heating

#else                //DEBUG PARAMS WITH POTENTIOMETER EMULATOR

#define DENATURE_TEMP             94.0    //denaturation phase temperature
#define ANNEALING_TEMP            60.0    //annealing phase temperature
#define EXTENSION_TEMP            72.0    //extension phase temperature
#define FAN_THRESHOLD_TEMP        0.75    //fan threshold temperature (in order to avoid underruning temperature in cooling)
#define SETPOINT_THRESHOLD_TEMP   0.5     //setpoint threshold temperature (in order stabilize setpoit holding)
#define STABILITY_THRESHOLD_TEMP  3.0     //max deviation from setpoint
#define APPROACH_THRESHOLD_TEMP   3.0     //approaching temp threshold in heating

#endif

//other heating parameters
#define MAX_PULSE_DURATION      650       //max duration of an heating pulse (mS)
#define PULSE_DURATION_MULT     600       //multiplication coefficient to get pulse duration proportional to temperature gap (adimensional)
#define MIN_PULSE_DURATION      80        //min duration of an heating pulse when approaching to setpoit (mS)
#define OVERHEAT_TEST           30        //number of iteration before each overheat test (adimensional)
#define OVERHEAT_TIME           250       //overheat wait pulse (no heating, mS)

//times
#define DENATURE_TIME         5000   //denaturation phase duration
#define ANNEALING_TIME        10000   //annealing phase duration
#define EXTENSION_TIME        5000   //extension phase duration
#define INITIAL_DENATURE_TIME 10000  //first denaturation phase duration
#define FINAL_EXTENSION_TIME  10000  //last extension phase duration
#define COOL_SAMPLE_TIME      300     //temperature sample time during cooling phase
#define HOLD_PULSE_DURATION   90      //hold temperature pulse duration
#define HOLD_PAUSE_DURATION   210     //hold temperature pulse duration

//safety params (°C)
#define ROOM_TEMP              18     //room temperature setpoit (to check for sensor failure)
#define MAX_ALLOWED_TEMP       100    //max allowed temperature (for safety purpose)
#define MAX_HEAT_INCREASE      10     //max slope of rising temperature (for safety purpose)
#define MAX_ALLOWED_TEMP_TIME  1000   //max allowed temp sample time
#define MAX_HEAT_INCREASE_TIME 1000   //max heat increase sample time

#endif

//Phases states
#define IDLE            0  //waiting for a cycle
#define HEATING         1  //heating rising temperature ramp
#define COOLING         2  //cooling falling temperature ramp
#define DENATURATION    3  //denaturation phase
#define ANNEALING       4  //annealing phase
#define EXTENSION       5  //extension phase
#define FAULT           6  //fault status

//Phases names
char phases[7][5]={"IDLE","HEAT","COOL","DEN ","ANN ","EXT ","FLT"};

//Utility function prototypes
void runPCR(void);                                
boolean heatUp(double setpoint);
boolean coolDown(double setpoint, int maxTimeout = COOL_SAMPLE_TIME);
boolean holdConstantTemp(long duration, double setpoint);
void log(boolean force=false);
double getTemperature(void);
void setStatus(int phase);
boolean buttonPressed(void);
void updateTemperature(void);

//global variables                          
int currInButton;               //current loop status of start/stop pushbutton     
int prevInButton;               //previous loop status of start/stop pushbutton
unsigned long startCycleTime;   //cycle start time;
unsigned long startPhaseTime;   //phase start time;
unsigned long timeCounter;      //
//int cycles;                   //cycle iterator
int currCycle;                  //current cycle value 
int currPhase;                  //current phase value
double currTemp;                //current temperature value  
double prevTemp;                //previuous temperature value  
char faultCode='0';             //fault code
//temperature device wrapper
MAX6675 thermocouple(THERMO_CLK, THERMO_CS, THERMO_DO); 
//lcd device wrapper
rgb_lcd lcd;   


//setup: run once at startup
void setup() {
  //init console
  Serial.begin(250000);
 
  //configure I/O
  pinMode(START_STOP_PIN,INPUT_PULLUP);
  pinMode(RED_PIN,OUTPUT);
  pinMode(GREEN_PIN,OUTPUT);
  pinMode(BLUE_PIN,OUTPUT);
  pinMode(HEAT_PIN,OUTPUT);
  pinMode(FAN_PIN,OUTPUT);
  //set I/O initial status
  setStatus(IDLE);
  digitalWrite(HEAT_PIN,LOW);
  digitalWrite(FAN_PIN,LOW);

 
  //init LCD
  lcd.begin(16,2);            //2 row, 16 columns format
  lcd.setRGB(23,161,165);     //Arduino Teal background color
  lcd.setCursor(0,0);
  lcd.print("DIY PCR");       //Application prompt
  lcd.setCursor(0,1);
  lcd.print("(C) iDP 2018");
  delay(5000);
  lcd.setCursor(0,0);
  lcd.print("--:--:-- -- IDLE");
  lcd.setCursor(0,1);
  lcd.print("--:-- ---.--");
  lcd.print(char(223));
  lcd.print("C");
    
}

//loop: run repeatedly forever
void loop() {
  //check for start button pressed (HIGH to LOW transition)
  if(buttonPressed()){
    //run a full PCR cycle on start event
    runPCR();
  }
  updateTemperature();
}

//runPCR: run a full PCR sequence
//repeat NUM_CYCLE times the PCR sequence:
// Heating to denaturation temp
// Denaturation phase
// Cooling to annealing temp
// Annealing phase
// Heating to extension temp
// Extension phase
// at the end of cycle cool to room temp

void runPCR(void){
  boolean ris;    //phase result
  
  //set start time
  startCycleTime=millis();
  timeCounter=0;
  faultCode='0';
  
  //repeat for NUM_CYCLES cycles
  for(currCycle=0;currCycle<NUM_CYCLES;currCycle++){
    
    //heatup to denature temperature
    setStatus(HEATING);
    ris=heatUp(DENATURE_TEMP);
    if(ris==false){
      //heating fault or stop button: stop sequence
      currCycle = NUM_CYCLES;
      setStatus(FAULT);
      log();
      break;
    }

    //denaturarion phase
    setStatus(DENATURATION);
    if(currCycle == 0) {
      //first cycle: special denature time
      ris=holdConstantTemp(INITIAL_DENATURE_TIME, DENATURE_TEMP);
    } 
    else {
      //no first cycle: standard denature time
      ris=holdConstantTemp(DENATURE_TIME, DENATURE_TEMP);
    }
    if(ris==false){
      //denaturation fault or stop button: stop sequence
      currCycle = NUM_CYCLES;
      setStatus(FAULT);
      log();
      break;
    }

    //cool to annealing temperature
    setStatus(COOLING);
    ris=coolDown(ANNEALING_TEMP);
    if(ris==false){
      //cooling fault or stop button: stop sequence
      currCycle = NUM_CYCLES;
      setStatus(FAULT);
      log();
      break;
    }

    //annealing phase
    setStatus(ANNEALING);
    ris=holdConstantTemp(ANNEALING_TIME, ANNEALING_TEMP);
    if(ris==false){
      //annealing fault or stop button: stop sequence
      currCycle = NUM_CYCLES;
      setStatus(FAULT);
      log();
      break;
    }

    //heatup to extension temperature
    setStatus(HEATING);
    ris=heatUp((EXTENSION_TEMP));
    if(ris==false){
      //heatup fault or stop button: stop sequence
      currCycle = NUM_CYCLES;
      setStatus(FAULT);
      log();
      break;
    }

    //extension phase phase
    setStatus(EXTENSION);
    if (currCycle<(NUM_CYCLES-1)) {
      //not last cycle: standard extension time
      ris=holdConstantTemp(EXTENSION_TIME, EXTENSION_TEMP);
    } 
    else {
      //last cycle: special extension time
      ris=holdConstantTemp(FINAL_EXTENSION_TIME, EXTENSION_TEMP);
    }
    if(ris==false){
      //extension fault or stop button: stop sequence
      currCycle = NUM_CYCLES;
      setStatus(FAULT);
      log();
      break;
    }
  }
  //final cooling (indefinite time)
  digitalWrite(FAN_PIN,HIGH);
  if(currPhase!=FAULT) { //normal termination
    setStatus(IDLE);
  }  
  log(true);  
}

//heatUp: heat up to maxTemp temperature setpoint
//return true if maxTemp reached, false if fault (sensor not working, temp not reached, overheat)
//
boolean heatUp(double setpoint){
  boolean ris=false;
  int curIteration = 0;
  digitalWrite(FAN_PIN,LOW);
  startPhaseTime=millis();
  prevTemp=getTemperature();
  currTemp=prevTemp;
  log();
  //test if sensor working: if reading under ROOM_TEMP we assume sensor not working and stop the sequence
  if (currTemp < ROOM_TEMP) { //sensor not working
    faultCode='2';
    ris=false;
  }
  else if(currTemp > setpoint){ //over setpoint entering heating
    faultCode='3';
    ris=false;    
  }
  else { //sensor ok: execute heating rise ramp
    //repeat indefinitely while temperature under setpoint
    while (currTemp < setpoint) { 
      //check stop button
      if(buttonPressed()){
        faultCode='1';
        ris=false;
        break;
      }
      curIteration++;
      int pulseDuration = min(MAX_PULSE_DURATION, ((PULSE_DURATION_MULT *(setpoint-currTemp))+ MIN_PULSE_DURATION)); // as we approach desired temp, heat up slower
      digitalWrite(HEAT_PIN, HIGH);
      delay(pulseDuration);
      currTemp=getTemperature();
      log();
      digitalWrite(HEAT_PIN, LOW);
      //check if setpoint reached
      if(currTemp >= setpoint) { //checkpoint reached: stop rise ramp with success
        faultCode='0';
        ris=true;
        break;
      }
      //approaching to setpoint insert a little wait between heat pulses to avoid overheat
      if((setpoint-currTemp) < APPROACH_THRESHOLD_TEMP || curIteration % OVERHEAT_TEST == 0) {
        do {
          prevTemp = currTemp;
          delay(OVERHEAT_TIME); 
          currTemp=getTemperature();
          log();
        } while(currTemp > prevTemp);
      }
      //check again if setpoint reached
      if(currTemp >= setpoint){ //checkpoint reached: stop rise ramp with success
        faultCode='0';
        ris=true;
        break;   
      }
      //check each two iteration if setpoint not stable
      if ((curIteration%2) == 0) { //each two iterations
        if(currTemp < (prevTemp-STABILITY_THRESHOLD_TEMP)) { //setpoint not stable stop rise ramp with failure
          faultCode='4';
          ris=false;
          break; 
        }
      } 
      else {   
        prevTemp = currTemp;
      } 
      //check if rise ramp too fast
      while ((currTemp-prevTemp) >= MAX_HEAT_INCREASE) { //rise ramp too fast: wait for slope reduction
        prevTemp = currTemp;
        delay(MAX_HEAT_INCREASE_TIME);
        currTemp=getTemperature();
        log();
      } 
      //check for max temperature
      while(currTemp >= MAX_ALLOWED_TEMP) { //temperature too high: wait for temperature reduction
        delay(MAX_ALLOWED_TEMP_TIME);
        currTemp=getTemperature();
        log();
      }
    } 
  }
  return ris;
}

//coolDown: Cool down to the desired temperature by turning on the fan. 
//          setpoint -> Temperature we want to cool off to
//          maxTimeout -> how often we poll the thermocouple (300ms is good)
boolean  coolDown(double setpoint, int maxTimeout = COOL_SAMPLE_TIME){
  boolean ris=true;
  startPhaseTime=millis();
  currTemp=getTemperature();
  log();
  while (currTemp > setpoint+FAN_THRESHOLD_TEMP) {
    digitalWrite(FAN_PIN, HIGH);
    delay(maxTimeout);
    currTemp=getTemperature();
    log();
    //check stop button
    if(buttonPressed()){
      faultCode='1';
      ris=false;
      break;
    }    
  } 
  digitalWrite(FAN_PIN, LOW);
  return ris;
}

//holdConstantTemp: Try to stay close to the desired temperature by making micro adjustments to the 
//                  resistors and fan. Assumes that the current temperature is already very close to the setpoint.
//                  setpoint -> desired temperature
//                  duration ->  how long we should keep the temperature (in ms)

boolean holdConstantTemp(long duration, double setpoint) {
  boolean ris=true;
  //setup phase timer
  startPhaseTime=millis();
  unsigned long holdTimeElapsed = 0;
 
  //hold temperature to setpoint for duration time
  while (holdTimeElapsed < duration) {
  
    //check stop button
    if(buttonPressed()){
      faultCode='1';
      ris=false;
      break;
    }

    //hold temperature
    currTemp = getTemperature();
    log();
    if (currTemp < setpoint) { //under setpoint: heat up 
      digitalWrite(HEAT_PIN, HIGH);
      delay(HOLD_PULSE_DURATION);
      digitalWrite(HEAT_PIN, LOW);
    } 
    else if (currTemp > (setpoint+SETPOINT_THRESHOLD_TEMP)) { //over setpoint: cool down
      digitalWrite(FAN_PIN, HIGH);
      delay(HOLD_PULSE_DURATION);
      digitalWrite(FAN_PIN, LOW);
    }
    delay(HOLD_PAUSE_DURATION);
    holdTimeElapsed = millis() - startPhaseTime;
  } 
  return ris;
}

//log: send a CSV data line to console & LCD diagnostic
//CSV log format:
//<total_time>;<cycle_number>;<phase>;<phase_time>;<temperature>;<faultcode><cr><lf>

void log(boolean force=false){
  unsigned long cycleTime=millis()-startCycleTime;
  unsigned long phaseTime=millis()-startPhaseTime;
  char buffer[9];
  //cycle time in HH:MM:SS format
  unsigned long ts=cycleTime/1000; //time in seconds
  int h = ts/3600;                  //hours
  int t = ts % 3600;                //remaining seconds
  int m = t/60;                    //minutes 
  int s = t%60;                    //seconds
  if((ts>timeCounter)||(force==true)) { //second elapsed: print log
    timeCounter=ts;
    sprintf(buffer,"%02d:%02d:%02d",h,m,s);
    Serial.print(buffer);
    Serial.print(";");
    lcd.setCursor(0,0);
    lcd.print(buffer);
    //cycle number (startig from 1)
    sprintf(buffer,"%02d",currCycle+1);
    Serial.print(buffer);
    Serial.print(";");
    lcd.setCursor(9,0);
    lcd.print(buffer);
    //cycle phase (Idle,Heating,Cooling,Denature,Annealig,Extension,Fault)
    Serial.write(phases[currPhase][0]);
    Serial.print(";");
    lcd.setCursor(12,0);
    lcd.print(phases[currPhase]);
    if(currPhase==FAULT){
      lcd.print((char)faultCode);
    }
    //phase time in HH:MM:SS format
    ts=phaseTime/1000;  //time in seconds
    h = ts/3600;                  //hours
    t = t % 3600;
    m = t/60;                    //minutes 
    s = t%60;                    //seconds
    sprintf(buffer,"%02d:%02d",m,s);
    Serial.print(buffer);
    Serial.print(";");
    lcd.setCursor(0,1);
    lcd.print(buffer);
    //temperature in tt.tt (°C) format
    int iPart=(int)currTemp;
    int dPart=(int)((currTemp - (double)iPart) * 100);
    sprintf(buffer, "%03d.%02d", iPart, dPart);  
    Serial.print(buffer);
    Serial.print(";");
    Serial.write(faultCode);
    Serial.println();
    lcd.setCursor(6,1);
    lcd.print(buffer);  
    lcd.setCursor(12,1);
    lcd.print((char)223);     // degree symbol
    lcd.print("C");  
  }  
}

//getTemperature: temperature reading abstraction
//run mode: read from thermocople by means of max6675 device
//DEBUG mode: read for potentiometer connected to A0 pin
double getTemperature(void){
  double t;
  #ifdef DEBUG_SENS    //DEBUG MODE: read temperature from potentiometer on A0 pin
    int value=analogRead(A0);
    t=map(value,0,1023,17.0,102);
  #else           //RUN MODE: read temperature from max6675 device
    t=thermocouple.readCelsius();
  #endif
  return t;
}

//setStatus: change phase status and update local diagn
//RGB diagn colors:
//Idle=green, Heating=red, Cooling=blue, Denaturation=fuchsia, Annealing=cyan, eXtension=yellow, Fault=white.
void setStatus(int phase){
  int red,green,blue;
  currPhase=phase;
  switch(phase){
    case IDLE:
      red=0; green=0xff; blue=0;      //green
      break;
    case HEATING:
      red=0xff; green=0; blue=0;      //red
      break;
    case COOLING:
      red=0; green=0; blue=0xff;      //blue
      break;
    case DENATURATION:  
      red=0xff; green=0; blue=0xff;   //fuchsia
      break;
    case ANNEALING:     
      red=0; green=0xff; blue=0xff;    //cyan
      break;
    case EXTENSION:  
      red=0xff; green=0xff; blue=0;    //yellow
      break;
    case FAULT:
      red=0xff; green=0xff; blue=0xff;  //white
      break;
  }
  digitalWrite(RED_PIN,red);  
  digitalWrite(GREEN_PIN,green);  
  digitalWrite(BLUE_PIN,blue);  
}

//buttonPressed: check for falling edge of start/stop button
//              returns true: button pressed, false: button not pressed
boolean buttonPressed(void){
  boolean ris=false;
  currInButton=digitalRead(START_STOP_PIN);
  if((currInButton!=prevInButton)&&(currInButton==LOW)){ //falling edge
    ris=true;
  }
  prevInButton=currInButton;  
  return ris;
}

//updateTemperature: update temperature reading each one second during the IDLE phase
void updateTemperature(void){
  static unsigned long updateTime;
  char buffer[9];
  if (millis()-updateTime > 1000){
    currTemp=getTemperature();
    //temperature in tt.tt (°C) format
    int iPart=(int)currTemp;
    int dPart=(int)((currTemp - (double)iPart) * 100);
    sprintf(buffer, "%03d.%02d", iPart, dPart);  
    lcd.setCursor(6,1);
    lcd.print(buffer);  
    lcd.setCursor(12,1);
    lcd.print((char)223);     // degree symbol
    lcd.print("C");  
    updateTime=millis();
  }   
}
