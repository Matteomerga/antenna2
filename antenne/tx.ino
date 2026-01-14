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
#define ENCODER_PIN  3
#define TX_PIN       8
#define RX_PIN       9
const int voltPin = A2;
const int currPin = A1;

const unsigned long delta = 4000;



float kv = 0.004605735;
float ki = 0.004665127;
unsigned long zeroCurr = 0;

// Radio setup
RF24 radio(CE_PIN, CSN_PIN);
uint8_t address[][6] = { "1Node", "2Node" };
bool radioNumber = 1;

//Gps setup
SoftwareSerial gpsSerial(TX_PIN, RX_PIN); // RX, TX
TinyGPSPlus gps;


// dati per calcolo velocita
const int n_fori = 60;
float circonferenza = 2 * PI * 0.265;
volatile unsigned long lastMicrov = 0;
volatile unsigned long deltatv = 0;
volatile unsigned long currentMicrov = 0;
volatile unsigned long lastMicrovm = 0;
volatile unsigned long deltatvm = 0;
volatile float velocitaCalcolata = 0;
volatile float velocitaCalcolatam = 0;
const unsigned long tempoMaxFermo = 1000000UL;
int j=0;
int n_media=10;


unsigned long currentMicros = 0;
unsigned long previousMicros = 0;



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
  pinMode(currPin, INPUT);
  pinMode(voltPin, INPUT);
  pinMode(SD_CS_PIN, OUTPUT);
  pinMode(CSN_PIN, OUTPUT);
  pinMode(ENCODER_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), calcolaVelocita, RISING);
  digitalWrite(SD_CS_PIN, HIGH);  // Disable SD


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
  dataFile = SD.open("datalog.csv", FILE_WRITE);
  digitalWrite(SD_CS_PIN, HIGH);
  if (!dataFile) {
    Serial.println(F("Errore apertura file SD!"));
   // while (0);
  } else {
    Serial.println(F("File aperto per scrittura."));
  }


  // RF24 radio initialization
  digitalWrite(CSN_PIN, HIGH);
  if (!radio.begin()) {
    Serial.println(F("Radio hardware non risponde!"));
    while (0);
  }
  Serial.println(F("Radio hardware pronto a trasmettere"));
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
  payload.lat = 45123456;
  payload.lng = 9123456;
  payload.micro = 0;

  for (int i = 0; i < 100; i++) {
    zeroCurr = zeroCurr + analogRead(currPin);
    delay(1);
  }
  zeroCurr = zeroCurr / 100;

}

void loop() {
  currentMicros = micros();

  if (currentMicros - previousMicros >= delta) {
    previousMicros = currentMicros;

    digitalWrite(SD_CS_PIN, HIGH);
    digitalWrite(CSN_PIN, LOW);

    payload.voltage = (analogRead(voltPin) - 1387) * kv;
    payload.current = (analogRead(currPin) - zeroCurr) * ki;
    payload.velocita = velocitaCalcolata;
    payload.micro = micros();
    payload.verifica = checksum(payload.velocita, payload.voltage, payload.current, payload.micro);

    
    // Send data via NRF24
    bool ACK = radio.write(&payload, sizeof(payload));

    digitalWrite(CSN_PIN, HIGH);
    digitalWrite(SD_CS_PIN, LOW);
  

    char dataStr[80];
    sprintf(dataStr,
      "%ld, %d, %d, %f, %ld, %ld\n",
      payload.micro,       // long  -> %ld
      payload.voltage,     // int   -> %d
      payload.current,     // int   -> %d
      payload.velocita,    // float -> %f
      payload.lat,         // long  -> %ld
      payload.lng          // long  -> %ld

    );

    dataFile.print(dataStr);


    if (currentMicros > (currentMicrov + tempoMaxFermo)) {
    velocitaCalcolata = 0;
    velocitaCalcolatam = 0;
    }


    if (i >= 300) {
      stampa();
      unsigned long B = millis();
      unsigned long C = B - A;
      Serial.println(C);
      Serial.println(F("300 pacchetti inviati"));
      i = 0;
      unsigned long A = millis();
      dataFile.flush();
    }
    i++;

  }



  while (gpsSerial.available()) {
      char c = gpsSerial.read();
      //Serial.print(c);  // Stampa i dati grezzi per vedere cosa riceve
      gps.encode(c);

    if (gps.location.isUpdated()) {
      payload.lat = gps.location.lat() * 1e6;
      payload.lng = gps.location.lng() * 1e6;  
    }
  }
}


void stampaVelocita()
{
  Serial.print("Velocità: ");
  Serial.print(velocitaCalcolata);
  Serial.print(" - Media: ");
  Serial.println(velocitaCalcolatam);
}


int checksum(int velocita, int voltage, int current, unsigned long int micro) {
  int micro4 = micro % 10000;
  return velocita + voltage + micro4;
}

void stampa()
{
    Serial.println(F("=== Pacchetto inviato ==="));
    Serial.print(F("raw verifica:   ")); Serial.println(payload.verifica);
    Serial.print(F("velocita:       ")); Serial.println(payload.velocita, 6);
    Serial.print(F("voltage:        ")); Serial.println(payload.voltage);
    Serial.print(F("current:        ")); Serial.println(payload.current);
    Serial.print(F("lat (raw):      ")); Serial.println(payload.lat);
    Serial.print(F("lng (raw):      ")); Serial.println(payload.lng);
    Serial.print(F("micro:          ")); Serial.println(payload.micro);
}

void calcolaVelocita() {
  currentMicrov = micros();
  deltatv = currentMicrov - lastMicrov;
  lastMicrov = currentMicrov; 
  velocitaCalcolata = ((circonferenza / (float)n_fori) / (float)deltatv) * 1e6;
  
  
  j++;
  if(j==n_media)
  {
    j=0;
    deltatvm = currentMicrov - lastMicrovm;
    lastMicrovm = currentMicrov; 

    velocitaCalcolatam = (circonferenza / n_fori) / deltatvm * 1e6 * n_media;
  }
}
