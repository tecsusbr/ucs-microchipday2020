/*
    \file   mqtt_client.h

    \brief  MQTT Client header file.

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

#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

#define PASSWORD_SPACE    456
#define TOPIC_SIZE        100
#define ID_SIZE           41

#include <stdint.h>
#include "winc/socket/include/socket.h"

extern char mqtt_topic[];
extern char mqtt_topic_sub[];
extern char mqtt_client_id[];

void initUmqtt(SOCKET *sokc);
void MQTT_CLIENT_publish(uint8_t *data, uint8_t datalen);
void MQTT_CLIENT_receive(uint8_t *data, uint8_t len);
void MQTT_CLIENT_connect(void);
void MQTT_CLIENT_subscribe (void);
uint8_t MQTT_CLIENT_isConnected(void);

#endif /* MQTT_CLIENT_H_ */