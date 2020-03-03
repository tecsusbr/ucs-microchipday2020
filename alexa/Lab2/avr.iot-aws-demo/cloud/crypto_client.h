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
#ifndef CRYPTO_CLIENT_H_
#define CRYPTO_CLIENT_H_

#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>

#include "cryptoauthlib/lib/cryptoauthlib.h"
#include "ecc_functions_tls.h"
#include "mqtt_client/mqtt_client.h"

struct atca_command  _gmyCommand;
struct atca_iface    _gmyIface;
extern struct atca_device   _gMyDevice;
extern uint8_t cryptoDeviceInitialized;
extern const ATCAIfaceCfg cfg_ateccx08a_aws;

void CRYPTO_CLIENT_rocessEccRequest(tstrEccReqInfo *ecc_request);
void CRYPTO_CLIENT_init(void);


#endif /* CRYPTO_CLIENT_H_ */