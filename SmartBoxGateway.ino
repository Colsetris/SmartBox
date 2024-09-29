#include <heltec_unofficial.h> // Cria as instâncias "radio", "display", "button" e "both" ("display" + Serial).

// Frequência em MHz. Sempre utilizar o ponto para indicar que é float.
#define FREQUENCY 915.0 // No Brasil é utilizado a banda ISM de 915.0 MHz.

// Largura de banda LoRa em kHz. Sempre utilizar o ponto para indicar que é float.
// Valores permitidos: 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 e 500.0
#define BANDWIDTH 125.0

// Fator de espalhamento, valor que vai de 5 até 12. Números mais alto significam
// maior alcance e mais robusto contra interferências.
#define SPREADING_FACTOR 9

// Poder de transmissão em dBm. 0 dBm = 1 mW, o qual é suficiente para testes. Pode
// ser definido com qualquer valor dentro de -9 dBm (0.125 mW) até 22 dBm (158 mW).
// A Potência Aparente Radiada (PAR) máxima das antenas é limitada pelas bandas ISM
// regionais.
// Nunca transmitir sem uma antena, isto pode danificar o hardware.
#define TRANSMIT_POWER 0

volatile bool rxFlag = false;
String rxData;

// TESTE Dado de transmissão
long counter = 0;

void setup() {
  heltec_setup();
  
  radioSetup();  
}

void loop() {
  heltec_loop();

  // Se um pacote chegar, mostre o seu conteúdo junto com o RSSI (Received Signal
  // Strength Indicator) e SNR (Signal-to-Noise Ratio).
  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxData);

    // O embarcado enviará um pacote contendo uma string "RX" para indicar que
    // está pronto para receber os pacotes.
    if (rxdata.compareTo("RX") == 0) {
      // TODO Comunicar com a API do servidor NodeJs e pegar os dados de
      // configuração através do getConfig()
      // TODO Transmitir esses dados para o embarcado
      transmit();
    }

    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("RX: %s\n", rxdata.c_str());
      both.printf("RSSI: %.2f dBm\n", radio.getRSSI());
      both.printf("SNR: %.2f dB\n", radio.getSNR());
    }

    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

void radioSetup() {
  // Inicializa a instância "radio".
  both.println("Inicializar rádio...");
  RADIOLIB_OR_HALT(radio.begin());

  // Configura uma interrupção no pino DIO1 para chamar setRxFlag() quando receber
  // um pacote.
  radio.setDio1Action(setRxFlag);

  // Configura os paramêtros do "radio".
  both.printf("Frequência: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));

  both.printf("Largura de banda: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));

  both.printf("Fator de espalhamento: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));

  both.printf("Poder TX: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));

  // Configura o método de recebimento de dados por interrupção do pino DIO1.
  // O tempo de espera para receber cada pacote é infinito.
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

void setRxFlag() {
  rxFlag = true;
}

void transmit() {
  both.printf("TX: %s\n", String(counter).c_str());
  radio.clearDio1Action();
  heltec_led(50);

  // Transmite o dado e calcula o tempo de tranmissão.
  tx_time = millis();
  RADIOLIB(radio.transmit(String(counter++).c_str()));
  tx_time = millis() - tx_time;
  heltec_led(0);
  if (_radiolib_status == RADIOLIB_ERR_NONE) {
    both.printf("OK (%i ms)\n", (int)tx_time);
  } else {
    both.printf("Erro (%i)\n", _radiolib_status);
  }

  radio.setDio1Action(setRxFlag);
}