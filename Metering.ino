/*
  Web Server

  A simple web server that shows the value of the analog input pins.
  using an Arduino Wiznet Ethernet shield.

  Circuit:
   Ethernet shield attached to pins 10, 11, 12, 13
   Analog inputs attached to pins A0 through A5 (optional)

  created 18 Dec 2009
  by David A. Mellis
  modified 9 Apr 2012
  by Tom Igoe
  modified 02 Sept 2015
  by Arturo Guadalupi

*/

#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>


#define NUM_COUNTERS  24

//int rtc[7];

byte numCounters = NUM_COUNTERS;
unsigned long eepromCountersCurrent[NUM_COUNTERS];
unsigned long eepromCountersLastSaved[NUM_COUNTERS];
byte oldPulseStates[NUM_COUNTERS];
const char *CounterNames[NUM_COUNTERS] = {
  "Varmepumpe 1d Gang",
  "Varmekabel Gang",
  "Sentral Stovsuger",
  "Torketrommel",
  "Vaskemaskin",
  "Oppvaskmaskin",
  "Vannpumpe",
  "Microbolgeovn",
  "Varmepumpe 2 Stue",
  "Varmekabel Bad 1.etg.",
  "Varmekabel Stue",
  "Stekeovn",
  "Komfyrtopp",
  "Varmtvannsbereder",
  "Varmekabel Bad 2.etg.",
  "Varmepumpe Kontor",
  "Lys og Stikk Kontor",
  "Lys og Stikk Garasje Loft",
  "Varme Garasje Loft",
  "Elbil Lading",
  "",
  "",
  "",
  "",
};
long CounterPorts[NUM_COUNTERS] = {
  26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48,
  27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49
};
long numPulsesPerCount[NUM_COUNTERS] = {500, 500, 500, 500, 500, 500, 500, 500};
long localCounters[NUM_COUNTERS];
long meterCounters[NUM_COUNTERS];

unsigned long cnt;
bool SetTime;

// start reading from the first byte (address 0) of the EEPROM
int address = 0;

const byte ledPin = 13;
const byte interruptPin = 2;
const byte timingPin = 3;
const byte timingValue = 127;
volatile byte state = LOW;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0x11, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 6);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

void ReadEeprom(bool setvars) {
  // Read counters from eeprom
  for (int eeprom_address = 0; eeprom_address < numCounters; eeprom_address++) {
    // Read an unsigned long from the current address of the EEPROM
    unsigned long value = 0;
    for (int n = 3; n >= 0; n--) {
      unsigned long v = EEPROM.read(address + (eeprom_address * 4) + n);
      value = (value * 256) + v;
    }
    if ( setvars)
    {
      eepromCountersCurrent[eeprom_address] = value;
      eepromCountersLastSaved[eeprom_address] = value;
    }
  }
}

void WriteEeprom(bool doclr) {
  // Write counters to eeprom
  for (int eeprom_address = 0; eeprom_address < numCounters; eeprom_address++) {
    // Write a unsigned long to the current address of the EEPROM
    unsigned long value = eepromCountersCurrent[eeprom_address] ;
    if ( doclr )
    {
      value = 0;
    }
    eepromCountersLastSaved[eeprom_address] = value;
    for (int n = 3; n >= 0; n--) {
      unsigned long v = value % 0x100;
      EEPROM.write(address + (eeprom_address * 4) + 3 - n, v);
      value = value / 256;
    }
  }
}


void setup() {
  cnt = 0;
  SetTime = false;

  // Prepare interrupts to arrive.
  pinMode(ledPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, CHANGE);

  pinMode(timingPin, OUTPUT);
  analogWrite(timingPin, timingValue);

  for (int i = 0; i < numCounters; i++) {
    pinMode(CounterPorts[i], INPUT);
  }

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  Wire.begin();
#if 0

  tmElements_t tm;
  RTC.read(tm);
  if (tm.Year < 16) {
    SetTime = true;
#if 0
    RTC.stop();
    RTC.set(DS1307_SEC, 0);
    RTC.set(DS1307_MIN, 35);
    RTC.set(DS1307_HR, 2);
    RTC.set(DS1307_DOW, 7);
    RTC.set(DS1307_DATE, 11);
    RTC.set(DS1307_MTH, 9);
    RTC.set(DS1307_YR, 16);
    RTC.start();
#endif
    Serial.println("RTC was initialized!!!!!!!!!!!!!!!!!!!");
  }

  //RTC.adjust(DateTime(__DATE__, __TIME__));

  //if (! RTC.isrunning()) {
  //  Serial.println("RTC is NOT running!");
  //}
#endif

  // FIX Read values from EEPROM into Counters;
  ReadEeprom(true);
}

void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 10");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");

          // output the counter and state
          client.print("Counter is: ");
          client.print(cnt);
          client.print(", State is: ");
          client.print(state);
          client.print(", SetTime is: ");
          client.print(((SetTime == true) ? "1" : "0"));
          
          client.print(", Code compiled:");
          client.print(__DATE__);
          client.print(" - ");
          client.print(__TIME__);
#if 1
          tmElements_t tm;
          RTC.read(tm);
          client.print(", RTC Date&Time: ");
          client.print(tm.Year+1970);
          client.print("/");
          client.print(tm.Month);
          client.print("/");
          client.print(tm.Day);
          client.print("-");
          client.print(tm.Hour);
          client.print(":");
          client.print(tm.Minute);
          client.print(":");
          client.print(tm.Second);
#endif
          client.println("<br />");

          // output contents of current counters
          for (int eeprom_address = 0; eeprom_address < numCounters; eeprom_address++) {
            // Output the initial eeprom values
            client.print("Counter ");
            if ( eeprom_address < 10 )
            {
              client.print("0");
            }
            client.print(eeprom_address);
            client.print(" have value: ");
            unsigned long v = eepromCountersCurrent[eeprom_address];
            unsigned long d = v % 2000;
            d = d / 2;
            v = v / 2000;
            if ( v < 10 )
            {
              client.print("0");
            }
            if ( v < 100 )
            {
              client.print("0");
            }
            if ( v < 1000 )
            {
              client.print("0");
            }
            client.print(v);
            client.print(",");
            if ( d < 10 )
            {
              client.print("0");
            }
            if ( d < 100 )
            {
              client.print("0");
            }
            client.print(d);
            client.print(" kWh (");
            client.print(CounterNames[eeprom_address]);
            client.print(")");
            client.println("<br />");
          }
#if 0
          // output the value of each analog input pin
          for (int analogChannel = 0; analogChannel < numCounters; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
#endif
          WriteEeprom(false);
          ReadEeprom(false);
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
  else
  {
    //Serial.print("No client ");
    //Serial.println(cnt);
    //cnt++;
    //delay(500);
  }
  digitalWrite(ledPin, state);
}

void blink() {
  state = !state;
  cnt++;

  // FIX Read values from board inputs and compare them to oldPulseStates
  // If the state went LOW, then increase the correct counter and update the oldPulseStes value.

  // FIX When the counter have counted the number of units it is supposed to, increase counter and store it in the EEPROM.
  //eepromCountersCurrent[0] = eepromCountersCurrent[0] + 1;

  for (int i = 0; i < numCounters; i++) {
    int val = digitalRead(CounterPorts[i]);
    if ( val )
    {
      if ( !oldPulseStates[i] )
      {
        oldPulseStates[i] = true;
        eepromCountersCurrent[i] = eepromCountersCurrent[i] + 1;
      }
    }
    else
    {
      oldPulseStates[i] = false;
    }
  }
}
