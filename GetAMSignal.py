import pyqtgraph as pg
from PyQt5.QtWidgets import QApplication
import numpy as np
import serial
from collections import deque
import sys
import threading
import time
from scipy.signal import butter, filtfilt

# Configuración
fs = 16000            # Frecuencia de muestreo (Hz)
N = 25000             # Número de muestras para la FFT
buffer = deque([0]*N, maxlen=N)

# Filtro Butterworth pasa bajos
fc = 100              # Frecuencia de corte del filtro (Hz)
orden = 4
b, a = butter(orden, fc / (fs / 2), btype='low')

# Serial
ser = serial.Serial('COM6', 115200)

# Qt App
app = QApplication([])
win = pg.GraphicsLayoutWidget(title="Señal AM en Tiempo Real")
win.resize(1000, 600)

# Señal en el tiempo
plot1 = win.addPlot(title="Señal en Voltaje (Tiempo Real)")
plot1.setYRange(2, 4)
curve1 = plot1.plot(pen='c')

# FFT
win.nextRow()
plot2 = win.addPlot(title="FFT en Tiempo Real")
plot2.setXRange(0, 200)
plot2.setYRange(-60, 0)  # Mostrar dB desde -60 a 0 (0 dB es el pico)
curve2 = plot2.plot(pen='m')
frecuencias = np.fft.fftfreq(N, 1/fs)[:N//2]

# Suavizado exponencial para FFT
fft_db_suavizada = np.zeros(N//2)
alpha = 0.2  # Factor de suavizado (0 = mucho suavizado, 1 = sin suavizado)

# Hilo de lectura serial
def leer_serial():
    while True:
        try:
            if ser.in_waiting:
                byte = ser.read(1)
                valor = int.from_bytes(byte, byteorder='big')
                voltaje = (valor / 255.0) * 5.0
                buffer.append(voltaje)
        except:
            pass
        time.sleep(0.0005)

# Iniciar hilo
hilo_serial = threading.Thread(target=leer_serial, daemon=True)
hilo_serial.start()

# Actualizar gráficos
def actualizar():
    global fft_db_suavizada

    datos_np = np.array(buffer)

    # Filtrar señal
    if len(datos_np) > orden:
        datos_filtrados = filtfilt(b, a, datos_np)
    else:
        datos_filtrados = datos_np

    # Señal en el tiempo
    curve1.setData(datos_filtrados)

    # Quitar offset y aplicar ventana
    ventana = np.hanning(len(datos_filtrados))
    datos_ventaneados = (datos_filtrados - np.mean(datos_filtrados)) * ventana

    # Calcular FFT
    fft = np.abs(np.fft.fft(datos_ventaneados))[:N//2]
    fft /= np.max(fft) + 1e-12  # Normalizar a 0 dB
    fft_db = 20 * np.log10(fft + 1e-12)

    # Limitar a rango visible y aplicar suavizado
    fft_db = np.clip(fft_db, -60, 0)
    fft_db_suavizada = alpha * fft_db + (1 - alpha) * fft_db_suavizada

    # Actualizar curva FFT
    curve2.setData(frecuencias, fft_db_suavizada)

# Timer de actualización
timer = pg.QtCore.QTimer()
timer.timeout.connect(actualizar)
timer.start(120)

# Mostrar ventana
win.show()
sys.exit(app.exec_())