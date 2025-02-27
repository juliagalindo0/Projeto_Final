# Sistema Inteligente de Monitoramento e Alerta de Ruído
Este projeto tem como objetivo monitorar o nível de ruído ambiente e ativar um indicador visual e sonoro caso o som ultrapasse um limite predefinido. Ele utiliza um microfone para capturar o som e um display OLED para exibir o número de ativações do sistema. Se o ruído for detectado por um período consecutivo, o LED azul acende e o buzzer soa. O usuário pode desativar o alarme pressionando um botão e resetar o contador de ativações com outro botão.
Link para o vídeo: [https://drive.google.com/file/d/1AbWN1fo4X-k8St-NgF5TG-kTROnyEOs_/view?usp=sharing](https://drive.google.com/file/d/1AbWN1fo4X-k8St-NgF5TG-kTROnyEOs_/view?usp=sharing)
## 🎯 Objetivos
Monitorar o nível de som ambiente usando o ADC do Raspberry Pi Pico.

Exibir o número total de ativações no display OLED.

Acionar LEDs e um buzzer caso o ruído ultrapasse um limite pré-definido.

Permitir ao usuário desligar o alarme e resetar o contador de ativações com botões físicos.

## 🛠️ Componentes Utilizados
Microcontrolador RP2040

Microfone - Captura o som ambiente

Buzzer - Emite alerta sonoro quando o ruído ultrapassa o limite

LED Verde -	Indica ambiente silencioso

LED Azul -	Indica detecção de ruído

Botão de Desativação - Desativa o alarme

Botão de Reset -	Reinicia o contador de ativações

Display OLED -	Exibe o número total de ativações

## ⚙️ Funcionamento
O microfone captura o nível de som e converte em um sinal analógico.

O sistema monitora esse nível e, se for superior ao limiar, ativa o alarme.

LEDs indicam o status:

Verde: Ambiente silencioso.

Azul: Ruído detectado.

O buzzer soa quando o ruído é detectado.

O display OLED exibe o número total de ativações.

O botão de desativação desliga o alarme e o botão de reset reinicia o contador.

