//TRASMETTITORE
#include <SPI.h>
#include "printf.h"
#include <RF24.h>
#include "Arduino.h"
#include <math.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// Pin definitions
#define CE_PIN       6  // pin NRF24
#define CSN_PIN      7  
#define SD_CS_PIN    4  // Pin for SD module
#define ENCODER_PIN  2
#define TX_PIN       8
#define RX_PIN       9

const unsigned long delta = 1000000;

// Radio setup
RF24 radio(CE_PIN, CSN_PIN);
uint8_t address[][6] = { "1Node", "2Node" };
bool radioNumber = 1;

//Gps setup
SoftwareSerial gpsSerial(TX_PIN, RX_PIN); // RX, TX
TinyGPSPlus gps;


// dati per calcolo velocita
int n_fori = 60;
float circonferenza = 2 * PI * 0.283;
volatile unsigned long lastMicrov = 0;
volatile unsigned long deltatv = 0;
volatile unsigned long currentMicrov = 0;
volatile unsigned long lastMicrovm = 0;
volatile unsigned long deltatvm = 0;
volatile int velocitaCalcolata = 0;
volatile int velocitaCalcolatam = 0;
int j=0;
int n_media=10;

// simulazione dati
float angolo = 0;
unsigned long previousMicros = 0;
unsigned long A;


int i = 0;
File dataFile; // SD file

//stuct dei dati da inviare
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
  pinMode(SD_CS_PIN, OUTPUT);
  pinMode(CSN_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);  // Disable SD for now


  Serial.begin(500000);
  while (!Serial) {}


// SD card initialization
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("Errore inizializzazione SD!"));
    while (1);
  }
  Serial.println(F("Scheda SD inizializzata con successo!"));

  // Open SD file for writing
  digitalWrite(SD_CS_PIN, LOW);
  dataFile = SD.open("datalog.txt", FILE_WRITE);
  digitalWrite(SD_CS_PIN, HIGH);


  if (!dataFile) {
    Serial.println(F("Errore apertura file SD!"));
    while (1);
  } else {
    Serial.println(F("File aperto per scrittura."));
  }


  // RF24 radio initialization
  digitalWrite(CSN_PIN, HIGH);
  if (!radio.begin()) {
    Serial.println(F("Radio hardware non risponde!"));
    while (1);
  }
  Serial.println(F("Radio hardware pronto a trasmettere"));
  

  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), calcolaVelocita, RISING);

  radio.setPALevel(RF24_PA_MAX);
  radio.setPayloadSize(sizeof(payload));
  radio.openWritingPipe(address[radioNumber]);
  radio.openReadingPipe(1, address[!radioNumber]);
  radio.stopListening();
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);

  //Gps setup
  gpsSerial.begin(9600);
  Serial.println("In attesa del fix GPS...");

  // Initial payload setup
  payload.velocita = 0;
  payload.voltage = 0;
  payload.current = 0;
  payload.lat = 0;
  payload.lng = 0;
  payload.micro = 0;

}

void loop() {
  unsigned long currentMicros = micros();

  if (currentMicros - previousMicros >= delta) {
    previousMicros = currentMicros;

    //GPS read and write
  while (gpsSerial.available()) {
      char c = gpsSerial.read();
      //Serial.print(c);  // Stampa i dati grezzi per vedere cosa riceve
      gps.encode(c);

    // Quando arriva un nuovo fix:
   if (gps.location.isUpdated()) {
      Serial.print("Latitudine: ");
      payload.lat = gps.location.lat() * 1e6;
      payload.lng = gps.location.lng() * 1e6;

    }
  }
  
    if (angolo > 360) angolo = 0;
    float rad = angolo * PI / 180.0;
    angolo += 0.1;
    float seno_val = sin(rad);

    digitalWrite(SD_CS_PIN, HIGH);
    digitalWrite(CSN_PIN, LOW);

    payload.voltage = seno_val * 500 + 500;
    payload.velocita = velocitaCalcolata;
    payload.micro = micros();
    payload.verifica = checksum(payload.velocita, payload.voltage, payload.current, payload.micro);

    
    // Send data via NRF24
    bool ACK = radio.write(&payload, sizeof(payload));
    digitalWrite(CSN_PIN, HIGH);

    // Write data to SD card
    digitalWrite(SD_CS_PIN, LOW);
  

char dataStr[80];


sprintf(dataStr,
        "%d, %d, %d, %ld, %ld, %ld\n",
        payload.velocita,   // int  -> %d
        payload.voltage,    // int  -> %d
        payload.current,    // int  -> %d
        payload.lat,        // long -> %ld
        payload.lng,        // long -> %ld
        payload.micro       // long -> %ld
);

dataFile.print(dataStr);

    i++;
    if (i > 1000) {
      unsigned long B = millis();
      unsigned long C = B - A;
      Serial.println(C);
      Serial.println(F("1000 pacchetti inviati"));
      Serial.println(payload.lat);
      Serial.println(payload.lng);
      i = 0;
      A = millis();
      dataFile.flush();
    }

  }
  delayMicroseconds(100);
}

int checksum(int velocita, int voltage, int current, unsigned long int micro) {
  int micro4 = micro % 10000;
  return velocita + voltage + micro4;
}


void calcolaVelocita() {
  currentMicrov = micros();
  deltatv = currentMicrov - lastMicrov;
  lastMicrov = currentMicrov; 

  velocitaCalcolata = (circonferenza / n_fori) / deltatv * 1e6;
  
  j++;
  if(j==n_media)
  {
    j=0;
    deltatvm = currentMicrov - lastMicrovm;
    lastMicrovm = currentMicrov; 

    velocitaCalcolatam = (circonferenza / n_fori) / deltatv * 1e6 * n_media;
  }
}
