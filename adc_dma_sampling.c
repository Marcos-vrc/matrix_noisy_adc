/* 
Marcos vinicius Rodrigues Costa
until now i´ve been trying to do dma with one channel only but the time has come to utilize a dma
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
uint16_t adc_buffer_0[capture_depth];
uint16_t adc_buffer_1[capture_depth];

int dma_adc0,control_channel;
bool led_on=true;
int adc=0;

uint16_t* buffer[]={adc_buffer_0,adc_buffer_1};

void set_adc(){
    if (adc<capture_depth)
    {
        adc_fifo_drain();
        adc_select_input(0);
        //dma_channel_transfer_to_buffer_now(dma_adc0,adc_buffer_0,capture_depth);
        dma_channel_set_write_addr(dma_adc0,&adc_buffer_0,true);
        adc+=capture_depth;
    }
    else
    {
        //printf("rodou\n");
        adc_select_input(1);
        //adc_fifo_drain();
        //dma_channel_transfer_to_buffer_now(dma_adc0,adc_buffer_1,capture_depth);
        dma_channel_set_write_addr(dma_adc0,&adc_buffer_1,true);
        adc-=capture_depth;
    }
    dma_hw->ints0 = 1u << dma_adc0;   
}

void update_pwm(){
    pwm_clear_irq(pwm_gpio_to_slice_num(PICO_DEFAULT_LED_PIN));
    pwm_set_gpio_level(15, adc_buffer_1[0] * adc_buffer_1[0]);
}

void setup_gpio(){
    gpio_set_function(PICO_DEFAULT_LED_PIN, GPIO_FUNC_PWM);
    gpio_set_function(15,GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PICO_DEFAULT_LED_PIN);
    uint led_num = pwm_gpio_to_slice_num(15);
    pwm_config config = pwm_get_default_config();
    pwm_init(slice_num, &config, true);
    pwm_init(led_num,&config, true);
    //pwm_clear_irq(slice_num);
    //pwm_clear_irq(led_num);
    //pwm_set_irq_enabled(slice_num, true);
    //pwm_set_irq_enabled(led_num, true);
    //irq_set_exclusive_handler(PWM_DEFAULT_IRQ_NUM(), update_pwm);
    //irq_set_enabled(PWM_DEFAULT_IRQ_NUM(), true);
    adc_gpio_init(ADC_PIN);
}

void setup_adc(){
    adc_init();
    adc_select_input(ADC_NUM);
    adc_fifo_setup(true,true,1,false,false);
    adc_set_clkdiv(0);
    adc_fifo_drain();
}

void setup_dma_adc(){
    int dma_adc0=dma_claim_unused_channel(true);
    dma_channel_config config=dma_channel_get_default_config(dma_adc0);
    channel_config_set_transfer_data_size(&config,DMA_SIZE_16);
    channel_config_set_read_increment(&config,false);
    channel_config_set_write_increment(&config,true);
    channel_config_set_dreq(&config,DREQ_ADC);
    dma_channel_configure(dma_adc0,&config,NULL,&adc_hw->fifo,capture_depth,true);
    dma_channel_set_irq0_enabled(dma_adc0, true);
    irq_set_exclusive_handler(DMA_IRQ_0, set_adc);
    irq_set_enabled(DMA_IRQ_0, true);
}

int main() {
    stdio_init_all();
    setup_gpio();
    setup_adc();
    setup_dma_adc();
    adc_run(true);
    set_adc();
    while (1) {
        printf("buffer 0 \n");
        for (int i = 0; i < capture_depth; ++i) {
            printf("%-3d,", adc_buffer_0[i]);
            //pwm_set_gpio_level(PICO_DEFAULT_LED_PIN, adc_buffer_0[i]* adc_buffer_0[i]);
            if (i % 10 == 9)
                printf("\n");
                
        }
        sleep_ms(250);
        printf("buffer 1 \n");
        for (int i = 0; i < capture_depth; ++i) {
            printf("%-3d,", adc_buffer_1[i]);
            //pwm_set_gpio_level(15, adc_buffer_1[i]* adc_buffer_1[i]);
            if (i % 10 == 9)
                printf("\n");
        }
        sleep_ms(250);
    }
}
