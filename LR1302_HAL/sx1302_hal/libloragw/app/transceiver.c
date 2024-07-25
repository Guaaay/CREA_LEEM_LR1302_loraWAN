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

#include <string.h>         /* memset */
#include <signal.h>         /* sigaction */
#include <time.h>           /* time, clock_gettime, strftime, gmtime */
#include <sys/time.h>       /* timeval */
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

void thread_rx(void);
void thread_tx(void);

int conectar_tun(){
    struct ifreq ifr;
    int fd;
    if ((fd = open(TUN_DEVICE, O_RDWR)) < 0) {
        perror("Opening TUN device");
        exit(1);
    }
    return fd;
}

int main(int argc, char ** argv)
{
    struct sigaction sigact; /* SIGQUIT&SIGINT&SIGTERM signal handling */
    pthread_t thrid_rx;
    pthread_t thrid_tx;
    


}


void thread_rx(void) {


}