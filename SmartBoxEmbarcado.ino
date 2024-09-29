// Macros que precisam estar definidos antes do "#include <heltec_unofficial.h>"

// Habilita a funcionalidade do botão PRG de ligar e desligar a placa com um
// clique longo.
#define HELTEC_POWER_BUTTON

// Define o código que será executado quando utilizar o macro RADIOLIB_OR_HALT
// quando algum comando do "radio" retornar um erro.
#define RADIOLIB_DO_DURING_HALT goToDeepSleep()

#include <heltec_unofficial.h> // Cria as instâncias "radio", "display", "button" e "both" ("display" + Serial).

// Tempo em segundos no modo Deep Sleep.
#define DEEP_SLEEP_TIME 60UL

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

bool wakeUpWasButton;
volatile bool rxFlag = false;

// Dados que serão enviados.
RTC_DATA_ATTR uint8_t count = 0; // TESTE
uint8_t batteryPercent = heltec_battery_percent();

String rxData;
void setup() {
  heltec_setup();

  wakeUpWasButton = heltec_wakeup_was_button();

  // TODO: Habilitar o display somente quando o embarcado foi acordado
  // do modo Deep Sleep pelo clique do botão PRG.
  if (wakeUpWasButton) {
    displayInfos();
  } else {
    heltec_display_power(false);
  }

  radioSetup();

  // TODO: Leitura dos sensores ultrasônicos.
  // TODO: Cálculo da porcentagem de lixo na caçamba.
  
  // TODO: Transmissão da porcentagem de lixo.
  // TODO: Formatar os dados para incluir a identificação do que trata cada
  // um deles.
  // Vetor de pacotes de envio.
  String uplinkData[2];
  uplinkData[0] = String(count++); // TESTE
  uplinkData[1] = "V" + String(heltec_battery_percent());

  both.println("Transmitindo pacotes.");
  for (uint8_t i = 0; i < sizeof(uplinkData) / sizeof(String); i++) {
    Serial.printf("TX: %s\n", uplinkData[i].c_str());
    if (wakeUpWasButton)
      display.printf("TX: %s\n", uplinkData[i].c_str());
    
    RADIOLIB(radio.transmit(uplinkData[i].c_str()));
    heltec_delay(100);
  }

  // Piscar o LED para indicar ao operador que a transmissão terminou.
  if (wakeUpWasButton) {
    heltec_led(20);
    heltec_delay(200);
    heltec_led(0);
  }

  // TODO: Espera de recebimento de algum pacote do servidor .
  // Configura o método de recebimento de dados por interrupção do pino DIO1.
  // O tempo de espera para receber cada pacote é de 10 segundos.
  RADIOLIB(radio.startReceive(1000000UL));

  // Transmite "RX" para indicar que está configurado para receber dados.
  both.println("Recebimento de dados.");
  radio.transmit(String("RX").c_str());
  while (true) {
    if (rxFlag) {
      rxFlag = false;

      radio.readData(rxData);

      // TODO Verificar o conteúdo do pacote e realizar as alterações de
      // configurações.  
      if (rxData.length() != 0) {
        Serial.printf("RX: %s\n", rxData.c_str());
        if (wakeUpWasButton){
          display.printf("RX: %s\n", rxData.c_str());

          // Piscar o LED para indicar ao operador que um dado foi recebido.
          heltec_led(20);
          heltec_delay(200);
          heltec_led(0);
        }
      }

      RADIOLIB(radio.startReceive(1000000UL));
    }

    if (radio.isRxTimeout()) {
      Serial.println("Nenhum pacote recebido.");
      if (wakeUpWasButton)
        both.println("Nenhum pacote recebido.");

      radio.clearDio1Action();
      break;
    }
  }

  goToDeepSleep();
}

void loop() {
  // Este código é de uma única iteração, ou seja, não há necessidade de
  // programar o loop(), visto que sempre entramos no modo Deep Sleep em algum
  // momento dentro do setup().
}

void displayInfos() {
  both.printf("Bateria: %i%%\n", batteryPercent);
  both.printf("Temperatura: %.2f\n\n", heltec_temperature());
}

void radioSetup() {
  // Inicializa a instância "radio".
  Serial.println("Inicializar rádio...");
  if (wakeUpWasButton) {
    display.println("Inicializar rádio...");
    heltec_delay(2000);
  }
  RADIOLIB_OR_HALT(radio.begin());

  // Configura os paramêtros do "radio".
  Serial.printf("Frequência: %.2f MHz\n", FREQUENCY);
  if (wakeUpWasButton) {
    display.printf("Frequência: %.2f MHz\n", FREQUENCY);
    heltec_delay(2000);
  }
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));

  Serial.printf("Largura de banda: %.1f kHz\n", BANDWIDTH);
  if (wakeUpWasButton) {
    display.printf("Largura de\nbanda: %.1f kHz\n", BANDWIDTH);
    heltec_delay(2000);
  }
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));

  Serial.printf("Fator de espalhamento: %i\n", SPREADING_FACTOR);
  if (wakeUpWasButton) {
    display.printf("Fator de\nespalhamento: %i\n", SPREADING_FACTOR);
    heltec_delay(2000);
  }
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));

  Serial.printf("Poder TX: %i dBm\n", TRANSMIT_POWER);
  if (wakeUpWasButton) {
    display.printf("Poder TX: %i dBm\n", TRANSMIT_POWER);
    heltec_delay(2000);
  }
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));

  // Configura uma interrupção no pino DIO1 para chamar setRxFlag() quando receber
  // um pacote.
  radio.setDio1Action(setRxFlag);
}

void setRxFlag() {
  rxFlag = true;
}

void goToDeepSleep() {
  Serial.print("Entrando no modo Deep Sleep");
  if (wakeUpWasButton) {
    display.print("Entrando no modo Deep\nSleep em...");
    for (uint8_t seconds = 0; seconds < 10; seconds++) {
      display.printf("%i... ", 10 - seconds);
      heltec_delay(1000);
      if (seconds % 3 == 0)
        display.print("\n");
    }
  }

  Serial.printf(".\nPróxima transmissão ocorrerá em %lu segundos.\n", DEEP_SLEEP_TIME);

  heltec_deep_sleep(DEEP_SLEEP_TIME);
}