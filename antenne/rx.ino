// RICEVITORE
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include "Arduino.h"

// Pin CE e CSN per il modulo RF24
#define CE_PIN 7
#define CSN_PIN 8

// Creazione oggetto radio
RF24 radio(CE_PIN, CSN_PIN);

// Indirizzi per la comunicazione tra i nodi
uint8_t address[][6] = {"1Node", "2Node"};
bool radioNumber = 0;

// Struct dati ricevuti (18 byte)
struct Mystruct {
  int velocita;
  int voltage;
  int current;
  unsigned long int lat;
  unsigned long int lng;
  unsigned long int micro;
  int verifica;
};
Mystruct payload;



void setup() {
  Serial.begin(500000);
  while (!Serial) {}


  // Verifica hardware
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}
  }

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
    if (payload.verifica = payload.velocita + payload.voltage + payload.current + (payload.micro % 10000)) 
    {
      //Serial.write((uint8_t *)&payload, sizeof(payload)); //invia i dati al computer
      stampa();
    }
  }

  delayMicroseconds(500);
}


void stampa()
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
