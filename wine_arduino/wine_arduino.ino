/* Name: wine_arduino.ino
 
 Read data from multiple DS18x20 digital temperature sensors
 DS18B20 <http://www.maxim-ic.com/datasheet/index.mvp/id/2812>
 DS18S20 <http://www.maxim-ic.com/datasheet/index.mvp/id/2815>
 
 * Detect number and type of sensors, and whether each is using parasitic power
 * Gracefully handles plugging/unplugging of sensors (but will not
   detect new sensors after initialization)
 
 Based on sample.pde from OneWire library (v2.0),
 copyright Jim Studt, Paul Stoffregen, Robin James and others
 Requires OneWire <http://www.pjrc.com/teensy/td_libs_OneWire.html>
  
 Credit also goes to: Kelsey Jordahl (kjordahl@alum.mit.edu)
*/
 
#include <OneWire.h>
 
#define ONEWIREPIN   3		/* OneWire bus on digital pin 7 */
#define MAXSENSORS   5		   /* Maximum number of sensors on OneWire bus */
 
// Model IDs  
#define DS18S20      0x10
#define DS18B20      0x28
#define DS1822       0x22
 
// OneWire commands
#define CONVERT_T       0x44  // Tells device to take a temperature reading and put it on the scratchpad
#define COPYSCRATCH     0x48  // Copy EEPROM
#define READSCRATCH     0xBE  // Read EEPROM
#define WRITESCRATCH    0x4E  // Write to EEPROM
#define RECALLSCRATCH   0xB8  // Reload from last known
#define READPOWERSUPPLY 0xB4  // Determine if device needs parasite power
#define ALARMSEARCH     0xEC  // Query bus for devices with an alarm condition
 
class Sensor /* hold info for a DS18 class digital temperature sensor */
{
 public:
  byte addr[8];
  boolean parasite;
  float temp;
 
};
 
 
int HighByte, LowByte, TReading, SignBit, Tc_100;
byte i, j, sensors;
byte present = 0;
boolean ready;
int dt;
byte data[12];
byte addr[8];
OneWire ds(ONEWIREPIN);		// DS18S20 Temperature chip i/o
Sensor DS[MAXSENSORS];		   /* array of digital sensors */
 
 
 
 
void setup(void) {
  // initialize inputs/outputs
  // start serial port
  Serial.begin(9600);
  Serial.println('wine-cellar');
  sensors = 0;
  Serial.println("Searching for sensors...");
  while ( ds.search(addr)) {
    if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print("CRC is not valid!\n");
      break;
    }
    delay(1000);
    ds.write(READPOWERSUPPLY);
    boolean parasite = !ds.read_bit();
    present = ds.reset();
    Serial.print("temp");
    Serial.print(sensors,DEC);
    Serial.print(": ");
    DS[sensors].parasite = parasite;
    for( i = 0; i < 8; i++) {
      DS[sensors].addr[i] = addr[i];
      Serial.print(addr[i], HEX);
      Serial.print(" ");
    }
    Serial.print('int addr: ');
    Serial.print(getSensorIntAddr(sensors));
    Serial.print(" ");
    //Serial.print(addr,HEX);
    if ( addr[0] == DS18S20) {
      Serial.print(" DS18S20");
    }
    else if ( addr[0] == DS18B20) {
      Serial.print(" DS18B20");
    }
    else {
      Serial.print(" unknown");
    }
    if (DS[sensors].parasite) {Serial.print(" parasite");} else {Serial.print(" powered");}
    Serial.println();
    sensors++;
  }
  Serial.print(sensors,DEC);
  Serial.print(" sensors found");
  Serial.println();
  for (i=0; i<sensors; i++) {
    Serial.print("temp");
    Serial.print(i,DEC);
    if (i<sensors-1) {
      Serial.print(",");
    }
  }
  Serial.println();
 
}
 
void get_ds(int sensors) {		/* read sensor data */
  for (i=0; i<sensors; i++) {
    ds.reset();
    ds.select(DS[i].addr);
    ds.write(CONVERT_T,DS[i].parasite);	// start conversion, with parasite power off at the end
 
    if (DS[i].parasite) {
      dt = 75;
      delay(750);      /* no way to test if ready, so wait max time */
    } else {
      ready = false;
      dt = 0;
      delay(10);
      while (!ready && dt<75) {	/* wait for ready signal */
      delay(10);
      ready = ds.read_bit();
      dt++;
      }
    }
 
    present = ds.reset();
    ds.select(DS[i].addr);    
    ds.write(READSCRATCH);         // Read Scratchpad
  
    for ( j = 0; j < 9; j++) {           // we need 9 bytes
      data[j] = ds.read();
    }
 
    /* check for valid data */
    if ( (data[7] == 0x10) || (OneWire::crc8( addr, 8) != addr[8]) ) {
      LowByte = data[0];
      HighByte = data[1];
      TReading = (HighByte << 8) + LowByte;
      SignBit = TReading & 0x8000;  // test most sig bit
      if (SignBit) // negative
      {
        TReading = (TReading ^ 0xffff) + 1; // 2's comp
	}
      if (DS[i].addr[0] == DS18B20) { /* DS18B20 0.0625 deg resolution */
      Tc_100 = (6 * TReading) + TReading / 4; // multiply by (100 * 0.0625) or 6.25
      }
      else if ( DS[i].addr[0] == DS18S20) { /* DS18S20 0.5 deg resolution */
      Tc_100 = (TReading*100/2);
      }
 
 
      if (SignBit) {
      DS[i].temp = - (float) Tc_100 / 100;
      } else {
      DS[i].temp = (float) Tc_100 / 100;
      }
    } else {		 /* invalid data (e.g. disconnected sensor) */
      DS[i].temp = NAN;
    }
  }
}
 
void loop(void) {
  get_ds(sensors);
  for (i=0; i<sensors; i++) {
    if (isnan(DS[i].temp)) {
      Serial.print("NaN");
    } else {
      //Serial.print(DS[i].addr)
      Serial.print(DS[i].temp,2);
    }
    if (i<sensors-1) {
      Serial.print(",");
    }
  }
  Serial.println();
  delay(5000);
}

int getSensorIntAddr(int sensorIndex) {
  int sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += DS[sensorIndex].addr[i] * (i + 1);
  }
  return sum;
}

