# En otra terminal, monitorea el tráfico
sudo tcpdump -i any -n host <IP_OBJETIVO>


# Contador de paquetes
sudo netstat -su | grep "packets sent"
