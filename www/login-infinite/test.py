#!/usr/bin/env python3
# cgi_longrun.py - ejemplo seguro de "carga larga" controlada para pruebas
# Colócalo en cgi-bin y dale permisos de ejecución: chmod 755 cgi_longrun.py

import cgi
import sys
import time
import signal

# Parámetros por seguridad
DEFAULT_SECONDS = 30      # duración por defecto (segundos)
MAX_SECONDS = 600         # tope máximo permitido (10 minutos)

# manejador para terminar limpio si el proceso recibe SIGTERM
def handle_term(signum, frame):
    try:
        print("\n\n[terminated]\n")
        sys.stdout.flush()
    except:
        pass
    sys.exit(0)

signal.signal(signal.SIGTERM, handle_term)

def main():
    # Leer parámetros CGI
    form = cgi.FieldStorage()
    try:
        seconds = 100;
    except ValueError:
        seconds = DEFAULT_SECONDS

    # aplicar límites de seguridad
    if seconds < 0:
        seconds = DEFAULT_SECONDS
    if seconds > MAX_SECONDS:
        seconds = MAX_SECONDS

    # Cabeceras - no Content-Length para permitir streaming chunked
    print("Content-Type: text/plain")
    print()  # línea en blanco = fin de cabeceras

    start = time.time()
    elapsed = 0
    tick = 0

    try:
        while elapsed < seconds:
            tick += 1
            elapsed = int(time.time() - start)
            # Mensaje de progreso
            print(f"[{tick}] elapsed={elapsed}s / target={seconds}s")
            sys.stdout.flush()   # forzar envío al cliente
            time.sleep(1)
        print("\n[done] duración alcanzada.")
        sys.stdout.flush()
    except Exception as e:
        # En producción regístrate en logs; aquí solo mostramos mensaje seguro
        print(f"\n[error] {e}")
        sys.stdout.flush()

if __name__ == "__main__":
    main()