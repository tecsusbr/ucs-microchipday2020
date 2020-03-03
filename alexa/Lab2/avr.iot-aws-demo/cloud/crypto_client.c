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

#include "crypto_client.h"
#include "cryptoauthlib/lib/tls/atcatls_cfg.h"

uint8_t cryptoDeviceInitialized;
struct atca_device   _gMyDevice = {&_gmyCommand, &_gmyIface};
uint8_t key_id_d[20];

const ATCAIfaceCfg cfg_ateccx08a_aws = {
    .iface_type             = ATCA_I2C_IFACE,
    .devtype                = ATECC608A,
    .atcai2c.slave_address  = 0xC0,
    .atcai2c.bus            = 2,
    .atcai2c.baud           = 400000,
    //.atcai2c.baud = 100000,
    .wake_delay             = 1500,
    .rx_retries             = 20
};

void CRYPTO_CLIENT_rocessEccRequest(tstrEccReqInfo *ecc_request)
{
    tstrEccReqInfo ecc_response;
    uint8 signature[80];
    uint16 response_data_size = 0;
    uint8 *response_data_buffer = NULL;
    
    ecc_response.u16Status = 1;
    
    switch (ecc_request->u16REQ)
    {
        case ECC_REQ_CLIENT_ECDH:
        ecc_response.u16Status = ecdh_derive_client_shared_secret(&(ecc_request->strEcdhREQ.strPubKey),
        ecc_response.strEcdhREQ.au8Key,
        &ecc_response.strEcdhREQ.strPubKey);
        break;

        case ECC_REQ_GEN_KEY:
        ecc_response.u16Status = ecdh_derive_key_pair(&ecc_response.strEcdhREQ.strPubKey);
        break;

        case ECC_REQ_SERVER_ECDH:
        ecc_response.u16Status = ecdh_derive_server_shared_secret(ecc_request->strEcdhREQ.strPubKey.u16PrivKeyID,
        &(ecc_request->strEcdhREQ.strPubKey),
        ecc_response.strEcdhREQ.au8Key);
        break;
        
        case ECC_REQ_SIGN_VERIFY:
        ecc_response.u16Status = ecdsa_process_sign_verify_request(ecc_request->strEcdsaVerifyREQ.u32nSig);
        break;
        
        case ECC_REQ_SIGN_GEN:
        ecc_response.u16Status = ecdsa_process_sign_gen_request(&(ecc_request->strEcdsaSignREQ), signature,
        &response_data_size);
        response_data_buffer = signature;
        break;
        
        default:
        // Do nothing
        break;
    }
    
    ecc_response.u16REQ      = ecc_request->u16REQ;
    ecc_response.u32UserData = ecc_request->u32UserData;
    ecc_response.u32SeqNo    = ecc_request->u32SeqNo;

    m2m_ssl_ecc_process_done();
    m2m_ssl_handshake_rsp(&ecc_response, response_data_buffer, response_data_size);
}

uint8_t subj_key_id[20];

void CRYPTO_CLIENT_init(void)
{
    uint8_t rv;
	uint8_t cert_buffer[900];
	    
    atcab_init_device(&_gMyDevice);
    
    rv = atcab_init(&cfg_ateccx08a_aws);
    if (rv != ATCA_SUCCESS)
    {
        cryptoDeviceInitialized = false;
    }
	//atcacert_get_subj_key_id
	read_slot(key_id_d);
}