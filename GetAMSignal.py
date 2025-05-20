import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np
import collections

# CONFIGURA ESTO:
puerto_serial = 'COM5'  # Cambia al puerto correcto
baudrate = 115200
fs = 1000  # Frecuencia de muestreo (aproximada, según delay en Arduino)

# Conectar al puerto serial
ser = serial.Serial(puerto_serial, baudrate)
print(f"Conectado a {puerto_serial}")

# Buffer circular para la señal (potencia de 2 para FFT eficiente)
N = 256
buffer = collections.deque([0]*N, maxlen=N)

# Configurar figura con dos subgráficas
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))

# Señal en el tiempo
linea_senal, = ax1.plot(range(N), [0]*N)
ax1.set_title("Señal analógica en tiempo real")
ax1.set_ylim(0, 1100)
ax1.set_ylabel("Valor ADC")
ax1.set_xlabel("Muestras")
ax1.grid(True)

# FFT en tiempo real
frecuencias = np.fft.fftfreq(N, 1/fs)
linea_fft, = ax2.plot(frecuencias[:N//2], [0]*(N//2))
ax2.set_title("FFT (frecuencia en tiempo real)")
ax2.set_ylim(0, 10000)
ax2.set_xlim(0, fs/2)
ax2.set_ylabel("Magnitud")
ax2.set_xlabel("Frecuencia (Hz)")
ax2.grid(True)

def actualizar(frame):
    try:
        if ser.in_waiting:
            dato = ser.readline().decode().strip()
            valor = int(dato)
            buffer.append(valor)

            # Actualizar señal
            linea_senal.set_ydata(buffer)

            # Calcular FFT y actualizar gráfica
            senal_np = np.array(buffer)
            fft = np.abs(np.fft.fft(senal_np - np.mean(senal_np)))  # Remover DC
            linea_fft.set_ydata(fft[:N//2])
    except:
        pass

    return linea_senal, linea_fft

ani = FuncAnimation(fig, actualizar, interval=10)
plt.tight_layout()
plt.show()

ser.close()