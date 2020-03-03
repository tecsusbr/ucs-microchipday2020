/*
    \file   mqtt_client.c

    \brief  MQTT Client source file.

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

#include <stdint.h>
#include <string.h>
#include "winc/socket/include/socket.h"
#include "umqtt/umqtt.h"
#include "cloud/cloud.h"
#include "atmel_start_pins.h"
#include "main.h"


#define TX_BUFF_SIZE 250
#define RX_BUFF_SIZE 250
#define USER_LENGTH 0
#define MQTT_KEEP_ALIVE_TIME 900

static uint8_t mqtt_txbuff[TX_BUFF_SIZE];
static uint8_t mqtt_rxbuff[RX_BUFF_SIZE];

static SOCKET *tcp_client_socket;

void initUmqtt(SOCKET *sokc) 
{
    tcp_client_socket = sokc;
}

static void umqtt_connected_cb (struct umqtt_connection * conn)
{
    CLOUD_receiveEvent(CLOUD_EVENT_SUCCESS);
}



static void umqtt_send_packet (struct umqtt_connection *conn)
{
    send(*tcp_client_socket, conn->txbuff.start, conn->txbuff.datalen, 0);
    umqtt_circ_init(&conn->txbuff);
}

struct umqtt_connection umqtt_conn = {
    .txbuff = {
        .start = mqtt_txbuff,
        .length = TX_BUFF_SIZE,
    },
    .rxbuff = {
        .start = mqtt_rxbuff,
        .length = RX_BUFF_SIZE,
    },
    
    .clientid = mqtt_client_id,
    .kalive = MQTT_KEEP_ALIVE_TIME,
    
    .connected_callback = umqtt_connected_cb,
    .new_packet_callback = umqtt_send_packet,
	.message_callback = umqtt_received_cb
};

void MQTT_CLIENT_publish(uint8_t *data, uint8_t datalen) {
    umqtt_publish(&umqtt_conn, mqtt_topic, data, datalen);
}

void MQTT_CLIENT_receive(uint8_t *data, uint8_t len) {
    umqtt_circ_push(&umqtt_conn.rxbuff, data, len);
    umqtt_process(&umqtt_conn);
}

void MQTT_CLIENT_connect(void) {
    umqtt_circ_init(&umqtt_conn.txbuff);
    umqtt_circ_init(&umqtt_conn.rxbuff);
    umqtt_connect(&umqtt_conn);	
}

void MQTT_CLIENT_subscribe (void){
	printf ("MQTT Subscribe\n\r");
	umqtt_subscribe(&umqtt_conn,&mqtt_topic_sub);
	printf ("MQTT Subscribed\n\r");
}


uint8_t MQTT_CLIENT_isConnected(void) {
    return umqtt_conn.state == UMQTT_STATE_CONNECTED;
}