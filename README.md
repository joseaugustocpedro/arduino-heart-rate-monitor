# Heart Rate Monitor — Arduino + OLED

> Protótipo embarcado para aquisição de sinal de pulso, calibração automática do limiar, detecção de batimentos e estimativa de frequência cardíaca em BPM.

![Arduino](https://img.shields.io/badge/Arduino-Embedded%20C%2B%2B-00979D?logo=arduino&logoColor=white)
![OLED](https://img.shields.io/badge/Display-SSD1306-4B5563)
![Signal](https://img.shields.io/badge/Signal-Analog%20Pulse%20Sensing-7C3AED)
![Status](https://img.shields.io/badge/status-prototype-orange)

## Visão geral

Este projeto implementa um **monitor de frequência cardíaca embarcado** utilizando Arduino, um sensor analógico de pulso e um display OLED SSD1306 de 128 × 64 pixels.

O firmware realiza a aquisição do sinal pelo conversor A/D do microcontrolador, executa uma etapa de **auto-calibração**, define dinamicamente um limiar de detecção, identifica transições associadas aos batimentos e estima a frequência cardíaca em **batimentos por minuto (BPM)** a partir dos intervalos temporais entre eventos válidos.

Além da interface local no display OLED, o sistema disponibiliza diagnóstico, comandos de operação e dados para visualização no **Serial Monitor / Serial Plotter**.

> **Aviso:** este é um protótipo educacional de instrumentação e sistemas embarcados. Não é um dispositivo médico e não deve ser utilizado para diagnóstico, monitoramento clínico ou tomada de decisão em saúde.

---

## Objetivos de engenharia

O projeto explora conceitos importantes de Engenharia de Controle e Automação e Sistemas Embarcados:

- aquisição de sinais analógicos;
- conversão A/D;
- instrumentação biomédica básica;
- detecção de eventos por limiar;
- calibração automática;
- processamento temporal de sinais;
- filtragem por restrições fisiológicas;
- buffers circulares;
- comunicação serial;
- interface homem-máquina com display OLED;
- programação embarcada em C/C++;
- integração entre sensor, microcontrolador e interface de visualização.

---

## Arquitetura do sistema

```text
        +-----------------------+
        |   Sensor de Pulso     |
        |    Saída Analógica    |
        +-----------+-----------+
                    |
                    | sinal analógico
                    v
        +-----------------------+
        |       Arduino         |
        |                       |
        |  ADC -> Calibração    |
        |       -> Limiar       |
        |       -> Detecção     |
        |       -> Intervalos   |
        |       -> BPM          |
        +-----+------------+----+
              |            |
              | I2C        | USB Serial
              v            v
      +---------------+  +----------------+
      | OLED SSD1306  |  | Serial Monitor |
      |   128 x 64    |  | / Plotter      |
      +---------------+  +----------------+
```

### Fluxo lógico

```text
Inicialização
     |
     v
Calibração do sensor
     |
     v
Aquisição analógica
     |
     v
Atualização de pico/vale
     |
     v
Cálculo do limiar adaptativo
     |
     v
Cruzamento ascendente do limiar?
     |
    sim
     |
     v
Validação do intervalo temporal
     |
     v
Armazenamento dos tempos
     |
     v
Média dos intervalos válidos
     |
     v
BPM = 60000 / intervalo médio
     |
     v
OLED + Serial
```

---

## Hardware

A implementação foi escrita para uma plataforma Arduino com entrada analógica `A0` e barramento I2C disponível.

### Componentes principais

| Componente | Função |
|---|---|
| Arduino compatível | Aquisição, processamento e controle |
| Sensor analógico de pulso | Geração do sinal relacionado ao pulso periférico |
| OLED SSD1306 128 × 64 | Exibição local de estado, sinal e BPM |
| LED | Indicação visual de calibração/monitoramento |
| Protoboard e jumpers | Interconexão do protótipo |
| Cabo USB | Alimentação, programação e comunicação serial |

### Pinagem utilizada no firmware

| Sinal | Pino |
|---|---:|
| Sensor de pulso | `A0` |
| LED | `D3` |
| OLED SDA | barramento `SDA` da placa |
| OLED SCL | barramento `SCL` da placa |
| Endereço I2C OLED | `0x3C` |

> Os pinos físicos de SDA/SCL variam conforme a placa Arduino utilizada. Consulte a pinagem oficial da sua placa.

---

## Dependências de software

O firmware utiliza:

- Arduino IDE;
- `Wire`;
- `Adafruit GFX Library`;
- `Adafruit SSD1306`.

As bibliotecas da Adafruit podem ser instaladas pelo **Library Manager** da Arduino IDE.

---

## Funcionamento do algoritmo

### 1. Auto-calibração

Ao receber o comando `c`, o sistema coleta amostras durante aproximadamente **8 segundos**.

Nesse período são estimados:

```text
pico = maior valor medido
vale = menor valor medido
```

O limiar inicial é definido como:

```text
limiar = (pico + vale) / 2
```

Essa estratégia permite adaptar a detecção a diferenças de posicionamento do dedo, intensidade do sinal e condições do sensor.

### 2. Detecção do batimento

Um evento é considerado candidato a batimento quando o sinal realiza uma **transição ascendente através do limiar**:

```cpp
sinalAtual > limiar && sinalAnterior <= limiar
```

Para reduzir dupla contagem, o firmware exige que o sinal volte para uma região abaixo do limiar antes de permitir uma nova detecção.

### 3. Restrição temporal

O firmware utiliza uma janela temporal aceitável entre batimentos:

```cpp
INTERVALO_MINIMO = 400 ms
INTERVALO_MAXIMO = 1500 ms
```

Esses valores correspondem aproximadamente ao intervalo de:

```text
150 BPM -> 400 ms
 40 BPM -> 1500 ms
```

A versão atual ainda aplica um filtro final mais restritivo na saída exibida, aceitando estimativas entre **50 e 120 BPM**.

### 4. Estimativa do BPM

Os instantes dos últimos batimentos são armazenados em um pequeno buffer circular.

Para intervalos válidos:

```text
BPM = 60000 / intervalo_médio_ms
```

A média de vários intervalos reduz variações instantâneas em comparação com a utilização de apenas dois batimentos consecutivos.

---

## Máquina de estados operacional

O sistema possui, conceitualmente, os seguintes estados:

```text
RESET
  |
  v
NÃO CALIBRADO
  |
  | comando 'c'
  v
CALIBRAÇÃO
  |
  v
PRONTO
  |
  | comando 'i'
  v
MONITORAMENTO
  |
  | comando 'p'
  v
PAUSADO
```

O comando `r` retorna o sistema ao estado inicial.

---

## Comandos pelo Serial Monitor

Configure a comunicação serial em **9600 baud**.

| Comando | Função |
|---|---|
| `c` | calibrar o sensor |
| `i` | iniciar o monitoramento |
| `p` | pausar o monitoramento |
| `s` | exibir o status do sistema |
| `r` | reiniciar variáveis e estado lógico |

### Sequência recomendada

```text
1. Energizar o circuito
2. Posicionar o dedo corretamente no sensor
3. Enviar: c
4. Permanecer imóvel durante a calibração
5. Enviar: i
6. Aguardar a estabilização da leitura
7. Utilizar p para pausar
```

---

## Saída para Serial Plotter

Durante o monitoramento são transmitidas três grandezas:

```text
Sinal:<valor>    Limiar:<valor>    BPM:<valor>
```

Isso permite acompanhar visualmente:

- forma de onda do sensor;
- posição do limiar de detecção;
- valor de BPM calculado.

Exemplo conceitual:

```text
Amplitude
  ^
  |           /\          /\
  |          /  \        /  \
  |---------/----\------/----\------- limiar
  |        /      \    /      \
  +---------------------------------> tempo
              ^           ^
           batimento    batimento
```

---

## Estrutura do repositório

```text
arduino-heart-rate-monitor/
├── heart_rate_monitor.ino
├── heart_rate_monitor_original.ino
├── README.md
└── .gitignore
```

### Arquivos

**`heart_rate_monitor.ino`**  
Versão revisada para portfólio. Mantém a arquitetura do firmware original e incorpora duas correções técnicas pequenas:

1. configuração explícita do pino do LED como saída;
2. correção da ordem cronológica do buffer circular usado no cálculo de BPM após o buffer completar uma volta.

**`heart_rate_monitor_original.ino`**  
Código fonte exatamente como fornecido originalmente, preservado para rastreabilidade e comparação.

---

## Como executar

1. Clone ou baixe este repositório.
2. Abra `heart_rate_monitor.ino` na Arduino IDE.
3. Instale `Adafruit GFX Library` e `Adafruit SSD1306`.
4. Conecte o sensor de pulso em `A0`.
5. Conecte o LED ao pino digital `3`, utilizando resistor apropriado.
6. Conecte o OLED ao barramento I2C.
7. Confirme se o endereço do display é `0x3C`.
8. Selecione a placa e a porta serial corretas.
9. Compile e faça o upload.
10. Abra o Serial Monitor em `9600 baud`.
11. Execute a calibração com `c`.
12. Inicie a aquisição com `i`.

---

## Decisões de projeto

### Limiar adaptativo

Um limiar fixo poderia funcionar apenas para uma condição específica de uso. A auto-calibração aumenta a robustez frente a mudanças na amplitude do sensor.

### Tempo mínimo entre eventos

A imposição de um intervalo mínimo reduz a possibilidade de contabilizar ruído ou oscilações do mesmo pulso como batimentos distintos.

### Média temporal

A estimativa com múltiplos intervalos busca reduzir jitter no BPM calculado.

### Interface serial + OLED

A combinação das duas interfaces permite tanto uso local quanto análise do sinal durante desenvolvimento e validação.

---

## Limitações atuais

Como todo protótipo acadêmico, existem oportunidades claras de evolução:

- o algoritmo é baseado principalmente em limiar e não em processamento digital avançado;
- movimento do dedo pode introduzir artefatos;
- pressão sobre o sensor pode alterar a amplitude;
- iluminação ambiente pode afetar determinados sensores ópticos;
- não existe validação clínica;
- a faixa final de 50–120 BPM é uma restrição definida no firmware, não uma classificação médica;
- os `delay()` existentes tornam a taxa efetiva de execução menor que a frequência teórica indicada por `INTERVALO_PLOT = 20 ms`;
- não há armazenamento persistente de medições;
- não há timestamp em RTC;
- não há conectividade sem fio.

---

## Possíveis evoluções

Algumas extensões tecnicamente interessantes:

- filtro passa-baixas digital;
- filtro para remoção de componente DC;
- média móvel ou filtro exponencial;
- detecção de pico com histerese;
- cálculo de variabilidade da frequência cardíaca;
- rejeição de artefatos de movimento;
- ESP32 com dashboard Web;
- transmissão via Bluetooth Low Energy;
- registro em cartão microSD;
- timestamp por RTC;
- exportação CSV;
- análise offline em Python;
- validação experimental comparativa com equipamento de referência.

---

## Aplicações educacionais

O projeto pode ser utilizado como estudo de caso para:

- Instrumentação;
- Sistemas Embarcados;
- Microcontroladores;
- Processamento Digital de Sinais;
- Eletrônica;
- Engenharia Biomédica introdutória;
- Engenharia de Controle e Automação.

---

## Observação sobre segurança e uso

Este firmware **não possui certificação, validação clínica ou mecanismos de segurança exigidos para equipamentos médicos**.

Os valores fornecidos devem ser interpretados apenas como dados experimentais do protótipo.

---

## Autor / manutenção

Repositório mantido por **José Augusto C. Pedro**.

Projeto de caráter educacional voltado ao estudo de sistemas embarcados, aquisição de sinais e instrumentação.

---

## Status

**Protótipo funcional em desenvolvimento.**

A arquitetura atual estabelece uma base para futuras implementações com filtragem digital, conectividade e aquisição de dados mais robusta.
