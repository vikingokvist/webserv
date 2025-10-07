#!/usr/bin/env python3
import csv
import smtplib
import time
from email.message import EmailMessage
import os

SMTP_HOST = "orodetorralba@gmail.com"   # ej. smtp.gmail.com
SMTP_PORT = 587
SMTP_USER = os.environ.get("SMTP_USER")   # mejor usar variables de entorno
SMTP_PASS = os.environ.get("SMTP_PASS")

CSV_FILE = "lista_emails.csv"  # columnas: email,nombre
FROM_NAME = "Tu Nombre"
FROM_EMAIL = "tucorreo@tudominio.com"
SUBJECT = "Asunto del correo"

# Plantilla sencilla (puedes usar jinja2 para plantillas más complejas)
HTML_TEMPLATE = """
<html>
  <body>
    <p>Hola {nombre},</p>
    <p>Este es un correo predefinido de prueba.</p>
    <p>Saludos,<br/>Equipo</p>
    <p><small>Si no quieres recibir más correos, haz click <a href="{unsubscribe}">aquí</a>.</small></p>
  </body>
</html>
"""

TEXT_TEMPLATE = """Hola {nombre},

Este es un correo predefinido de prueba.

Saludos,
Equipo

Si no quieres recibir más correos, visita: {unsubscribe}
"""

UNSUB_LINK_BASE = "https://tudominio.com/unsubscribe?email="

def send_mail(smtp, to_email, to_name):
    msg = EmailMessage()
    msg["From"] = f"{FROM_NAME} <{FROM_EMAIL}>"
    msg["To"] = to_email
    msg["Subject"] = SUBJECT
    unsubscribe_link = UNSUB_LINK_BASE + to_email
    html = HTML_TEMPLATE.format(nombre=to_name or "", unsubscribe=unsubscribe_link)
    text = TEXT_TEMPLATE.format(nombre=to_name or "", unsubscribe=unsubscribe_link)
    msg.set_content(text)
    msg.add_alternative(html, subtype="html")
    # Headers útiles
    msg["List-Unsubscribe"] = f"<{unsubscribe_link}>"
    smtp.send_message(msg)

def main():
    # Conectar SMTP
    with smtplib.SMTP(SMTP_HOST, SMTP_PORT) as smtp:
        smtp.ehlo()
        smtp.starttls()
        smtp.login(SMTP_USER, SMTP_PASS)

        with open(CSV_FILE, newline="", encoding="utf-8") as csvfile:
            reader = csv.DictReader(csvfile)
            for i, row in enumerate(reader, 1):
                to = row.get("email")
                nombre = row.get("nombre", "")
                try:
                    send_mail(smtp, to, nombre)
                    print(f"[{i}] Enviado a {to}")
                except Exception as e:
                    print(f"[{i}] Error enviando a {to}: {e}")
                # Throttle: espera entre envíos para no saturar
                time.sleep(1.0)  # ajustar según limitaciones del proveedor

if __name__ == "__main__":
    main()
