# am_gui_real_time.py
import sys
import time
import threading
from collections import deque

import numpy as np
import serial
import pyqtgraph as pg
from PyQt5.QtWidgets import QApplication
from scipy.signal import butter, filtfilt, sosfiltfilt

# ---------------- Configuración ----------------
PORT = 'COM6'          # Cambia por tu puerto, p.ej. '/dev/ttyUSB0'
BAUD = 115200
fs = 5000              # Debe coincidir con Arduino -> 5000 Hz
N = 8192               # Tamaño del buffer / FFT (aprox 1.6 s a 5 kHz)
buffer = deque([0.0]*N, maxlen=N)

# Filtro envolvente (pasa-bajas)
fc_envolvente = 120.0  # cutoff para suavizar la envolvente (Hz). tu envolvente es 50 Hz -> 120 está bien
orden_lp = 4

# Bandpass alrededor de la portadora (para eliminar DC y componentes muy bajas)
bp_low = 300.0
bp_high = 2000.0

# ---------------- Iniciar Serial ----------------
ser = serial.Serial(PORT, BAUD, timeout=0.5)

# ---------------- Qt / pyqtgraph ----------------
app = QApplication([])
win = pg.GraphicsLayoutWidget(title="AM - lectura en tiempo real")
win.resize(1200, 800)

# -- Señal (filtrada alrededor de la portadora) --
p1 = win.addPlot(title="Señal (banda portadora) y Envolvente")
p1.showGrid(x=True, y=True)
curve_signal = p1.plot(pen='c', name='señal')
curve_env = p1.plot(pen='g', name='envolvente')
label_rms = pg.TextItem(anchor=(0,1), color='w')
p1.addItem(label_rms)

# -- FFT --
win.nextRow()
p2 = win.addPlot(title="Espectro (dB)")
p2.setLogMode(x=False, y=False)
p2.showGrid(x=True, y=True)
curve_fft = p2.plot(pen='m')
label_peaks = pg.TextItem(anchor=(0,1), color='y')
p2.addItem(label_peaks)

# ---------------- Funciones DSP ----------------
def make_sos_bp(low, high, fs, order=4):
    return butter(order, [low/(fs/2), high/(fs/2)], btype='band', output='sos')

def make_ba_lp(fc, fs, order=4):
    return butter(order, fc/(fs/2), btype='low', output='ba')

sos_bp = make_sos_bp(bp_low, bp_high, fs, order=4)
b_lp, a_lp = make_ba_lp(fc_envolvente, fs, orden_lp)

# ---------------- Hilo de lectura Serial ----------------
def leer_serial():
    while True:
        try:
            data = ser.read(2)  # leemos 2 bytes (little-endian)
            if len(data) == 2:
                valor = int.from_bytes(data, byteorder='little', signed=False)
                volt = (valor / 1023.0) * 5.0
                buffer.append(volt)
            else:
                # si se recibe menos de 2 bytes, esperar un poco
                time.sleep(0.0005)
        except Exception as e:
            # no romper la hebra por excepciones momentáneas
            # print("Serial read error:", e)
            time.sleep(0.01)

threading.Thread(target=leer_serial, daemon=True).start()

# ---------------- Procesamiento para cada actualización ----------------
def procesar(datos):
    # 1) Filtrado banda (centro portadora) para aislar la portadora
    datos_bp = sosfiltfilt(sos_bp, datos)

    # 2) Envolvente: abs + filtro pasa-bajas (suavizado)
    envol = filtfilt(b_lp, a_lp, np.abs(datos_bp))

    return datos_bp, envol

def obtener_fft_db(signal):
    n = len(signal)
    if n < 4:
        return np.array([]), np.array([])
    windowed = (signal - np.mean(signal)) * np.hanning(n)
    fft = np.fft.rfft(windowed)
    fft_mag = np.abs(fft)
    fft_mag[fft_mag == 0] = 1e-12
    fft_db = 20 * np.log10(fft_mag / np.max(fft_mag))
    freqs = np.fft.rfftfreq(n, d=1.0/fs)
    return freqs, fft_db

def top_n_peaks(freqs, fft_mag_db, n=3, min_freq=1.0):
    # devuelve las n frecuencias con mayor magnitud (ignorando DC si min_freq>0)
    valid = freqs >= min_freq
    if not np.any(valid):
        return []
    mags = fft_mag_db.copy()
    mags[~valid] = -1e6
    idx = np.argsort(mags)[-n:][::-1]
    peaks = [(freqs[i], mags[i]) for i in idx]
    return peaks

# ---------------- Actualización gráfica ----------------
def actualizar():
    datos_np = np.array(buffer)
    if len(datos_np) < 256:
        return

    signal_bp, envolvente = procesar(datos_np)

    # RMS
    rms_val = np.sqrt(np.mean(signal_bp**2))

    # FFT
    freqs, fft_db = obtener_fft_db(signal_bp)
    peaks = top_n_peaks(freqs, fft_db, n=3, min_freq=5.0)

    # Actualizar curvas
    curve_signal.setData(signal_bp)
    curve_env.setData(envolvente)
    curve_fft.setData(freqs, fft_db)

    # Labels
    label_rms.setText(f"RMS: {rms_val:.3f} V")
    label_rms.setPos(0, np.max(signal_bp))

    txt_peaks = "Top-3 frecuencias (Hz, dB):\n" + "\n".join([f"{f:.1f} Hz, {amp:.1f} dB" for f, amp in peaks])
    label_peaks.setText(txt_peaks)
    label_peaks.setPos(freqs.max()*0.02, -10)

# Timer Qt
timer = pg.QtCore.QTimer()
timer.timeout.connect(actualizar)
timer.start(40)  # ~25 fps de actualización de GUI

win.show()
sys.exit(app.exec_())
