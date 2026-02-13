//TRASMETTITORE
#include <SPI.h>
#include "printf.h"
#include <RF24.h>
#include "Arduino.h"
#include <math.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

int block=0;

// Pin definitions
#define CE_PIN       6  // pin NRF24
#define CSN_PIN      7 
#define SD_CS_PIN    4  // Pin for SD module
#define ENCODER_PIN  3
#define TX_PIN       8
#define RX_PIN       9
const int voltPin = A2;
const int currPin = A1;

const int pacchetti_al_secondo = 10;
const unsigned long delta = 1000000/pacchetti_al_secondo;

float kv = 0.0533154;
float ki = - 0.073982;
double zeroCurr = 512.2;
float sumCurrent = 0;
float sumVoltage = 0;
int ncampioni = 0;


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

int i = 0;
unsigned long currentMicros = 0;
unsigned long previousMicros = 0;



int fileNumber = 1;
char fileName[15];


//stuct dei dati da inviare
struct Mystruct {
  int velocita;
  float voltage;
  float current;
  unsigned long int lat;
  unsigned long int lng;
  unsigned long int micro;
  int verifica;
};

Mystruct payload;
File dataFile; // SD file


void setup() {
  pinMode(currPin, INPUT);
  pinMode(voltPin, INPUT);
  pinMode(SD_CS_PIN, OUTPUT);
  pinMode(CSN_PIN, OUTPUT);
  pinMode(ENCODER_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), calcolaVelocita, RISING);
  digitalWrite(SD_CS_PIN, HIGH);  // Disable SD


  Serial.begin(115200);
  while (!Serial) {}


  // SD card initialization
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("Errore inizializzazione SD!"));
    while (block);
  }
  else Serial.println(F("Scheda SD inizializzata con successo!"));

  // Open SD file for writing
  digitalWrite(SD_CS_PIN, LOW);
  while (true) {
    sprintf(fileName, "%d.csv", fileNumber);

    if (!SD.exists(fileName)) {
      break;  // trovato numero libero
    }

    fileNumber++;
  }

  dataFile = SD.open(fileName, FILE_WRITE);
  digitalWrite(SD_CS_PIN, HIGH);
  if (!dataFile) {
    Serial.println(F("Errore apertura file SD!"));
    while (block);
  } 
  else Serial.println(F("File aperto per scrittura."));
  


  // RF24 radio initialization
  digitalWrite(CSN_PIN, HIGH);
  if (!radio.begin()) {
    Serial.println(F("Radio hardware non risponde!"));
    while (block);
  }
  else Serial.println(F("Radio hardware pronto a trasmettere"));

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
  payload.velocita = 1;
  payload.voltage = 0;
  payload.current = 0;
  payload.lat = 45123456;
  payload.lng = 9123456;
  payload.micro = 0;

  tara_zeroCurr();

}

void loop() {
  currentMicros = micros();

  if(currentMicros - previousMicros >= delta) {
    previousMicros = currentMicros;

    digitalWrite(SD_CS_PIN, HIGH);
    digitalWrite(CSN_PIN, LOW);

    if (currentMicros > (currentMicrov + tempoMaxFermo)) {
      velocitaCalcolata = 0;
      velocitaCalcolatam = 0;
    }

    payload.voltage = sumVoltage / ncampioni;
    payload.current = sumCurrent / ncampioni;
    payload.velocita = velocitaCalcolata;
    payload.micro = micros();
    payload.verifica = checksum();

    
    // Send data via NRF24
    bool ACK = radio.write(&payload, sizeof(payload));

    digitalWrite(CSN_PIN, HIGH);
    digitalWrite(SD_CS_PIN, LOW);
  

    char dataStr[80];  
    char vStr[10];
    char cStr[10];
    dtostrf(payload.voltage, 6, 2, vStr);
    dtostrf(payload.current, 6, 2, cStr);

    sprintf(dataStr, "%ld, %s, %s, %d, %ld, %ld\n",
      payload.micro,
      vStr,
      cStr,
      payload.velocita,
      payload.lat,
      payload.lng
    );

    dataFile.print(dataStr);




    if (i >= 10) {
      stampa1();
      Serial.println(F("10 pacchetti inviati"));
      i = 0;
      dataFile.flush();
    }
    i++;

    ncampioni = 0;
    sumVoltage = 0;
    sumCurrent = 0;
  }

  ncampioni++;
  sumVoltage = sumVoltage + ((analogRead(currPin) - zeroCurr) * ki);
  sumCurrent = sumCurrent + ((analogRead(voltPin)) * kv);





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




int checksum() {
  int micro4 = payload.micro % 10000;
  return payload.velocita + payload.voltage + payload.current;
}


void calcolaVelocita() {
  //Serial.println("scatto");
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

void tara_zeroCurr()
{
  for (int i = 0; i < 30000; i++) {
    zeroCurr = zeroCurr + analogRead(currPin) - 512;
    delay(1);
  }
  zeroCurr = zeroCurr / 30000;
  Serial.println(zeroCurr);
  float gdg = zeroCurr * ki;
  Serial.println(gdg);

}

void stampa1()
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

void stampa2()
{
  Serial.println(F("=== valori letti ==="));
  Serial.print(F("tensione raw:   ")); Serial.println(analogRead(voltPin));
  Serial.print(F("corrente raw:   ")); Serial.println(analogRead(currPin));
  Serial.print(F("voltage:        ")); Serial.println(payload.voltage);
  Serial.print(F("current:        ")); Serial.println(payload.current);
}

void stampa3()
{
  Serial.println(F("=== valori velocita ==="));
  Serial.print("Velocità: "); Serial.println(velocitaCalcolata);
  Serial.print(" - Media: "); Serial.println(velocitaCalcolatam);
}

