const unsigned long t1 = 1000;
const unsigned long t2 = 2000;
const unsigned long t3 = 15000;
const unsigned long t4 = 4000;
int i = 1;  //ATTENZIONE: indica l'incremento di corrente, modificare per avere rampa piu o meno accentuata
int delta = 100;
int h=1;

unsigned long previousMicros = 0;

//stuct dei dati da inviare
struct Mystruct {
    float speed;
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

    // Initial payload setup
    payload.speed = 0;
    payload.voltage = 5;
    payload.current = 0;
    payload.lat = 45123456;
    payload.lng = 9123456;
    payload.micro = 0;
}

void loop() {
    payload.speed = 0;
    payload.current = 0;
    unsigned long currentMicros = millis();

    previousMicros = currentMicros;
    while (currentMicros - previousMicros < t1) {
        currentMicros = millis();
        delay(delta);
        payload.micro = micros();
        if(h) Serial.write((uint8_t *)&payload, sizeof(payload));
        else stampa();
    }

    payload.speed = 10;
    previousMicros = currentMicros;
    while (currentMicros - previousMicros < t4) {
        currentMicros = millis();
        delay(delta);
        payload.micro = micros();
        if(h) Serial.write((uint8_t *)&payload, sizeof(payload));
        else stampa();
    }

    previousMicros = currentMicros;
    while (currentMicros - previousMicros < t3) {
        currentMicros = millis();
        delay(delta);
        payload.current = payload.current + i;
        payload.micro = micros();
        if(h) Serial.write((uint8_t *)&payload, sizeof(payload));
        else stampa();
    }

    payload.current = 0;
    previousMicros = currentMicros;
    while (currentMicros - previousMicros < t1) {
        currentMicros = millis();
        delay(delta);
        payload.micro = micros();
        if(h) Serial.write((uint8_t *)&payload, sizeof(payload));
        else stampa();
    }

    previousMicros = currentMicros;
    while (currentMicros - previousMicros < t3) {
        currentMicros = millis();
        delay(delta);
        payload.current = payload.current + i;
        payload.micro = micros();
        if(h) Serial.write((uint8_t *)&payload, sizeof(payload));
        else stampa();
    }

    payload.current = 0;
    previousMicros = currentMicros;
    while (currentMicros - previousMicros < t3) {
        currentMicros = millis();
        delay(delta);
        payload.micro = micros();
        if(h) Serial.write((uint8_t *)&payload, sizeof(payload));
        else stampa();
    }

    payload.speed = 0;
    payload.current = 0;
    previousMicros = currentMicros;
    while (currentMicros - previousMicros < t2) {
        currentMicros = millis();
        delay(delta);
        payload.micro = micros();
        if(h) Serial.write((uint8_t *)&payload, sizeof(payload));
        else stampa();
    }
}

void stampa() {
    Serial.print("speed:");
    Serial.print(payload.speed);
    Serial.print('\t');

    Serial.print("volt:");
    Serial.print(payload.voltage);
    Serial.print('\t');

    Serial.print("curr:");
    Serial.print(payload.current);
    Serial.print('\t');

    Serial.print("t:");
    Serial.println(payload.micro);
}


