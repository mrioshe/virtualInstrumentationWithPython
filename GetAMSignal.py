import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque
import serial

# Parámetros
N = 2000              # Número de muestras a mostrar
fs = 1000             # Frecuencia de muestreo (Hz)
buffer = deque([0]*N, maxlen=N)  # Buffer deslizante
ser = serial.Serial('COM6', 9600)  # Ajusta el puerto

# Configuración de la figura
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))

# Señal en el tiempo
linea_senal, = ax1.plot(range(N), [0]*N)
ax1.set_title("Señal en voltaje (tiempo real)")
ax1.set_ylim(0, 5.5)
ax1.set_xlim(0, N)  # <-- Aseguramos el eje X de 0 a 2000 muestras
ax1.set_ylabel("Voltaje (V)")
ax1.set_xlabel("Muestras")
ax1.grid(True)

# FFT en tiempo real
frecuencias = np.fft.fftfreq(N, 1/fs)
linea_fft, = ax2.plot(frecuencias[:N//2], [0]*(N//2))
ax2.set_title("FFT (frecuencia en tiempo real)")
ax2.set_ylim(0, 200)
ax2.set_xlim(0, 200)
ax2.set_ylabel("Magnitud")
ax2.set_xlabel("Frecuencia (Hz)")
ax2.grid(True)

# Función de actualización de la animación
def actualizar(frame):
    try:
        if ser.in_waiting:
            dato = ser.readline().decode().strip()
            valor = int(dato)
            voltaje = (valor / 1023.0) * 5.0  # Conversión ADC -> Voltaje
            buffer.append(voltaje)

            # Actualizar la gráfica de señal
            linea_senal.set_ydata(buffer)

            # FFT y gráfica
            senal_np = np.array(buffer)
            fft = np.abs(np.fft.fft(senal_np - np.mean(senal_np)))
            linea_fft.set_ydata(fft[:N//2])
    except:
        pass

    return linea_senal, linea_fft

# Animación en tiempo real
ani = FuncAnimation(fig, actualizar, interval=10)
plt.tight_layout()
plt.show()

ser.close()
