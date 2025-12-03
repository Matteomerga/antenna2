import matplotlib
matplotlib.use('Agg')  # Usa backend non-interattivo per evitare GUI nei thread
import matplotlib.pyplot as plt
import serial
import struct
import numpy as np
import threading
import time
import queue
#from plotterrampa import plotterrampa
#from plottergiro import plotterrampa
#from reader import reader

# --- CONFIGURAZIONE DELLA PORTA SERIALE ---
PORTA_SERIAL = "/dev/tty.usbmodem101"
BAUD_RATE = 500000    #deve essere quello di Arduino
ser = serial.Serial(PORTA_SERIAL, BAUD_RATE, timeout=1) 
FORMATO_DATI = "<fffiiih"  # Formato dati ricevuti (26 byte totali)
data_queue_rampa = queue.Queue() # Thread‐safe queue per passare i pacchetti di dati
data_queue_giro = queue.Queue() # Thread‐safe queue per passare i pacchetti di dati
stop_event = threading.Event() #Se stop_event.is_set == True ferma tutti i thread 

# --- DEFINIZIONE DEGLI ARRAY RAMPA---
current_costante = 0  #corrente costante che la macchina consuma




# -------------------------------------------------------------------------------------------------------------------------------
# 1) THREAD READ SERIALE
# -------------------------------------------------------------------------------------------------------------------------------
def serial_reader(ser,stop_event):
    try:
        while not stop_event.is_set():  # Loop infinito per ricevere dati continuamente
            data = ser.read(26)             # Leggi dimensione della struttura Mystruct
            
            if len(data) == 26:  # Se i dati letti sono esattamente 26 byte
                velocita, voltage, current, lat, lon, micros, verifica = struct.unpack(FORMATO_DATI, data)
            else:
                continue
            if(velocita>0):
                    data_queue_giro.put((velocita, voltage, current, lat, lon, micros)) # mettiamo in coda tutti i campi che ci servono
            
            if(current>current_costante):
                data_queue_rampa.put((velocita, voltage, current, lat, lon, micros)) # mettiamo in coda tutti i campi che ci servono
            
    except Exception as e:
        print("Serial reader error:", e)
    finally:
        ser.close()


# -------------------------------------------------------------------------------------------------------------------------------
# 2) THREAD RAMPA
# -------------------------------------------------------------------------------------------------------------------------------
def plotterrampa(stop_event):
    TEMPO_TIMEOUT = 3.0
    while not stop_event.is_set():  # → giri infiniti finché non interrompi il programma

        # 1) Inizio di un nuovo giro: resetto tutte le liste
        velocita_rampa     = []
        voltage_rampa      = []
        current_rampa     = []
        latitudine_rampa   = []
        longitudine_rampa  = []
        micros_rampa       = []

        last_read_time = time.monotonic()


        while True:
            # timeout di 3 s dall'ultima lettura
            remaining = TEMPO_TIMEOUT - (time.monotonic() - last_read_time)
            if remaining <= 0:
                # sono passati più di 3 s dall'ultima lettura
                break

            try:
                # blocca al massimo 'remaining' secondi
                v, volt, curr, lat, lon, micros = data_queue_rampa.get(timeout=remaining)
            except queue.Empty:
                # scaduto il timeout senza nuovi dati
                break
            else:
                last_read_time = time.monotonic()  # aggiorna il timestamp
                # accumulo i singoli valori nelle rispettive liste
                velocita_rampa.append(v)
                voltage_rampa.append(volt)
                current_rampa.append(curr)
                latitudine_rampa.append(lat)
                longitudine_rampa.append(lon)
                micros_rampa.append(micros)

        
        #converto in numpy per fare le operazioni
        velocita_arr      = np.array(velocita_rampa)
        voltage_arr       = np.array(voltage_rampa)
        current_arr       = np.array(current_rampa)
        latitudine_arr    = np.array(latitudine_rampa)
        longitudine_arr   = np.array(longitudine_rampa)
        tempo_arr        = np.array(micros_rampa) / 1000000

        #Calcolo il dt
        dt_arr = np.diff(tempo_arr) # Calcolo delle differenze temporali

        #Calcolo la differenza di spostamento x_arr
        area_arr = (velocita_arr [:-1] + velocita_arr [1:]) / 2 * dt_arr  #Calcolo delle aree dei trapezi
        x_arr = np.concatenate(([0.0], np.cumsum(area_arr)))  #Somma cumulata per ottenere lo spostamento in ogni istante

        potenza_arr = voltage_arr*current_arr

        #Calcolo l'energia
        area_arr_energia = (potenza_arr [:-1] + potenza_arr [1:]) / 2 * dt_arr  #Calcolo delle aree dei trapezi
        energia_arr = np.concatenate(([0.0], np.cumsum(area_arr_energia)))  #Somma cumulata per ottenere energia in ogni istante

        #PLOT POTENZA ENERGIA E VELOCITA'
        fig1, axs1 = plt.subplots(3, 1, sharex=True, figsize=(8, 10))
        axs1[0].plot(x_arr, velocita_arr)
        axs1[0].set_ylabel('Velocità (m/s)')
        axs1[0].set_title('Velocità vs Distanza')
        axs1[0].grid(True)
        axs1[1].plot(x_arr, potenza_arr)
        axs1[1].set_ylabel('Potenza (W)')
        axs1[1].set_title('Potenza vs Distanza')
        axs1[1].grid(True)
        axs1[2].plot(x_arr, energia_arr)
        axs1[2].set_xlabel('Distanza (m)')
        axs1[2].set_ylabel('Energia (Joule)')
        axs1[2].set_title('Energia vs Distanza')
        axs1[2].grid(True)
        plt.tight_layout()
        plt.show()

        #PLOT TENSIONE E CORRENTE
        fig2, axs2 = plt.subplots(2, 1, sharex=True, figsize=(8, 10))
        axs2[0].plot(x_arr, voltage_arr)
        axs2[0].set_ylabel('Tensione (V)')
        axs2[0].set_title('Tensione vs Distanza')   # corretto
        axs2[0].grid(True)
        axs2[1].plot(x_arr, current_arr)
        axs2[1].set_ylabel('Corrente (A)')           # etichetta in amperè
        axs2[1].set_title('Corrente vs Distanza')   # corretto
        axs2[1].grid(True)
        plt.tight_layout()
        plt.show()

        #SVUOTO GLI ARRAY:
        velocita_rampa.clear()
        voltage_rampa.clear()
        current_rampa.clear()
        latitudine_rampa.clear()
        longitudine_rampa.clear()
        micros_rampa.clear()
        #--numpy
        velocita_arr      = velocita_arr[:0]
        voltage_arr       = voltage_arr[:0]
        current_arr       = current_arr[:0]
        latitudine_arr    = latitudine_arr[:0]
        longitudine_arr   = longitudine_arr[:0]
        tempo_arr         = tempo_arr[:0]








# -------------------------------------------------------------------------------------------------------------------------------
# 3) THREAD GIRO
# -------------------------------------------------------------------------------------------------------------------------------
def plottergiro(stop_event):

    while not stop_event.is_set():
    # --- DEFINIZIONE DEGLI ARRAY ULTIMO GIRO---
        velocita_giro = []
        voltage_giro = []
        current_giro = []
        latitudine_giro = []
        longitudine_giro = []
        micros_giro = []


        last_read_time = time.monotonic()
        first_time = None
        energia_best1 = None
        energia_best2 = None
        energia_last = None
        TEMPO_LIMITE = 190.9
        current_time = 0
        
    
        while True:
            # calcola quanti secondi mancano ai 3 s di timeout
            remaining = 3.0 - (time.monotonic() - last_read_time)
            if remaining <= 0:
                # sono passati più di 3 s dall'ultima lettura
                first_time = None
                break

            try:
                # blocca al massimo 'remaining' secondi
                v, volt, curr, lat, lon, micros = data_queue_giro.get(timeout=remaining)
            except queue.Empty:
                # scaduto il timeout senza nuovi dati
                break
            else:
                last_read_time = time.monotonic()  # aggiorna il timestamp

                #TIMER TEMPO REALE
                if first_time is None:
                    first_time = time.monotonic()
                    current_time = 0
                    last_time = 0
                    x = 0
                    print("\n>> Inizio giro! Cronometro a 0s")
                else:
                # tempo trascorso dall’inizio del giro
                    current_time = time.monotonic() - first_time
                    if(current_time-last_time) >= 0.5:
                        dx = v * (current_time-last_time)
                        x =  x + dx
                        print(f"\r{current_time:6.2f}s  {x:6.2f}m", end="")
                        last_time = current_time

                # accumulo i singoli valori nelle rispettive liste
                velocita_giro.append(v)
                voltage_giro.append(volt)
                current_giro.append(curr)
                latitudine_giro.append(lat)
                longitudine_giro.append(lon)
                micros_giro.append(micros)


        #converto in numpy per fare le operazioni
        velocita_arr      = np.array(velocita_giro)
        voltage_arr       = np.array(voltage_giro)
        current_arr       = np.array(current_giro)
        latitudine_arr    = np.array(latitudine_giro)
        longitudine_arr   = np.array(longitudine_giro)
        tempo_arr        = np.array(micros_giro) / 1000000

        if len(tempo_arr) < 2:
            print("Giro: dati insufficienti, salto grafico.")
            continue

        #Calcolo il dt
        dt_arr = np.diff(tempo_arr) # Calcolo delle differenze temporali

        #Calcolo la differenza di spostamento x_arr
        area_arr = (velocita_arr [:-1] + velocita_arr [1:]) / 2 * dt_arr  #Calcolo delle aree dei trapezi
        x_arr = np.concatenate(([0.0], np.cumsum(area_arr)))  #Somma cumulata per ottenere lo spostamento in ogni istante

        potenza_arr = voltage_arr*current_arr

        #Calcolo l'energia
        area_arr_energia = (potenza_arr [:-1] + potenza_arr [1:]) / 2 * dt_arr  #Calcolo delle aree dei trapezi
        energia_arr = np.concatenate(([0.0], np.cumsum(area_arr_energia)))  #Somma cumulata per ottenere lo spostamento in ogni istante

        #CALCOLO I GIRI MIGLIORI
        energia_last= energia_arr
        if current_time < TEMPO_LIMITE:

            if(energia_last == None):
                energia_best1 = energia_arr
                energia_best2 = energia_arr
                #copio
                velocita_giro1 = velocita_arr.copy()
                voltage_giro1 = voltage_arr.copy()
                current_giro1 = current_arr.copy()
                tempo_giro1 = tempo_arr.copy()
                x_giro1 = x_arr.copy()
                potenza_giro1 = potenza_arr.copy()
                #
                velocita_giro2 = velocita_arr.copy()
                voltage_giro2 = voltage_arr.copy()
                current_giro2 = current_arr.copy()
                tempo_giro2 = tempo_arr.copy()
                x_giro2 = x_arr.copy()
                potenza_giro2 = potenza_arr.copy()

            elif(energia_last<energia_best1):
                energia_best2 = energia_best1
                energia_best1 = energia_last
                #
                velocita_giro2 = velocita_giro1.copy()
                voltage_giro2 = voltage_giro1.copy()
                current_giro2 = current_giro1.copy()
                tempo_giro2 = tempo_giro1.copy()
                x_giro2 = x_giro1.copy()
                potenza_giro2 = potenza_giro1.copy()
                #
                velocita_giro1 = velocita_arr.copy()
                voltage_giro1 = voltage_arr.copy()
                current_giro1 = current_arr.copy()
                tempo_giro1 = tempo_arr.copy()
                x_giro1 = x_arr.copy()
                potenza_giro1 = potenza_arr.copy()

            elif(energia_last<energia_best2 & energia_last>energia_best1):
                energia_best2 = energia_last
                #
                velocita_giro2 = velocita_arr.copy()
                voltage_giro2 = voltage_arr.copy()
                current_giro2 = current_arr.copy()
                tempo_giro2 = tempo_arr.copy()
                x_giro2 = x_arr.copy()
                potenza_giro2 = potenza_arr.copy()



        #PLOT POTENZA ENERGIA E VELOCITA'
        fig1, axs1 = plt.subplots(3, 1, sharex=True, figsize=(8, 10))
        #Velocità
        axs1[0].plot(x_arr, velocita_arr, label='current lap')
        axs1[0].plot(x_giro1, velocita_giro1, label='best lap')
        axs1[0].plot(x_giro2, velocita_giro2, label='2nd best lap')
        axs1[0].set_ylabel('Velocità (m/s)')
        axs1[0].set_title('Velocità vs Distanza')
        axs1[0].grid(True)
        axs1[0].legend()
        # Potenza
        axs1[1].plot(x_arr, potenza_arr,       label='current lap')
        axs1[1].plot(x_giro1, potenza_giro1,  label='best lap')
        axs1[1].plot(x_giro2, potenza_giro2,  label='2nd best lap')
        axs1[1].set_ylabel('Potenza (W)')
        axs1[1].set_title('Potenza vs Distanza')
        axs1[1].grid(True)
        axs1[1].legend()
        # Energia
        axs1[2].plot(x_arr, energia_arr,       label='current lap')
        axs1[2].plot(x_giro1, energia_best1,  label='best lap')
        axs1[2].plot(x_giro2, energia_best2,  label='2nd best lap')
        axs1[2].set_xlabel('Distanza (m)')
        axs1[2].set_ylabel('Energia (J)')
        axs1[2].set_title('Energia vs Distanza')
        axs1[2].grid(True)
        axs1[2].legend()
        #
        plt.tight_layout()
        plt.show()

        # PLOT TENSIONE E CORRENTE
        fig2, axs2 = plt.subplots(2, 1, sharex=True, figsize=(8, 10))

        # Tensione
        axs2[0].plot(x_arr,      voltage_arr,      label='current lap')
        axs2[0].plot(x_giro1,    voltage_giro1,    label='best lap')
        axs2[0].plot(x_giro2,    voltage_giro2,    label='2nd best lap')
        axs2[0].set_ylabel('Tensione (V)')
        axs2[0].set_title('Tensione vs Distanza')
        axs2[0].grid(True)
        axs2[0].legend()

        # Corrente
        axs2[1].plot(x_arr,      current_arr,      label='current lap')
        axs2[1].plot(x_giro1,    current_giro1,    label='best lap')
        axs2[1].plot(x_giro2,    current_giro2,    label='2nd best lap')
        axs2[1].set_ylabel('Corrente (A)')
        axs2[1].set_title('Corrente vs Distanza')
        axs2[1].grid(True)
        axs2[1].legend()

        plt.tight_layout()
        plt.show()


        #SVUOTO GLI ARRAY:
        velocita_giro.clear()
        voltage_giro.clear()
        current_giro.clear()
        latitudine_giro.clear()
        longitudine_giro.clear()
        micros_giro.clear()
        #--numpy
        velocita_arr      = velocita_arr[:0]
        voltage_arr       = voltage_arr[:0]
        current_arr       = current_arr[:0]
        latitudine_arr    = latitudine_arr[:0]
        longitudine_arr   = longitudine_arr[:0]
        tempo_arr         = tempo_arr[:0]


  # -------------------------------------------------------------------------------------------------------------------------------  


reader_t = threading.Thread(
    target=serial_reader,
    args=(ser, stop_event),
    daemon=True
)
plotterrampa_t = threading.Thread(
    target=plotterrampa,
    args=(stop_event,),
    daemon=True
)
plottergiro_t = threading.Thread(
    target=plottergiro,
    args=(stop_event,),
    daemon=True
)

reader_t.start()
plotterrampa_t.start()
plottergiro_t.start()

# 4) Mantieni vivo il main thread

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\nInterrotto dall'utente: fermo i thread…")
    stop_event.set()

    reader_t.join()
    plotterrampa_t.join()
    plottergiro_t.join()
    print("Tutti i thread sono terminati. Esco.")
