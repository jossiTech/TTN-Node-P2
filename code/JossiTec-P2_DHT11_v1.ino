// necessary libraries##################################
  #include <lmic.h>
  #include <hal/hal.h>
  #include <CayenneLPP.h>
  #include <OneWire.h>
  #include <DallasTemperature.h>
  #include <DHT.h>
// necessary libraries##################################

//prepare 1-Wire-Busses#################################
  #define ONE_WIRE_BUS1 4
  #define ONE_WIRE_BUS2 5
  #define ONE_WIRE_BUS3 8
  #define ONE_WIRE_BUS4 9
  OneWire oneWire1(ONE_WIRE_BUS1);
  OneWire oneWire2(ONE_WIRE_BUS2);
  OneWire oneWire3(ONE_WIRE_BUS3);
  OneWire oneWire4(ONE_WIRE_BUS4);

  DallasTemperature sensor1(&oneWire1);
  DallasTemperature sensor2(&oneWire2);
  DallasTemperature sensor3(&oneWire3);
  DallasTemperature sensor4(&oneWire4);
//prepare 1-Wire-Busses#################################

//prepare DHT11-Sensor PIN18 (A0), Typ DHT11############
  DHT dht11 (18, DHT11);
//prepare DHT11-Sensor PIN18 (A0), Typ DHT11############


// Support for CayenneLPP Dataformat####################
  CayenneLPP lpp(51);
// Support for CayenneLPP Dataformat####################

//OTAA (DevEUI=LSB, APPEUI=LSB, AppKey=MSB)#############
  static const u1_t PROGMEM APPEUI[8]={ "fill in with your values" };
  void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
  static const u1_t PROGMEM DEVEUI[8]={ "fill in with your values" };
  void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
  static const u1_t PROGMEM APPKEY[16] = { "fill in with your values" };
  void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}
//OTAA (DevEUI=LSB, APPEUI=LSB, AppKey=MSB)#############

//Interval for Sending...###############################
  const unsigned TX_INTERVAL = 60;
//Interval for Sending...###############################

// PIN-Mapping Arduino Pro Micro 3.3V & RFM95W#########
  const lmic_pinmap lmic_pins = {
        .nss = 10,
        .rxtx = LMIC_UNUSED_PIN,
        .rst = 9,
        .dio = {2, 6, 7},};

  static osjob_t sendjob;
// PIN-Mapping Arduino Pro Micro 3.3V & RFM95W#########


// Pin mapping for 32U4. Timing setting after LMIC_reset nessesary. Comment Serial-Commands for 32U4
/*const lmic_pinmap lmic_pins = {
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {7, 6, LMIC_UNUSED_PIN},}*/



//Sensors still not read...
  bool validData = false;

// Status Relais
  bool RelaisOn = false;

//##########################################################################################################
void setup() {

    // Start up the Dallas-library######################
    sensor1.begin();
    sensor2.begin();
    sensor3.begin();
    sensor4.begin();
    // Start up the Dallas-library######################

    // Start up DHT11 ##################################
    dht11.begin();
    // Start up DHT11 ##################################

    // Define Relais-Port###############################
    pinMode(A1, OUTPUT);
    // Define Relais-Port###############################

    // LMIC init########################################
    os_init();
    LMIC_reset();
         //Timing für 32U4 anpassen
         //LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
    do_send(&sendjob);
    // LMIC init########################################
}
//##########################################################################################################


//##########################################################################################################
void loop() {os_runloop_once();}
//##########################################################################################################

//##########################################################################################################
void onEvent (ev_t ev) {
    switch(ev) {
        case EV_JOINED:
                        LMIC_setDrTxpow(DR_SF9,14);
                        // Disable link check validation (automatically enabled
                        // during join, but not supported by TTN at this time).
                        LMIC_setLinkCheckMode(0);
                        break;
       case EV_TXCOMPLETE:
                          if (LMIC.dataLen) {

                                              //processing downlink
                                               uint8_t result0 = LMIC.frame[LMIC.dataBeg + 0];//Serial.print(" #Result0: "); Serial.print(result0);
                                               uint8_t result1 = LMIC.frame[LMIC.dataBeg + 1];//Serial.print(" #Result1: "); Serial.print(result1);
                                               uint8_t result2 = LMIC.frame[LMIC.dataBeg + 2];//Serial.print(" #Result2: "); Serial.print(result2);
                                               uint8_t result3 = LMIC.frame[LMIC.dataBeg + 3];//Serial.print(" #Result3: "); Serial.println(result3);

                                               if (result2 == 100)  {digitalWrite(A1, HIGH);RelaisOn=true;}
                                               if (result2 == 0)    {digitalWrite(A1, LOW);RelaisOn=false;}

                                              delay(1000);}
                                              // Schedule next transmission
                                              os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
                                              break;
       default:
            break;
                }
}
//##########################################################################################################


//##########################################################################################################
void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {}
      else {
             ReadSensors();
             if (validData == true) {
                                      // Prepare upstream data transmission at the next possible time.
                                      LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);}

             else {  //No valid data, waiting for next iteration. Schedule next transmission
                     os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);}
           }
    // Next TX is scheduled after TX_COMPLETE event.
}
//##########################################################################################################


//##########################################################################################################
void ReadSensors() {
  validData = false;
  //Read Temp-Sensors
  sensor1.requestTemperatures(); float temp1=(sensor1.getTempCByIndex(0));
  sensor2.requestTemperatures(); float temp2=(sensor2.getTempCByIndex(0));
  sensor3.requestTemperatures(); float temp3=(sensor3.getTempCByIndex(0));
  sensor4.requestTemperatures(); float temp4=(sensor4.getTempCByIndex(0));
  // Why "byIndex"?  You can have more than one DS18B20 on the same wire. Here are 2/4 wires. 0 refers to the first IC on the wire

  // Read DHT11
  float humidity = dht11.readHumidity();

  //Data in Cayenne LPP-Format
  lpp.reset();
  lpp.addTemperature(1, temp1);
  lpp.addTemperature(2, temp2);
  lpp.addTemperature(3, temp3);
  lpp.addTemperature(4, temp4);
  lpp.addRelativeHumidity(5, humidity);

  //if (RelaisOn == true) {lpp.addDigitalOutput(6, 1);}
  //if (RelaisOn == false) {lpp.addDigitalOutput(6, 0);}

  validData = true;
}
//##########################################################################################################
