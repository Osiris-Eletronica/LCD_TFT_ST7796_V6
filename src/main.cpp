/*******************************************************************************
 * PROJETO: Gerador de Funções 
 * 
 * FUNÇÃO: SENOIDAL, QUADRADA, TRIANGULO, DENTE DE SERRA
 * 
 * DESENVOLVEDOR: Osiris Silva
 * 
 * CARGO: Técnico em Eletrônica
 * 
 * DATA: 10/01/2026
 * 
 * HARDWARE: Tela TFT - ST7796 - Resolução: 480x320
 * - Modelo: ESP WROOM 32 DEV KIT
 * - Não conjugados
 *
 * NOTAS TÉCNICAS:
 *
 * Versão 6
 *******************************************************************************/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <driver/dac.h>

// Pinos de Controle
#define BTN_MODE_PIN 22             
#define BTN_INC_PIN  13 
#define BTN_DEC_PIN  12 
#define BTN_STEP_PIN 14             
#define DAC_CHAN     DAC_CHANNEL_1 
// VOLTMETER_PIN removido

TFT_eSPI tft = TFT_eSPI();

// --- VARIÁVEIS GLOBAIS ---
volatile int modoOnda = 0;       
volatile float frequencia = 1.0; 
volatile float dutyCycle = 50.0;
// measuredVoltage removido
float fase = 0;
String nomes[] = {"SENOIDAL", "QUADRADA", "TRIANGULO", "DENTE SERRA"};
uint16_t cores[] = {TFT_CYAN, TFT_GREEN, TFT_YELLOW, TFT_MAGENTA};

// Parâmetros de ajuste
int currentStepIndex = 1; 
float freqSteps[] = {1.0, 10.0, 100.0};
const float FREQ_MIN = 5.0;
const float FREQ_MAX = 20000.0; 

// Controle de Debounce/Tempo
unsigned long ultimoDebounceBtnInc = 0;
unsigned long ultimoDebounceBtnDec = 0;
unsigned long tempoPressionadoStep = 0;
unsigned long ultimoDebounceBtnMode = 0;


// Variáveis para o Gráfico
#define GRAPH_HEIGHT 120
#define GRAPH_Y_START 70


// --- FUNÇÕES DE DESENHO DA UI ---

void desenharTelaPrincipal() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("GERADOR ESTAVEL 4 BTNS", 240, 10, 2); 
    tft.drawFastHLine(0, GRAPH_Y_START - 2, 480, TFT_DARKGREY); 
    tft.drawFastHLine(0, GRAPH_Y_START + GRAPH_HEIGHT + 2, 480, TFT_DARKGREY); 
}

// NOVO: Função para desenhar a onda estática no gráfico com escala dinâmica
void desenharOndaEstatica() {
    tft.fillRect(0, GRAPH_Y_START, 480, GRAPH_HEIGHT + 2, TFT_BLACK); // Limpa o gráfico
    tft.drawFastHLine(0, GRAPH_Y_START - 2, 480, TFT_DARKGREY);
    tft.drawFastHLine(0, GRAPH_Y_START + GRAPH_HEIGHT + 2, 480, TFT_DARKGREY);

    int oldY = map(0, 0, 255, GRAPH_Y_START + GRAPH_HEIGHT, GRAPH_Y_START);

    // Define o número de ondas baseado no passo de frequência atual
    int numWaves = 3; // Padrão para 1Hz step
    if (freqSteps[currentStepIndex] == 10.0) numWaves = 6;
    else if (freqSteps[currentStepIndex] == 100.0) numWaves = 10;


    for (int x = 0; x < 480; x++) {
        // Ajusta a fase para o número de ondas dinâmico
        float faseEstatica = (TWO_PI * (float)numWaves / 480.0) * x; 
        uint8_t out = 0;
        
        if (modoOnda == 0) out = (uint8_t)((sin(faseEstatica) + 1) * 127);
        else if (modoOnda == 1) { 
            float transitionPoint = TWO_PI * (dutyCycle / 100.0); 
            out = (faseEstatica < transitionPoint || (faseEstatica > TWO_PI && faseEstatica < TWO_PI + transitionPoint) || (faseEstatica > 2*TWO_PI && faseEstatica < 2*TWO_PI + transitionPoint) || (faseEstatica > 3*TWO_PI && faseEstatica < 3*TWO_PI + transitionPoint) || (faseEstatica > 4*TWO_PI && faseEstatica < 4*TWO_PI + transitionPoint) || (faseEstatica > 5*TWO_PI && faseEstatica < 5*TWO_PI + transitionPoint) || (faseEstatica > 6*TWO_PI && faseEstatica < 6*TWO_PI + transitionPoint) || (faseEstatica > 7*TWO_PI && faseEstatica < 7*TWO_PI + transitionPoint) || (faseEstatica > 8*TWO_PI && faseEstatica < 8*TWO_PI + transitionPoint) || (faseEstatica > 9*TWO_PI && faseEstatica < 9*TWO_PI + transitionPoint) ) ? 255 : 0; 
        }
        else if (modoOnda == 2) out = (faseEstatica < TWO_PI / 2.0) ? (uint8_t)((faseEstatica/(TWO_PI/2.0))*255) : (uint8_t)(255-((faseEstatica-(TWO_PI/2.0))/(TWO_PI/2.0))*255);
        else if (modoOnda == 3) out = (uint8_t)((faseEstatica / TWO_PI) * 255);
        
        int newY = map(out, 0, 255, GRAPH_Y_START + GRAPH_HEIGHT, GRAPH_Y_START);
        if (x > 0) tft.drawLine(x - 1, oldY, x, newY, cores[modoOnda]);
        oldY = newY;
    }
}


void atualizarDadosNaTela() {
    tft.setTextPadding(400); 
    tft.setTextColor(cores[modoOnda], TFT_BLACK);
    tft.drawCentreString(nomes[modoOnda], 240, 35, 4);
    
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawCentreString(String(frequencia, 1) + " Hz   ", 240, 210, 4); 
    
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawCentreString("Passo: " + String((int)freqSteps[currentStepIndex]) + " Hz", 240, 240, 2);

    if (modoOnda == 1) {
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawCentreString("DC: " + String((int)dutyCycle) + "%", 240, 275, 2); 
    } else {
        tft.fillRect(150, 270, 180, 25, TFT_BLACK); 
    }
    
    // Voltímetro removido, limpando o espaço
    tft.fillRect(350, 300, 120, 20, TFT_BLACK);
}


// --- SETUP E LOOP PRINCIPAL ---
void setup() {
    Serial.begin(115200);
    dac_output_enable(DAC_CHAN);
    pinMode(BTN_MODE_PIN, INPUT_PULLUP);
    pinMode(BTN_INC_PIN, INPUT_PULLUP);
    pinMode(BTN_DEC_PIN, INPUT_PULLUP);
    pinMode(BTN_STEP_PIN, INPUT_PULLUP); 
    tft.init();
    tft.setRotation(1);
    // analogSetAttenuation removido
    // analogRead/measuredVoltage removidos

    desenharTelaPrincipal(); 
    desenharOndaEstatica(); // Desenha a onda inicial de 1Hz com 3 ciclos
    atualizarDadosNaTela();
}

void loop() {
    // --- 1. GERAÇÃO DA ONDA (Roda sempre) ---
    float incremento = (frequencia * TWO_PI) / 10000.0; uint8_t out = 0;
    if (modoOnda == 0) out = (uint8_t)((sin(fase) + 1) * 127);
    else if (modoOnda == 1) { float transitionPoint = TWO_PI * (dutyCycle / 100.0); out = (fase < transitionPoint) ? 255 : 0; }
    else if (modoOnda == 2) out = (fase < TWO_PI / 2.0) ? (uint8_t)((fase/(TWO_PI/2.0))*255) : (uint8_t)(255-((fase-(TWO_PI/2.0))/(TWO_PI/2.0))*255);
    else if (modoOnda == 3) out = (uint8_t)((fase / TWO_PI) * 255);
    dac_output_voltage(DAC_CHAN, out);
    fase += incremento; if (fase >= TWO_PI) fase = 0;


    // --- 2. LEITURA DE BOTÕES ---
    static unsigned long timerUI = 0;
    if (millis() - timerUI > 100) { 
        timerUI = millis(); bool atualizaTela = false; unsigned long agora = millis();
        bool atualizaGrafico = false;

        // Leitura do Voltímetro removida

        // Botão MODO (Alterna modo de onda)
        if (digitalRead(BTN_MODE_PIN) == LOW && (agora - ultimoDebounceBtnMode > 300)) {
            modoOnda = (modoOnda + 1) % 4; fase = 0; ultimoDebounceBtnMode = agora; atualizaTela = true; atualizaGrafico = true; }
        

        // Lógica do botão STEP (Clique Rápido vs Segurar)
        if (digitalRead(BTN_STEP_PIN) == LOW) {
            if (tempoPressionadoStep == 0) { tempoPressionadoStep = agora; } 
            else if (agora - tempoPressionadoStep > 5000) { 
                frequencia = 1.0; 
                tempoPressionadoStep = agora; 
                atualizaTela = true;
                tempoPressionadoStep = 0; 
            }
        } else {
            if (tempoPressionadoStep != 0) {
                if (agora - tempoPressionadoStep < 5000) {
                    currentStepIndex = (currentStepIndex + 1) % 3;
                    atualizaTela = true;
                    atualizaGrafico = true; // NOVO: Atualiza gráfico ao mudar o passo
                }
                tempoPressionadoStep = 0; 
            }
        }


        // Botão Incrementar/Decrementar
        if (digitalRead(BTN_INC_PIN) == LOW && (agora - ultimoDebounceBtnInc > 150)) {
            if(modoOnda == 1) { if(dutyCycle < 100) dutyCycle += 1; atualizaGrafico = true; } 
            else { frequencia += freqSteps[currentStepIndex]; if (frequencia > FREQ_MAX) frequencia = FREQ_MAX; }
            ultimoDebounceBtnInc = agora; atualizaTela = true; }
        if (digitalRead(BTN_DEC_PIN) == LOW && (agora - ultimoDebounceBtnDec > 150)) {
            if(modoOnda == 1) { if(dutyCycle > 0) dutyCycle -= 1; atualizaGrafico = true; } 
            else { frequencia -= freqSteps[currentStepIndex]; if (frequencia < FREQ_MIN) frequencia = FREQ_MIN; }
            ultimoDebounceBtnDec = agora; atualizaTela = true; }

        if (atualizaGrafico) { desenharOndaEstatica(); } 
        if (atualizaTela) { atualizarDadosNaTela(); }
    }
    yield(); 
}
