/*
    \file   led.c

    \brief  Manage board LED's

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

#include "led.h"
#include "rstctrl.h"

#define LEDS_TEST_INTERVAL      (50)

bool isInternetConnection = true;

#define TOGGLE_DELAY            (1000)
timer_struct_t toggle_led;
uint32_t wdt_connect	= 0;

static void LED_testSequence(uint8_t ledState)
{
    LED_BLUE_set_level(ledState);
    _delay_ms(LEDS_TEST_INTERVAL);
    LED_GREEN_set_level(ledState);
    _delay_ms(LEDS_TEST_INTERVAL);
    LED_YELLOW_set_level(ledState);
    _delay_ms(LEDS_TEST_INTERVAL);
    LED_RED_set_level(ledState);
    _delay_ms(LEDS_TEST_INTERVAL);
}

void LED_test(void)
{
    LED_testSequence(LED_ON);
    LED_testSequence(LED_OFF);
    LED_testSequence(LED_ON);
    LED_testSequence(LED_OFF);
}

absolutetime_t LED_toggle(void *payload)
{
     LED_BLUE_toggle_level();
	 if (wdt_connect >= 100)
	 {
		RSTCTRL_reset();
	}else{
		wdt_connect++;
	}
	
    return TOGGLE_DELAY;
}

void LED_setter(bool blue, bool green, bool yellow, bool red)
{
    LED_BLUE_set_level(blue);
    LED_GREEN_set_level(green);
    LED_YELLOW_set_level(yellow);
    LED_RED_set_level(red);
}

void LED_userHandler(cloudState_t state, cloudEvent_t ev)
{
    switch(ev)
    {
        case CLOUD_EVENT_FAIL:
        {
            
            switch(state)
            {
                case CLOUD_STATE_INIT:
                LED_setter(LED_OFF, LED_OFF, LED_OFF, LED_ON);
                break;
                
                case CLOUD_STATE_WIFI_CONNECT:
                LED_setter(LED_ON, LED_OFF, LED_OFF, LED_ON);
                break;
                
                case CLOUD_STATE_WAIT_FOR_INTERNET:
                if(isInternetConnection)
                {
                    isInternetConnection = false; 
                        
                    toggle_led.callback_ptr = LED_toggle;
                    scheduler_timeout_delete(&toggle_led);
                    scheduler_timeout_create(&toggle_led, 0);

                }                        
                break;
                
                case CLOUD_STATE_DNS_REQUEST:
                LED_setter(LED_OFF, LED_ON, LED_OFF, LED_ON);
                break;
                
                case CLOUD_STATE_TLS_CONNECT:
                LED_setter(LED_OFF, LED_ON, LED_OFF, LED_ON);
                break;

                case CLOUD_STATE_MQTT:
                LED_setter(LED_OFF, LED_ON, LED_OFF, LED_ON);
                break;
                
                case CLOUD_STATE_SENDING:
                LED_setter(LED_OFF, LED_OFF, LED_ON, LED_ON);
                break;
                
                default:
                LED_setter(LED_OFF, LED_OFF, LED_OFF, LED_ON);
                break;
            }
            
            break;
        }
        
        case CLOUD_EVENT_SUCCESS:
        {   
            switch(state)
            {
                case CLOUD_STATE_INIT:
                LED_setter(LED_OFF, LED_OFF, LED_OFF, LED_OFF);
                break;
                
                case CLOUD_STATE_WIFI_CONNECT:
                LED_setter(LED_ON, LED_OFF, LED_OFF, LED_OFF);
                break;
                
                case CLOUD_STATE_WAIT_FOR_INTERNET:
                isInternetConnection = true;
                break;
                
                case CLOUD_STATE_DNS_REQUEST:
                break;
                
                case CLOUD_STATE_TLS_CONNECT:
                break;

                case CLOUD_STATE_MQTT:
                LED_setter(LED_ON, LED_ON, LED_OFF, LED_OFF);
                break;
                
                case CLOUD_STATE_SENDING:
                break;
                
                default:
                break;
            }
            
            break;
        }
    }
}