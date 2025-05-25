import numpy as np
import matplotlib.pyplot as plt
import serial
import time

# Parámetros de la señal
fs = 2000       # Frecuencia de muestreo (Hz)
duracion = 30    # Duración en segundos
fc = 120        # Frecuencia de la portadora (Hz)
fm = 8        # Frecuencia de la envolvente (Hz)
Am = 0.15        # Amplitud de la envolvente
Ac = 0.3          # Amplitud de la portadora
offset = 2.5     # Offset para mantener la señal en positivo

# Vector de tiempo
t = np.linspace(0, duracion, int(fs * duracion))

# Señales
envolvente = Am * np.sin(2 * np.pi * fm * t)
portadora = Ac * np.sin(2 * np.pi * fc * t)
m = Am / Ac
senal_am = Ac * (1 + m * np.sin(2 * np.pi * fm * t)) * np.sin(2 * np.pi * fc * t)

# Señal con offset
senal_am_offset = senal_am + offset

# Envío por puerto serial (ajusta COM7)
puerto = serial.Serial('COM9', 9600)
time.sleep(2)

# Enviar los valores de la señal AM con offset
for valor in senal_am_offset:
    mensaje = f"{valor:.4f}\n"  # Enviar con 4 decimales de precisión
    puerto.write(mensaje.encode())
    time.sleep(1/fs)

puerto.close()
print("Señal enviada completamente.")

# Gráficas en el dominio del tiempo
plt.figure(figsize=(12, 10))

plt.subplot(5, 1, 1)
plt.plot(t, envolvente, color='orange')
plt.title('Envolvente (10 Hz)')
plt.ylabel('Amplitud')
plt.grid(True)

plt.subplot(5, 1, 2)
plt.plot(t, portadora, color='red')
plt.title('Portadora (60 Hz)')
plt.ylabel('Amplitud')
plt.grid(True)

plt.subplot(5, 1, 3)
plt.plot(t, senal_am, color='blue')
plt.title('Señal AM (sin offset)')
plt.ylabel('Amplitud')
plt.grid(True)

plt.subplot(5, 1, 4)
plt.plot(t, senal_am_offset, color='purple')
plt.title('Señal AM con offset (+2)')
plt.ylabel('Amplitud')
plt.grid(True)

plt.subplot(5, 1, 5)
plt.plot(t, senal_am_offset, color='green')
plt.title('Señal AM enviada al ESP32')
plt.xlabel('Tiempo (s)')
plt.ylabel('Amplitud')
plt.grid(True)

plt.tight_layout()
plt.show()

# FFT de la señal AM con offset
fft_offset = np.fft.fft(senal_am_offset)
frecuencias = np.fft.fftfreq(len(t), 1/fs)
n = len(t) // 2

# Gráfica de la FFT
plt.figure(figsize=(12, 6))

plt.subplot(1, 1, 1)
plt.plot(frecuencias[:n], np.abs(fft_offset[:n]), color='darkblue')
plt.title('FFT de la Señal AM con Offset')
plt.xlabel('Frecuencia (Hz)')
plt.ylabel('Magnitud')
plt.grid(True)

plt.tight_layout()
plt.show()