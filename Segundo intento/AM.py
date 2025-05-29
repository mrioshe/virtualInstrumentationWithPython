import numpy as np
import serial
import time
import struct
import matplotlib.pyplot as plt

# Parámetros de la señal
SAMPLE_RATE = 5000            # Hz
MOD_FREQ = 10              # Hz
CARRIER_FREQ = 100             # Hz
MOD_DEPTH = 0.75               # 75%
NUM_CYCLES = 5                 # Ciclos de la onda moduladora
DURATION = NUM_CYCLES / MOD_FREQ  # Duración para ciclos completos
VOLTAGE_OFFSET = 2.0           # Offset de 2V

# Configuración del puerto serial
SERIAL_PORT = 'COM7'
BAUD_RATE = 921600
CHUNK_SIZE = 256

def generate_am_signal_no_filter():
    """Genera señal AM con offset de 2V"""
    t = np.linspace(0, DURATION, int(SAMPLE_RATE * DURATION), endpoint=False)
    
    # Señal portadora y moduladora
    carrier = np.sin(2 * np.pi * CARRIER_FREQ * t)
    modulator = np.sin(2 * np.pi * MOD_FREQ * t)

    # Señal AM sin filtro con offset de 2V
    am_signal = (1 + MOD_DEPTH * modulator) * carrier + VOLTAGE_OFFSET

    # Normalización para rango de 8 bits [1, 254]
    # Considerando que el offset centra la señal en 2V
    # La señal varía entre 2V ± (1 + MOD_DEPTH)
    max_voltage = VOLTAGE_OFFSET + (1 + MOD_DEPTH)
    min_voltage = VOLTAGE_OFFSET - (1 + MOD_DEPTH)
    
    # Mapeo lineal a 8 bits
    am_normalized = 1 + 253 * (am_signal - min_voltage) / (max_voltage - min_voltage)
    am_normalized = np.clip(am_normalized, 1, 254)

    return np.uint8(am_normalized), am_signal

def plot_comparison(original, normalized):
    """Grafica la señal original y la normalizada"""
    plt.figure(figsize=(12, 6))
    
    # Señal original con offset
    plt.subplot(2, 1, 1)
    plt.plot(original[:1000], 'b-')
    plt.title(f"Señal AM Original con offset de {VOLTAGE_OFFSET}V")
    plt.ylabel("Voltaje (V)")
    plt.grid(True)
    plt.axhline(y=VOLTAGE_OFFSET, color='r', linestyle='--', label='Offset')
    plt.legend()

    # Señal normalizada
    plt.subplot(2, 1, 2)
    plt.plot(normalized[:1000], 'r-')
    plt.title("Señal AM Normalizada para transmisión (8 bits)")
    plt.ylabel("Valor digital (1-254)")
    plt.grid(True)
    plt.tight_layout()
    plt.show()

def send_to_esp32(signal):
    """Envía la señal por puerto serial con delimitadores"""
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            time.sleep(2.5)
            print(f"Transmitiendo {len(signal)} muestras...")

            # Encabezado de inicio
            ser.write(b'\xAA\x55')
            packed = struct.pack(f'{len(signal)}B', *signal)

            chunk_delay = CHUNK_SIZE / (BAUD_RATE / 10)
            for i in range(0, len(packed), CHUNK_SIZE):
                ser.write(packed[i:i+CHUNK_SIZE])
                ser.flush()
                time.sleep(chunk_delay)

            # Final
            ser.write(b'\x55\xAA')
            print("Transmisión completada con éxito")
    except Exception as e:
        print(f"Error en transmisión: {str(e)}")

if __name__ == "__main__":
    am_filtered, am_original = generate_am_signal_no_filter()
    plot_comparison(am_original, am_filtered)

    np.save('am_original_no_filter.npy', am_original)
    np.save('am_filtered_no_filter.npy', am_filtered)

    send_to_esp32(am_filtered)