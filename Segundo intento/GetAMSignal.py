import pyqtgraph as pg
from PyQt5.QtWidgets import QApplication
import numpy as np
import serial
from collections import deque
import sys
import threading
import time
from scipy.signal import butter, filtfilt, sosfiltfilt

# Configuración
fs = 5000
N = 25000
buffer = deque([0]*N, maxlen=N)

# Filtros
fc_baja = 15
orden = 4

# Filtro pasa-bajos para la envolvente
sos_bajas = butter(orden, fc_baja / (fs / 2), btype='low', output='sos')

# Serial
ser = serial.Serial('COM8', 115200)

# Qt App
app = QApplication([])
win = pg.GraphicsLayoutWidget(title="Análisis de Señal AM")
win.resize(1000, 600)

# === Señal original ===
plot_original = win.addPlot(title="Señal AM Filtrada (Original)")
plot_original.setYRange(2, 4)
curve_original = plot_original.plot(pen='c')
label_original = pg.TextItem(color='c')
plot_original.addItem(label_original)

# === Envolvente ===
win.nextRow()
plot_bajas = win.addPlot(title="Componente de Baja Frecuencia (<15Hz)")
plot_bajas.setYRange(2, 4)
curve_bajas = plot_bajas.plot(pen='g')
label_bajas = pg.TextItem(color='g')
plot_bajas.addItem(label_bajas)

# === FFT ===
win.nextRow()
plot_fft = win.addPlot(title="Espectro de Frecuencia")
plot_fft.setXRange(0, 200)
plot_fft.setYRange(-60, 0)
curve_fft = plot_fft.plot(pen='m')
frecuencias = np.fft.fftfreq(N, 1/fs)[:N//2]

# Hilo de lectura
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

threading.Thread(target=leer_serial, daemon=True).start()

# Procesamiento
def procesar_componentes(datos):
    b, a = butter(4, 120/(fs/2), btype='low')
    datos_filtrados = filtfilt(b, a, datos)
    envolvente = sosfiltfilt(sos_bajas, datos_filtrados)
    return datos_filtrados, envolvente

# Frecuencia dominante
def frecuencia_dominante(signal):
    windowed = (signal - np.mean(signal)) * np.hanning(len(signal))
    fft_vals = np.abs(np.fft.fft(windowed))[:N//2]
    idx = np.argmax(fft_vals)
    return frecuencias[idx]

# Actualización gráfica
def actualizar():
    datos_np = np.array(buffer)
    if len(datos_np) < orden * 3:
        return

    original, bajas = procesar_componentes(datos_np)

    curve_original.setData(original)
    curve_bajas.setData(bajas)

    f1 = frecuencia_dominante(original)
    f2 = frecuencia_dominante(bajas)

    label_original.setText(f"f = {f1:.1f} Hz")
    label_original.setPos(N * 0.8, original[int(N*0.8)])

    label_bajas.setText(f"f = {f2:.1f} Hz")
    label_bajas.setPos(N * 0.8, bajas[int(N*0.8)])

    # FFT
    windowed = (original - np.mean(original)) * np.hanning(len(original))
    fft = np.abs(np.fft.fft(windowed))[:N//2]
    fft_db = 20 * np.log10(fft / np.max(fft) + 1e-12)
    fft_db = np.clip(fft_db, -60, 0)
    curve_fft.setData(frecuencias, fft_db)

# Timer
timer = pg.QtCore.QTimer()
timer.timeout.connect(actualizar)
timer.start(20)

win.show()
sys.exit(app.exec_())
