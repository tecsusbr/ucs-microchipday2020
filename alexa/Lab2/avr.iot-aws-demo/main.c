/* 
    (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#include "./config/clock_config.h"
#include <atmel_start.h>
#include "cloud/cloud.h"
#include <sensors_handling.h>
#include <led.h>

#define PUBLISH_INTERVAL    (10000)			//TODO: Alterar Intervalo de tempo de envio para 10 segundos  1000 ->10000
#define JSON_SIZE           (100)

uint8_t sub_init = 0;
uint8_t IO_status = 0;
uint8_t counter = 0;

//Callback de recebimento do dados Publicados no tópico $aws/things/%s/shadow/update/delta
void umqtt_received_cb (struct umqtt_connection * conn,char *topic, uint8_t *data, int len)
{
	uint16 i;
	printf ("Mensagem Recebida: %s\n\r",data);
	for ( i= 0; i<=len;i++)
	if(data[i]=='D')
	break;
	
	switch (data[i+3])
	{
		case '0':
		IO_status = 0;
//		LED_YELLOW_set_level(1);		//TODO: Descomente a linha para manipulação do LED Amarelo
		break;
		case '1':
		IO_status = 1;
// 		LED_YELLOW_set_level(0);		//TODO: Descomente a linha para manipulação do LED Amarelo
		break;
		default:
		
		break;
	}
	CLOUD_receiveEvent(CLOUD_EVENT_SUCCESS);
}

//Callback tempo para publicação dos dados dos sensores no tópico $aws/things/%s/shadow/update
absolutetime_t publishADC(void *payload) 
{
    char json[JSON_SIZE];
    counter++;
    uint16_t rawTemperature = SENSORS_getTempValue();
    uint16_t light = SENSORS_getLightValue();
    sprintf(json, "{\"state\":{\"reported\":{\"L\":%d,\"T\":%d,\"D\":%d}}}", counter, rawTemperature / 100, IO_status);		//TODO: Alterar para enviar o luminosidade ao inves de counter; counter -> light
    printf ("Mensagem enviada: %s\n\r",json);    
    CLOUD_publish((uint8_t*) json);
	
	if (sub_init==0)
	{
		MQTT_CLIENT_subscribe();
		sub_init = 1;
	}

    return PUBLISH_INTERVAL;
}

int main(void)
{
    atmel_start_init();
    ENABLE_INTERRUPTS();
   
    LED_test();
    printf ("AVR-IoT\r\n");
	
    CLOUD_setSendFunction(publishADC);
    CLOUD_setObserver(LED_userHandler);
    CLOUD_startApp();
    
    while (1)
    {
		asm("WDR");
        CLOUD_runTask();           
    }
}