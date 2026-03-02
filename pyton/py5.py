import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import serial
import struct
import numpy as np
import threading
import time
import queue
import csv
import os
import tkinter as tk

# --- CONFIGURAZIONE DELLA PORTA SERIALE ---
PORTA_SERIAL = "/dev/cu.usbmodem101"
#PORTA_SERIAL = "/dev/tty.usbserial-A5069RR4"
BAUD_RATE = 115200
ser = serial.Serial(PORTA_SERIAL, BAUD_RATE, timeout=1)
FORMATO_DATI = "<fffiiih"
PACCHETTO_SIZE = struct.calcsize(FORMATO_DATI)

# code thread-safe per rampa e giro
data_queue_rampa = queue.Queue()
data_queue_giro = queue.Queue()
stop_event = threading.Event()

# corrente costante per rampa
current_costante = 0.4

# cartella desktop per dati e grafici
desktop = os.path.join(os.path.expanduser("~"), "Desktop")
FOLDER_PATH = os.path.join(desktop, "dati")
os.makedirs(FOLDER_PATH, exist_ok=True)


pacchetti_da_ricevere = 10
pacchetti_ricevuti = 0


# --- DASHBOARD ---
t_aggiornamento_dashboard = 500  # ms
root = tk.Tk()
root.title("Dashboard Arduino Live")
root.geometry("600x400")

vel_label = tk.Label(root, text="Velocità: ---", font=("Helvetica", 50))
volt_label = tk.Label(root, text="Tensione: ---", font=("Helvetica", 50))
curr_label = tk.Label(root, text="Corrente: ---", font=("Helvetica", 50))
time_label = tk.Label(root, text="Tempo: ---", font=("Helvetica", 50))
pacchetti_persi_label = tk.Label(root, text="Pacchetti persi: ---", font=("Helvetica", 50))

vel_label.pack(pady=10)
volt_label.pack(pady=10)
curr_label.pack(pady=10)
time_label.pack(pady=10)
pacchetti_persi_label.pack(pady=10)

labels = {'vel': vel_label, 'volt': volt_label, 'curr': curr_label, 'time': time_label, 'pacchetti persi': pacchetti_persi_label}

dashboard_vel = 0.0
dashboard_volt = 0.0
dashboard_curr = 0.0
dashboard_time = 0.0

def update_dashboard():
    global pacchetti_ricevuti
    pacchetti_persi = pacchetti_da_ricevere - (pacchetti_ricevuti * 1000 // t_aggiornamento_dashboard)
    pacchetti_ricevuti = 0
    labels['vel'].config(text=f"Velocità: {dashboard_vel:.2f} m/s")
    labels['volt'].config(text=f"Tensione: {dashboard_volt:.2f} V")
    labels['curr'].config(text=f"Corrente: {dashboard_curr:.2f} A")
    labels['time'].config(text=f"Tempo: {dashboard_time:.2f} s")
    labels['pacchetti persi'].config(text=f"Pacchetti persi: {pacchetti_persi}")
    root.after(t_aggiornamento_dashboard, update_dashboard)

# -------------------------------------------------------------------------------------------------------------------------------
# THREAD READ SERIALE
# -------------------------------------------------------------------------------------------------------------------------------
def serial_reader(ser, stop_event, root, labels):
    file_path = os.path.join(FOLDER_PATH, "serial_data.csv")
    is_new = not os.path.exists(file_path)
    with open(file_path, mode='a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        if is_new:
            writer.writerow(["velocita", "voltage", "current", "lat", "lon", "micros"])
            csvfile.flush()

        try:
            # sincronizzazione pacchetto valido
            buffer = b''
            started = False
            print("Attendo pacchetto valido da Arduino...")
            while not started and not stop_event.is_set():
                buffer += ser.read(ser.in_waiting or 1)

                while len(buffer) >= PACCHETTO_SIZE:
                    pacchetto = buffer[:PACCHETTO_SIZE]
                    try:
                        velocita, voltage, current, lat, lon, micros, verifica = struct.unpack(FORMATO_DATI, pacchetto)
                    except struct.error:
                        # scarta 1 byte e riprova
                        buffer = buffer[1:]
                        continue

                    if -10.0 <= velocita <= 10.0 and -10 <= voltage <= 50.0 and -10 <= current <= 10.0:
                        started = True
                        print("Pacchetto valido ricevuto, sincronizzato.")
                        break
                    else:
                        # pacchetto non valido, scarta il primo byte
                        buffer = buffer[1:]

            # lettura continua
            global dashboard_vel, dashboard_volt, dashboard_curr, dashboard_time, pacchetti_ricevuti
            while not stop_event.is_set():
                data = ser.read(PACCHETTO_SIZE)
                if len(data) != PACCHETTO_SIZE:
                    continue
                velocita, voltage, current, lat, lon, micros, verifica = struct.unpack(FORMATO_DATI, data)
                pacchetti_ricevuti += 1

                # code per rampa/giro
                if velocita >= 0:
                    data_queue_giro.put((velocita, voltage, current, lat, lon, micros))
                if current > current_costante:
                    data_queue_rampa.put((velocita, voltage, current, lat, lon, micros))

                # scrivo su CSV
                writer.writerow([velocita, voltage, current, lat, lon, micros])
                csvfile.flush()

                # --- aggiornamento dashboard tramite root.after ---


                dashboard_vel = velocita
                dashboard_volt = voltage
                dashboard_curr = current
                dashboard_time = micros / 1000000


        except Exception as e:
            print("Serial reader error:", e)
        finally:
            ser.close()

# -------------------------------------------------------------------------------------------------------------------------------
# 2) THREAD RAMPA
# -------------------------------------------------------------------------------------------------------------------------------
def plotterrampa(stop_event):
    while not stop_event.is_set():  # → giri infiniti finché non interrompi il programma

        # 1) Inizio di un nuovo giro: resetto tutte le liste
        velocita_rampa     = []
        voltage_rampa      = []
        current_rampa      = []
        latitudine_rampa   = []
        longitudine_rampa  = []
        micros_rampa       = []

        # =========================
        # 1) ATTESA INIZIO RAMPA
        # =========================
        try:
            v, volt, curr, lat, lon, micros = data_queue_rampa.get(timeout=1)
        except queue.Empty:
            continue  # nessuna rampa iniziata, continuo ad aspettare

        # primo dato valido → inizio rampa
        velocita_rampa.append(v)
        voltage_rampa.append(volt)
        current_rampa.append(curr)
        latitudine_rampa.append(lat)
        longitudine_rampa.append(lon)
        micros_rampa.append(micros)

        ultimo_dato = time.monotonic()
        TEMPO_FINE_RAMPA = 2.0  # secondi senza dati prima di chiudere la rampa

        # =========================
        # 2) RAMPA ATTIVA
        # =========================
        while True:
            try:
                v, volt, curr, lat, lon, micros = data_queue_rampa.get(timeout=0.5)

                velocita_rampa.append(v)
                voltage_rampa.append(volt)
                current_rampa.append(curr)
                latitudine_rampa.append(lat)
                longitudine_rampa.append(lon)
                micros_rampa.append(micros)

                ultimo_dato = time.monotonic()

            except queue.Empty:
                if time.monotonic() - ultimo_dato > TEMPO_FINE_RAMPA:
                    break

        # ======================================================
        # Conversione in numpy per fare le operazioni e plot
        # ======================================================
        velocita_arr      = np.array(velocita_rampa)
        voltage_arr       = np.array(voltage_rampa)
        current_arr       = np.array(current_rampa)
        latitudine_arr    = np.array(latitudine_rampa)
        longitudine_arr   = np.array(longitudine_rampa)
        tempo_arr         = np.array(micros_rampa) / 1000000

        if len(tempo_arr) < 2:
            print("Rampa: dati insufficienti, salto grafico.")
            continue

        # Calcolo dt e spostamento
        dt_arr = np.diff(tempo_arr)
        area_arr = (velocita_arr[:-1] + velocita_arr[1:]) / 2 * dt_arr
        x_arr = np.concatenate(([0.0], np.cumsum(area_arr)))

        # Potenza ed energia
        potenza_arr = voltage_arr * current_arr
        area_arr_energia = (potenza_arr[:-1] + potenza_arr[1:]) / 2 * dt_arr
        energia_arr = np.concatenate(([0.0], np.cumsum(area_arr_energia)))

        # PLOT POTENZA, ENERGIA E VELOCITA'
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
        fname = os.path.join(FOLDER_PATH, f"plot1_{threading.current_thread().name}_{int(time.time())}.png")
        fig1.savefig(fname)
        print(f"Salvato grafico in {fname}")
        plt.close(fig1)

        # PLOT TENSIONE E CORRENTE
        fig2, axs2 = plt.subplots(2, 1, sharex=True, figsize=(8, 10))
        axs2[0].plot(x_arr, voltage_arr)
        axs2[0].set_ylabel('Tensione (V)')
        axs2[0].set_title('Tensione vs Distanza')
        axs2[0].grid(True)
        axs2[1].plot(x_arr, current_arr)
        axs2[1].set_ylabel('Corrente (A)')
        axs2[1].set_title('Corrente vs Distanza')
        axs2[1].grid(True)
        plt.tight_layout()
        fname = os.path.join(FOLDER_PATH, f"plot2_{threading.current_thread().name}_{int(time.time())}.png")
        fig2.savefig(fname)
        print(f"Salvato grafico in {fname}")
        plt.close(fig2)

        # SVUOTO GLI ARRAY
        velocita_rampa.clear()
        voltage_rampa.clear()
        current_rampa.clear()
        latitudine_rampa.clear()
        longitudine_rampa.clear()
        micros_rampa.clear()
        velocita_arr[:0]
        voltage_arr[:0]
        current_arr[:0]
        latitudine_arr[:0]
        longitudine_arr[:0]
        tempo_arr[:0]








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
                        dx = v * ((current_time-last_time))
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

            if(energia_last is None):
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
        fname = f"plot1_{threading.current_thread().name}_{int(time.time())}.png"
        fig1.savefig(fname)
        print(f"Salvato grafico in {fname}")
        plt.close(fig1)

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
        fname = f"plot2_{threading.current_thread().name}_{int(time.time())}.png"
        fig2.savefig(fname)
        print(f"Salvato grafico in {fname}")
        plt.close(fig2)


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
    args=(ser, stop_event, root, labels),  # <- aggiungi root e labels qui
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
update_dashboard()



try:
    root.mainloop()
except KeyboardInterrupt:
    stop_event.set()
    reader_t.join()
    # plotterrampa_t.join()
    # plottergiro_t.join()
    print("Programma terminato.")

