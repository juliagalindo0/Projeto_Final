#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

const uint SDA_PIN = 14;
const uint SCL_PIN = 15;
const uint SENSOR_AUDIO = 28;     
const uint ALARME_BUZZER = 21;   
const uint BOTAO_DESATIVAR = 5;  
const uint BOTAO_RESET = 6;  
const uint LED_VERDE = 11; 
const uint LED_AZUL = 12;  

const float NIVEL_SILENCIO = 1.65; 
const float LIMIAR_RUIDO = 0.10; 
const float REFERENCIA_ADC = 3.3;         
const int RESOLUCAO_ADC = 4095;          
const int DETECCOES_NECESSARIAS = 5;

bool alarme_ativo = false;
int contador_ruidos = 0; 
int total_acionamentos = 0; 

void inicializar_pwm(uint pino) {
    gpio_set_function(pino, GPIO_FUNC_PWM);
    uint canal = pwm_gpio_to_slice_num(pino);
    pwm_config configuracao = pwm_get_default_config();
    pwm_config_set_clkdiv(&configuracao, 4.0f); 
    pwm_init(canal, &configuracao, true);
    pwm_set_gpio_level(pino, 0); 
}

void configurar_leds() {
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_put(LED_VERDE, 1);  

    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_put(LED_AZUL, 0); 
}

void alterar_estado_leds(bool som_detectado) {
    if (som_detectado) {
        gpio_put(LED_AZUL, 1);  
        gpio_put(LED_VERDE, 0); 
    } else {
        gpio_put(LED_AZUL, 0);  
        gpio_put(LED_VERDE, 1); 
    }
}

void ligar_buzzer(uint pino) {
    uint canal = pwm_gpio_to_slice_num(pino);
    pwm_set_wrap(canal, 5000);
    pwm_set_gpio_level(pino, 2500);
}

void desligar_buzzer(uint pino) {
    pwm_set_gpio_level(pino, 0);
}

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

    adc_init();
    adc_gpio_init(SENSOR_AUDIO);
    adc_select_input(2); 

    inicializar_pwm(ALARME_BUZZER);
    configurar_leds();

    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    ssd1306_init();

    gpio_init(BOTAO_DESATIVAR);
    gpio_set_dir(BOTAO_DESATIVAR, GPIO_IN);
    gpio_pull_up(BOTAO_DESATIVAR);

    gpio_init(BOTAO_RESET);
    gpio_set_dir(BOTAO_RESET, GPIO_IN);
    gpio_pull_up(BOTAO_RESET);

    struct render_area area_oled = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&area_oled);

    uint8_t buffer_oled[ssd1306_buffer_length];
    memset(buffer_oled, 0, ssd1306_buffer_length);
    render_on_display(buffer_oled, &area_oled);

    atualizar_oled(buffer_oled, &area_oled);

    while (true) {
        if (gpio_get(BOTAO_DESATIVAR) == 0 && alarme_ativo) {
            alarme_ativo = false;
            desligar_buzzer(ALARME_BUZZER);
            alterar_estado_leds(false);
            sleep_ms(200);
        }

        if (gpio_get(BOTAO_RESET) == 0) {
            total_acionamentos = 0;
            atualizar_oled(buffer_oled, &area_oled);
            sleep_ms(200);
        }

        uint16_t leitura_adc = adc_read();
        float tensao = (leitura_adc * REFERENCIA_ADC) / RESOLUCAO_ADC;
        float nivel_som = fabs(tensao - NIVEL_SILENCIO);

        if (nivel_som > LIMIAR_RUIDO) {
            contador_ruidos++;

            if (contador_ruidos >= DETECCOES_NECESSARIAS && !alarme_ativo) {
                alarme_ativo = true;
                alterar_estado_leds(true);
                ligar_buzzer(ALARME_BUZZER);
                total_acionamentos++;  
                atualizar_oled(buffer_oled, &area_oled); 
            }
        } else {
            contador_ruidos = 0; 
        }

        sleep_ms(100);
    }

    return 0;
}
