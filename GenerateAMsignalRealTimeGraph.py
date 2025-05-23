import serial
import numpy as np
import matplotlib.pyplot as plt
import time

# Configuración del puerto serial
puerto = serial.Serial('COM5', 115200)  # Ajusta el COM según tu sistema
time.sleep(2)  # Espera para establecer conexión

# Parámetros
ventana = 256  # Número de muestras por ventana (para el gráfico y la FFT)
fs_muestras = []  # Lista para calcular frecuencia de muestreo real

# Preparar la gráfica
plt.ion()
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))
linea1, = ax1.plot([], [], color='blue')
linea2, = ax2.plot([], [], color='red')
ax1.set_title('Señal en tiempo real')
ax2.set_title('FFT en tiempo real')
ax1.set_ylim(0, 1024)
ax1.set_xlim(0, ventana)
ax2.set_xlim(0, 500)  # Frecuencia hasta 500 Hz
ax2.set_ylim(0, 1000)

# Variable de control
running = True

def on_close(event):
    global running
    running = False

fig.canvas.mpl_connect('close_event', on_close)

# Bucle de lectura
datos = []
try:
    while running:
        if puerto.in_waiting:
            inicio = time.time()
            valor = puerto.readline().decode().strip()
            try:
                dato = int(valor)
                datos.append(dato)
                if len(datos) >= ventana:
                    # Tiempo entre muestras
                    duracion = time.time() - inicio
                    fs_muestras.append(1/duracion)

                    # Actualizar señal
                    x = np.arange(ventana)
                    y = datos[-ventana:]
                    linea1.set_data(x, y)

                    # FFT
                    fft = np.abs(np.fft.fft(y))
                    freqs = np.fft.fftfreq(ventana, d=1/np.mean(fs_muestras[-10:]))
                    linea2.set_data(freqs[:ventana//2], fft[:ventana//2])

                    # Redibujar
                    ax1.set_ylim(min(y)-10, max(y)+10)
                    ax2.set_ylim(0, max(fft[:ventana//2])+10)
                    fig.canvas.draw()
                    fig.canvas.flush_events()
            except ValueError:
                pass

except KeyboardInterrupt:
    print("Interrupción manual.")

finally:
    puerto.close()
    print("Puerto serial cerrado.")