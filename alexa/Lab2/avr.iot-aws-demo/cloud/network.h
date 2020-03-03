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

#ifndef NETWORK_H_
#define NETWORK_H_

#include "cloud.h"
#include "crypto_client.h"
#include "ecc_functions_tls.h"
#include "winc/socket/include/m2m_socket_host_if.h"
#include "winc/socket/include/socket.h"
#include "winc/driver/include/m2m_wifi.h"
#include "winc/driver/include/m2m_periph.h"

extern uint8_t gau8SocketBuffer[];
extern SOCKET tcpClientSocket;

void NETWORK_wifiSocketHandler(SOCKET sock, uint8 u8Msg, void *pMsg);
void NETWORK_wifiCallback(uint8 u8MsgType, void *pvMsg);
void NETWORK_wifiSslCallback(uint8 u8MsgType, void *pvMsg);
void NETWORK_dnsCallback(uint8* domainName, uint32 serverIP);
void NETWORK_pingCallback(uint32 u32IPAddr, uint32 u32RTT, uint8 u8ErrorCode);

extern uint32_t endpoint_ip;

#endif /* NETWORK_H_ */