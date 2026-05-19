// TRASMETTITORE
#include <SPI.h>
#include "printf.h"
#include <RF24.h>
#include "Arduino.h"
#include <math.h>
#include <SD.h>
#include <SoftwareSerial.h>


enum WaypointType {
    STOP_ACCELERATION = 0,
    START_ACCELERATION = 1
};

struct Waypoint {
    unsigned long int latitude;  // Coordinata moltiplicata per 10^7 (es. 45.1234567 -> 451234567)
    unsigned long int longitude; // Coordinata moltiplicata per 10^7
    WaypointType type; // START_ACCELERATION o STOP_ACCELERATION
};




int block = 0;
const int pacchetti_al_secondo = 2;


Waypoint waypoints[] = {
  {455051638,  91658194,  START_ACCELERATION},
  {455048417,  91659417,  STOP_ACCELERATION}, 
  {455045528,  91660528,  START_ACCELERATION}, 
  {455044750,  91660889,  STOP_ACCELERATION}
  
};

const float ACCEPTANCE_RADIUS = 3.0;
const int LOOK_AHEAD_WINDOW = 3;














// Pin definitions
#define CE_PIN       8
#define CSN_PIN      9
#define SD_CS_PIN    10
#define TX_PIN       255
#define RX_PIN       6
#define SIGNAL_PIN   5

const int voltPin = A2;
const int currPin = A1;
const int curr_motorPin = A3;

const unsigned long delta = 1000000 / pacchetti_al_secondo;

const int NUM_WAYPOINTS = sizeof(waypoints) / sizeof(waypoints[0]);
bool acceleration_status = 0;
float distance = 0;
int waypointAttuale = 0;

int v_offset = 1;
long int zeroCurr = 0;
long int zeroCurrMotor = 0;
long int sumCurrent_raw = 0;
long int sumVoltage_raw = 0;
long int sumCurrentMotor_raw = 0;
int ncampioni = 0;

char dataStr[120];

// Radio setup
RF24 radio(CE_PIN, CSN_PIN);
uint8_t address[][6] = { "1Node", "2Node" };
bool radioNumber = 1;

// FC setup
SoftwareSerial FCSerial(RX_PIN, TX_PIN); // RX, TX
int heading_bussola = 0;
int acc_Y = 0;
int acc_X = 0;

int i = 0;
unsigned long currentMicros = 0;
unsigned long previousMicros = 0;

int fileNumber = 1;
char fileName[15];

// Struct dei dati da inviare
struct Mystruct { //massimo 32 byte!!!!!!!!
  int velocita_fc;
  int voltage_raw;
  int current_raw;
  int currentMotor_raw;
  unsigned long int lat;
  unsigned long int lng;
  unsigned long int micro;
  int verifica;
};

Mystruct payload;
File dataFile;


#define NANO_SYNC_1 0xA5
#define NANO_SYNC_2 0x5A
#define NANO_PAYLOAD_LEN 16

enum RxState {
  WAIT_SYNC_1,
  WAIT_SYNC_2,
  RECEIVING_PAYLOAD,
  WAIT_CHECKSUM
};

RxState currentState = WAIT_SYNC_1;
uint8_t payloadIdx = 0;

struct __attribute__((__packed__)) NanoData {
  int32_t lat;
  int32_t lng;
  int16_t speed_cm_s;
  int16_t acc_X_cm_s2;
  int16_t acc_Y_cm_s2;
  uint16_t heading_decideg;
};

union NanoPacket {
  NanoData data;
  uint8_t bytes[NANO_PAYLOAD_LEN];
};

NanoPacket nanoPacket;




void setup() {
  // Pin setup
  pinMode(currPin, INPUT);
  pinMode(voltPin, INPUT);
  pinMode(curr_motorPin, INPUT);
  pinMode(SD_CS_PIN, OUTPUT);
  pinMode(CSN_PIN, OUTPUT);
  pinMode(SIGNAL_PIN, OUTPUT);
  digitalWrite(SIGNAL_PIN, LOW); 


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

  // FC setup
  FCSerial.begin(38400);

  // Initial payload setup
  payload.velocita_fc = 0;
  payload.voltage_raw = 0;
  payload.current_raw = 0;
  payload.currentMotor_raw = 0;
  payload.lat = 45123456;
  payload.lng = 9123456;
  payload.micro = 0;

  tara_zeroCurr();

  Serial.println(F("System Ready!"));
}

void loop() {
  currentMicros = micros();

  if (currentMicros - previousMicros >= delta) {
    previousMicros = currentMicros;
    

    manda_dati_antenna();
    salva_dati_sd();

    if (i >= pacchetti_al_secondo) {
      stampa4();
      Serial.print(pacchetti_al_secondo);
      Serial.println(F(" pacchetti inviati"));
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

  leggi_seriale();
  check_waypoint();
}


void manda_dati_antenna() {
  payload.voltage_raw      = (int)round((float)sumVoltage_raw      / ncampioni);
  payload.current_raw      = (int)round((float)sumCurrent_raw      / ncampioni);
  payload.currentMotor_raw = (int)round((float)sumCurrentMotor_raw / ncampioni);
  payload.micro            = micros();
  payload.verifica         = checksum();

  radio.write(&payload, sizeof(payload));
}


void salva_dati_sd() {
  sprintf(dataStr, "%ld, %d, %d, %d, %d, %ld, %ld, %d, %d, %d, %d\n",
  payload.micro,
  payload.voltage_raw,
  payload.current_raw,
  payload.currentMotor_raw,
  payload.velocita_fc,
  payload.lat,
  payload.lng,
  acc_X,
  acc_Y,
  heading_bussola,
  acceleration_status? 1 : 0
  );

  dataFile.print(dataStr);
}


void leggi_seriale() {
  while (FCSerial.available() > 0) {
    uint8_t c = FCSerial.read();

    switch (currentState) {
      case WAIT_SYNC_1:
        if (c == NANO_SYNC_1) {
          currentState = WAIT_SYNC_2;
        }
        break;

      case WAIT_SYNC_2:
        if (c == NANO_SYNC_2) {
          payloadIdx = 0;
          currentState = RECEIVING_PAYLOAD;
        } else {
          currentState = WAIT_SYNC_1;
          if (c == NANO_SYNC_1) currentState = WAIT_SYNC_2;
        }
        break;

      case RECEIVING_PAYLOAD:
        // Inserisce il byte direttamente nella posizione di memoria corretta della union
        nanoPacket.bytes[payloadIdx] = c;
        payloadIdx++;

        if (payloadIdx >= NANO_PAYLOAD_LEN) {
          currentState = WAIT_CHECKSUM;
        }
        break;

      case WAIT_CHECKSUM: {
        // Calcolo checksum XOR sui 16 byte
        uint8_t cs_calcolato = 0;
        for (uint8_t k = 0; k < NANO_PAYLOAD_LEN; k++) {
          cs_calcolato ^= nanoPacket.bytes[k];
        }

        if (c == cs_calcolato) {
          // --- ACCESSO DIRETTO AI DATI SENZA SPOSTAMENTO DI BIT ---
          // Copiamo i dati dalla union alle tue variabili globali e alla tua struct 'payload'
          
          payload.lat         = nanoPacket.data.lat;
          payload.lng         = nanoPacket.data.lng;
          payload.velocita_fc = nanoPacket.data.speed_cm_s;
          
          acc_X               = nanoPacket.data.acc_X_cm_s2;
          acc_Y               = nanoPacket.data.acc_Y_cm_s2;
          heading_bussola     = nanoPacket.data.heading_decideg;
          
        } else {
          // Se vedi questo messaggio, significa che c'è un disturbo elettrico sul cavo RX/TX
          Serial.println(F("[SERIAL RATTO] Errore Checksum!"));
        }

        payloadIdx = 0;
        currentState = WAIT_SYNC_1;
        break;
      }
    }
  }
}


int checksum() {
  int micro4 = payload.micro % 10000;
  return payload.currentMotor_raw + payload.voltage_raw + payload.current_raw + micro4;
}


float getDistance(long lat1, long lon1, long lat2, long lon2) {

    long dLat_raw = lat2 - lat1;
    long dLon_raw = lon2 - lon1;

    float dLat_deg = dLat_raw / 10000000.0;
    float dLon_deg = dLon_raw / 10000000.0;

    float latMedia_deg = ((float)(lat1 + lat2) / 2.0) / 10000000.0;

    float dLat_rad = dLat_deg * DEG_TO_RAD;
    float dLon_rad = dLon_deg * DEG_TO_RAD;
    float latMedia_rad = latMedia_deg * DEG_TO_RAD;

    float R = 6371000.0; // Raggio della Terra in metri
    
    float x = dLon_rad * cos(latMedia_rad);
    float y = dLat_rad;
    
    return sqrt(x * x + y * y) * R; // Restituisce la distanza esatta in metri (float)
}

void check_waypoint() {

  for (int i = 0; i < LOOK_AHEAD_WINDOW; i++) {
    
    int indiceVerifica = (waypointAttuale + i) % NUM_WAYPOINTS;
    
    distance = getDistance(payload.lat, payload.lng, waypoints[indiceVerifica].latitude, waypoints[indiceVerifica].longitude);

    if (distance <= ACCEPTANCE_RADIUS) {
        
        waypointAttuale = indiceVerifica; 
        
        if (waypoints[waypointAttuale].type == START_ACCELERATION) {
            digitalWrite(SIGNAL_PIN, HIGH);
            acceleration_status = 1;
            Serial.print(F("Waypoint ")); Serial.print(waypointAttuale); Serial.println(F(": START ACCELERATION (1)"));
        } else if (waypoints[waypointAttuale].type == STOP_ACCELERATION) {
            digitalWrite(SIGNAL_PIN, LOW);
            acceleration_status = 0;
            Serial.print(F("Waypoint ")); Serial.print(waypointAttuale); Serial.println(F(": STOP ACCELERATION (0)"));
        }
        

        waypointAttuale = (waypointAttuale + 1) % NUM_WAYPOINTS; 
        
        
        break;
    }
  }
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


void stampa1() {
  Serial.println(F("=== Pacchetto inviato ==="));
  Serial.print(F("micro:          ")); Serial.println(payload.micro);
  Serial.print(F("voltage:        ")); Serial.println(payload.voltage_raw);
  Serial.print(F("current:        ")); Serial.println(payload.current_raw);
  Serial.print(F("motorCurrent:   ")); Serial.println(payload.currentMotor_raw);
  Serial.print(F("zerocurrmotor:  ")); Serial.println(zeroCurrMotor);  
  Serial.print(F("lat (raw):      ")); Serial.println(payload.lat);
  Serial.print(F("lng (raw):      ")); Serial.println(payload.lng);
  Serial.print(F("velocita GPS:   ")); Serial.println(payload.velocita_fc);
  Serial.print(F("Acc X.          ")); Serial.println(acc_X); 
  Serial.print(F("Acc Y.          ")); Serial.println(acc_Y); 
  Serial.print(F("Heading bussola ")); Serial.println(heading_bussola); 
  Serial.print(F("verifica:       ")); Serial.println(payload.verifica);
}

void stampa2() {
  Serial.println(F("=== valori letti ==="));
  Serial.print(F("tensione raw:       ")); Serial.println(analogRead(voltPin));
  Serial.print(F("corrente raw:       ")); Serial.println(analogRead(currPin));
  Serial.print(F("correntemotore raw: ")); Serial.println(analogRead(curr_motorPin));
  Serial.print(F("voltage:            ")); Serial.println(payload.voltage_raw);
  Serial.print(F("current:            ")); Serial.println(payload.current_raw);
}

void stampa3() {
  Serial.println(F("\n[GPS/EKF] -------------------------------------"));
  Serial.print(F(" Posizione: ")); Serial.print(payload.lat); Serial.print(F(", ")); Serial.println(payload.lng);
  Serial.print(F(" Velocità Direzionale: ")); Serial.print(payload.velocita_fc); Serial.println(F(" cm/s"));
  Serial.print(F(" Heading Bussola: ")); Serial.print(heading_bussola); Serial.println(F("°"));

  Serial.println(F("[IMU SENSOR] ----------------------------------"));
  Serial.print(F(" Accel Lineare -> X (Avanti): ")); Serial.print(acc_X);
  Serial.print(F(" | Y (Laterale): ")); Serial.println(acc_Y);
}

void stampa4() {
  Serial.print(payload.lat);
  Serial.print(F("  ")); // Anche i semplici spazi vuoti traggono beneficio dalla macro F()
  Serial.println(payload.lng);
}
