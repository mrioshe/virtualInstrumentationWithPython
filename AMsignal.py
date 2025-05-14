import numpy as np
import matplotlib.pyplot as plt
import serial
import time

# Parámetros de la señal
fs = 1000       # Frecuencia de muestreo (Hz)
duracion = 2    # Duración en segundos
fc = 60         # Frecuencia de la portadora (Hz)
fm = 10        # Frecuencia de la envolvente (Hz)
Am = 0.5        # Amplitud de la envolvente
Ac = 1          # Amplitud de la portadora
offset = 2   # Offset para mantener la señal en positivo

# Vector de tiempo
t = np.linspace(0, duracion, int(fs * duracion))

# Señales
envolvente = Am * np.sin(2 * np.pi * fm * t)
portadora = Ac * np.sin(2 * np.pi * fc * t)
m = Am / Ac
senal_am = Ac * (1 + m * np.sin(2 * np.pi * fm * t)) * np.sin(2 * np.pi * fc * t)

# Señal con offset
senal_am_offset = senal_am + offset

# Normalización para PWM
senal_am_normalizada = np.interp(senal_am_offset, (min(senal_am_offset), max(senal_am_offset)), (0, 255))

# Envío por puerto serial (ajusta COMX)
puerto = serial.Serial('COM5', 9600)
time.sleep(2)

for valor in senal_am_normalizada:
    mensaje = f"{int(valor)}\n"
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
plt.plot(t, senal_am_normalizada, color='green')
plt.title('PWM normalizada enviada al Arduino')
plt.xlabel('Tiempo (s)')
plt.ylabel('PWM (0–255)')
plt.grid(True)

plt.tight_layout()
plt.show()

# FFT de la señal normalizada
fft_normalizada = np.fft.fft(senal_am_normalizada)
fft_am = np.fft.fft(senal_am)
frecuencias = np.fft.fftfreq(len(t), 1/fs)
n = len(t) // 2

# Gráfica de FFTs
plt.figure(figsize=(12, 6))

plt.subplot(2, 1, 1)
plt.plot(frecuencias[:n], np.abs(fft_normalizada[:n]), color='darkblue')
plt.title('FFT de la Señal PWM Normalizada')
plt.xlabel('Frecuencia (Hz)')
plt.ylabel('Magnitud')
plt.grid(True)

plt.subplot(2, 1, 2)
plt.plot(frecuencias[:n], np.abs(fft_am[:n]), color='darkred') 
plt.title('FFT de la Señal AM Original (sin offset, sin normalizar)')
plt.xlabel('Frecuencia (Hz)')
plt.ylabel('Magnitud')
plt.grid(True)

plt.tight_layout()
plt.show()