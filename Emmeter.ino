// coding by Jerome Favrou in 2020

#include <RF24.h>//https://github.com/nRF24/RF24
#include <nRF24L01.h>//https://github.com/nRF24/RF24/blob/master/nRF24L01.h
#include <SPI.h>//https://www.arduino.cc/en/reference/SPI

#define DEBUG

#ifdef DEBUG
    // baude rate definition vitesse de transmition protocol serie Serial (uart)(usb port)
    #define Speed_Serial0 19200
#endif

#ifnedef nullprt
    nullptr = 0x00
#endif

typedef const uint8_t Pin_t;

// address pour la communication ( sender , receiver )
const uint8_t address[2][6] ={"FFFFE" ,"EFFFF"};

//variable de channel n + 2.4 gHz
const int channel = 0;

Pin_t CSN = 7;
Pin_t CE = 8;

RF24* radio;

template<typename T> inline void printLog(T const & msg)
{
    #ifdef DEBUG
        Serial.println(msg)
    #endif
}

bool Rf24Init(RF24 * _radio )
{
    if (__radio != nullptr )
        return true;

    _radio = new RF24(CE , CSN);
        // init spi
    _radio->begin();

      // set spi speed horloge to 8Mhz ( 16/2 )
     SPI.setClockDivider(SPI_CLOCK_DIV2)

    //init channel
    _radio->setChannel(channel);

    //set retry et delay
    _radio->setRetries(15,15);

    //set puissance d'emission
    _radio->setPALevel(RF24_PA_MIN);

    //pipe d'ecriture
    _radio->openWritingPipe(address[0]);

    //pipe de l'ecture
    _radio->openReadingPipe(0, address[1]);

    //set de l'auto aquitement
    _radio->setAutoAck(true);

    //definition de la taille de la somme de control
    _radio->setCRCLength(RF24_CRC_8);

    //definition du debit d'echange
    _radio->setDataRate(RF24_250KBPS);

    // definition clair du mode d'ecoute
    _radio->startListening();

    //affichage des parametre initialiser
    _radio->printDetails();

    // test de diponibiliter channel
    if(_radio->testCarrier())
   {
        printLog<String>(F("canal indisponible"));
        return false;
   }
}

void setup()
{
    #ifdef DEBUG
        //init port usb pour debubage avec moniteur serie

        Serial.begin(Speed_Serial0);
        while(!Serial){;}
    #endif

    // set spi speed horloge to 8Mhz ( 16/2 )
    SPI.setClockDivider(SPI_CLOCK_DIV2)

    printLog<String>(F("Initialize System"));

    Rf24Init(radio);

    printLog<String>(F("Initialize end System start"));
}

void loop()
{

}