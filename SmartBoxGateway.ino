#include <heltec_unofficial.h> // Cria as instâncias "radio", "display", "button" e "both" ("display" + Serial).
#include <WiFi.h>
#include <HTTPClient.h>

#define ENABLE_TESTING 1 // Habilita os testes.

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
String rxData = "";
String txData = "";

// TESTE: Dado de transmissão LoRa.
#ifdef ENABLE_TESTING
  long counter = 0;
#endif

#define WIFI_SSID "Zampier-Network" // Trocar o SSID.
#define WIFI_PASSWORD "J7txRzJ.QXK@fA" // Trocar senha.
#define SERVER_NAME "http://codelabs.dev.br:48592/home/teste"

// TESTE: Temporizador de envio de requisição HTTP.
#ifdef ENABLE_TESTING
  #define TIMER_DELAY 300000
  unsigned long lastTime = 0;
#endif

void setup() {
  heltec_setup();

  both.printf("SmartBox Gateway.\n");
  
  radioSetup();
  wifiSetup(); 
}

void loop() {
  heltec_loop();

  communicateWithEndNode();

  #ifdef ENABLE_TESTING
    if ((millis() - lastTime) > TIMER_DELAY || button.isSingleClick()) {
      requestFromServer();

      lastTime = millis();

      if (txData.length() == 0)
        txData = String(counter++);

      transmitPackageLoRa();

      rxData = txData = "";
    }
  #endif
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

  Serial.printf("Largura de banda: %.1f kHz\n", BANDWIDTH);
  display.println("Largura de banda:");
  display.printf("%.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));

  both.printf("Fator de espalhamento: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));

  both.printf("Poder TX: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));

  // Configura o método de recebimento de dados por interrupção do pino DIO1.
  // O tempo de espera para receber cada pacote é infinito.
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

void wifiSetup() {
  both.println("Inicializar WiFI...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  both.print("Conectando");
  while (WiFi.status() != WL_CONNECTED) {
    heltec_delay(500);
    both.print(".");
  }
  Serial.printf("\nConectado à rede WiFi %s.\nEndereço IP: ",
              WIFI_SSID);
  display.println("\nConectado à rede WiFi");
  display.println(WIFI_SSID);
  display.println("Endereço IP:");
  both.println(WiFi.localIP());

  // TESTE: Mensagem de aviso de configuração do temporizador.
  #ifdef ENABLE_TESTING
    Serial.printf("Temporizador configurado para %d segundos.\n", TIMER_DELAY / 1000 / 60);
    display.printf("Temporizador configurado\npara %d segundos.\n", TIMER_DELAY / 1000 / 60);
  #endif
}

void setRxFlag() {
  rxFlag = true;
}

void communicateWithEndNode() {
  receivePackageLoRa();

  if (rxData.length() == 0) {
    radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
    return;
  }

  #ifndef ENABLE_TESTING
    Serial.print("rxData é igual à RX? ");
    if (rxData.equals("RX")) {
      Serial.println("Sim.");
    } else {
      Serial.println("Não.");
    }
  #endif

  // O embarcado enviará um pacote contendo uma string "RX" para indicar que
  // está pronto para receber os pacotes.
  if (rxData.equals("RX")) {
    // TODO Comunicar com a API do servidor Node.js e pegar os dados de
    // configuração através do getConfig().
    // TODO Transmitir esses dados para o embarcado.
    rxData = String(counter++);
    requestFromServer();

    if (txData.length() > 0)
      transmitPackageLoRa();

    rxData = txData = "";

    radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
  }
}

void receivePackageLoRa() {
  // Se um pacote chegar, mostre o seu conteúdo junto com o RSSI (Received Signal
  // Strength Indicator) e SNR (Signal-to-Noise Ratio).
  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxData);

    // TESTE: Mostrar informações sobre o pacote LoRa recebido.  
    #ifdef ENABLE_TESTING
      if (_radiolib_status == RADIOLIB_ERR_NONE) {
        both.printf("RX: %s\n", rxData.c_str());
        both.printf("RSSI: %.2f dBm\n", radio.getRSSI());
        both.printf("SNR: %.2f dB\n", radio.getSNR());
      }
    #endif
  }
}

void transmitPackageLoRa() {
  both.printf("TX: %s\n", txData.c_str());
  
  radio.clearDio1Action();
  heltec_led(50);

  // Transmite o dado e calcula o tempo de tranmissão.
  unsigned int tx_time = millis();
  radio.transmit(txData.c_str());
  tx_time = millis() - tx_time;
  heltec_led(0);
  if (_radiolib_status == RADIOLIB_ERR_NONE) {
    both.printf("OK (%i ms)\n", (int)tx_time);
  } else {
    both.printf("Erro (%i)\n", _radiolib_status);
  }

  radio.setDio1Action(setRxFlag);
}

void requestFromServer() {
  both.println("Enviando requisição HTTP.");
  // TESTE: Envio de requisição HTTP POST contendo um JSON.
  #ifdef ENABLE_TESTING
    // Verifica o estado da conexão.
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      http.begin(client, SERVER_NAME);

      http.addHeader("Content-Type", "application/json");

      int httpResponseCode;
      if (rxData.length() > 0) {
        httpResponseCode = http.POST("{\"Vl1\":" + rxData + ",\"Vl2\":" + rxData + "}");
      } else {
        httpResponseCode = http.POST("{\"Vl1\":2,\"Vl2\":2}");
      }

      both.printf("Código de resposta HTTP: %d\n", httpResponseCode);

      if (httpResponseCode == 200) {
        String content = http.getString();
        both.println("Conteúdo:");
        both.println(content);
        txData = content;
      }

      // Libera os recursos.
      http.end();
    } else {
      both.println("WiFi desconectado.");
    }
  #endif
}
