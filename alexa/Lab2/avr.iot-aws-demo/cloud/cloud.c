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

#include "cloud.h"

#define STATES_NUM      (7)
#define KEY_ID_SIZE     (20)
#define TEST_IP         (0x08080808)
#define TTL             (0)

static cloudState_t wifiConnect(void);
static cloudState_t waitForInternet(void);
static cloudState_t requestDns(void);
static cloudState_t tlsConnect(void);
static cloudState_t mqttConnect(void);
static cloudState_t startSending(void);
static cloudState_t errorHandler(void);

static void mqttTopicInit(void);
static cloudEvent_t wifiInit(void);
static cloudEvent_t tlsInit(void);

cloudState_t current_state = CLOUD_STATE_INIT;
cloudEvent_t occured_event = CLOUD_NO_EVENT;

timer_struct_t send_task;
void (* cloudObserver)(cloudState_t state, cloudEvent_t ev);

struct sockaddr_in addr;

char mqtt_client_id[ID_SIZE];
char mqtt_topic[TOPIC_SIZE];
char mqtt_topic_sub[TOPIC_SIZE];
extern uint8_t key_id_d[20];

handlerFunc_t handler[STATES_NUM] = 
{    
    wifiConnect,        /* INIT_STATE           */
    waitForInternet,    /* WIFI_CONNECT_STATE   */
    requestDns,         /* WAIT_STATE           */
    tlsConnect,         /* DNS STATE            */
    mqttConnect,        /* TLS_CONNECT_STATE    */
    startSending,       /* MQTT_CONNECT_STATE   */
    startSending,       /* RUNNING_STATE        */
};

void CLOUD_startApp(void)
{
    CRYPTO_CLIENT_init();
    
    atcab_lock_data_zone();
    atcab_lock_config_zone();


    if(wifiInit() != CLOUD_EVENT_SUCCESS)
    {
        CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
        return;
    }

    mqttTopicInit();
    
    if(tlsInit() != CLOUD_EVENT_SUCCESS)
    {
        CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
        return;
    }
    
    CLOUD_receiveEvent(CLOUD_EVENT_SUCCESS);  
}

static cloudState_t wifiConnect(void)
{
    sint8 rc;

    socketInit();
    rc = m2m_wifi_connect((char *)MAIN_WLAN_SSID,
                            sizeof(MAIN_WLAN_SSID),
                            MAIN_WLAN_AUTH,
                            (void *)MAIN_WLAN_PSK,
                            M2M_WIFI_CH_ALL);
    if(rc != M2M_SUCCESS)
    {
        CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
    }
    
    return CLOUD_STATE_WIFI_CONNECT;
}

static cloudState_t waitForInternet(void)
{
    sint8 rc;
    
    rc = m2m_ping_req(TEST_IP, TTL, NETWORK_pingCallback);
    
    return CLOUD_STATE_WAIT_FOR_INTERNET;
}

static cloudState_t requestDns(void)
{
    sint8 rc;
    
    endpoint_ip = 0;
    rc = gethostbyname((uint8_t*)AWS_HOST_ENDPOINT);
    
    if (rc != M2M_SUCCESS)
    {
        CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
    }
        
    return CLOUD_STATE_DNS_REQUEST;
}

static cloudState_t tlsConnect(void)
{
    sint8 rc;
    
    tcpClientSocket = socket(AF_INET, SOCK_STREAM, SOCKET_FLAGS_SSL);
    if(tcpClientSocket < 0) 
    {
        CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = _htons(PORT);
    addr.sin_addr.s_addr = endpoint_ip;
    
    rc = connect(tcpClientSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    
    if(rc != SOCK_ERR_NO_ERROR)
    {
        CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
    }
    
    return CLOUD_STATE_TLS_CONNECT;
}

static cloudState_t mqttConnect(void)
{
    initUmqtt(&tcpClientSocket);
    MQTT_CLIENT_connect();
    
    return CLOUD_STATE_MQTT;
}

static cloudState_t startSending(void)
{
    scheduler_timeout_delete(&send_task);
    scheduler_timeout_create(&send_task, 0);
    
    cloudObserver(CLOUD_STATE_SENDING, CLOUD_EVENT_SUCCESS);
    
    return CLOUD_STATE_SENDING;
}

static cloudState_t errorHandler(void)
{
    /* Flow through intended */
    switch(current_state)
    {
        case CLOUD_STATE_INIT:
        
        case CLOUD_STATE_WIFI_CONNECT:
            return CLOUD_STATE_INIT;
        
        case CLOUD_STATE_WAIT_FOR_INTERNET:
            return CLOUD_STATE_WIFI_CONNECT;
        
        case CLOUD_STATE_DNS_REQUEST:
            return CLOUD_STATE_WAIT_FOR_INTERNET;
        
        case CLOUD_STATE_TLS_CONNECT:
        
        case CLOUD_STATE_MQTT:
            return CLOUD_STATE_DNS_REQUEST;
        
        case CLOUD_STATE_SENDING:
            scheduler_timeout_delete(&send_task);
        
        default:
            return CLOUD_STATE_INIT;
    }
}

void CLOUD_setSendFunction(sendFunction_t function)
{
    send_task.callback_ptr = function;
}

void CLOUD_setObserver(observerFunction_t function)
{
    cloudObserver = function;
}

void CLOUD_runTask(void)
{
    m2m_wifi_handle_events(NULL);
    scheduler_timeout_call_next_callback();
    
    if(occured_event != CLOUD_NO_EVENT)
    {
        cloudEvent_t ev = occured_event;
        occured_event = CLOUD_NO_EVENT;
        
        if(cloudObserver != NULL)
        {
            cloudObserver(current_state, ev);
        }
        
        if(ev == CLOUD_EVENT_FAIL)
        {
            current_state = errorHandler();
            
        }
        
        current_state = handler[current_state]();
    }
}

void CLOUD_receiveEvent(cloudEvent_t ev)
{
    occured_event = ev;
}

void CLOUD_publish(uint8_t *msg)
{
    MQTT_CLIENT_publish(msg, strlen(msg));
}

void intToHex(char *destination, uint8_t source)
{
    destination[0] = "0123456789abcdef"[source >> 4];
    destination[1] = "0123456789abcdef"[source & 0x0F];
}

char num_to_ascii(char c){
	if (c<10)
		return c+0x30;
	else
		return c+87;
}


static void mqttTopicInit(void)
{
	char key_id_ascii[40];
 	for (int i =0; i<20;i++)
	 {
		key_id_ascii[i*2+1] = num_to_ascii(key_id_d[i]&0xf);
		key_id_ascii[i*2] = num_to_ascii((key_id_d[i] &0xf0)>>4);	
	 } 
	memcpy(mqtt_client_id, key_id_ascii, 40);
	printf ("ID: %s\n\r",mqtt_client_id);
    sprintf(mqtt_topic_sub, "$aws/things/%s/shadow/update/delta", mqtt_client_id);
	printf ("SUB topic: %s\n\r",mqtt_topic_sub);
    sprintf(mqtt_topic, "$aws/things/%s/shadow/update", mqtt_client_id);
	printf ("PUB topic: %s\n\r",mqtt_topic);
}

static cloudEvent_t wifiInit(void)
{
    sint8 rc;

    tstrWifiInitParam wifi_paramaters;
    m2m_memset((uint8*)&wifi_paramaters, 0, sizeof(wifi_paramaters));
    wifi_paramaters.pfAppWifiCb = NETWORK_wifiCallback;

    rc = nm_bsp_init();
    if(rc != M2M_SUCCESS)
    {
        CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
        return CLOUD_EVENT_FAIL;
    }
    
    rc = m2m_wifi_init(&wifi_paramaters);
    if(rc != M2M_SUCCESS)
    {
        CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
        return CLOUD_EVENT_FAIL;
    }
    
    return CLOUD_EVENT_SUCCESS;
}

static cloudEvent_t tlsInit(void)
{
    sint8 rc;
    
    registerSocketCallback(NETWORK_wifiSocketHandler, NETWORK_dnsCallback);

    rc = m2m_ssl_init(NETWORK_wifiSslCallback);
    if(rc != M2M_SUCCESS)
    {
        CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
        return CLOUD_EVENT_FAIL;
    }
    
    rc = m2m_ssl_set_active_ciphersuites((uint32)SSL_ECC_ONLY_CIPHERS);
    if(rc != SOCK_ERR_NO_ERROR)
    {
        CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
        return CLOUD_EVENT_FAIL;
    }

    return CLOUD_EVENT_SUCCESS;
}