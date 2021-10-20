
#define BLYNK_PRINT Serial
#define BLYNK_MAX_READBYTES 512

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>


BlynkTimer timer;
Adafruit_INA219 ina219;
//#define SCREEN_WIDTH 128 // OLED display width, in pixels
//#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//   Virtual Pins - Base


#define vPIN_VOLTAGE           V0
#define vPIN_CURRENT           V1
#define vPIN_POWER             V2
#define vPIN_ENERGY            V3
#define vPIN_CAPACITY          V4
#define vPIN_TEMP              V5
#define vPIN_CURRENT_GRAPH     V6
#define vPIN_ENERGY_PRICE      V7
#define vPIN_ENERGY_COST       V8
#define vPIN_BUTTON_AUTORANGE  V9


const int oneWireBus = 1;    
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);



float shuntvoltage = 0.00;
float busvoltage = 0.00;
float current_mA = 0.00;
float loadvoltage = 0.00;
float energy = 0.00,  energyCost, energyPrevious, energyDifference;
float energyPrice = 21 ;
float power = 0.00;
float tempC=0.00;
float tempF=0.00;
float capacity=0.00;
int sendTimer, pollingTimer, priceTimer, graphTimer, autoRange, countdownResetCon, countdownResetClock, counter2, secret, stopwatchTimer;
long stopwatch;
int splitTimer1, splitTimer2, splitTimer3, splitTimer4, splitTimer5;
int sendTimer1, sendTimer2, sendTimer3, sendTimer4, sendTimer5;
unsigned long previousMillis = 0;
unsigned long interval = 100;



char auth[] = "xxxx";   // Enter Blynk auth id
char ssid[] = "xx";  //Enter your WIFI Name
char pass[] = "xxx";  //Enter your WIFI Password

/****************************************************************************/

void get_sensor_data() {

  // get the INA219 and DS18B20 Sensor values and throw some basic math at them  

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  loadvoltage = busvoltage + (shuntvoltage / 1000); // V
  power = current_mA * loadvoltage ; // mW
  energy = energy + (power / 1000 / 1000); //Wh
  capacity = capacity + current_mA/1000;  

  // nothing connected? set all to 0, otherwise they float around 0.

  if (loadvoltage < 1.2 )loadvoltage = 0;
  if(current_mA < 2 ) 
  {    
    current_mA = 0;
    power = 0;
    energy = 0;
    capacity=0;
    }    

 // sensors.requestTemperatures(); // get temperatures
//  tempC = sensors.getTempCByIndex(0);
  //tempF = sensors.getTempFByIndex(0);  
}


// this function is for updating the REAL TIME values and is on a timer

void display_data() {

  // VOLTAGE

  Blynk.virtualWrite(vPIN_VOLTAGE, String(loadvoltage, 2) + String(" V") );


  // CURRENT 

  if (current_mA > 1000 && autoRange == 1) {
    Blynk.virtualWrite(vPIN_CURRENT, String((current_mA / 1000), 3) + String(" A") );
   
  } else {

    Blynk.virtualWrite(vPIN_CURRENT, String(current_mA, 2) + String(" mA"));
   
  }   

  // POWER

  if (power > 1000 && autoRange == 1) {
   Blynk.virtualWrite(vPIN_POWER, String((power / 1000), 2) + String(" W") );
       

  } else {
   Blynk.virtualWrite(vPIN_POWER, String(power, 0) + String(" mW") );
   
  }  

  energyDifference = energy - energyPrevious;
  // ENERGY CONSUMPTION
  if (energy > 1000 && autoRange == 1) {
    Blynk.virtualWrite(vPIN_ENERGY, String((energy / 1000), 3) + String(" kWh"));
        
  } else {

    Blynk.virtualWrite(vPIN_ENERGY, String(energy, 3) + String(" Wh"));
   
  }
  energyPrevious = energy;   

  // ENERGY COST
  energyCost = energyCost + ((energyPrice / 1000 / 100) * energyDifference);
  if (energyCost > 9.999) {
    Blynk.virtualWrite(vPIN_ENERGY_COST, String((energyCost),7));
  } else {
    Blynk.virtualWrite(vPIN_ENERGY_COST, String((energyCost), 7));
  }
 // CAPACITY 

  if (capacity > 1000 && autoRange == 1){
  Blynk.virtualWrite(vPIN_CAPACITY, String((capacity/ 1000), 2) + String(" Ah") );
  
  }
  else{
  Blynk.virtualWrite(vPIN_CAPACITY, String((capacity), 2) + String(" mAh") );

  }


}

// AUTO RANGE BUTTON

BLYNK_WRITE(vPIN_BUTTON_AUTORANGE) {
  autoRange = param.asInt();
  display_data();
}



// the stopwatch counter which is run on a timer

void stopwatchCounter() {
  stopwatch++;
  long days = 0, hours = 0, mins = 0, secs = 0;
  String secs_o = ":", mins_o = ":", hours_o = ":";
  secs = stopwatch; //convect milliseconds to seconds
  mins = secs / 60; //convert seconds to minutes
  hours = mins / 60; //convert minutes to hours
  days = hours / 24; //convert hours to days
  secs = secs - (mins * 60); //subtract the coverted seconds to minutes in order to display 59 secs max
  mins = mins - (hours * 60); //subtract the coverted minutes to hours in order to display 59 minutes max
  hours = hours - (days * 24); //subtract the coverted hours to days in order to display 23 hours max
  if (secs < 10) secs_o = ":0";
  if (mins < 10) mins_o = ":0";
  if (hours < 10) hours_o = ":0";
 // Blynk.virtualWrite(vPIN_ENERGY_TIME, days + hours_o + hours + mins_o + mins + secs_o + secs);

}  


/****************************************************************************/

void setup() {
  Serial.begin(115200); 
  Blynk.begin(auth, ssid, pass);

  // START INA219
  ina219.begin();
 
  
  // TIMERS 

  pollingTimer = timer.setInterval(1000, get_sensor_data);
  graphTimer = timer.setInterval(4000, []() {
  Blynk.virtualWrite(vPIN_CURRENT_GRAPH, current_mA);
  });

  stopwatchTimer = timer.setInterval(1000, stopwatchCounter); 

  // setup split-task timers so we dont overload ESP
  // with too many virtualWrites per second
  timer.setTimeout(200, []() {
    sendTimer1 = timer.setInterval(2000, display_data); 

  });    

 

  // start in auto-range mode & sync widget to hardware state
  autoRange = 0;
  Blynk.virtualWrite(vPIN_CURRENT_GRAPH, 1);
  Blynk.virtualWrite(vPIN_ENERGY_PRICE, String(energyPrice, 4) );
}
/****************************************************************************/

void loop() {
  // the loop... dont touch or add to this!
  Blynk.run();
  timer.run(); 
}
