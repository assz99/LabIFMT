#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SSD1306.h>

const char *ssid     = "Cleunice";
const char *password = "28467591";

#define BAND    915E6  //é possivel setar a frequencia de 433MHz(433E6), 868Mhz(868E6),915Mhz(915E6)
#define PABOOST true
String mensagem;
 
byte localAddress = 0x02;     // Endereco deste dispositivo LoRa

byte destination = 0x01;      // Endereco do dispositivo para enviar a mensagem (0xFF envia para todos devices)
long lastSendTime = 0;        // TimeStamp da ultima mensagem enviada
int interval = 5000;          // Intervalo em ms no envio das mensagens (inicial 5s)
String ip = "192.168.1.5";

//Variável para controlar o display
SSD1306 display(0x3c, 4, 15,16);

HTTPClient http;

void setupDisplay(){
  //O estado do GPIO16 é utilizado para controlar o display OLED
  pinMode(16, OUTPUT);
  //Reseta as configurações do display OLED
  digitalWrite(16, LOW);
  //Para o OLED permanecer ligado, o GPIO16 deve permanecer HIGH
  //Deve estar em HIGH antes de chamar o display.init() e fazer as demais configurações,
  //não inverta a ordem
  digitalWrite(16, HIGH);

  //Configurações do display
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
}

void setup() {
  setupDisplay();
  Serial.begin(9600);
  LoRa.setPins(18,14,26);
  if (!LoRa.begin(BAND,PABOOST))
  {             // tenta inicializar a Rede LoRa
    Serial.println("LoRa nao conseguiu inicializar..");
    display.drawString(0, 16, "LoRa nao conseguiu inicializar..");
    display.display();
    while (true);                       
  }

      Serial.println("LoRa inicializado com sucesso");  
      Serial.println("Aguardando conexão");
      display.drawString(0, 0, "LoRa inicializado com sucesso.");
      display.drawString(0, 16, "Aguardando conexão...");;
      display.display();
      // Tenta conexão com Wi-fi
      WiFi.begin(ssid, password);
      while ( WiFi.status() != WL_CONNECTED ) {
              delay(100);
              Serial.print(".");
      }
      Serial.print("\nWI-FI conectado com sucesso: ");
      display.drawString(0, 32, "WI-FI conectado com sucesso:");;
      display.display();
}

void loop(){
      delay(100);
      String cmd = "http://"+ip+"/app/comando.php?cmd=";
      String cmd_upd = "http://"+ip+"/app/comando.php?update";
      //cmd += String(incoming);
      http.begin(cmd);
      int httpCode = http.GET();
      if (httpCode > 0) { 
              String payload = http.getString();
              char pay[100];
              payload.toCharArray(pay, 100);
              char * InfoPay[2];
              InfoPay[0] = strtok (pay,"?");
              InfoPay[1] = strtok (NULL,"?");
              if(payload != "0"){
                Serial.println(InfoPay[0]);
                Serial.println(InfoPay[1]);
                char input[10];
                String de = InfoPay[0];
                de.toCharArray(input, 10);
                int destin;
                char *endptr;
                destin = strtol(input, &endptr, 16);
                delay(100);
                Enviar_CMD(String(InfoPay[1]), String(destin));
                delay(100);
                http.begin(cmd_upd);
                http.GET();
                delay(100);
              }
      }
      http.end();
    delay(100);
    onReceive(LoRa.parsePacket());
}

void Enviar_CMD(String cmd, String dest) 
{
  String desti = String(dest.toInt(), HEX);
  byte dest_byte = (byte) desti.toInt();
  LoRa.beginPacket();                   // Inicia o pacote da mensagem
  LoRa.write(dest_byte);                // Adiciona o endereco de destino
  LoRa.write(localAddress);             // Adiciona o endereco do remetente
  LoRa.write(cmd.length());             // Tamanho da mensagem em bytes
  LoRa.print(cmd);                      // Vetor da mensagem 
  LoRa.endPacket();                     // Finaliza o pacote e envia
}
 
// Funcao para receber mensagem 
void onReceive(int packetSize) 
{
  if (packetSize == 0) return;          // Se nenhuma mesnagem foi recebida, retorna nada
 
  // Leu um pacote, vamos decodificar? 
  int recipient = LoRa.read();          // Endereco de quem ta recebendo
  byte sender = LoRa.read();            // Endereco do remetente
  byte incomingLength = LoRa.read();    // Tamanho da mensagem
 
  String incoming = "";
  while (LoRa.available())
  {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) 
  {   
    Serial.println("erro!: o tamanho da mensagem nao condiz com o conteudo!");
    return;                        
  }
  if (recipient != localAddress && recipient != destination)
  {
    Serial.println("Esta mensagem não é para mim.");
    return;
  }
 
  // Caso a mensagem seja para este dispositivo, imprime os detalhes
  Serial.println("Recebido do dispositivo: 0x" + String(sender, HEX));
  Serial.println("Enviado para: 0x" + String(recipient, HEX));
  Serial.println("Tamanho da mensagem: " + String(incomingLength));
  Serial.println("Mensagem: " + incoming);
  Serial.println();

      String endWeb = "http://"+ip+"/app/alterar.php?string=";
      endWeb += String(incoming);
      http.begin(endWeb);
      int httpCode = http.GET();
      http.end(); //Free the resources
}
