import serial
import struct
import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets, QtCore
import numpy as np
import os

# --- CONFIGURAZIONE DELLA PORTA SERIALE ---
PORTA_SERIAL = "/dev/tty.usbmodem101"
BAUD_RATE = 500000
FORMATO_DATI = "hh2iih"  # Formato dati ricevuti (16 byte totali, con GPS nei 2i)

# Variabile per la percentuale di pacchetti da visualizzare (0-1)
n_pacchetti = 1

# --- PARAMETRI PER IL GRAFICO ---
delta = 700
t = 10
N = int(delta * t * n_pacchetti)  
tempi = np.zeros(N, dtype=np.float32)
joule_valori = np.zeros(N, dtype=np.float32)
velocita_valori = np.zeros(N, dtype=np.float32)

tempo_iniziale = None  

# --- CONFIGURAZIONE FILE DI SALVATAGGIO ---
FILE_TXT = os.path.expanduser("~/Desktop/dati_seriale.txt")
BUFFER_SCRITTURA = 1000  # Scrive ogni 1000 campioni (~1 secondo)

# Inizializza la lista buffer
buffer_dati = []

# Crea il file e scrive l'intestazione solo se non esiste
if not os.path.exists(FILE_TXT):
    with open(FILE_TXT, mode="w") as file:
        file.write("Tempo (s)\tJoule\tVelocità\tLatitudine\tLongitudine\n")

# --- CREAZIONE FINESTRA PyQtGraph ---
app = QtWidgets.QApplication([])
win = pg.GraphicsLayoutWidget(show=True)
win.setWindowTitle("Grafico Real-Time")
win.resize(800, 500)

# Creazione plot principale
plot = win.addPlot(title="Dati in Tempo Reale")
plot.setLabel("left", "Velocità (m/s)", color="b")
plot.setLabel("bottom", "Tempo (s)")
plot.setYRange(0, 50)  

# Creazione ViewBox secondaria per l'asse destro
right_vb = pg.ViewBox()
plot.showAxis("right")
plot.getAxis("right").setLabel("Joule", color="r")
plot.scene().addItem(right_vb)
plot.getAxis("right").linkToView(right_vb)
right_vb.setXLink(plot)
right_vb.setYRange(0, 1000)  

# Linee per i dati
curve_velocita = plot.plot(pen="b", name="Velocità")  
curve_joule = pg.PlotCurveItem(pen="r")  
right_vb.addItem(curve_joule)

# --- SINCRONIZZAZIONE DELLE VIEWBOX ---
def update_views():
    right_vb.setGeometry(plot.vb.sceneBoundingRect())
    right_vb.linkedViewChanged(plot.vb, right_vb.XAxis)

plot.vb.sigResized.connect(update_views)

# --- LETTURA SERIALE ---
class LetturaSeriale(QtCore.QThread):
    dati_ricevuti = QtCore.pyqtSignal(float, int, int)

    def __init__(self, porta, baudrate, n_pacchetti):
        super().__init__()
        self.running = True
        self.n_pacchetti = max(0, min(1, n_pacchetti))  
        self.contatore_pacchetti = 0  
        self.pacchetti_persi = 0  # Variabile per i pacchetti persi
        self.tempo_iniziale = None  
        
        self.timer = QtCore.QTimer(self)  # Timer per ogni secondo
        self.timer.timeout.connect(self.stampa_pacchetti_persi)
        self.timer.start(1000)  # 1000 ms = 1 secondo
        
        try:
            self.ser = serial.Serial(porta, baudrate, timeout=0)
        except serial.SerialException as e:
            print(f"Errore apertura seriale: {e}")
            self.running = False

    def run(self):
        global buffer_dati  
        
        ultimo_joule = 0  # Per tracciare l'ultimo valore di joule ricevuto
        ultimo_micros = None  # Per tracciare l'ultimo timestamp
        expected_interval_micros = 1000000 / delta  # Intervallo atteso in microsecondi
        
        while self.running:
            if self.ser.in_waiting >= 18:
                dati = self.ser.read(18)
                velocita, joule, latitudine, longitudine, micros, verifica = struct.unpack(FORMATO_DATI, dati)

                if self.tempo_iniziale is None:
                    self.tempo_iniziale = micros
                    ultimo_micros = micros
                else:
                    # Calcola l'intervallo attuale tra pacchetti
                    actual_interval = micros - ultimo_micros
                    
                    # Controlla la perdita dei pacchetti basandosi sull'intervallo di tempo
                    if actual_interval > expected_interval_micros * 1.5:  # Tolleranza del 50%
                        # Calcola quanti pacchetti probabilmente sono stati persi
                        # sottraiamo 1 perché stiamo contando i pacchetti mancanti tra quello attuale e l'ultimo ricevuto
                        pacchetti_perduti = int(round(actual_interval / expected_interval_micros) - 1)
                        self.pacchetti_persi += pacchetti_perduti
                        #print(f"Intervallo di silenzio: {actual_interval/1000:.2f}ms, ~{pacchetti_perduti} pacchetti persi")
                    
                    # Aggiorna l'ultimo timestamp
                    ultimo_micros = micros

                tempo_secondi = (micros - self.tempo_iniziale) / 1e6  

                # Salva il dato nel buffer
                buffer_dati.append(f"{tempo_secondi}\t{joule}\t{velocita}\t{latitudine}\t{longitudine}\n")

                # Scrive i dati ogni BUFFER_SCRITTURA campioni
                if len(buffer_dati) >= BUFFER_SCRITTURA:
                    with open(FILE_TXT, mode="a") as file:
                        file.writelines(buffer_dati)
                    buffer_dati = []  # Svuota il buffer dopo la scrittura

                # Filtro per la visualizzazione dei pacchetti
                self.contatore_pacchetti += 1
                if self.n_pacchetti > 0 and (self.contatore_pacchetti % int(1 / self.n_pacchetti)) != 0:
                    continue  

                self.dati_ricevuti.emit(tempo_secondi, joule, velocita)

    def stop(self):
        global buffer_dati
        self.running = False
        self.ser.close()

        # Scrive gli ultimi dati rimasti nel buffer
        if buffer_dati:
            with open(FILE_TXT, mode="a") as file:
                file.writelines(buffer_dati)
            buffer_dati = []

    def stampa_pacchetti_persi(self):
        print(f"Pacchetti persi negli ultimi 1000 pacchetti: {self.pacchetti_persi}")
        self.pacchetti_persi = 0  # Reset pacchetti persi ogni secondo

# --- FUNZIONE DI AGGIORNAMENTO GRAFICO ---
def aggiorna_grafico(tempo, joule, velocita):
    global tempi, joule_valori, velocita_valori
    
    # Shift a sinistra per simulare lo scorrimento
    tempi[:-1] = tempi[1:]
    joule_valori[:-1] = joule_valori[1:]
    velocita_valori[:-1] = velocita_valori[1:]
    
    # Inserisce il nuovo valore all'ultima posizione
    tempi[-1] = tempo
    joule_valori[-1] = joule
    velocita_valori[-1] = velocita
    
    # Aggiorna il grafico
    curve_velocita.setData(tempi, velocita_valori)
    curve_joule.setData(tempi, joule_valori)

    plot.setYRange(0, 50)  
    right_vb.setYRange(0, 1000)  

# --- AVVIO DEL THREAD E GESTIONE CHIUSURA ---
thread_seriale = LetturaSeriale(PORTA_SERIAL, BAUD_RATE, n_pacchetti)
thread_seriale.dati_ricevuti.connect(aggiorna_grafico)
thread_seriale.start()

def chiudi_app():
    thread_seriale.stop()
    app.quit()

win.closeEvent = lambda event: chiudi_app()

# --- AVVIO DELL'INTERFACCIA ---
app.exec()
