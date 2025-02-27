/* 
Marcos vinicius Rodrigues Costa
based on dma_capture from raspberry pi pico example, but continous instead of a batch,
utilizing dma_channel_transfer_to_buffer_now inside a isr and a potentiometer to generate the values.
with the noisy adc is possible to create matrix like animation using the random numbers
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"

#define ADC_NUM 0
#define ADC_PIN (26 + ADC_NUM)
#define ADC_VREF 3.3

#define capture_depth 100
uint16_t adc_buffer[capture_depth];

int dma_channel;
bool led_on=true;

bool update_dma_batch(__unused struct repeating_timer *t){
    dma_channel_transfer_to_buffer_now(dma_channel,adc_buffer,capture_depth);
    return true;
}

bool blink_led(__unused struct repeating_timer *t){
    if (led_on)
    {
        gpio_put(PICO_DEFAULT_LED_PIN,true);
        led_on=false;
    }
    else{
        gpio_put(PICO_DEFAULT_LED_PIN,false);
        led_on=true;
    }
    return true;
}

void setup_gpio(){
    gpio_set_function(PICO_DEFAULT_LED_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PICO_DEFAULT_LED_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.f);
    pwm_init(slice_num, &config, true);
    //gpio_init(PICO_DEFAULT_LED_PIN);
    //gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    adc_gpio_init( ADC_PIN);
}

void setup_adc(){
    adc_init();
    adc_select_input(ADC_NUM);
    adc_fifo_setup(true,true,1,false,false);
    adc_set_clkdiv(0);
}

void setup_dma(){
    int dma_channel=dma_claim_unused_channel(true);
    dma_channel_config config=dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&config,DMA_SIZE_16);
    channel_config_set_read_increment(&config,false);
    channel_config_set_write_increment(&config,true);
    channel_config_set_dreq(&config,DREQ_ADC);
    dma_channel_configure(dma_channel,&config,adc_buffer,&adc_hw->fifo,capture_depth,true);
}

int main() {
    stdio_init_all();
    setup_gpio();
    setup_adc();
    setup_dma();
    adc_run(true);
    struct repeating_timer adc_update;
    add_repeating_timer_us(200,update_dma_batch,NULL,&adc_update);
    while (1) {
        for (int i = 0; i < capture_depth; ++i) {
            printf("%-3d,", adc_buffer[i]);
            pwm_set_gpio_level(PICO_DEFAULT_LED_PIN, adc_buffer[i]*adc_buffer[i]);
            if (i % 10 == 9)
                printf("\n");
        }
    }
}
