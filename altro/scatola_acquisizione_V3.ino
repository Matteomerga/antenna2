/*
  Esempio di codice per scrivere più elementi su una riga di testo

  Pin Della Scheda SD da collegare ad Arduino:
  GND  - Ground
  VCC  - +5 V
  MISO - Pin 12 bianco
  MOSI - Pin 11 arancione
  SCK  - Pin 13 azzurro
  CS   - Pin 4 giallo
*/

#include <SPI.h>
#include <SD.h>



int voltPin = A2;
long voltVal = 0;
float kv = 0.004605735;
float voltage = 0.0;

int currPin = A1;
long currentVal = 0;
float ki = 0.004665127;// 0.00366;// 0.0001882;
float current = 0.0;
unsigned long zeroCurr = 0;

unsigned long Speed = 0;
unsigned long PrevSpeed = 0;

volatile byte hall_rising = 0; // interrupt flag
volatile unsigned long irqMicros;
volatile unsigned long irqMicrosPrev = 0;

unsigned long startMicros;
unsigned long differenceTimeMicros;

unsigned long hallEffectCount = 0;


int dt = 100; // scrittura a 10 Hz
unsigned long t = 100;

int dtMes = 500; // tempo tra le acquisizioni in microsecondi
unsigned long tMesCurr = 0;
unsigned long tMesPrev = 0;
int n = 0; // numero di acquisizioni

int ns = 0;
unsigned long sumSpeed = 0;

// VARIABILI LETTURA PWM
unsigned long tStartPWM = 0;
unsigned long tFinPWM = 0;
unsigned long PWMval = 0;
int laststatoPWM = 0;
int statoPWM = 0;
unsigned long tCurrPWM = 0;
unsigned long tLastPWM = 0;

void wheel_IRQ()
{
  irqMicros = micros();
  hall_rising = 1;
}



void setup() {
  analogReadResolution(14); //change to 14-bit resolution
  pinMode(currPin, INPUT);
  pinMode(voltPin, INPUT);
  Serial.begin( 115200 ); // can this be faster? faster would be better
  attachInterrupt( digitalPinToInterrupt(2), wheel_IRQ, FALLING ); // pin 2 looks for LOW to HIGH change
  for (int i = 0; i < 100; i++) {
    zeroCurr = zeroCurr + analogRead(currPin);
    delay(1);
  }
  zeroCurr = zeroCurr / 100;

  SD.begin(4);
}

void loop() {

  // acquisisco i segnali
  tMesCurr = micros();
  if (tMesCurr - tMesPrev > dtMes) {
    tMesPrev = tMesCurr;
    currentVal = currentVal + analogRead(currPin) - zeroCurr;
    voltVal = voltVal + analogRead(voltPin) - 1387;
    n++;
  }

  if (hall_rising == 1) {
    differenceTimeMicros = irqMicros - startMicros;
    startMicros = irqMicros;
    hall_rising = 0;
    Speed =  30000000 / differenceTimeMicros;

    sumSpeed = sumSpeed + Speed;
    ns++;
  }

  if (millis() >= t) { // print every time dt
    t = millis() + dt;

    // calcolo i valori di tensione e corrente
    current = currentVal / n;
    current = currentVal * ki;
    voltVal = voltVal / n;
    voltage = voltVal * kv;
    if (ns > 0) {
      Speed = sumSpeed / ns;
      sumSpeed = 0;
      ns = 0;
    }
/*
    Serial.print(float(t / 1000.0));
    Serial.print(",");
    Serial.print(current);
    Serial.print(",");
    Serial.print(voltage);
    Serial.print(",");
    Serial.println(Speed);
    */


        // definisco la stringa che costituirà la riga successiva del file di testo
    String dataRow = "";// Reinizializzo la stringa che verrà scritta nel file di testo

    dataRow += String(float(millis() / 1000.0)); //Aggiungo il primo elemento della riga alla stringa
    dataRow += " "; //Aggiungo alla stringa il separatore tra elementi nella riga
    dataRow += String(current);//Aggiungo il valore della corrente
    dataRow += " ";//Aggiungo alla stringa il separatore tra elementi nella riga
    dataRow += String(voltage);//Aggiungo il valore della tensione
    dataRow += " ";//Aggiungo alla stringa il separatore tra elementi nella riga
    dataRow += String(Speed);//Aggiungo il valore della velocità

    // Scrivo sulla SD
    //Come nome del file non usare nomi troppo lunghi, es: "test.txt"
    // N.B. IL NOME DEL FILE VIENE SCRITTO IN OGNI CASO IN MAIUSCOLO

    File myFile = SD.open("TEST.txt", FILE_WRITE);         // Apro il file di testo

    // Scrittura dei dati
    myFile.println(dataRow);
    // myFile.println(String(PWM));
    myFile.close();

    
    n = 0;
    currentVal = 0;
    voltVal = 0;
  }
}// end of void loop
