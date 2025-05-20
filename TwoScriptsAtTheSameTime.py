import subprocess

# Ejecuta script1.py en segundo plano
subprocess.Popen(["python", "AMsignalESP32.py"])

# Ejecuta script2.py también en segundo plano
subprocess.Popen(["python", "GetAMSignal.py"])