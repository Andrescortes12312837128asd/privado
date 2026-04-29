#!/bin/bash

# Script de instalación y configuración para Chimera Core Apex

echo "Instalando dependencias necesarias..."

# Actualizar sistema
sudo apt update && sudo apt upgrade -y

# Instalar dependencias
sudo apt install -y python3 python3-pip gcc build-essential make tor proxychains4

# Instalar paquetes Python necesarios
pip3 install requests socks

# Configurar Tor
echo "Configurando Tor..."
sudo systemctl enable tor
sudo systemctl start tor

# Configurar proxychains
echo "Configurando proxychains..."
sudo cp /etc/proxychains4.conf /etc/proxychains4.conf.bak
sudo sed -i 's/#socks5 127.0.0.1 9050/socks5 127.0.0.1 9050/' /etc/proxychains4.conf

# Compilar el archivo C
echo "Compilando el archivo C..."
gcc -o Chimera_Core_Apex_C Chimera_Core_Apex.c -lpthread

# Dar permisos de ejecución
chmod +x Chimera_Core_Apex.py
chmod +x Chimera_Core_Apex_C

# Crear directorio para logs
mkdir -p logs

echo "Instalación completada. Para ejecutar el ataque:"
echo "Python: python3 Chimera_Core_Apex.py <IP> <PUERTO> <DURACION>"
echo "C: ./Chimera_Core_Apex_C <IP> <PUERTO> <DURACION>"
echo "Con Tor: proxychains4 python3 Chimera_Core_Apex.py <IP> <PUERTO> <DURACION>"
