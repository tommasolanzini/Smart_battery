# Battery Logger - CSV (Robusto)

import serial
import time
import datetime
import os


# -------------------------------------------------
# CONFIG
PORT = "COM4"          # Cambia se serve
BAUDRATE = 9600

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
SAVE_FOLDER = os.path.join(BASE_DIR, "Battery Testing")


# -------------------------------------------------
# FILE NAME
def create_file_name():

    now = datetime.datetime.now()
    return now.strftime("test_%Y%m%d_%H%M%S.csv")


# -------------------------------------------------
def main():

    # Create folder
    os.makedirs(SAVE_FOLDER, exist_ok=True)

    filename = create_file_name()
    path = os.path.join(SAVE_FOLDER, filename)

    print("File:", path)

    # Open serial
    try:
        ser = serial.Serial(PORT, BAUDRATE, timeout=1)
    except Exception as e:
        print("Serial error:", e)
        return

    time.sleep(2)

    print("Connected on", PORT)
    print("Logging... CTRL+C to stop\n")

    start_time = time.time()

    # Open file
    with open(path, "w", buffering=1) as f:

        # CSV HEADER
        f.write("Time (s), Current (A), Voltage (V), Temperature (°C)\n")

        try:

            while True:

                # Read serial for debug
               # raw = ser.readline().decode(errors="ignore").strip()

                # Debug: show raw data
                if raw:
                    print("RAW >>", raw)

                if not raw:
                    continue

                # Split + clean
                data = [x.strip() for x in raw.split(",")]

                # Need at least 5 fields
                # if len(data) < 5:
                #     print("Bad packet:", data)
                #     continue

                try:

                    current = float(data[1])
                    voltage = float(data[2])
                    temp = float(data[3])


                    t = time.time() - start_time

                    csv = (
                        f"{t:.2f},"
                        f"{current:.2f},"
                        f"{voltage:.2f},"
                        f"{temp:.2f},"
                    )
                    # for debug
                    # print("LOG >>", csv)

                    f.write(csv + "\n")

                except Exception as e:

                    print("Parse error:", e, "->", data)

        except KeyboardInterrupt:

            print("\nStopped by user.")

        finally:

            ser.close()

            print("Serial closed.")
            print("Saved:", path)


# -------------------------------------------------
if __name__ == "__main__":
    main()
