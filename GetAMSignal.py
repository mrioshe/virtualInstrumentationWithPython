import numpy as np
import matplotlib.pyplot as plt
import serial
import time

# Parámetros
fs = 1000  # Frecuencia de muestreo, debe coincidir con el Arduino
duracion = 2  # Duración esperada en segundos
n_muestras = fs * duracion

# Configura el puerto serial (ajusta el nombre del puerto)
puerto = serial.Serial('COM5', 115200)
time.sleep(2)  # Espera para asegurar conexión

datos_recibidos = []

print("Recibiendo datos...")

while len(datos_recibidos) < n_muestras:
    if puerto.in_waiting > 0:
        linea = puerto.readline().decode('utf-8').strip()
        try:
            valor = int(linea)
            datos_recibidos.append(valor)
        except ValueError:
            continue  # Ignora líneas inválidas

puerto.close()
print("Recepción completa.")

# Convertir a arreglo numpy
senal_adc = np.array(datos_recibidos)

# Vector de tiempo
t = np.linspace(0, duracion, len(senal_adc))

# Normaliza la señal (0–5V si es una entrada de 10 bits)
senal_voltaje = (senal_adc / 1023) * 5

# FFT
fft_senal = np.fft.fft(senal_voltaje)
frecuencias = np.fft.fftfreq(len(t), 1/fs)
n = len(t) // 2  # Solo mitad positiva

# --- Gráficas ---
plt.figure(figsize=(12, 6))

plt.subplot(2, 1, 1)
plt.plot(t, senal_voltaje, color='blue')
plt.title('Señal recibida desde Arduino')
plt.xlabel('Tiempo (s)')
plt.ylabel('Voltaje (V)')
plt.grid(True)

plt.subplot(2, 1, 2)
plt.plot(frecuencias[:n], np.abs(fft_senal[:n]), color='purple')
plt.title('FFT de la señal recibida')
plt.xlabel('Frecuencia (Hz)')
plt.ylabel('Magnitud')
plt.grid(True)

plt.tight_layout()
plt.show()