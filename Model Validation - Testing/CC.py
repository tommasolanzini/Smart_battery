# Ground Station + CSV Logger stabile con CTRL+C

import serial
import time
import threading
import os
import datetime
from collections import deque
import sys
import signal

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

import tkinter as tk
from tkinter.scrolledtext import ScrolledText
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

# ---------------- CONFIG ----------------
PORT = "COM7"
BAUD = 9600
MAX_DATA = 1000
REFRESH = 100
TIME_WINDOW = 60

# ---------------- SAVE FOLDER ----------------
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
SAVE_FOLDER = os.path.join(BASE_DIR, "Battery Testing")
os.makedirs(SAVE_FOLDER, exist_ok=True)

# ---------------- FILE NAME ----------------
def create_file_name():
    now = datetime.datetime.now()
    return now.strftime("test_%Y%m%d_%H%M%S.csv")

# ---------------- BUFFERS ----------------
time_data = deque(maxlen=MAX_DATA)
current_data = deque(maxlen=MAX_DATA)
voltage_data = deque(maxlen=MAX_DATA)
soc_data = deque(maxlen=MAX_DATA)
temp_data = deque(maxlen=MAX_DATA)

mode = "Normal"
start_time = time.time()
running = True
filepath = None

# ---------------- GUI ----------------
root = tk.Tk()
root.title("Battery Telemetry")

# Terminal
terminal = ScrolledText(root, width=60, height=20, state='disabled', font=("Consolas", 12))
terminal.pack(side="left", fill="both", expand=True)

def log_msg(msg):
    terminal.config(state="normal")
    terminal.insert(tk.END, msg + "\n")
    terminal.see(tk.END)
    terminal.config(state="disabled")

# Plots
fig, axs = plt.subplots(4, 1, figsize=(6, 10))
lines = []
labels = ["Current [A]", "Voltage [V]", "SOC [%]", "Temp [°C]"]
data_lists = [current_data, voltage_data, soc_data, temp_data]

for ax, label in zip(axs, labels):
    line, = ax.plot([], [], lw=2)
    ax.set_ylabel(label)
    ax.grid(True)
    ax.set_autoscale_on(False)
    lines.append(line)

axs[-1].set_xlabel("Time [s]")
axs[0].set_ylim(-5, 5)
axs[1].set_ylim(2.5, 4.3)
axs[2].set_ylim(0, 100)
axs[3].set_ylim(0, 60)

# Update plot
def update_plot(frame):
    if not time_data:
        return lines
    for line, data in zip(lines, data_lists):
        line.set_data(time_data, data)
    tmax = time_data[-1]
    tmin = max(0, tmax - TIME_WINDOW)
    for ax in axs:
        ax.set_xlim(tmin, tmax)
    fig.suptitle(f"Mode: {mode}", fontsize=14)
    return lines

ani = FuncAnimation(fig, update_plot, interval=REFRESH, blit=False)
canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().pack(side="right", fill="both", expand=True)

# ---------------- HANDLE EXIT ----------------
def handle_exit(signum=None, frame=None):
    global running
    running = False
    root.quit()  # chiude mainloop Tkinter

root.protocol("WM_DELETE_WINDOW", handle_exit)
signal.signal(signal.SIGINT, handle_exit)  # intercetta CTRL+C

# ---------------- SERIAL + LOGGER ----------------
def serial_thread():
    global mode, running, filepath
    filename = create_file_name()
    filepath = os.path.join(SAVE_FOLDER, filename)

    try:
        logfile = open(filepath, "w", buffering=1)
    except Exception as e:
        log_msg("File error: " + str(e))
        return

    logfile.write("time_s,mode,current,voltage,temp,soc\n")

    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
        log_msg(f"Connected to {PORT}")
    except Exception as e:
        log_msg("Serial error: " + str(e))
        logfile.close()
        return

    try:
        while running:
            line = ser.readline().decode(errors="ignore").strip()
            if not line:
                continue
            data = [x.strip() for x in line.split(",")]
            if len(data) < 5:
                log_msg("Bad packet: " + line)
                continue
            try:
                mode = data[0]
                current = float(data[1])
                voltage = float(data[2])
                temp = float(data[3])
                soc = float(data[4])
                t = time.time() - start_time

                # Save buffers
                time_data.append(t)
                current_data.append(current)
                voltage_data.append(voltage)
                temp_data.append(temp)
                soc_data.append(soc)

                # Log CSV
                csv = f"{t:.2f},{mode},{current:.3f},{voltage:.3f},{temp:.2f},{soc:.2f}"
                logfile.write(csv + "\n")

                # Terminal
                log_msg(f"{mode} | I={current:.2f}A | V={voltage:.2f}V | SOC={soc:.1f}% | T={temp:.1f}C")

            except Exception as e:
                log_msg("Parse error: " + str(e))
    finally:
        try: ser.close()
        except: pass
        try: logfile.close()
        except: pass
        print(f"\nTest ended. Data saved in {SAVE_FOLDER} with name {os.path.basename(filepath)}")
        running = False
        os._exit(0)  # esce da tutti i thread in sicurezza

# ---------------- THREAD ----------------
t = threading.Thread(target=serial_thread, daemon=True)
t.start()

# ---------------- MAIN LOOP ----------------
root.mainloop()
