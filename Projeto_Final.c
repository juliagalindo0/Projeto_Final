#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

// Definição dos pinos utilizados
const uint SDA_PIN = 14;
const uint SCL_PIN = 15;
const uint SENSOR_AUDIO = 28;     
const uint ALARME_BUZZER = 21;   
const uint BOTAO_DESATIVAR = 5;  
const uint BOTAO_RESET = 6;  
const uint LED_VERDE = 11; 
const uint LED_AZUL = 12;  

// Definição de constantes para detecção de som
const float NIVEL_SILENCIO = 1.65; // Nível médio de silêncio do sensor
const float LIMIAR_RUIDO = 0.10;   // Sensibilidade para detecção de ruído
const float REFERENCIA_ADC = 3.3;  // Tensão de referência do ADC
const int RESOLUCAO_ADC = 4095;    // Resolução do ADC (12 bits)
const int DETECCOES_NECESSARIAS = 5; // Número mínimo de detecções consecutivas antes de ativar o alarme

// Variáveis de controle do sistema
bool alarme_ativo = false;
int contador_ruidos = 0; 
int total_acionamentos = 0; 

// Inicializa o PWM para o buzzer
void inicializar_pwm(uint pino) {
    gpio_set_function(pino, GPIO_FUNC_PWM);
    uint canal = pwm_gpio_to_slice_num(pino);
    pwm_config configuracao = pwm_get_default_config();
    pwm_config_set_clkdiv(&configuracao, 4.0f); 
    pwm_init(canal, &configuracao, true);
    pwm_set_gpio_level(pino, 0); 
}

// Configura os LEDs de indicação
void configurar_leds() {
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_put(LED_VERDE, 1);  // Indica silêncio inicialmente

    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_put(LED_AZUL, 0);   // LED azul apagado inicialmente
}

// Altera os LEDs conforme o estado do ruído detectado
void alterar_estado_leds(bool som_detectado) {
    if (som_detectado) {
        gpio_put(LED_AZUL, 1);  // Liga LED azul quando há ruído
        gpio_put(LED_VERDE, 0); // Desliga LED verde
    } else {
        gpio_put(LED_AZUL, 0);  // Desliga LED azul quando não há ruído
        gpio_put(LED_VERDE, 1); // Liga LED verde
    }
}

// Ativa o buzzer com PWM
void ligar_buzzer(uint pino) {
    uint canal = pwm_gpio_to_slice_num(pino);
    pwm_set_wrap(canal, 5000);
    pwm_set_gpio_level(pino, 2500); // Define nível de acionamento do som
}

// Desliga o buzzer
void desligar_buzzer(uint pino) {
    pwm_set_gpio_level(pino, 0);
}

// Atualiza o display OLED com a quantidade de ativações do alarme
void atualizar_oled(uint8_t *buffer, struct render_area *area) {
    char mensagem[20];
    snprintf(mensagem, sizeof(mensagem), "Ativacoes: %d", total_acionamentos);

    memset(buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(buffer, 0, 16, "Sistema Alerta");
    ssd1306_draw_string(buffer, 0, 32, mensagem);
    render_on_display(buffer, area);
}

int main() {
    stdio_init_all();

    // Inicializa o ADC e configura o pino do sensor de áudio
    adc_init();
    adc_gpio_init(SENSOR_AUDIO);
    adc_select_input(2); 

    // Inicializa o PWM para o buzzer e configura os LEDs
    inicializar_pwm(ALARME_BUZZER);
    configurar_leds();

    // Configuração do barramento I2C para o display OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // Inicializa o display OLED
    ssd1306_init();

    // Configuração dos botões de desativação do alarme e reset
    gpio_init(BOTAO_DESATIVAR);
    gpio_set_dir(BOTAO_DESATIVAR, GPIO_IN);
    gpio_pull_up(BOTAO_DESATIVAR);

    gpio_init(BOTAO_RESET);
    gpio_set_dir(BOTAO_RESET, GPIO_IN);
    gpio_pull_up(BOTAO_RESET);

    // Área de renderização do display OLED
    struct render_area area_oled = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&area_oled);

    // Limpa o display OLED inicialmente
    uint8_t buffer_oled[ssd1306_buffer_length];
    memset(buffer_oled, 0, ssd1306_buffer_length);
    render_on_display(buffer_oled, &area_oled);

    // Exibe o estado inicial no OLED
    atualizar_oled(buffer_oled, &area_oled);

    while (true) {
        // Verifica se o botão de desativação foi pressionado
        if (gpio_get(BOTAO_DESATIVAR) == 0 && alarme_ativo) {
            alarme_ativo = false;
            desligar_buzzer(ALARME_BUZZER);
            alterar_estado_leds(false);
            sleep_ms(200);
        }

        // Verifica se o botão de reset foi pressionado
        if (gpio_get(BOTAO_RESET) == 0) {
            total_acionamentos = 0;
            atualizar_oled(buffer_oled, &area_oled);
            sleep_ms(200);
        }

        // Faz a leitura do nível de som no sensor
        uint16_t leitura_adc = adc_read();
        float tensao = (leitura_adc * REFERENCIA_ADC) / RESOLUCAO_ADC;
        float nivel_som = fabs(tensao - NIVEL_SILENCIO);

        // Verifica se o som captado ultrapassa o limiar de detecção
        if (nivel_som > LIMIAR_RUIDO) {
            contador_ruidos++;

            // Se atingir o número necessário de detecções, ativa o alarme
            if (contador_ruidos >= DETECCOES_NECESSARIAS && !alarme_ativo) {
                alarme_ativo = true;
                alterar_estado_leds(true);
                ligar_buzzer(ALARME_BUZZER);
                total_acionamentos++;  
                atualizar_oled(buffer_oled, &area_oled); 
            }
        } else {
            contador_ruidos = 0; // Reinicia a contagem caso o som diminua
        }

        sleep_ms(100);
    }

    return 0;
}
