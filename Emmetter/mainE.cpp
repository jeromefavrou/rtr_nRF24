
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <RF24/nRF24L01.h>
#include <RF24/RF24.h>
#include <memory>
#include <array>
#include <chrono>
#include <thread>

#define version 0

#define IN 0

using namespace std;

struct Rf24_t
{
    // address pour la communication ( sender , receiver )
    const array< array<uint8_t , 6> , 2> address={"FFFFE" ,"EFFFF"};

    //vitesse spi en hz bridage a 8mHz,  car hardware NRF24L01 limité a 10Mhz
    const long int bcmSpiFreq8 = 8000000 ;

    //variable de channel n + 2.4 gHz
    int channel;
};

// lecteur de parametre hardisk
template<typename T> T read_conf(string const & _configFile)
{
    T res;

    fstream ifConf ;

    ifConf.exceptions (  ifstream::badbit );

    ifConf.open(_configFile, ios::in);

    //si pas pu acceder au fichier / repertoir de la memoir disk
    if(!ifConf.good())
        throw string("basic_ios:: not good() -> " + _configFile);

    //mise en ram de la memoir disk
    ifConf >> res;

    //affichage du parametre charger
    cout << "load -> " << _configFile << " = " << res <<endl;
    return res;
}

// initilisateur modul radio
bool Rf24Init(unique_ptr<RF24> & _radio , struct Rf24_t const &  _param)
{
    // creation de l'instance pour gestion radio
    _radio = make_unique<RF24>(22, 0, _param.bcmSpiFreq8 );

    // init spi
    _radio->begin();

    //init channel
    _radio->setChannel(_param.channel);

    //set retry et delay
    _radio->setRetries(15,15);

    //set puissance d'emission
    _radio->setPALevel(RF24_PA_MIN);

    //pipe d'ecriture
    _radio->openWritingPipe(_param.address[0].data());

    //pipe de l'ecture
    _radio->openReadingPipe(0, _param.address[1].data() );

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
        cerr << "canal indisponible" << endl;
        return false;
   }

   return true;
}

//gpio setup via sysfs (driver unix)
void gpioSetup(int const &_gpio , bool _mode )
{

    fstream ofConf ;

    ofConf.exceptions (  ifstream::badbit );

    ofConf.open("/sys/class/gpio/export", ios::out);

    //si pas pu acceder au fichier / repertoir de la memoir disk
    if(!ofConf.good())
        throw string("basic_ios:: not setup gpio" );

    //mise en ram de la memoir disk
    ofConf << _gpio;

    ofConf.close();

    std::this_thread::sleep_for(1s);

    ofConf.open("/sys/class/gpio/gpio" + std::to_string(_gpio) + "/direction", ios::out);

    if(!ofConf.good())
        throw string("basic_ios:: not direction gpio" );

    ofConf << (!_mode ? "in" : "out");

    std::this_thread::sleep_for(1s);
}

//nettoye gpio
void gpioFree(int const &_gpio)
{
    fstream ofConf ;

    ofConf.exceptions (  ifstream::badbit );

    ofConf.open("/sys/class/gpio/unexport", ios::out);

    if(!ofConf.good())
        throw string("basic_ios:: not free gpio" );

    ofConf << _gpio;
}

//digital read via sysfs (driver unix)
// lecture etat gpio
bool digitalRead(int const &_gpio)
{
    fstream ifConf ;

    ifConf.exceptions (  ifstream::badbit );

    ifConf.open("/sys/class/gpio/gpio" + std::to_string(_gpio) + "/value", ios::in);

    if(!ifConf.good())
        throw string("basic_ios:: not read gpio" );

    //mise en ram
    bool res;
    ifConf >> res;

    return res;
}

int main()
{
    //creation du poiteur de gestion radio
    unique_ptr<RF24> radio;

    //structure de parametre radio
    struct Rf24_t param;


    int gpioPinEchoSignal = -1;

    try
    {

    //chargement parametres
    param.channel = read_conf<int>("Lparam/channel");

    gpioPinEchoSignal = read_conf<int>("Lparam/inputGpio");

    //gpio init
    gpioSetup(gpioPinEchoSignal , IN);

    cout << "GPIO " << gpioPinEchoSignal << " set as " << (!IN?"input":"output") <<endl;
    }
    catch(string const & e)
    {
        cerr << e << endl;

        gpioFree(gpioPinEchoSignal);

        return -1;
    }

    //initilisation radio
    if(!Rf24Init(radio , param))
        return -1;


    //variable a echangé
    bool statu = false ;


    // boucle  principal (equivalent loop arduino)
    while(true)
    {
        // test de diponibiliter  channel
        if(radio->testCarrier())
            continue;


        if(digitalRead(gpioPinEchoSignal) && !statu)
        {
            statu = true ;

            // definition clair du mode d'emission
            radio->stopListening();

            //envoie da la donné
            radio->write((char*) &statu, sizeof(bool));

            // definition clair du mode d'ecoute
            radio->startListening();

            cout << " send 1" <<endl;
        }
        else if(!digitalRead(gpioPinEchoSignal) && statu)
        {
            statu = false ;

            // definition clair du mode d'emission
            radio->stopListening();

            //envoie da la donné
            radio->write((char*) &statu, sizeof(bool));

            // definition clair du mode d'ecoute
            radio->startListening();

            cout << " send 0" <<endl;
        }
    }

    gpioFree(gpioPinEchoSignal);

    cout << "press a touch for exit" <<endl;
    cin.get();

    return 0;
}
