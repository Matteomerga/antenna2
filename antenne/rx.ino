// RICEVITORE
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include "Arduino.h"

int stampa = 0;

// Pin CE e CSN per il modulo RF24
#define CE_PIN 10
#define CSN_PIN 9
#define led 8

int i=0;

// Creazione oggetto radio
RF24 radio(CE_PIN, CSN_PIN);

// Indirizzi per la comunicazione tra i nodi
uint8_t address[][6] = {"1Node", "2Node"};
bool radioNumber = 0;

// Struct dati ricevuti
struct Mystruct {
  float velocita;
  float voltage;
  float current;
  unsigned long int lat;
  unsigned long int lng;
  unsigned long int micro;
  int verifica;
};
Mystruct payload;



void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  pinMode(led, OUTPUT);


  // Verifica hardware
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}
  }
  if(stampa) Serial.println(F("pronto!!"));
  
  //configurazione antenna
  radio.setPALevel(RF24_PA_MAX);
  radio.setPayloadSize(sizeof(payload));
  radio.setDataRate(RF24_250KBPS);
  radio.setAutoAck(false);
  radio.openWritingPipe(address[radioNumber]);         // Pipe 0 per invio
  radio.openReadingPipe(1, address[!radioNumber]);     // Pipe 1 per ricezione
  radio.startListening(); // Modalità ricezione
}

void loop() {
  uint8_t pipe;

  if (radio.available(&pipe)) //contorlla se il moduloNRF24 ha ricevuto quslcosa
  { 
    uint8_t bytes = radio.getPayloadSize();
    radio.read(&payload, bytes);

    // Invia i dati solo se il checksum è corretto
    if (payload.verifica == payload.velocita + payload.voltage + payload.current + (payload.micro % 10000)) 
    {
      digitalWrite(led, HIGH);
      if(stampa) stampa_dati(); 
      else Serial.write((uint8_t *)&payload, sizeof(payload)); //invia i dati al computer
      digitalWrite(led, LOW);


    }
  }
  delayMicroseconds(500);
 
}


void stampa_dati()
{
    Serial.println(F("=== Pacchetto ricevuto ==="));
    Serial.print(F("raw verifica:   ")); Serial.println(payload.verifica);
    Serial.print(F("velocita:       ")); Serial.println(payload.velocita, 6);
    Serial.print(F("voltage:        ")); Serial.println(payload.voltage);
    Serial.print(F("current:        ")); Serial.println(payload.current);
    Serial.print(F("lat (raw):      ")); Serial.println(payload.lat);
    Serial.print(F("lng (raw):      ")); Serial.println(payload.lng);
    Serial.print(F("micro:          ")); Serial.println(payload.micro);
}
