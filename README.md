
# Pruebas LR1302

### Test rx 28/06/2024
```
leem@raspberrypi5:~/LR1302_loraWAN/LR1302_HAL/sx1302_hal/libloragw $ sudo ./test_loragw_hal_rx -k 0 -a 868.5 -m 0 -r 1250
```

```
sudo ./test_loragw_hal_tx -r 1250 -n 10 -n 1 -z 16 -m LORA -b 125 -j -l 10
./test_loragw_hal_rx -r 1250 -n 128 -m 0 -j
```

### Test envio de imagen 05/07/2024
Envio imagen LORA OK
```
./transmitter -r 1250 -n 1 -z 255 -m LORA -b 125 -j -l 10 -s 5 < ~/crea.jpg
sudo ./receiver -r 1250 -n 128 -m 0 -z 255 2> crea.jpg
```

Tiempos de duración: 847ms, 831ms => 12kbps aprox

### Test FSK ...
```
./transmitter -r 1250 -n 1 -z 255 -m FSK -c 0 < ~/crea.jpg
```





# Transmisión de video por UART

### Paso 1. Instalación de ppp

Inicialmente se ha de instalar el paquete ppp (point to point protocol) para crear una interfaz virtual de red por uart. Para ello configurar el archivo ```/etc/rc.local``` añadiendo configuración antes de ```exit 0``` (configurar baudeos por segundo, puerto UART y direcciones IP). Finalizada la configuración conectar los dispositivos fisicamente por uart, reiniciar y verificar mediante ```ifconfig``` la nueva interfaz de red

Instalación del paquete:
```
sudo apt-get update -y && sudo apt-get upgrade -y
sudo apt-get install ppp -y
```

Insertar en ```/etc/rc.local``` Raspberry Pi A:
```
stty -F /dev/ttyAMA0 raw
stty -F /dev/ttyAMA0 -a
pppd /dev/ttyAMA0 460800 192.168.11.1:192.168.11.2 noauth local debug dump  nocrtscts persist maxfail 0 holdoff 1
```

Insertar en ```/etc/rc.local``` Raspberry Pi B:
```
stty -F /dev/ttyAMA0 raw
stty -F /dev/ttyAMA0 -a
pppd /dev/ttyAMA0 460800 192.168.11.2:192.168.11.1 noauth local debug dump  nocrtscts persist maxfail 0 holdoff 1
```



### Servidor de video:

Crear un archivo fifo y un servidor de video ffmpeg redireccionado a un puerto


Requisitos de instalación:
```
sudo apt-get install netcat
sudo apt-get install ffmpeg
```

Crear el archivo fifo
```
mkfifo video_pipe
```

Ejecutar por separado el servidor de video y la redirección del buffer fifo al puerto. (Configurable la resolución del video, los fps y la velocidad de transmisión)
```
nc -l 12345 < video_pipe
```
```
ffmpeg -i Video.mp4 -f mpegts -codec:v libx264 -s 640x480 -b:v 150k -r 30 -an - > video_pipe
```


### Cliente:

Visualización del video desde el cliente

```
nc 192.168.11.1 12345 | ffplay -
```
