# Ground Station for Battery Telemetry

import serial
import time
import threading
from collections import deque
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import tkinter as tk
from tkinter.scrolledtext import ScrolledText
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

# -------------------------------------------------
# CONFIG
Port = "COM7"
BAUD = 9600
MAX_DATA = 1000
REFRESH = 100
TIME_WINDOW = 60   # secondi visibili

# -------------------------------------------------
# BUFFERS
time_data = deque(maxlen=MAX_DATA)
current_data = deque(maxlen=MAX_DATA)
voltage_data = deque(maxlen=MAX_DATA)
soc_data = deque(maxlen=MAX_DATA)
temp_data = deque(maxlen=MAX_DATA)

mode = "Normal"
start_time = time.time()

# -------------------------------------------------
# GUI
root = tk.Tk()
root.title("Battery Telemetry")

# -------------------------------------------------
# TERMINAL
terminal = ScrolledText(
    root,
    width=60,
    height=20,
    state='disabled',
    font=("Consolas", 14)
)

terminal.pack(side="left", fill="both", expand=True)


def log_msg(msg):
    terminal.config(state="normal")
    terminal.insert(tk.END, msg + "\n")
    terminal.see(tk.END)
    terminal.config(state="disabled")


# -------------------------------------------------
# PLOTS
fig, axs = plt.subplots(4, 1, figsize=(6, 10))
lines = []

labels = ["Current [A]", "Voltage [V]", "SOC [%]", "Temp [°C]"]
data_lists = [current_data, voltage_data, soc_data, temp_data]

for ax, label in zip(axs, labels):

    line, = ax.plot([], [], lw=2)
    ax.set_ylabel(label)
    ax.grid(True)

    # BLOCCA AUTOSCALE
    ax.set_autoscale_on(False)

    lines.append(line)

axs[-1].set_xlabel("Time [s]")


# Y LIMITS FISSI
axs[0].set_ylim(-5, 5)
axs[1].set_ylim(2.5, 4.3)
axs[2].set_ylim(0, 100)
axs[3].set_ylim(0, 60)


# -------------------------------------------------
# UPDATE
def update_plot(frame):

    if not time_data:
        return lines

    # Update lines
    for line, data in zip(lines, data_lists):
        line.set_data(time_data, data)

    # X scorre manuale
    tmax = time_data[-1]
    tmin = max(0, tmax - TIME_WINDOW)

    for ax in axs:
        ax.set_xlim(tmin, tmax)

    fig.suptitle(f"Mode: {mode}", fontsize=14)

    return lines


# -------------------------------------------------
# ANIMATION
ani = FuncAnimation(fig, update_plot, interval=REFRESH)

canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().pack(side="right", fill="both", expand=True)


# -------------------------------------------------
# SERIAL
def read_serial():

    global mode

    try:
        ser = serial.Serial(Port, BAUD, timeout=1)
        log_msg(f"Connected to {Port}")

        while True:

            line = ser.readline().decode(errors="ignore").strip()

            if not line:
                continue

            dati = line.split(",")

            # Safe mode or other non-data message
            if len(dati) == 1:
                log_msg(line)
                continue

            try:
                mode = dati[0]

                current = float(dati[1])
                voltage = float(dati[2])
                temp = float(dati[3])
                soc = float(dati[4])

                t = time.time() - start_time

                time_data.append(t)
                current_data.append(current)
                voltage_data.append(voltage)
                temp_data.append(temp)
                soc_data.append(soc)

                log_msg(
                    f"{mode} | "
                    f"I={current:.2f}A | "
                    f"V={voltage:.2f}V | "
                    f"SOC={soc:.1f}% | "
                    f"T={temp:.1f}C"
                )

            except Exception as e:
                log_msg("Parse error: " + str(e))

    except Exception as e:
        log_msg("Serial error: " + str(e))


# -------------------------------------------------
# THREAD
t = threading.Thread(target=read_serial, daemon=True)
t.start()


# -------------------------------------------------
# START
root.mainloop()