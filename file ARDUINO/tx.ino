// TRASMETTITORE
#include <SPI.h>
#include "printf.h"
#include <RF24.h>
#include "Arduino.h"
#include <math.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

int block = 0;

// Pin definitions
#define CE_PIN       8
#define CSN_PIN      9
#define SD_CS_PIN    10
#define ENCODER_PIN  3
#define TX_PIN       7
#define RX_PIN       6

const int voltPin = A2;
const int currPin = A1;
const uint8_t curr_motorPin = A3;

const int pacchetti_al_secondo = 2;
const unsigned long delta = 1000000 / pacchetti_al_secondo;

int v_offset = 1;
long int zeroCurr = 0;
long int zeroCurrMotor = 0;
long int sumCurrent_raw = 0;
long int sumVoltage_raw = 0;
long int sumCurrentMotor_raw = 0;
int ncampioni = 0;

char dataStr[80];
bool headerScritto = false;

// Radio setup
RF24 radio(CE_PIN, CSN_PIN);
uint8_t address[][6] = { "1Node", "2Node" };
bool radioNumber = 1;

// GPS setup
SoftwareSerial gpsSerial(TX_PIN, RX_PIN); // RX, TX
TinyGPSPlus gps;

// Dati per calcolo velocità
const int n_fori = 60;
float circonferenza = 2 * PI * 0.265;
// volatile unsigned long lastMicrov = 0;
// volatile unsigned long deltatv = 0;
// volatile unsigned long currentMicrov = 0;
// volatile unsigned long lastMicrovm = 0;
// volatile unsigned long deltatvm = 0;
// volatile float velocitaCalcolata = 0;
// volatile float velocitaCalcolatam = 0;
volatile int tick = 0;
const unsigned long tempoMaxFermo = 1000000UL;
int j = 0;
int n_media = 10;

int i = 0;
unsigned long currentMicros = 0;
unsigned long previousMicros = 0;

int fileNumber = 1;
char fileName[15];

// Struct dei dati da inviare
struct Mystruct {
  int velocita_gps;
  int voltage_raw;
  int current_raw;
  int currentMotor_raw;
  int tick_count;
  unsigned long int lat;
  unsigned long int lng;
  unsigned long int micro;
  int verifica;
};

Mystruct payload;
File dataFile; // SD file

void setup() {
  // Pin setup
  pinMode(currPin, INPUT);
  pinMode(voltPin, INPUT);
  pinMode(curr_motorPin, INPUT);
  pinMode(SD_CS_PIN, OUTPUT);
  pinMode(CSN_PIN, OUTPUT);
  pinMode(ENCODER_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), contaTick, RISING);

  Serial.begin(115200);
  while (!Serial) {}

  // SD card initialization
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("Errore inizializzazione SD!"));
    while (block);
  } else Serial.println(F("Scheda SD inizializzata con successo!"));


  // Open SD file for writing
  while (true) {
    sprintf(fileName, "%d.csv", fileNumber);
    if (!SD.exists(fileName)) break;
    fileNumber++;
  }

  dataFile = SD.open(fileName, FILE_WRITE);
  if (!dataFile) {
    Serial.println(F("Errore apertura file SD!"));
    while (block);
  } else Serial.println(F("File aperto per scrittura."));


  // RF24 radio initialization
  if (!radio.begin()) {
    Serial.println(F("Radio hardware non risponde!"));
    while (block);
  } else Serial.println(F("Radio hardware pronto a trasmettere"));


  radio.setPALevel(RF24_PA_MAX);
  radio.setPayloadSize(sizeof(payload));
  radio.openWritingPipe(address[radioNumber]);
  radio.openReadingPipe(1, address[!radioNumber]);
  radio.stopListening();
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);

  // GPS setup
  gpsSerial.begin(9600);
  Serial.println("In attesa del fix GPS...");

  // Initial payload setup
  payload.velocita_gps = 1;
  payload.voltage_raw = 0;
  payload.current_raw = 0;
  payload.currentMotor_raw = 0;
  payload.tick_count = 0;
  payload.lat = 45123456;
  payload.lng = 9123456;
  payload.micro = 0;

  tara_zeroCurr();

  Serial.println("Ready for the loop!");
}

void loop() {
  currentMicros = micros();

  if (currentMicros - previousMicros >= delta) {
    previousMicros = currentMicros;

    payload.voltage_raw      = (int)round((float)sumVoltage_raw      / ncampioni);
    payload.current_raw      = (int)round((float)sumCurrent_raw      / ncampioni);
    payload.currentMotor_raw = (int)round((float)sumCurrentMotor_raw / ncampioni);
    payload.tick_count       = tick;
    payload.micro            = micros();
    payload.verifica         = checksum();

    radio.write(&payload, sizeof(payload));
    tick = 0;

    sprintf(dataStr, "%ld, %d, %d, %d, %d, %d, %ld, %ld\n",
      payload.micro,
      payload.voltage_raw,
      payload.current_raw,
      payload.currentMotor_raw,
      payload.tick_count,
      payload.velocita_gps
      payload.lat,
      payload.lng
    );

    dataFile.print(dataStr);

    if (i >= pacchetti_al_secondo) {
      stampa1();
      Serial.print(pacchetti_al_secondo);
      Serial.println(" pacchetti inviati");
      i = 0;
      dataFile.flush();
    }
    i++;

    ncampioni = 0;
    sumVoltage_raw = 0;
    sumCurrent_raw = 0;
    sumCurrentMotor_raw = 0;
  }

  ncampioni++;
  sumVoltage_raw      += (analogRead(voltPin) * 10 - v_offset);
  sumCurrent_raw      += (analogRead(currPin) * 10 - zeroCurr);
  sumCurrentMotor_raw += (analogRead(curr_motorPin) * 10 - zeroCurrMotor);


  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    gps.encode(c);

    if (gps.location.isUpdated()) {
      payload.lat = gps.location.lat() * 1e6;
      payload.lng = gps.location.lng() * 1e6;
      payload.velocita_gps = (int)round(gps.speed.kmph() * 100);

      if (!headerScritto) scriviHeader();
    }
  }
}

int checksum() {
  int micro4 = payload.micro % 10000;
  return payload.tick_count + payload.voltage_raw + payload.current_raw + micro4;
}

// void calcolaVelocita() {
//   currentMicrov = micros();
//   deltatv = currentMicrov - lastMicrov;
//   lastMicrov = currentMicrov;
//   velocitaCalcolata = ((circonferenza / (float)n_fori) / (float)deltatv) * 1e6;

//   j++;
//   if (j == n_media) {
//     j = 0;
//     deltatvm = currentMicrov - lastMicrovm;
//     lastMicrovm = currentMicrov;
//     velocitaCalcolatam = (circonferenza / n_fori) / deltatvm * 1e6 * n_media;
//   }
// }

// void calcolaFrequenza() {
//   currentMicrov = micros();
//   deltatv = currentMicrov - lastMicrov;
//   lastMicrov = currentMicrov;
//   velocitaCalcolata = (1 / ((float)deltatv / 1e6));

//   j++;
//   if (j == n_media) {
//     j = 0;
//     deltatvm = currentMicrov - lastMicrovm;
//     lastMicrovm = currentMicrov;
//     velocitaCalcolatam = (1 / ((float)deltatvm / 1e6 / (float)n_media));
//   }
// }

void contaTick(){
  tick++;
}

void tara_zeroCurr() {
  for (int i = 0; i < 3000; i++) {
    zeroCurr += analogRead(currPin);
    delay(1);
    zeroCurrMotor += analogRead(curr_motorPin);
    delay(1);
  }
  zeroCurr = zeroCurr / 3000 * 10;
  zeroCurrMotor = zeroCurrMotor / 3000 * 10;
}

void scriviHeader() {
  char header[40];
  sprintf(header, "DATA: %04d-%02d-%02d %02d:%02d:%02d,0,0,0,0,0,0\n",
    gps.date.year(),
    gps.date.month(),
    gps.date.day(),
    gps.time.hour(),
    gps.time.minute(),
    gps.time.second()
  );
  dataFile.print(header);
  headerScritto = true;
}

void stampa1() {
  Serial.println(F("=== Pacchetto inviato ==="));
  Serial.print(F("velocita GPS:   ")); Serial.println(payload.velocita_gps, 6);
  Serial.print(F("voltage:        ")); Serial.println(payload.voltage_raw);
  Serial.print(F("current:        ")); Serial.println(payload.current_raw);
  Serial.print(F("motorCurrent:   ")); Serial.println(payload.currentMotor_raw);
  Serial.print(F("tick_count:   ")); Serial.println(payload.tick_count);
  Serial.print(F("lat (raw):      ")); Serial.println(payload.lat);
  Serial.print(F("lng (raw):      ")); Serial.println(payload.lng);
  Serial.print(F("micro:          ")); Serial.println(payload.micro);
  Serial.print(F("verifica:          ")); Serial.println(payload.verifica);
  Serial.print(F("zerocurrmotor:          ")); Serial.println(zeroCurrMotor);
}

void stampa2() {
  Serial.println(F("=== valori letti ==="));
  Serial.print(F("tensione raw:       ")); Serial.println(analogRead(voltPin));
  Serial.print(F("corrente raw:       ")); Serial.println(analogRead(currPin));
  Serial.print(F("correntemotore raw: ")); Serial.println(analogRead(curr_motorPin));
  Serial.print(F("voltage:            ")); Serial.println(payload.voltage_raw);
  Serial.print(F("current:            ")); Serial.println(payload.current_raw);
}

// void stampa3() {
//   Serial.println(F("=== valori velocita ==="));
//   Serial.print(F("Velocità: ")); Serial.println(velocitaCalcolata);
//   Serial.print(F(" - Media: ")); Serial.println(velocitaCalcolatam);
//   Serial.print(F(" - GPS:   ")); Serial.println(payload.velocita_gps);
// }
