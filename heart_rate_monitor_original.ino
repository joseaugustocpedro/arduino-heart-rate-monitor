#define ledPin 3 //Define o pino ledPin do Arduino  
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
const int pinoSensor = A0;

// Variáveis para auto-calibragem
int limiar = 512;
int sinalAnterior = 0;
unsigned long ultimoBatimento = 0;
int BPM = 0;
int batimentos[10];
int indiceBatimento = 0;

// Variáveis para encontrar picos e vales dinamicamente
int pico = 0;
int vale = 1023;
unsigned long ultimaAtualizacao = 0;

// Variáveis de controle
bool monitorando = false;
bool calibrado = false;

// Novas variáveis para medição precisa
unsigned long temposBatimentos[5]; // Array para últimos tempos de batimento
int indiceTempo = 0;
int totalTempos = 0;
bool batimentoDetectado = false;
const unsigned long INTERVALO_MINIMO = 400; // Mínimo 400ms entre batimentos (150 BPM)
const unsigned long INTERVALO_MAXIMO = 1500; // Máximo 1500ms entre batimentos (40 BPM)

// Variáveis para Serial Plotter rápido
unsigned long ultimoPlot = 0;
const unsigned long INTERVALO_PLOT = 20; // Atualiza a cada 20ms (50Hz)

void setup() {
    Serial.begin(9600);
  // Configuração do display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("MONITOR DE BATIMENTO CARDIACO POR MINUTO");
  display.display();
  Serial.println("=== MONITOR DE BATIMENTO CARDÍACO ===");
  Serial.println("Comandos:");
  Serial.println("'c' - Calibrar sensor");
  Serial.println("'i' - Iniciar monitoramento");
  Serial.println("'p' - Parar monitoramento");
  Serial.println("'s' - Status do sistema");
  Serial.println("'r' - Reiniciar sistema");
  Serial.println();
  digitalWrite(ledPin, LOW);
}

void calibrarSensor() {
  Serial.println("=== INICIANDO CALIBRAÇÃO ===");
  Serial.println("Coloque o dedo no sensor e mantenha parado...");
      display.clearDisplay();
      display.setCursor(0, 10);
      display.setTextSize(1);
      display.println("Coloque o dedo");
      display.setTextSize(1);
      display.println("e mantenha parado");
      display.setTextSize(2);
      display.print("Aguarde...");
      display.display();
      delay(100); 
  digitalWrite(ledPin, HIGH); //ledPin do Arduino acende
  
  pico = 0;
  vale = 1023;
  calibrado = false;
  
  // Período de calibração inicial
  unsigned long inicioCalibracao = millis();
  int leituras = 0;
  
  while (millis() - inicioCalibracao < 8000) { // 8 segundos de calibração
    int leitura = analogRead(pinoSensor);
    
    // Atualiza pico e vale dinamicamente
    if (leitura > pico) {
      pico = leitura;
    }
    if (leitura < vale) {
      vale = leitura;
    }
    
    leituras++;
    
    // Feedback visual da calibração
    if (leituras % 50 == 0) {
      Serial.print(".");
    }
    
    delay(50);
  }
  
  // Calcula limiar como ponto médio com margem de segurança
  limiar = (pico + vale) / 2;
  
  calibrado = true;
  Serial.println();
  Serial.println("=== CALIBRAÇÃO CONCLUÍDA ===");
  Serial.print("Pico detectado: ");
  Serial.println(pico);
  Serial.print("Vale detectado: ");
  Serial.println(vale);
  Serial.print("Limiar ajustado para: ");
  Serial.println(limiar);
  Serial.println("Pronto para iniciar monitoramento!");
  Serial.println("Pressione 'i' para iniciar.");  
  display.clearDisplay();
  display.setCursor(0, 10);
  display.setTextSize(2);
  display.println("CALIBRACAO");
  display.setTextSize(2);
  display.println("CONCLUIDA");
  display.display();
  digitalWrite(ledPin, LOW);
  delay(100); 
}

void iniciarMonitoramento() {
  if (!calibrado) {
    Serial.println("ERRO: Sensor não calibrado! Pressione 'c' para calibrar.");
    return;
  }
  
  monitorando = true;
  indiceBatimento = 0;
  indiceTempo = 0;
  totalTempos = 0;
  BPM = 0;
  
  // Limpa arrays
  for (int i = 0; i < 10; i++) {
    batimentos[i] = 0;
  }
  for (int i = 0; i < 5; i++) {
    temposBatimentos[i] = 0;
  }
  
  Serial.println("=== MONITORAMENTO INICIADO ===");
  Serial.println("Sinal\tLimiar\tBPM"); // Cabeçalho para Serial Plotter
  Serial.println("Monitorando batimentos cardíacos...");
  Serial.println("Pressione 'p' para parar.");
  digitalWrite(ledPin, HIGH); //ledPin do Arduino acende
}

void pararMonitoramento() {
  monitorando = false;
  Serial.println("=== MONITORAMENTO PARADO ===");
  if (BPM > 0) {
    Serial.print("Último BPM registrado: ");
    Serial.println(BPM);
    digitalWrite(ledPin, LOW); //ledPin do apaga
      display.clearDisplay();
      display.setCursor(0, 10);
      display.setTextSize(1);
      display.println("Monitoramento pausado");
      display.setTextSize(2); 
      display.print("R: resetar");
      display.setTextSize(2); 
      display.print("I: iniciar");
      display.display();
      delay(100); 
  }
}

void mostrarStatus() {
  Serial.println("=== STATUS DO SISTEMA ===");
  Serial.print("Calibrado: ");
  Serial.println(calibrado ? "SIM" : "NÃO");
  Serial.print("Monitorando: ");
  Serial.println(monitorando ? "SIM" : "NÃO");
  Serial.print("Limiar atual: ");
  Serial.println(limiar);
  Serial.print("Último BPM: ");
  Serial.println(BPM);
  Serial.print("Sinal atual: ");
  Serial.println(analogRead(pinoSensor));
  digitalWrite(ledPin, LOW);
}

void reiniciarSistema() {
  monitorando = false;
  calibrado = false;
  indiceBatimento = 0;
  indiceTempo = 0;
  totalTempos = 0;
  BPM = 0;
  pico = 0;
  vale = 1023;
  limiar = 512;
  
  Serial.println("=== SISTEMA REINICIADO ===");
  Serial.println("Todos os valores foram resetados.");
  Serial.println("Pressione 'c' para calibrar.");
  display.clearDisplay();
  display.setCursor(0, 10);
  display.setTextSize(1);
  display.println("MONITOR DE BATIMENTO CARDIACO POR MINUTO");
  display.display();
   digitalWrite(ledPin, LOW);
}

void processarComando(char comando) {
  switch (comando) {
    case 'c':
    case 'C':
      calibrarSensor();
      break;
      
    case 'i':
    case 'I':
      iniciarMonitoramento();
      break;
      
    case 'p':
    case 'P':
      pararMonitoramento();
      break;
      
    case 's':
    case 'S':
      mostrarStatus();
      break;
      
    case 'r':
    case 'R':
      reiniciarSistema();
      break;
      
    default:
      Serial.println("Comando não reconhecido. Use: c, i, p, s, r");
      break;
  }
}

int calcularBPM() {
  if (totalTempos < 2) return 0;
  
  // Calcula a média dos intervalos entre os últimos batimentos
  unsigned long somaIntervalos = 0;
  int intervalosValidos = 0;
  
  for (int i = 0; i < totalTempos - 1; i++) {
    int nextIndex = (i + 1) % 5;
    unsigned long intervalo = temposBatimentos[nextIndex] - temposBatimentos[i];
    
    // Verifica se o intervalo está dentro da faixa fisiológica
    if (intervalo >= INTERVALO_MINIMO && intervalo <= INTERVALO_MAXIMO) {
      somaIntervalos += intervalo;
      intervalosValidos++;
    }
  }
  
  if (intervalosValidos > 0) {
    unsigned long mediaIntervalo = somaIntervalos / intervalosValidos;
    int bpmCalculado = 60000 / mediaIntervalo;
    
    // Filtro final rigoroso para adulto (50-120 BPM)
    if (bpmCalculado >= 50 && bpmCalculado <= 120) {
      return bpmCalculado;
    }
  }
  
  return 0;
}

void loop() {
  // Verifica comandos do teclado
  if (Serial.available() > 0) {
    char comando = Serial.read();
    processarComando(comando);
    
    // Limpa buffer serial
    while (Serial.available() > 0) {
      Serial.read();
    }
  }
  
  // Se estiver monitorando, processa os batimentos
  if (monitorando && calibrado) {
    int sinalAtual = analogRead(pinoSensor);
    unsigned long tempoAtual = millis();
    
    if (tempoAtual - ultimoPlot >= INTERVALO_PLOT) {
      ultimoPlot = tempoAtual;
      
      // Envia dados formatados para Serial Plotter
      Serial.print("Sinal:");
      Serial.print(sinalAtual);
      Serial.print("\tLimiar:");
      Serial.print(limiar);
      Serial.print("\tBPM:");
      Serial.println(BPM);
      display.clearDisplay();
      display.setCursor(0, 10);
      display.setTextSize(1);
      display.println("Monitoramento: ");
      display.setTextSize(2);
      display.print("Batimentos: ");
      display.println(BPM); // Exibir o valor atualizado
      display.display();
      display.setTextSize(1);
      display.print("Pulsos: ");
      display.println(sinalAtual); // Exibir o valor atualizado
      display.display();
      delay(100); 
    }
    
    // Atualiza pico e vale periodicamente para adaptar às mudanças
    if (tempoAtual - ultimaAtualizacao > 2000) {
      pico = max(pico - 1, limiar + 50);
      vale = min(vale + 1, limiar - 50);
      limiar = (pico + vale) / 2;
      ultimaAtualizacao = tempoAtual;
    }
    
    // Detecção do batimento - procura por transição de subida através do limiar
    if (sinalAtual > limiar && sinalAnterior <= limiar) {
      if (!batimentoDetectado) {
        batimentoDetectado = true;
        
        // Verifica intervalo mínimo entre batimentos
        if (tempoAtual - ultimoBatimento > INTERVALO_MINIMO) {
          
          // Armazena o tempo deste batimento
          temposBatimentos[indiceTempo] = tempoAtual;
          indiceTempo = (indiceTempo + 1) % 5;
          totalTempos = min(totalTempos + 1, 5);
          
          // Calcula BPM apenas se tiver pelo menos 2 batimentos
          if (totalTempos >= 2) {
            int novoBPM = calcularBPM();
            
            if (novoBPM > 0 && novoBPM != BPM) {
              BPM = novoBPM;
              // Não imprime BPM aqui para não interferir no Serial Plotter
            }
          }
          
          ultimoBatimento = tempoAtual;
        }
      }
    }
    
    // Reset da detecção quando o sinal cai abaixo do limiar
    if (sinalAtual < limiar - 20) {
      batimentoDetectado = false;
    }
    
    sinalAnterior = sinalAtual;
  }
  delay(50);
}