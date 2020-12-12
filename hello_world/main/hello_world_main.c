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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
 
 // periodo entre passagem de produtos nas esteiras 
#define TEMPO_EST_1 1000
#define TEMPO_EST_2 500
#define TEMPO_EST_3 100

// periodo entre atualizações do display
#define TEMPO_ATUALIZACAO 2000

// peso dos produtos nas esteiras
#define PESO_EST_1 5.0
#define PESO_EST_2 2.0
#define PESO_EST_3 0.5
#define TAMANHO 200

static int num_produtos = 0;
static float pesos[TAMANHO] = {0x0};
int bandeira = 0;
TaskHandle_t vTaskHandleEsteira1 = NULL;
TaskHandle_t vTaskHandleEsteira2 = NULL;
TaskHandle_t vTaskHandleEsteira3 = NULL;
TaskHandle_t vTaskHandleSoma = NULL;

void soma_pesos()
{
    // suspender as demais tasks
    
    float peso_total = 0;
    
    // criar threads para realizar a soma paralelamente
    for(int i = 0; i < TAMANHO; i++)
    {
        peso_total += pesos[i];
    }

    printf("O peso total dos prodos foi %f \n", peso_total);    

    vTaskResume(vTaskHandleEsteira1);
    vTaskResume(vTaskHandleEsteira2);
    vTaskResume(vTaskHandleEsteira3);

    vTaskSuspend(vTaskHandleSoma);
    
    // retornar as demais tasks
}

void soma_produto(float peso)
{
    // mutex (semaforo)
    pesos[num_produtos] = peso;
    num_produtos++;

    if (num_produtos >= TAMANHO)
    {   
        bandeira=1;
        num_produtos = 0;
        vTaskSuspend(vTaskHandleEsteira1);
        vTaskSuspend(vTaskHandleEsteira2);
        vTaskSuspend(vTaskHandleEsteira3);
        vTaskResume(vTaskHandleSoma);
    }
    // end mutex
}

void esteira_1(void *pvParameter)
{    
    TickType_t xLastWakeTime;

    // tempo atual
    xLastWakeTime = xTaskGetTickCount ();

	while(1)
	{
        // aguardar produto
        vTaskDelayUntil(&xLastWakeTime, TEMPO_EST_1 / portTICK_RATE_MS);

	    // somar produto
        soma_produto(PESO_EST_1);
	}
}

void esteira_2(void *pvParameter)
{
    TickType_t xLastWakeTime;

    // tempo atual
    xLastWakeTime = xTaskGetTickCount ();

    while(1)
	{
         // aguardar produto
        vTaskDelayUntil(&xLastWakeTime, TEMPO_EST_2 / portTICK_RATE_MS);

	    // somar produto
        soma_produto(PESO_EST_2);
	}
}

void esteira_3(void *pvParameter)
{
    TickType_t xLastWakeTime;

    // tempo atual
    xLastWakeTime = xTaskGetTickCount ();

    while(1)
	{
         // aguardar produto
        vTaskDelayUntil(&xLastWakeTime, TEMPO_EST_3 / portTICK_RATE_MS);

	    // somar produto
        soma_produto(PESO_EST_3);
	}
}


void display(void *pvParameter)
{
    
    TickType_t xLastWakeTime;

    // tempo atual
    xLastWakeTime = xTaskGetTickCount ();

    while(1) 
    {
        vTaskDelayUntil(&xLastWakeTime, TEMPO_ATUALIZACAO / portTICK_RATE_MS);
        printf("Quantidade produtos %d\n", num_produtos);
    }
}




void app_main()
{
    // inicia o flash
    nvs_flash_init();
    //xTaskCreate(&touch, "Touch", 2048, NULL, 5, NULL);
    xTaskCreate(&display, "display", 2048, NULL, 5, NULL);
    xTaskCreate(&esteira_1, "esteira_1", 2048, NULL, 5, &vTaskHandleEsteira1);
    xTaskCreate(&esteira_2, "esteira_2", 2048, NULL, 5, &vTaskHandleEsteira2);
    xTaskCreate(&esteira_3, "esteira_3", 2048, NULL, 5, &vTaskHandleEsteira3);
    xTaskCreate(&soma_pesos, "soma_pesos", 2048, NULL, 5, &vTaskHandleSoma);
    vTaskSuspend(vTaskHandleSoma);
    // criar task para monitoramento do botão de parada



}