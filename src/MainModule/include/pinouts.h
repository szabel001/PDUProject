#ifndef pinouts_h
#define pinouts_h

//===================================RS485 IEC GPIO=====================================
#define RS485_IEC_DE_RE_PIN 32
#define RS485_IEC_TX_GPIO   20   //master PDU
#define RS485_IEC_RX_GPIO   34

//===================================RS485 PDU GPIO=====================================
#define RS485_PDU_DE_RE_PIN 33
#define RS485_PDU_TX_GPIO   7
#define RS485_PDU_RX_GPIO   35

//===================================Ethernet GPIO======================================
#define ETH_MOSI  12
#define ETH_MISO  13
#define ETH_SCK   14
#define ETH_CS    15
#define ETH_RST   27
#define ETH_INTn  4

#define ETH_SPI_FREQ 16

//===================================TFT Display GPIO===================================
#define TFT_MOSI  22
#define TFT_SCK   8
#define TFT_CS    5
#define TFT_A0    21

#define TFT_SPI_FREQ  32000000

//===================================AHT10 GPIO=========================================
#define AHT_SCL  25
#define AHT_SDA  26

//===================================BUTTONS GPIO=========================================
#define BTN_DOWN  36
#define BTN_UP  37
#define BTN_BACK  38
#define BTN_CONFIRM  39

#endif