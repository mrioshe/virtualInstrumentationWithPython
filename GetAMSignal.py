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
fc = 150              # Frecuencia de corte del filtro (Hz)
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
curve1 = plot1.plot(pen='c')  # Señal filtrada

# FFT
win.nextRow()
plot2 = win.addPlot(title="FFT en Tiempo Real")
plot2.setXRange(0, 200)
plot2.setYRange(-60, 0)
curve2 = plot2.plot(pen='m')
frecuencias = np.fft.fftfreq(N, 1/fs)[:N//2]

# Indicadores de frecuencia
linea_portadora = pg.InfiniteLine(angle=90, pen=pg.mkPen('y', width=2), movable=False)
linea_envolvente = pg.InfiniteLine(angle=90, pen=pg.mkPen('g', width=2), movable=False)
plot2.addItem(linea_portadora)
plot2.addItem(linea_envolvente)

# Suavizado exponencial para FFT
fft_db_suavizada = np.zeros(N//2)
alpha = 0.2

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

hilo_serial = threading.Thread(target=leer_serial, daemon=True)
hilo_serial.start()

# Actualizar gráficos
def actualizar():
    global fft_db_suavizada

    datos_np = np.array(buffer)

    if len(datos_np) > orden:
        datos_filtrados = filtfilt(b, a, datos_np)
    else:
        datos_filtrados = datos_np

    # Señal en el tiempo (filtrada)
    curve1.setData(datos_filtrados)

    # Señal original (para portadora)
    ventana = np.hanning(len(datos_np))
    datos_ventaneados_original = (datos_np - np.mean(datos_np)) * ventana
    fft_original = np.abs(np.fft.fft(datos_ventaneados_original))[:N//2]
    fft_original /= np.max(fft_original) + 1e-12

    # Señal filtrada (para envolvente)
    ventana_f = np.hanning(len(datos_filtrados))
    datos_ventaneados = (datos_filtrados - np.mean(datos_filtrados)) * ventana_f
    fft = np.abs(np.fft.fft(datos_ventaneados))[:N//2]
    fft /= np.max(fft) + 1e-12
    fft_db = 20 * np.log10(fft + 1e-12)

    fft_db = np.clip(fft_db, -60, 0)
    fft_db_suavizada = alpha * fft_db + (1 - alpha) * fft_db_suavizada

    # Actualizar curva FFT
    curve2.setData(frecuencias, fft_db_suavizada)

    # Detectar frecuencia portadora
    idx_max_portadora = np.argmax(fft_original)
    freq_portadora = frecuencias[idx_max_portadora]
    linea_portadora.setValue(freq_portadora)

    # Detectar frecuencia envolvente (solo bajas frecuencias)
    rango_envolvente = frecuencias < 300
    idx_max_env = np.argmax(fft[rango_envolvente])
    freq_envolvente = frecuencias[rango_envolvente][idx_max_env]
    linea_envolvente.setValue(freq_envolvente)

# Timer de actualización
timer = pg.QtCore.QTimer()
timer.timeout.connect(actualizar)
timer.start(15)

win.show()
sys.exit(app.exec_())
