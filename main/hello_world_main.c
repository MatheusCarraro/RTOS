/*
Arquivo: RTOS.c
Autor:  Andriy Fujii
        Jonath Herdt
        Matheus Emanuel
Função do arquivo: 
        Cria threads para contagem, pesagem e exibição de
        produtos em 3 esteiras.
Criado em 02 de dezembro de 2020
Modificado em 02 de dezembro de 2020
*/

#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/touch_pad.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_timer.h"

// Tempo do display
#define TEMPO_DISPLAY 2000

 // Tempo das esteiras
#define TEMPO_1 1000
#define TEMPO_2 500
#define TEMPO_3 100

// Peso dos produtos
#define PESO_1 5.0
#define PESO_2 2.0
#define PESO_3 0.5

// Qtd produtos
#define TAM 200

#define TOUCH_THRESH_NO_USE   (0)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (100)

static int num_produtos;
static float pesos[TAM];

static bool s_pad_activated[TOUCH_PAD_MAX];
static uint32_t s_pad_init_val[TOUCH_PAD_MAX];

TaskHandle_t vTaskHandleEsteira1 = NULL;
TaskHandle_t vTaskHandleEsteira2 = NULL;
TaskHandle_t vTaskHandleEsteira3 = NULL;
TaskHandle_t vTaskHandleDisplay = NULL;
TaskHandle_t vTaskHandleSoma = NULL;

SemaphoreHandle_t binSemaphore = NULL;

int64_t start_total, end_total, start_soma, end_soma;
double total_time;

// Soma o vetor de pesos
void soma_pesos()
{
    float peso_total = 0;
    
    for(int i = 0; i < TAM; i++)
    {
        peso_total += pesos[i];
    }

    printf("Peso total: %f \n", peso_total);    
}

// Acrescenta um produto no vetor de produtos
void soma_produto(float peso)
{
    // Mutex utilizando semáfaro binário
    xSemaphoreTake(binSemaphore, portMAX_DELAY);

    pesos[num_produtos] = peso;
    num_produtos++;

    if (num_produtos >= TAM)
    {   

        start_soma = esp_timer_get_time();

        num_produtos = 0;
        soma_pesos();

        // Calculando tempo da soma
        end_soma = esp_timer_get_time();
        total_time = ((double) (end_soma - start_soma)) / 1000000;
        printf("Tempo soma total: %f\n", total_time);

        // Calculando tempo de execução
        total_time = ((double) (end_soma - start_total)) / 1000000;
        printf("Tempo execução total: %f\n", total_time);
        start_total = esp_timer_get_time();
    }
    
    // Fim da sessão crítica
    xSemaphoreGive(binSemaphore);
}

void esteira_1(void *pvParameter)
{    
    TickType_t xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount ();

	while(1)
	{
        vTaskDelayUntil(&xLastWakeTime, TEMPO_1 / portTICK_RATE_MS);

        soma_produto(PESO_1);
	}
}
void esteira_2(void *pvParameter)
{
    TickType_t xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount ();

    while(1)
	{
        vTaskDelayUntil(&xLastWakeTime, TEMPO_2 / portTICK_RATE_MS);

        soma_produto(PESO_2);
	}
}
void esteira_3(void *pvParameter)
{
    TickType_t xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount ();

    while(1)
	{
        vTaskDelayUntil(&xLastWakeTime, TEMPO_3 / portTICK_RATE_MS);

        soma_produto(PESO_3);
	}
}


void display(void *pvParameter)
{
    TickType_t xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount ();

    while(1) 
    {
        vTaskDelayUntil(&xLastWakeTime, TEMPO_DISPLAY / portTICK_RATE_MS);
        printf("Quantidade produtos: %d\n", num_produtos);
    }
}


void touch(void *pvParameter)
{
    while(1) 
    {
        touch_pad_intr_enable();
        for (int i = 0; i < TOUCH_PAD_MAX; i++) 
        {
            if(s_pad_activated[i] == true)
            {
                s_pad_activated[i] = false;

                printf("Encerrando\n");

                vTaskDelete(vTaskHandleEsteira1);
                vTaskDelete(vTaskHandleEsteira2);
                vTaskDelete(vTaskHandleEsteira3);
                vTaskDelete(vTaskHandleDisplay);
                vTaskDelete(NULL);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static void tp_example_set_thresholds(void)
{
    uint16_t touch_value;
    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
        //read filtered value
        touch_pad_read_filtered(i, &touch_value);
        s_pad_init_val[i] = touch_value;

        //set interrupt threshold.
        ESP_ERROR_CHECK(touch_pad_set_thresh(i, touch_value * 2 / 3));

    }
}

static void tp_example_rtc_intr(void *arg)
{
    uint32_t pad_intr = touch_pad_get_status();
    //clear interrupt
    touch_pad_clear_status();

    for (int i = 0; i < TOUCH_PAD_MAX; i++)
    {
        if ((pad_intr >> i) & 0x01)
        {
            s_pad_activated[i] = true;
        }
    }
}

static void tp_example_touch_pad_init(void)
{
    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
        //init RTC IO and mode for touch pad.
        touch_pad_config(i, TOUCH_THRESH_NO_USE);
    }
}


void app_main()
{
    num_produtos = 0;

    nvs_flash_init();
    vSemaphoreCreateBinary(binSemaphore);

    // Para usar o touch
    touch_pad_init();
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    tp_example_touch_pad_init();
    touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
    tp_example_set_thresholds();
    touch_pad_isr_register(tp_example_rtc_intr, NULL);

    xTaskCreate(&touch, "touch", 2048, NULL, 5, NULL);

    xTaskCreate(&display, "display", 2048, NULL, 5, &vTaskHandleDisplay);

    start_total = esp_timer_get_time();
    xTaskCreate(&esteira_1, "esteira_1", 2048, NULL, 5, &vTaskHandleEsteira1);
    xTaskCreate(&esteira_2, "esteira_2", 2048, NULL, 5, &vTaskHandleEsteira2);
    xTaskCreate(&esteira_3, "esteira_3", 2048, NULL, 5, &vTaskHandleEsteira3);
}