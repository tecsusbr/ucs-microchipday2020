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

#include "network.h"

#define MAIN_WIFI_M2M_BUFFER_SIZE (500)

uint8_t gau8SocketBuffer[MAIN_WIFI_M2M_BUFFER_SIZE];
SOCKET tcpClientSocket = -1;
uint32_t endpoint_ip = 0;

void NETWORK_wifiSocketHandler(SOCKET sock, uint8 u8Msg, void *pMsg)
{
    tstrSocketRecvMsg        *pstrRecv;
    tstrSocketConnectMsg    *pstrConnect;
    //printf ("SOCKET: %d\n\r",u8Msg);    
    switch (u8Msg)
    {
        case SOCKET_MSG_CONNECT:
        {
            pstrConnect = (tstrSocketConnectMsg *)pMsg;
            if (pstrConnect && pstrConnect->s8Error >= 0)
            {
				printf ("SOCKET_MSG_CONNECT: Success\n\r");
                CLOUD_receiveEvent(CLOUD_EVENT_SUCCESS);
            }
            else
            {
				printf ("SOCKET_MSG_CONNECT: Fail\n\r");
                close(sock);
                CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
            }
            
            break;
        }
        
        case SOCKET_MSG_SEND:
        {
            recv(tcpClientSocket, gau8SocketBuffer, sizeof(gau8SocketBuffer), 0);
            
            break;
        }
        
        case SOCKET_MSG_RECV:
        {
			
            tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pMsg;
            //printf ("SOCKET MSG: %d - %x\n\r",pstrRecv->s16BufferSize,pstrRecv->pu8Buffer[0]);
 			if (pstrRecv && pstrRecv->s16BufferSize > 0)
            {
                MQTT_CLIENT_receive(pstrRecv->pu8Buffer, pstrRecv->s16BufferSize);
            }
            else
            {
                close(sock);
                CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
            }
            
            break;
        }
    }
}

void NETWORK_wifiCallback(uint8 u8MsgType, void *pvMsg)
{
    switch (u8MsgType)
    {
        case M2M_WIFI_REQ_DHCP_CONF:
        {
            CLOUD_receiveEvent(CLOUD_EVENT_SUCCESS);
            break;
        }
        
        case M2M_WIFI_RESP_CON_STATE_CHANGED:
        {
            tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
            if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED)
            {
                close(tcpClientSocket);
                CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
            }
            break;
        }
    }
}


void NETWORK_wifiSslCallback(uint8 u8MsgType, void *pvMsg)
{
    switch (u8MsgType)
    {
        case M2M_SSL_REQ_ECC:
        {
            tstrEccReqInfo *ecc_request = (tstrEccReqInfo*)pvMsg;
            CRYPTO_CLIENT_rocessEccRequest(ecc_request);
            
            break;
        }
        
        case M2M_SSL_RESP_SET_CS_LIST:
        {
            tstrSslSetActiveCsList *pstrCsList = (tstrSslSetActiveCsList *)pvMsg;
            M2M_INFO("ActiveCS bitmap:%04x\n", pstrCsList->u32CsBMP);
            
            break;
        }
        
        default:
            break;
    }
}

void NETWORK_dnsCallback(uint8* domainName, uint32 serverIP)
{
    if(serverIP != 0)
    {
        endpoint_ip = serverIP;
        CLOUD_receiveEvent(CLOUD_EVENT_SUCCESS);
    }
    else
    {
        CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
    }
}

void NETWORK_pingCallback(uint32 u32IPAddr, uint32 u32RTT, uint8 u8ErrorCode)
{
    switch (u8ErrorCode)
    {
        case PING_ERR_SUCCESS:
        {
            CLOUD_receiveEvent(CLOUD_EVENT_SUCCESS);
            break;
        }
        
        case PING_ERR_DEST_UNREACH:
        {
            CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
            break;
        }
        
        case PING_ERR_TIMEOUT:
        {
            CLOUD_receiveEvent(CLOUD_EVENT_FAIL);
            break;
        }
        
        default:
        break;
    }
}