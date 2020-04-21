

# Program an ESP-01 or Sonoff: use CHIP=esp8266, FLASH_DEF=1M, hook FTDI-USB serial to chip
# 1) Hook ESP to serial BEFORE plugging in USB.  Sonoff square contact is +3.3v
# 2) Hold down GPIO0 (button on SonOff, pin on ESP-01)
# 3) Plug in USB serial adapter
# 4) Run make upload 

# Program ESP32, use CHIP=esp32, BOARD=heltec_wifi_kit_32 


#BOARD=nodemcu
BOARD=generic
CHIP=esp8266
FLASH_DEF=1M

#CHIP=esp32
#BOARD=nodemcu-32s

OTA_ADDR=192.168.43.90
IGNORE_STATE=1

include ${HOME}/Arduino/makeEspArduino/makeEspArduino.mk

print-%  : ; @echo $* = $($*)

fixtty:
	stty -F ${UPLOAD_PORT} -hupcl -crtscts -echo raw 19200

setap1:
	wget 'http://192.168.4.1/wifisave?s=ChloeNet&p=niftyprairie7'

