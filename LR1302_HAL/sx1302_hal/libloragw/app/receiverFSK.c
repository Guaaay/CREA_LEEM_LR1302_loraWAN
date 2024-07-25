/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2019 Semtech

Description:
    Minimum test program for HAL RX capability

License: Revised BSD License, see LICENSE.TXT file include in the project
*/


/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

/* fix an issue between POSIX and C99 */
#if __STDC_VERSION__ >= 199901L
    #define _XOPEN_SOURCE 600
#else
    #define _XOPEN_SOURCE 500
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <getopt.h>

#include "loragw_hal.h"
#include "loragw_reg.h"
#include "loragw_aux.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define COM_TYPE_DEFAULT LGW_COM_SPI
#define COM_PATH_DEFAULT "/dev/spidev0.0"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define RAND_RANGE(min, max) (rand() % (max + 1 - min) + min)

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

#define DEFAULT_FREQ_HZ     868500000U

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

static int exit_sig = 0; /* 1 -> application terminates cleanly (shut down hardware, close open files, etc) */
static int quit_sig = 0; /* 1 -> application terminates without shutting down the hardware */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */

static void sig_handler(int sigio) {
    if (sigio == SIGQUIT) {
        quit_sig = 1;
    } else if ((sigio == SIGINT) || (sigio == SIGTERM)) {
        exit_sig = 1;
    }
}

void usage(void) {
    fprintf(stderr,"Library version information: %s\n", lgw_version_info());
    fprintf(stderr,"Available options:\n");
    fprintf(stderr," -h print this help\n");
    fprintf(stderr," -u            set COM type as USB (default is SPI)\n");
    fprintf(stderr," -d <path>     COM path to be used to connect the concentrator\n");
    fprintf(stderr,"               => default path: " COM_PATH_DEFAULT "\n");
    fprintf(stderr," -k <uint>     Concentrator clock source (Radio A or Radio B) [0..1]\n");
    fprintf(stderr," -r <uint>     Radio type (1255, 1257, 1250)\n");
    fprintf(stderr," -a <float>    Radio A RX frequency in MHz\n");
    fprintf(stderr," -b <float>    Radio B RX frequency in MHz\n");
    fprintf(stderr," -o <float>    RSSI Offset to be applied in dB\n");
    fprintf(stderr," -n <uint>     Number of packet received with CRC OK for each HAL start/stop loop\n");
    fprintf(stderr," -n <uint>     Number of packet received with CRC OK for each HAL start/stop loop\n");
    fprintf(stderr," -z <uint>     Size of the RX packet array to be passed to lgw_receive()\n");
    fprintf(stderr," -m <uint>     Channel frequency plan mode [0:LoRaWAN-like, 1:Same frequency for all channels (-400000Hz on RF0)]\n");
    fprintf(stderr," -j            Set radio in single input mode (SX1250 only)\n");
    fprintf(stderr, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
    fprintf(stderr," --fdd         Enable Full-Duplex mode (CN490 reference design)\n");
    fprintf(stderr," --br <uint>   Datarate for FSK comms\n");
}

/* -------------------------------------------------------------------------- */
/* --- MAIN FUNCTION -------------------------------------------------------- */

int main(int argc, char **argv)
{
    /* SPI interfaces */
    const char com_path_default[] = COM_PATH_DEFAULT;
    const char * com_path = com_path_default;
    lgw_com_type_t com_type = COM_TYPE_DEFAULT;

    struct sigaction sigact; /* SIGQUIT&SIGINT&SIGTERM signal handling */

    int i, j, x;
    uint32_t fa = DEFAULT_FREQ_HZ;
    uint32_t fb = DEFAULT_FREQ_HZ;
    double arg_d = 0.0;
    unsigned int arg_u;
    uint8_t clocksource = 0;
    lgw_radio_type_t radio_type = LGW_RADIO_TYPE_NONE;
    uint8_t max_rx_pkt = 16;
    bool single_input_mode = false;
    float rssi_offset = 0.0;
    bool full_duplex = false;

    struct lgw_conf_board_s boardconf;
    struct lgw_conf_rxrf_s rfconf;
    struct lgw_conf_rxif_s ifconf;

    float xf = 0.0;
    float br_kbps = 50;

    unsigned long nb_pkt_crc_ok = 0, nb_loop = 0, cnt_loop;
    int nb_pkt;

    uint8_t channel_mode = 0; /* LoRaWAN-like */

    const int32_t channel_if_mode0[9] = {
        -400000,
        -200000,
        0,
        -400000,
        -200000,
        0,
        200000,
        400000,
        -200000 /* lora service */
    };

    const int32_t channel_if_mode1[9] = {
        -400000,
        -400000,
        -400000,
        -400000,
        -400000,
        -400000,
        -400000,
        -400000,
        -400000 /* lora service */
    };

    const uint8_t channel_rfchain_mode0[9] = { 1, 1, 1, 0, 0, 0, 0, 0, 1 };

    const uint8_t channel_rfchain_mode1[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    /* Parameter parsing */
    int option_index = 0;
    static struct option long_options[] = {
        {"fdd",  no_argument, 0, 0},
        {"br",  required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    /* parse command line options */
    while ((i = getopt_long(argc, argv, "hja:b:k:r:n:z:m:o:d:u", long_options, &option_index)) != -1) {
        switch (i) {
            case 'h':
                usage();
                return -1;
                break;
            case 'd': /* <char> COM path */
                if (optarg != NULL) {
                    com_path = optarg;
                }
                break;
            case 'u': /* Configure USB connection type */
                com_type = LGW_COM_USB;
                break;
            case 'r': /* <uint> Radio type */
                i = sscanf(optarg, "%u", &arg_u);
                if ((i != 1) || ((arg_u != 1255) && (arg_u != 1257) && (arg_u != 1250))) {
                    fprintf(stderr,"ERROR: argument parsing of -r argument. Use -h to print help\n");
                    return EXIT_FAILURE;
                } else {
                    switch (arg_u) {
                        case 1255:
                            radio_type = LGW_RADIO_TYPE_SX1255;
                            break;
                        case 1257:
                            radio_type = LGW_RADIO_TYPE_SX1257;
                            break;
                        default: /* 1250 */
                            radio_type = LGW_RADIO_TYPE_SX1250;
                            break;
                    }
                }
                break;
            case 'k': /* <uint> Clock Source */
                i = sscanf(optarg, "%u", &arg_u);
                if ((i != 1) || (arg_u > 1)) {
                    fprintf(stderr,"ERROR: argument parsing of -k argument. Use -h to print help\n");
                    return EXIT_FAILURE;
                } else {
                    clocksource = (uint8_t)arg_u;
                }
                break;
            case 'j': /* Set radio in single input mode */
                single_input_mode = true;
                break;
            case 'a': /* <float> Radio A RX frequency in MHz */
                i = sscanf(optarg, "%lf", &arg_d);
                if (i != 1) {
                    fprintf(stderr,"ERROR: argument parsing of -f argument. Use -h to print help\n");
                    return EXIT_FAILURE;
                } else {
                    fa = (uint32_t)((arg_d*1e6) + 0.5); /* .5 Hz offset to get rounding instead of truncating */
                }
                break;
            case 'b': /* <float> Radio B RX frequency in MHz */
                i = sscanf(optarg, "%lf", &arg_d);
                if (i != 1) {
                    fprintf(stderr,"ERROR: argument parsing of -f argument. Use -h to print help\n");
                    return EXIT_FAILURE;
                } else {
                    fb = (uint32_t)((arg_d*1e6) + 0.5); /* .5 Hz offset to get rounding instead of truncating */
                }
                break;
            case 'n': /* <uint> NUmber of packets to be received before exiting */
                i = sscanf(optarg, "%u", &arg_u);
                if (i != 1) {
                    fprintf(stderr,"ERROR: argument parsing of -n argument. Use -h to print help\n");
                    return EXIT_FAILURE;
                } else {
                    nb_loop = arg_u;
                }
                break;
            case 'z': /* <uint> Size of the RX packet array to be passed to lgw_receive() */
                i = sscanf(optarg, "%u", &arg_u);
                if (i != 1) {
                    fprintf(stderr,"ERROR: argument parsing of -z argument. Use -h to print help\n");
                    return EXIT_FAILURE;
                } else {
                    max_rx_pkt = arg_u;
                }
                break;
            case 'm':
                i = sscanf(optarg, "%u", &arg_u);
                if ((i != 1) || (arg_u > 1)) {
                    fprintf(stderr,"ERROR: argument parsing of -m argument. Use -h to print help\n");
                    return EXIT_FAILURE;
                } else {
                    channel_mode = arg_u;
                }
                break;
            case 'o': /* <float> RSSI offset in dB */
                i = sscanf(optarg, "%lf", &arg_d);
                if (i != 1) {
                    fprintf(stderr,"ERROR: argument parsing of -o argument. Use -h to print help\n");
                    return EXIT_FAILURE;
                } else {
                    rssi_offset = (float)arg_d;
                }
                break;

            case 0:
                if (strcmp(long_options[option_index].name, "fdd") == 0) {
                    full_duplex = true;
                }
                else if(strcmp(long_options[option_index].name, "br") == 0){
                    i = sscanf(optarg, "%f", &xf);
                    if ((i != 1) || (xf < 0.5) || (xf > 250)) {
                        printf("ERROR: invalid FSK bitrate\n");
                        return EXIT_FAILURE;
                    } else {
                        br_kbps = xf;
                    }
                }
                 else {
                    fprintf(stderr,"ERROR: argument parsing options. Use -h to print help\n");
                    return EXIT_FAILURE;
                }
                break;
            default:
                fprintf(stderr,"ERROR: argument parsing\n");
                usage();
                return -1;
        }
    }

    /* configure signal handling */
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = sig_handler;
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);

    fprintf(stderr,"===== sx1302 HAL RX test =====\n");

    /* Configure the gateway */
    memset( &boardconf, 0, sizeof boardconf);
    boardconf.lorawan_public = true;
    boardconf.clksrc = clocksource;
    boardconf.full_duplex = full_duplex;
    boardconf.com_type = com_type;
    strncpy(boardconf.com_path, com_path, sizeof boardconf.com_path);
    boardconf.com_path[sizeof boardconf.com_path - 1] = '\0'; /* ensure string termination */
    if (lgw_board_setconf(&boardconf) != LGW_HAL_SUCCESS) {
        fprintf(stderr,"ERROR: failed to configure board\n");
        return EXIT_FAILURE;
    }

    /* set configuration for RF chains */
    memset( &rfconf, 0, sizeof rfconf);
    rfconf.enable = true;
    rfconf.freq_hz = fa;
    rfconf.type = radio_type;
    rfconf.rssi_offset = rssi_offset;
    rfconf.tx_enable = false;
    rfconf.single_input_mode = single_input_mode;
    if (lgw_rxrf_setconf(0, &rfconf) != LGW_HAL_SUCCESS) {
        fprintf(stderr,"ERROR: failed to configure rxrf 0\n");
        return EXIT_FAILURE;
    }

    memset( &rfconf, 0, sizeof rfconf);
    rfconf.enable = true;
    rfconf.freq_hz = fb;
    rfconf.type = radio_type;
    rfconf.rssi_offset = rssi_offset;
    rfconf.tx_enable = false;
    rfconf.single_input_mode = single_input_mode;
    if (lgw_rxrf_setconf(1, &rfconf) != LGW_HAL_SUCCESS) {
        fprintf(stderr,"ERROR: failed to configure rxrf 1\n");
        return EXIT_FAILURE;
    }

    /* set configuration for LoRa multi-SF channels (bandwidth cannot be set) */
    memset(&ifconf, 0, sizeof(ifconf));
    for (i = 0; i < 8; i++) {
        ifconf.enable = true;
        if (channel_mode == 0) {
            ifconf.rf_chain = channel_rfchain_mode0[i];
            ifconf.freq_hz = channel_if_mode0[i];
        } else if (channel_mode == 1) {
            ifconf.rf_chain = channel_rfchain_mode1[i];
            ifconf.freq_hz = channel_if_mode1[i];
        } else {
            fprintf(stderr,"ERROR: channel mode not supported\n");
            return EXIT_FAILURE;
        }
        ifconf.datarate = DR_LORA_SF7;
        if (lgw_rxif_setconf(i, &ifconf) != LGW_HAL_SUCCESS) {
            fprintf(stderr,"ERROR: failed to configure rxif %d\n", i);
            return EXIT_FAILURE;
        }
    }

    /* set configuration for LoRa Service channel */
    memset(&ifconf, 0, sizeof(ifconf));
    ifconf.rf_chain = channel_rfchain_mode0[i];
    ifconf.freq_hz = channel_if_mode0[i];
    ifconf.datarate = DR_LORA_SF7;
    ifconf.bandwidth = BW_250KHZ;
    if (lgw_rxif_setconf(8, &ifconf) != LGW_HAL_SUCCESS) {
        fprintf(stderr,"ERROR: failed to configure rxif for LoRa service channel\n");
        return EXIT_FAILURE;
    }

    // Configuration structure for the FSK channel
    struct lgw_conf_rxif_s fsk_ifconf = {
        .enable = false,            // Enable the IF chain
        .rf_chain = 0,             // RF chain to be used for FSK
        .freq_hz = 0,              // Set the appropriate IF frequency
        .bandwidth = BW_125KHZ,    // Set the bandwidth (e.g., BW_125KHZ)
        .datarate = br_kbps*1000,         // Set the FSK datarate (e.g., 50000)
        .sync_word_size = 3,       // Set the sync word size (e.g., 3 bytes)
        .sync_word = 0xC194C1,     // Set the FSK sync word
        .implicit_hdr = false,     // Disable implicit header for LoRa (not applicable for FSK)
        .implicit_payload_length = 0,
        .implicit_crc_en = false,
        .implicit_coderate = CR_UNDEFINED,
    };

    if (lgw_rxif_setconf(9, &fsk_ifconf) != LGW_HAL_SUCCESS) {
        fprintf(stderr,"ERROR: failed to configure FSK for channel 10\n");
        return EXIT_FAILURE;
    }

    /* set the buffer size to hold received packets */
    struct lgw_pkt_rx_s rxpkt[max_rx_pkt];
    fprintf(stderr,"INFO: rxpkt buffer size is set to %u\n", max_rx_pkt);
    fprintf(stderr,"INFO: Select channel mode %u\n", channel_mode);

    /* Loop until user quits */
    cnt_loop = 0;
    while( (quit_sig != 1) && (exit_sig != 1) )
    {
        cnt_loop += 1;

        if (com_type == LGW_COM_SPI) {
            /* Board reset */
            if (system("./reset_lgw.sh start") != 0) {
                fprintf(stderr,"ERROR: failed to reset SX1302, check your reset_lgw.sh script\n");
                exit(EXIT_FAILURE);
            }
        }

        /* connect, configure and start the LoRa concentrator */
        x = lgw_start();
        if (x != 0) {
            fprintf(stderr,"ERROR: failed to start the gateway\n");
            return EXIT_FAILURE;
        }

        /* Loop until we have enough packets with CRC OK */
        fprintf(stderr,"Waiting for packets...\n");
        nb_pkt_crc_ok = 0;
        while ((quit_sig != 1) && (exit_sig != 1)) {
            /* fetch N packets */
            nb_pkt = lgw_receive(ARRAY_SIZE(rxpkt), rxpkt);

            if (nb_pkt == 0) {
                // wait_ms(1);
            } else {
                for (i = 0; i < 1; i++) {
                    if (rxpkt[i].status == STAT_CRC_OK) {
                        nb_pkt_crc_ok += 1;
                    }
                    // fprintf(stderr,"\n----- %s packet -----\n", (rxpkt[i].modulation == MOD_LORA) ? "LoRa" : "FSK");
                    // fprintf(stderr,"  count_us: %u\n", rxpkt[i].count_us);
                    // fprintf(stderr,"  size:     %u\n", rxpkt[i].size);
                    // fprintf(stderr,"  chan:     %u\n", rxpkt[i].if_chain);
                    // fprintf(stderr,"  status:   0x%02X\n", rxpkt[i].status);
                    // fprintf(stderr,"  datr:     %u\n", rxpkt[i].datarate);
                    // fprintf(stderr,"  codr:     %u\n", rxpkt[i].coderate);
                    // fprintf(stderr,"  rf_chain  %u\n", rxpkt[i].rf_chain);
                    // fprintf(stderr,"  freq_hz   %u\n", rxpkt[i].freq_hz);
                    // fprintf(stderr,"  snr_avg:  %.1f\n", rxpkt[i].snr);
                    // fprintf(stderr,"  rssi_chan:%.1f\n", rxpkt[i].rssic);
                    // fprintf(stderr,"  rssi_sig :%.1f\n", rxpkt[i].rssis);
                    // fprintf(stderr,"  crc:      0x%04X\n", rxpkt[i].crc);
                    char mensaje[255];
                    memcpy(mensaje, rxpkt[i].payload + 9, rxpkt[i].size - 9);

                    write(STDOUT_FILENO, mensaje, rxpkt[i].size - 9);
                    //}
                    //fprintf(stderr,"\n");
                }
                //fprintf(stderr,"Received %d packets (total:%lu)\n", nb_pkt, nb_pkt_crc_ok);
            }
        }

        //fprintf(stderr, "\nNb valid packets received: %lu CRC OK (%lu)\n", nb_pkt_crc_ok, cnt_loop );

        /* Stop the gateway */
        x = lgw_stop();
        if (x != 0) {
            fprintf(stderr,"ERROR: failed to stop the gateway\n");
            return EXIT_FAILURE;
        }

        if (com_type == LGW_COM_SPI) {
            /* Board reset */
            if (system("./reset_lgw.sh stop") != 0) {
                fprintf(stderr,"ERROR: failed to reset SX1302, check your reset_lgw.sh script\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    fprintf(stderr,"=========== Test End ===========\n");

    return 0;
}

/* --- EOF ------------------------------------------------------------------ */
