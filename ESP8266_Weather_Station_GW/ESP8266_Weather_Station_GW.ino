// Includes
#include <ESP8266WiFi.h>        //ESP8266 Core WiFi Library
#include <ESP8266WiFiMulti.h>   //ESP8266 MultiWifi Library
#include <ESP8266HTTPClient.h>  //ESP8266 HTTP Client Library

// Defines
#define DEBUG
#define INVALID_DATA -1
#define VALID_DATA 1

// Constants
const long interval = 900000;
const float LOCAL_ALTITUDE_METERS = 278.0;
const String WU_USER = "INEWMARK17";
const String WU_PASS = "8obbbera";
//const String WU_USER = "INEWMARK16";
//const String WU_PASS = "3unh2q5c";


// Global Variables
float   winddir, windspeedmph, windgustmph, windgustdir, windspdmph_avg2m, humidity, tempf, rainin, dailyrainin, baromin, dewptf;
int     dollarsign, hash, comma1, comma2, valid, datastat, TIMER;
String  subdata, data;

// Classes
ESP8266WiFiMulti WiFiMulti;
HTTPClient http;

// Function to convert pressure based on local attitude
float convertToInHg(float pressure_Pa)
{
  float pressure_mb = pressure_Pa / 100; //pressure is now in millibars, 1 pascal = 0.01 millibars
  float part1 = pressure_mb - 0.3; //Part 1 of formula
  float part2 = 8.42288 / 100000.0;
  float part3 = pow((pressure_mb - 0.3), 0.190284);
  float part4 = LOCAL_ALTITUDE_METERS / part3;
  float part5 = (1.0 + (part2 * part4));
  float part6 = pow(part5, (1.0/0.190284));
  float altimeter_setting_pressure_mb = part1 * part6; //Output is now in adjusted millibars
  float baromin = altimeter_setting_pressure_mb * 0.02953;
  return(baromin);
}

// Parse input string into discrete global variables
//
int parsewispdata() {
  int retvalue = 1;
  
  dollarsign = data.indexOf('$');
  hash = data.indexOf('#');
  #ifdef DEBUG
  Serial.print("$=");
  Serial.println(dollarsign);
  Serial.print("#=");
  Serial.println(hash);
  #endif
  if ( dollarsign != -1 && hash != -1 ) {
    #ifdef DEBUG
    Serial.println("Valid Data");
    #endif
    valid = 1;
    comma1 = data.indexOf(',');
       
    while ( valid ) {
      comma2 = data.indexOf(',', comma1 + 1);
      if ( comma1 != -1 && comma2 != -1 ) {
          subdata = data.substring(comma1 + 1, comma2 );
          if ( subdata.startsWith("winddir=")) {
            String temp = subdata.substring(8, subdata.length());
            winddir = temp.toFloat();
        }
        
        if ( subdata.startsWith("windspeedmph=")) {
          String temp = subdata.substring(13, subdata.length());
          windspeedmph = temp.toFloat();
        }

        if ( subdata.startsWith("windgustmph=")) {
          String temp = subdata.substring(12, subdata.length());
          windgustmph = temp.toFloat();
        }

        if ( subdata.startsWith("windgustdir=")) {
          String temp = subdata.substring(12, subdata.length());
          windgustdir = temp.toFloat();
        }

        if ( subdata.startsWith("windspdmph_avg2m=")) {
          String temp = subdata.substring(17, subdata.length());
          windspdmph_avg2m = temp.toFloat();
        }

        if ( subdata.startsWith("humidity=")) {
          String temp = subdata.substring(9, subdata.length());
          humidity = temp.toFloat();
        }
        
        if ( subdata.startsWith("tempf=")) {
            String temp = subdata.substring(6, subdata.length());
            tempf = temp.toFloat();
        }
          
        if ( subdata.startsWith("pressure=")) {
            String temp = subdata.substring(9, subdata.length());
            baromin = convertToInHg(temp.toFloat());
            //baromin = 0.295300 * temp.toFloat();
        }
        comma1 = comma2;
      }
      else {
        valid = 0;
      }
    }
    #ifdef DEBUG
    Serial.println("Valid Data");
    #endif
    retvalue = VALID_DATA;
  }
  else {
    #ifdef DEBUG
    Serial.println("Invalid Data");
    #endif
    retvalue = INVALID_DATA;
  }
  return retvalue;
}



void setup() {
  // Use WiFiMulti to seutp WiFi, the ESP will connect to the configured AP's
  // configured.
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("WHINE", "AndrewHerdman");
  WiFiMulti.addAP("NewMakeIt_24", "YorkMakers");
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    #ifdef DEBUG
    Serial.print(".");
    #endif
  }

  // start up the serial connection
  Serial.begin(115200) ;
  // set the timeout for reading to 1 second
  Serial.setTimeout(1000) ;

  #ifdef DEBUG
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  #endif
}

void loop() {
 // Main Loop Code
 
 // Initialize Variables
  winddir = -1;
  windspeedmph = -1;
  windgustmph = -1;
  humidity = -1;
  tempf = -1000;
  rainin = -1;
  dailyrainin = -1;
  baromin = -1;
  dewptf = -1000;
  
  int validInput = 0;
 
  // trash any garbage found at the beginning
  if ( Serial.available() > 0 ) {
    Serial.read();
  }

  // - Send WISP "!", this asks the WISP to tell us about the weather.
  while ( 1 != Serial.write("!") ) {
    // we didn't send the right command over, sleep and re-try
    if ( Serial.available() > 0 ) {
      Serial.read();
    }
    delay(100) ;
  }

  // - Get Data line Start $ End #
  data = Serial.readStringUntil( 0x0A ) ;
  if ( data.length() > 0 )
  {
    #ifdef DEBUG
    Serial.println("Valid Data from readString!!!");
    #endif
    // we got something, check the string we got back
    validInput = parsewispdata();
  }
  else
  {
    // we got nothing?!!
    #ifdef DEBUG
    Serial.println("Invalid Data from readString?!!");
    #endif
  }

  if ( validInput ) {
    // - WiFi On
    WiFi.mode(WIFI_STA);
    while ( WiFiMulti.run() != WL_CONNECTED) {
      delay(500);
    }
    
    // - Send Wunderground http request
    String URL = "http://rtupdate.wunderground.com/weatherstation/updateweatherstation.php?ID=" + WU_USER + 
                 "&PASSWORD=" + WU_PASS + 
                 "&dateutc=now";
    if ( winddir > -1 ) {
      URL = URL + "&winddir=" + winddir;         
    }
    if ( windspeedmph > -1 ) {
      URL = URL + "&windspeedmph=" + windspeedmph;
    }
    if (windgustmph > -1 ) {
      URL = URL + "&windgustmph=" + windgustmph;
    }
    if ( humidity > -1 ) {
      URL = URL + "&humidity=" + humidity;
    }
    if ( tempf > -1000 ) {
      URL = URL + "&tempf=" + tempf;
    }
    if ( rainin > -1 ) {
      URL = URL + "&rainin=" + rainin;
    }
    if ( dailyrainin > -1 ) {
      URL = URL + "&dailyrainin=" + dailyrainin;
    }
    if ( baromin > -1 ) {
      URL = URL + "&baromin=" + baromin;
    }
    if ( dewptf > -1000 ) {
      URL = URL + "&dewptf=" + dewptf;
    }
    http.begin(URL);
    int httpCode = http.GET();
    if ( httpCode > 0 ) {
      #ifdef DEBUG
        Serial.printf("[HTTP] Get return code: %d\n", httpCode);
      #endif
      if ( httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        #ifdef DEBUG
          Serial.println("Web site response:");
          Serial.println(payload);
          // success
          // INVALIDPASSWORDID|Password or key and/or id are incorrect
        #endif
      }
    }
    else {
      #ifdef DEBUG
        Serial.printf("[HTTP] Get... failed, error %s\n", http.errorToString(httpCode).c_str());
      #endif
    }

    // - WiFi Off - uses much less power, delay required for sleep to take
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(100);

    // - Sleep 5 Minutes
    unsigned long previousMillis = 0;
    unsigned long currentMillis = 0;
    int sleeploop = 1; 
    while ( sleeploop == 1) {
      currentMillis = millis();
      if ( currentMillis - previousMillis >= interval ) {
        previousMillis = currentMillis;
        sleeploop = 0;
      }
      delay(1000);
    }
  }
}

/*
 * 
 *  Serial.print("$,winddir=");
  Serial.print(winddir);
  Serial.print(",windspeedmph=");
  Serial.print(windspeedmph, 1);
  Serial.print(",windgustmph=");
  Serial.print(windgustmph, 1);
  Serial.print(",windgustdir=");
  Serial.print(windgustdir);
  Serial.print(",windspdmph_avg2m=");
  Serial.print(windspdmph_avg2m, 1);
  Serial.print(",winddir_avg2m=");
  Serial.print(winddir_avg2m);
  Serial.print(",windgustmph_10m=");
  Serial.print(windgustmph_10m, 1);
  Serial.print(",windgustdir_10m=");
  Serial.print(windgustdir_10m);
  Serial.print(",humidity=");
  Serial.print(humidity, 1);
  Serial.print(",tempf=");
  Serial.print(tempf, 1);
  Serial.print(",rainin=");
  Serial.print(rainin, 2);
  Serial.print(",dailyrainin=");
  Serial.print(dailyrainin, 2);
  Serial.print(",&pressure="); //Don't print pressure= because the agent will be doing calcs on the number
  Serial.print(pressure, 2);
  Serial.print(",batt_lvl=");
  Serial.print(batt_lvl, 2);
  Serial.print(",light_lvl=");
  Serial.print(light_lvl, 2);

#ifdef LIGHTNING_ENABLED
  Serial.print(",lightning_distance=");
  Serial.print(lightning_distance);
#endif

  Serial.print(",");
  Serial.println("#");

#ifdef DEBUG
    Serial.println("Waiting for data...");
  #endif
  data = Serial.readStringUntil('\n');
  #ifdef DEBUG
    Serial.println(data);
  #endif

*/

