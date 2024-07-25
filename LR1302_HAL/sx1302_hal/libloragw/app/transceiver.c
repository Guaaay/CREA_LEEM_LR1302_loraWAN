/*
Programa con capacidad de recepción y transmisión "simultáneas" (todavía no tengo claro que sea full duplex)
- 2 hilos tx y rx
- mutex para acceder al lr1302 (no se puede enviar comandos al mismo tiempo desde dos hilos).
- Conexión con interfaz TUN del dispositivo. Lo que leo de la TUN lo transmito por el módulo y lo que recibo por el módulo lo escribo en la TUN.
- Hablo siempre desde la perspectiva del módulo (RX significa recibir por el módulo y TX transmitir por el módulo)
/*

/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

/* fix an issue between POSIX and C99 */
#if __STDC_VERSION__ >= 199901L
    #define _XOPEN_SOURCE 600
#else
    #define _XOPEN_SOURCE 500
#endif

#include <stdint.h>         /* C99 types */
#include <stdbool.h>        /* bool type */
#include <stdio.h>          /* printf, fprintf, snprintf, fopen, fputs */
#include <inttypes.h>       /* PRIx64, PRIu64... */
#include <fcntl.h>          /* file control options */  

#include <string.h>         /* memset */
#include <signal.h>         /* sigaction */
#include <time.h>           /* time, clock_gettime, strftime, gmtime */
#include <sys/time.h>       /* timeval */
#include <sys/ioctl.h>      /* ioctl */
#include <unistd.h>         /* getopt, access */
#include <stdlib.h>         /* atoi, exit */
#include <errno.h>          /* error messages */
#include <math.h>           /* modf */

#include <sys/socket.h>     /* socket specific definitions */
#include <netinet/in.h>     /* INET constants and stuff */
#include <arpa/inet.h>      /* IP address conversion stuff */
#include <netdb.h>          /* gai_strerror */

#include <pthread.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include "trace.h"
#include "base64.h"
#include "loragw_hal.h"
#include "loragw_aux.h"
#include "loragw_reg.h"
#include "loragw_gps.h"

#define TUN "/dev/net/tun"
#define COM_PATH_DEFAULT "/dev/spidev0.0"

int tun_fd;

void thread_rx(void);
void thread_tx(void);


void configurar_lr1302(){
    struct lgw_conf_board_s boardconf;
    const char com_path_default[] = COM_PATH_DEFAULT;
    const char * com_path = com_path_default;

    memset( &boardconf, 0, sizeof boardconf);
    boardconf.lorawan_public = true;
    boardconf.clksrc = 0;
    boardconf.full_duplex = true;
    boardconf.com_type = LGW_COM_SPI;
    strncpy(boardconf.com_path, com_path, sizeof boardconf.com_path);
    boardconf.com_path[sizeof boardconf.com_path - 1] = '\0'; /* ensure string termination */
    if (lgw_board_setconf(&boardconf) != LGW_HAL_SUCCESS) {
        printf("ERROR: failed to configure board\n");
        return EXIT_FAILURE;
    }

    
}

int conectar_tun(char * dev){
    struct ifreq ifr;
    int fd;
    int err;
    if ((fd = open(TUN, O_RDWR)) < 0) {
        perror("Abriendo dispositivo TUN");
        exit(1);
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    if (*dev){
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }
    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
        close(fd);
        perror("ioctl(TUNSETIFF)");
        exit(1);
    }
    strcpy(dev, ifr.ifr_name);

    return fd;
}

int main(int argc, char ** argv)
{
    struct sigaction sigact; /* SIGQUIT&SIGINT&SIGTERM signal handling */
    pthread_t thrid_rx;
    pthread_t thrid_tx;
    char devname[IFNAMSIZ] = "tun0";
    tun_fd = conectar_tun(devname);
    pthread_create(&thrid_rx, NULL, thread_rx, NULL);
    pthread_create(&thrid_tx, NULL, thread_tx, NULL);

    pthread_join(thrid_rx, NULL);
    pthread_join(thrid_tx, NULL);

}


void thread_rx(void) {


}

void thread_tx(void) {

}