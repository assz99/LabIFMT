#include <SPI.h>
#include <LoRa.h>
#include "DHT.h"
#include <Wire.h>  
#include "SSD1306.h" 
#include "EmonLib.h"

#define BAND    915E6  //Define a frequencia do LoRa 868E6,915E6
#define PABOOST true

String S_localAddress = "0x01"; // Endereço deste dispositivo
byte localAddress = 0xBB;       // Endereço deste dispositivo
byte destination = 0xFD;        // Endereço do dispositivo enviado
int rede = 124;
long ultimoenvio = 0;           // tempo do ultimo envio
int intervalo = 5000;           // tempo para ele começar fazer as medições(5 segundos) .
int trava = 1;
int ultimaleitura;
double corrente;
double potencia;
double KWH;

DHT dht(2, DHT22);
EnergyMonitor emon1;
SSD1306  display(0x3c, 4, 15, 16);
void setup(){
Serial.begin(115200);                   // Inicializa o Monitor Serial
while (!Serial);
dht.begin();                            // Inicializa o sensor DHT
emon1.current(34, 15);                  // Inicializa o sensor de SCT-013
Serial.println("LoRa IFMT");            
SPI.begin(5,19,27,18);                  
LoRa.setPins(18 ,14,26);                // Inicializa o LoRa
display.init();                         // Inicializa o OLED
display.flipScreenVertically();  
display.setFont(ArialMT_Plain_10);
if (!LoRa.begin(BAND,PABOOST)){             // Coloca a frequencia do LoRa como 915 MHz
  Serial.println("LoRa nao incializado");
  display.drawString(0, 0, "LoRa nao inicializado");
  display.display();
  while (true);             //Função para caso aja falha em inciar o LoRa
  }

Serial.println("LoRa inicializado com sucesso!");
display.drawString(0, 0, "LoRa inicializado com sucesso!");
display.display();
delay(1000);
}

void loop(){
if (millis() - ultimoenvio > intervalo){   //Ele sempre tentara enviar o codigo de 1 em 1 segundo
  float t = dht.readTemperature();           // Le a Temperatura
  Serial.print("Temperatura = ");
  Serial.println(t);
  float h = dht.readHumidity();              //Le a Humidade
  Serial.print("Humidade = ");
  Serial.println(h);
  double Irms = emon1.calcIrms(1480);       //Le a corrente
  if(Irms >= 0 and Irms <=0.2){             //O sensor nao consegue se manter 0 quando não esta passando corrente entao anulamos o valor que ele lê
    Irms = 0;
    trava = 1;
  Serial.println(Irms);
  }else{
    corrente = (Irms + corrente)/2;
    potencia = rede * corrente;
    if(trava = 1){
      int startmillis=millis();
      ultimaleitura = startmillis;
      trava = 0;
      }
    KWH = potencia * (((millis() - ultimaleitura)/1000)/3600)/1000;
    }
  Serial.print("Corrente : ");
  Serial .println(Irms); // Irms
  ultimoenvio = millis();             
  intervalo = random(1000) + 1000;  
  display.drawString(0, 0, "Corrente = "+String(corrente)+" A");
  display.drawString(0, 16, "Potencia = "+String(potencia)+" W" );
  display.drawString(0, 32, "KWH = "+String(KWH, 2));
  display.drawString(0, 48, "Temp= "+String(t,1)+"C");
  display.drawString(64, 48,"Humid= "+String(h,1)+"%");  
  display.display();
  String info = S_localAddress+"x0x"+String(t)+"x0x"+String(h)+"x0x"+String(corrente)+"x0x"+String(potencia)+"x0x"+String(KWH, 2);
  sendMessage(info);//funçao para enviar os valores
  }
onReceive(LoRa.parsePacket());//Callback para receber informações via LoRa
}

void sendMessage(String mes)
{
  LoRa.beginPacket();                   // Abre o pacore
  LoRa.write(destination);              // Adiciona o endereco de destino
  LoRa.write(localAddress);             // Adiciona o endereco do remetente
  LoRa.write(mes.length());             // Tamanho da mensagem em bytes
  LoRa.print(mes);                      // Mensagem
  LoRa.endPacket();                     // Fecha o Pacote
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;          // caso o pacote nao exista ele sai da função
  // read packet header bytes:
  int recipient = LoRa.read();          // Endereço do destino
  byte sender = LoRa.read();            // Endereço do remetente
  int comando = LoRa.read();

  // caso a mensagem nao seja para esse dispositivo ele fecha o pacote
  if (recipient != localAddress) {
    Serial.println("Esta mensagem nao e para mim.");
    return;                             // 
  }

  // Caso seja para esse dispositivo ele printa e faz o comando:
  Serial.println("Recebido de: 0x" + String(sender, HEX));
  Serial.println("Enviado para: 0x" + String(recipient, HEX));
 
}
