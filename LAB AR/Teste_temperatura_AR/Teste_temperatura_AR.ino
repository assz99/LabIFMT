#include <SPI.h>
#include <LoRa.h>
#include "DHT.h"
#include <Wire.h>  
#include "SSD1306.h" 

// Pin definetion of WIFI LoRa 32
// HelTec AutoMation 2017 support@heltec.cn 
#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
DHT dht(2, DHT22);

#define BAND    433E6  //you can set band here directly,e.g. 868E6,915E6
#define PABOOST true

String outgoing;              // outgoing message

byte localAddress = 0xBB;     // Endereço deste dispositivo
byte destination = 0xFD;      // Endereço do dispositivo enviado

byte msgCount = 0;            // count of outgoing messages
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends.

SSD1306  display(0x3c, 4, 15, 16);
void setup()
{
  Serial.begin(115200);                   // initialize serial
  while (!Serial);
  dht.begin();
  Serial.println("LoRa Duplex");

  SPI.begin(5,19,27,18);
  LoRa.setPins(18 ,14,26);// set CS, reset, IRQ pin
  display.init();
  display.flipScreenVertically();  
  display.setFont(ArialMT_Plain_10);
  if (!LoRa.begin(BAND,PABOOST))
  {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    display.drawString(0, 0, "Starting LoRa failed!");
    display.display();
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
  display.drawString(0, 0, "LoRa Initial success!");
  display.display();
  delay(1000);
}

void loop()
{
  if (millis() - lastSendTime > interval)
  {
    float t = dht.readTemperature();
    sendMessage(t);
    Serial.print("Temperatura = ");
    Serial.println(t);
    
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;    // 2-3 seconds
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}

void sendMessage(float t)
{
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(t);
  LoRa.endPacket();                     // finish packet and send it
  display.drawString(0, 0, "Enviando:");
  display.drawString(90, 0, String(t));
  display.display();
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
 
}
