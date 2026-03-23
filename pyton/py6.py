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

stop_event = threading.Event()

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


def serial_reader(ser, stop_event, root, labels):
    file_path = os.path.join(FOLDER_PATH, "serial_data.csv")
    is_new = not os.path.exists(file_path)
    with open(file_path, mode='a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        if is_new:
            writer.writerow(["micros", "voltage", "current", "velocita", "lat", "lon"])
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

                    if int(velocita + voltage + current + micros % 10000) == verifica:
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


                # scrivo su CSV
                writer.writerow([micros, voltage, current, velocita, lat, lon])
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



reader_t = threading.Thread(
    target=serial_reader,
    args=(ser, stop_event, root, labels),  # <- aggiungi root e labels qui
    daemon=True
)


reader_t.start()
update_dashboard()



try:
    root.mainloop()
except KeyboardInterrupt:
    stop_event.set()
    reader_t.join()
    # plotterrampa_t.join()
    # plottergiro_t.join()
    print("Programma terminato.")

