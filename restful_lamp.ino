/* 
* RESTful WS2801 LED strip
* 
* Jonathan Deprizio <jondeprizio@gmail.com> 
* http://www.techthrob.com
* 
*
* Credits and Licensing
* Much of the ws2801 code is based off of or copied from the adafruit the ws2801 examples 
* included in that library; written by Limor Fried/Ladyada for Adafruit Industries
* and licensed under the BSD license.
*
* The uHTTP server code is based in large part on the uHTTP_REST_INTERFACE
* example included with that library, and released into the public domain.
*
* Any code not covered under the above is hereby released into the public domain. 
*
*
* Components used:
* 1x Arduino Uno
* 1x http://www.seeedstudio.com/wiki/Ethernet_Shield v1.1
* 1x http://www.adafruit.com/products/322
*
*/

#include <uHTTP.h>
#include <EthernetClient.h>
#include <Ethernet.h>

#include "Adafruit_WS2801.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma
#ifdef __AVR_ATtiny85__
 #include <avr/power.h>
#endif



/*********** PIN Definitions ****************/

#define LEDdataPin 4
#define LEDclockPin 5
#define pingSignal 6 

/********************************************/


/********** REQUEST DEFINITIONS *************/

#define NOCODE -1
#define RAINBOW 1
#define OFF 2
#define COLORWIPE 3
#define SOLID 4
#define RAINBOWCYCLE 5
#define PULSATE 6

int controllerCode = NOCODE;
/********************************************/


/*********** LED Stuff **********************/
#define LED_COUNT 25
Adafruit_WS2801 strip = Adafruit_WS2801(LED_COUNT, LEDdataPin, LEDclockPin);
/********************************************/


/************ Ethernet Stuff ****************/
byte mac[] = {
  0xBA, 0xDA, 0x55, 0xBA, 0xDA, 0x55
};
byte ip4addr[4] = {192,168,1,177};
uHTTP *server = new uHTTP(80);
EthernetClient response;
/********************************************/

  
  
void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

  strip.begin();
  strip.show();


  Serial.begin(9600);


  Ethernet.begin(mac, ip4addr);
  server->begin();
  

}


/** Light profiles that are continously executed are of the type EthernetClient
*   because they check internally whether a new client (command) is available, and then return
*   it if so.  This is necessary because if, for example, you set the lamp to be in 'rainbow' mode,
*   we live inside the rainbow function for a lengthy period of time before returning (due to the
*   calls to delay), and the client would otherwise timeout before we got to it.
*/

EthernetClient pulsate(int originalr, int originalg, int originalb, uint8_t wait, uHTTP *server) {
  int i, j;
   
   int r = originalr;
   int g = originalg;
   int b = originalb;
   
   if (r <= 0 && g <= 0 && g <= 0)
   {
     r = originalr;
     g = originalg;
     b = originalg;
   }
   else
   { 
      r-=5;
      g-=5;
      b-=5;
   }

    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Color(r,g,b));
    }  
    strip.show();   // write all the pixels out
    
    // Check to see if we're interrupted 
    if (response = server->available())
    {
      Serial.println("Found someone waiting.");
      return response;
    }
    else
    {
      Serial.println("No one waiting.");
    }
    
    delay(wait);
}

EthernetClient rainbow(uint8_t wait, uHTTP *server) {
  int i, j;
   
  for (j=0; j < 256; j++) {     // 3 cycles of all 256 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 255));
    }  
    strip.show();   // write all the pixels out
    
    // Check to see if we're interrupted 
    if (response = server->available())
    {
      Serial.println("Found someone waiting.");
      return response;
    }
    else
    {
      Serial.println("No one waiting.");
    }
    
    delay(wait);
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
EthernetClient rainbowCycle(uint8_t wait, uHTTP *server) {
  int i, j;
  
  for (j=0; j < 256 * 5; j++) {     // 5 cycles of all 25 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 96 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel( ((i * 256 / strip.numPixels()) + j) % 256) );
    }  
    strip.show();   // write all the pixels out
    if (response = server->available())
    {
      Serial.println("Found someone waiting.");
      return response;
    }
    else
    {
      Serial.println("No one waiting.");
    }
    delay(wait);

  }
}


/** "Solid" mode does not have any delays and is executed "instantly", thus no need
* to check for a waiting client. */
void solid(uint32_t c)
{
  Serial.println("Set solid color");
  Serial.println(c);
  int i;
  for (i=0; i< strip.numPixels(); i++)
  {
    strip.setPixelColor(i, c);
    strip.show();
  }
}

/** Here, assume delay is 50ms, 50*25 (assume strip length 25) = 1.25 seconds
* maximum before a client is heard.  Fine.  Maybe TODO fix this */
// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint32_t c, uint8_t wait) {
  int i;
  
  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}


/* Helper functions */

// Create a 24 bit color value from R,G,B
uint32_t Color(int r, int g, int b)
{

  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85) {
   return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170; 
   return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}



void httpResponse(EthernetClient client, int rc)
{
  String rcs;
  switch (rc) {
    case 200:
      rcs = "200 OK";
      break;
    case 404:
      rcs = "404 Not Found";
      break;
    default:
      rcs = "500 Internal Server Error";
      break;
  }
  client.print("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close"); // close connection once response is delivered
  client.println("");
  client.println("<!DOCTYPE HTML>");
  client.println("<html>Reponse.</html>");
}


  int R = 0;
  int G = 0;
  int B = 0;
  int interval = 20;

void loop() {
  // put your main code here, to run repeatedly:
  response = server->available();
  Serial.println("loop.");

  if (controllerCode != NOCODE && (!response))
  {
      Serial.println("No client.");
      switch (controllerCode)
      {
        case RAINBOW:
          response = rainbow(interval, server);
          break;
        case RAINBOWCYCLE:
          response = rainbowCycle(interval, server);
          break;
        case COLORWIPE:
          colorWipe(Color(255, 0, 0), interval);
          colorWipe(Color(0, 255, 0), interval);
          colorWipe(Color(0, 0, 255), interval);
          break;
        case SOLID:
          Serial.println("Solid");
          solid(Color(R, G, B));
          break;
        case PULSATE:
          Serial.println("Pulsate.");
          pulsate(R, G, B, interval, server);
          break;
        default: 
          Serial.print("Waiting.");
          break;
       }
  }
  
  if (response)
  {
    Serial.println("Client connection");
    if (server->method(uHTTP_METHOD_GET))
    {
      
      if (strcmp(server->uri(1), "rainbow") == 0)
      {
        controllerCode = RAINBOW;
        interval = atoi((server->uri(2)));
      }
      else if (strcmp(server->uri(1), "colorwipe") == 0)
      {
        controllerCode = COLORWIPE;
        interval = atoi(server->uri(2));

      }
      else if (strcmp(server->uri(1), "rainbowcycle") == 0)
      {
        controllerCode = RAINBOWCYCLE;
        interval = atoi(server->uri(2));

      }
      else if (strcmp(server->uri(1), "solid") == 0)
      {
        R = atoi(server->uri(2));
        G = atoi(server->uri(3));
        B = atoi(server->uri(4));
        controllerCode = SOLID;
        Serial.println("Solid");
        Serial.print("R: ");
        Serial.println(R);
        Serial.print("G: ");
        Serial.println(G);
        Serial.print("B: ");
        Serial.println(B);
      }
      else
      {
        httpResponse(response, 404);
      }
      httpResponse(response, 200);
    }
    else
    {
      // Only GETs are supported. 
      httpResponse(response, 500);
    }
  }
  delay(1);
  response.stop();
  Serial.println("Closed connection.");  
}

