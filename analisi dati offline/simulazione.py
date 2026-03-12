import csv
import os
import time
import threading
import tkinter as tk

# --- CARTELLA PER SALVARE I DATI ---
desktop = os.path.join(os.path.expanduser("~"), "Desktop")
FOLDER_PATH = os.path.join(desktop, "dati")
os.makedirs(FOLDER_PATH, exist_ok=True)
file_path = os.path.join(FOLDER_PATH, "simulated_data.csv")

# --- PARAMETRI SIMULAZIONE ---
t1 = 1000      # ms
t2 = 2000      # ms
t3 = 15000     # ms
t4 = 4000      # ms
delta = 100    # ms tra pacchetti
i = 1          # incremento corrente per rampa

# --- VARIABILI SIMULATE ---
payload = {
    "speed": 0.0,
    "voltage": 5.0,
    "current": 0.0,
    "lat": 45123456,
    "lon": 9123456,
    "micro": 0
}

# --- DASHBOARD ---
root = tk.Tk()
root.title("Simulazione Arduino Live")
root.geometry("600x400")

vel_label = tk.Label(root, text="Velocità: ---", font=("Helvetica", 50))
volt_label = tk.Label(root, text="Tensione: ---", font=("Helvetica", 50))
curr_label = tk.Label(root, text="Corrente: ---", font=("Helvetica", 50))
time_label = tk.Label(root, text="Tempo: ---", font=("Helvetica", 50))

vel_label.pack(pady=10)
volt_label.pack(pady=10)
curr_label.pack(pady=10)
time_label.pack(pady=10)



dashboard_vel = 0.0
dashboard_volt = 0.0
dashboard_curr = 0.0
dashboard_time = 0.0

def update_dashboard():
    vel_label.config(text=f"Velocità: {dashboard_vel:.2f} m/s")
    volt_label.config(text=f"Tensione: {dashboard_volt:.2f} V")
    curr_label.config(text=f"Corrente: {dashboard_curr:.2f} A")
    time_label.config(text=f"Tempo: {dashboard_time:.2f} s")
    root.after(100, update_dashboard)  # aggiornamento ogni 100 ms

# --- FUNZIONE DI SIMULAZIONE RAMPE ---
def simulate_loop():
    global dashboard_vel, dashboard_volt, dashboard_curr, dashboard_time
    start_time = time.time()  # in secondi

    sequence = [
        {"duration": t1, "speed": 0, "current_ramp": False},
        {"duration": t4, "speed": 10, "current_ramp": False},
        {"duration": t3, "speed": 10, "current_ramp": True},
        {"duration": t1, "speed": 10, "current_ramp": False},
        {"duration": t3, "speed": 10, "current_ramp": True},
        {"duration": t3, "speed": 10, "current_ramp": False},
        {"duration": t2, "speed": 0, "current_ramp": False}
    ]

    with open(file_path, mode='w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["micros", "voltage", "current", "velocita", "lat", "lon"])

        while True:
            for step in sequence:
                step_start = time.time()
                current = 0.0
                while (time.time() - step_start) * 1000 < step["duration"]:
                    now = time.time()
                    payload["micro"] = int((now - start_time) * 1e6)
                    payload["speed"] = step["speed"]
                    if step["current_ramp"]:
                        current += i
                        payload["current"] = current
                    else:
                        payload["current"] = 0.0

                    writer.writerow([payload["micro"], payload["voltage"], payload["current"],
                                    payload["speed"], payload["lat"], payload["lon"]])
                    csvfile.flush()

                    # aggiornamento dashboard
                    dashboard_vel = payload["speed"]
                    dashboard_volt = payload["voltage"]
                    dashboard_curr = payload["current"]
                    dashboard_time = payload["micro"] / 1e6

                    time.sleep(delta / 1000)  # simula intervallo invio pacchetto
        






# --- AVVIO DASHBOARD E SIMULAZIONE ---
threading.Thread(target=simulate_loop, daemon=True).start()
update_dashboard()
root.mainloop()
