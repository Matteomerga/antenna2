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
  while (!Serial) {}// Alcune schede richiedono di aspettare che la seriale sia disponibile


  // Verifica hardware
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}
  }

  radio.setPALevel(RF24_PA_MAX);
  radio.setPayloadSize(sizeof(payload));
  radio.setDataRate(RF24_250KBPS);
  radio.setAutoAck(false);

  // Configurazione degli indirizzi
  radio.openWritingPipe(address[radioNumber]);         // Pipe 0 per invio
  radio.openReadingPipe(1, address[!radioNumber]);     // Pipe 1 per ricezione

  radio.startListening(); // Modalità ricezione
}

void loop() {
  uint8_t pipe;

  if (radio.available(&pipe)) { //contorlla se il moduloNRF24 ha ricevuto quslcosa
    uint8_t bytes = radio.getPayloadSize();
    radio.read(&payload, bytes);

    // Invia i dati solo se il checksum è corretto
    if (payload.verifica = payload.velocita + payload.voltage + payload.current + (payload.micro % 10000)) {
      //Serial.write((uint8_t *)&payload, sizeof(payload)); //invia i dati al computer

          // Stampa i dati ricevuti in modo leggibile
      Serial.println("---- Dati Ricevuti ----");
      Serial.print("Velocità: ");
      Serial.println(payload.velocita);

      Serial.print("Voltaggio: ");
      Serial.println(payload.voltage);

      Serial.print("Corrente: ");
      Serial.println(payload.current);

      Serial.print("Latitudine (grezza): ");
      Serial.println(payload.lat);  // verrà stampato come unsigned long int

      Serial.print("Longitudine (grezza): ");
      Serial.println(payload.lng);

      Serial.print("Timestamp (us): ");
      Serial.println(payload.micro);

      Serial.print("Checksum (verifica): ");
      Serial.println(payload.verifica);

      Serial.println("------------------------\n");
    }
  }

  delayMicroseconds(500);
}
