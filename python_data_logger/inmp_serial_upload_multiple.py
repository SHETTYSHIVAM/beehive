import serial
import wave
import os
from datetime import datetime


# ==========================================
# Configuration
# ==========================================

SERIAL_PORT = "COM5"
BAUD_RATE = 921600

SAMPLE_RATE = 16000
CHANNELS = 1
SAMPLE_WIDTH = 2          # 16-bit = 2 bytes

RECORD_SECONDS = 5

OUTPUT_FOLDER = "bee_recordings"


EXPECTED_BYTES = (
    SAMPLE_RATE
    * RECORD_SECONDS
    * SAMPLE_WIDTH
)


# ==========================================
# Create output folder
# ==========================================

os.makedirs(OUTPUT_FOLDER, exist_ok=True)

print(f"Recordings will be saved in: {os.path.abspath(OUTPUT_FOLDER)}")


# ==========================================
# Connect to ESP32
# ==========================================

print(f"\nConnecting to {SERIAL_PORT}...")

ser = serial.Serial(
    SERIAL_PORT,
    BAUD_RATE,
    timeout=5
)

print("Connected!")


# ==========================================
# Clear old serial data
# ==========================================

ser.reset_input_buffer()


# ==========================================
# Wait for AUDIO_START
# ==========================================

print("\nWaiting for AUDIO_START...")

while True:

    line = ser.readline().decode(
        errors="ignore"
    ).strip()

    if line:
        print("ESP32:", line)

    if line == "AUDIO_START":
        break


print("\nRecording started.")
print("Press Ctrl+C to stop.\n")


# ==========================================
# Continuous recording loop
# ==========================================

recording_number = 1

try:

    while True:

        print(
            f"Recording #{recording_number} "
            f"({RECORD_SECONDS} seconds)..."
        )

        # --------------------------------------
        # Receive one audio segment
        # --------------------------------------

        audio_data = bytearray()

        while len(audio_data) < EXPECTED_BYTES:

            remaining = (
                EXPECTED_BYTES
                - len(audio_data)
            )

            chunk = ser.read(
                min(4096, remaining)
            )

            if chunk:
                audio_data.extend(chunk)

                progress = (
                    len(audio_data)
                    / EXPECTED_BYTES
                    * 100
                )

                print(
                    f"\rProgress: {progress:5.1f}%",
                    end=""
                )

        print()

        # --------------------------------------
        # Generate filename
        # --------------------------------------

        timestamp = datetime.now().strftime(
            "%Y%m%d_%H%M%S_%f"
        )

        filename = (
            f"bee_audio_{timestamp}.wav"
        )

        filepath = os.path.join(
            OUTPUT_FOLDER,
            filename
        )

        # --------------------------------------
        # Save WAV
        # --------------------------------------

        with wave.open(filepath, "wb") as wav:

            wav.setnchannels(CHANNELS)

            wav.setsampwidth(SAMPLE_WIDTH)

            wav.setframerate(SAMPLE_RATE)

            wav.writeframes(audio_data)

        print(
            f"Saved: {filepath}"
        )

        print()

        recording_number += 1


# ==========================================
# Stop with Ctrl+C
# ==========================================

except KeyboardInterrupt:

    print("\n\nStopping recording...")


# ==========================================
# Cleanup
# ==========================================

finally:

    ser.close()

    print("Serial connection closed.")

    print(
        f"Recordings saved in: "
        f"{os.path.abspath(OUTPUT_FOLDER)}"
    )

    print("Done.")