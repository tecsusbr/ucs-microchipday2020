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

#define DEVICE_KEY_SLOT            (0)

#include "ecc_functions_tls.h"

static uint32 g_ecdh_key_slot_index = 0;
static uint16 g_ecdh_key_slot[] = {2};

sint8 ecdh_derive_client_shared_secret(tstrECPoint *server_public_key, uint8 *ecdh_shared_secret, tstrECPoint *client_public_key)
{
    sint8 status = M2M_ERR_FAIL;
    uint8_t ecdh_mode;
    uint16_t key_id;
    
    if ((g_ecdh_key_slot_index < 0) ||
    (g_ecdh_key_slot_index >= (sizeof(g_ecdh_key_slot) / sizeof(g_ecdh_key_slot[0]))))
    {
        g_ecdh_key_slot_index = 0;
    }
    
    if(_gDevice->mIface->mIfaceCFG->devtype == ATECC608A)
    {
        //do special ecdh functions for the 608, keep ephemeral keys in SRAM
        ecdh_mode = ECDH_MODE_SOURCE_TEMPKEY | ECDH_MODE_COPY_OUTPUT_BUFFER;
        key_id = GENKEY_PRIVATE_TO_TEMPKEY;
    }
    else
    {
        //specializations for the 508, use an EEPROM key slot
        ecdh_mode = ECDH_PREFIX_MODE;
        key_id = g_ecdh_key_slot[g_ecdh_key_slot_index];
        g_ecdh_key_slot_index++;
    }
    
    //generate an ephemeral key
    //TODO - add loop to make sure we get an acceptable private key
    if(atcab_genkey(key_id, client_public_key->X) == ATCA_SUCCESS)
    {
        client_public_key->u16Size = 32;
        //do the ecdh from the private key in tempkey, results put in ecdh_shared_secret
        if(atcab_ecdh_base(ecdh_mode, key_id, server_public_key->X, ecdh_shared_secret, NULL) == ATCA_SUCCESS)
        {
            status = M2M_SUCCESS;
        }
    }

    return status;
}

sint8 ecdh_derive_key_pair(tstrECPoint *server_public_key)
{
    sint8 status = M2M_ERR_FAIL;
    
    if ((g_ecdh_key_slot_index < 0) ||
    (g_ecdh_key_slot_index >= (sizeof(g_ecdh_key_slot) / sizeof(g_ecdh_key_slot[0]))))
    {
        g_ecdh_key_slot_index = 0;
    }

    if( (status = atcab_genkey(g_ecdh_key_slot[g_ecdh_key_slot_index], server_public_key->X) ) == ATCA_SUCCESS)
    {
        server_public_key->u16Size      = 32;
        server_public_key->u16PrivKeyID = g_ecdh_key_slot[g_ecdh_key_slot_index];

        g_ecdh_key_slot_index++;

        status = M2M_SUCCESS;
    }

    return status;
}

sint8 ecdsa_process_sign_verify_request(uint32 number_of_signatures)
{
    sint8 status = M2M_ERR_FAIL;
    tstrECPoint    Key;
    uint32 index = 0;
    uint8 signature[80];
    uint8 hash[80] = {0};
    uint16 curve_type = 0;
    
    for(index = 0; index < number_of_signatures; index++)
    {
        status = m2m_ssl_retrieve_cert(&curve_type, hash, signature, &Key);

        if (status != M2M_SUCCESS)
        {
            M2M_ERR("m2m_ssl_retrieve_cert() failed with ret=%d", status);
            return status;
        }

        if(curve_type == EC_SECP256R1)
        {
            bool is_verified = false;
            
            status = atcab_verify_extern(hash, signature, Key.X, &is_verified);
            if(status == ATCA_SUCCESS)
            {
                status = (is_verified == true) ? M2M_SUCCESS : M2M_ERR_FAIL;
                if(is_verified == false)
                {
                    M2M_INFO("ECDSA SigVerif FAILED\n");
                }
            }
            else
            {
                status = M2M_ERR_FAIL;
            }
            
            if(status != M2M_SUCCESS)
            {
                m2m_ssl_stop_processing_certs();
                break;
            }
        }
    }

    return status;
}

sint8 ecdsa_process_sign_gen_request(tstrEcdsaSignReqInfo *sign_request, uint8 *signature, uint16 *signature_size)
{
    sint8 status = M2M_ERR_FAIL;
    uint8 hash[32];
    
    status = m2m_ssl_retrieve_hash(hash, sign_request->u16HashSz);
    if (status != M2M_SUCCESS)
    {
        M2M_ERR("m2m_ssl_retrieve_hash() failed with ret=%d", status);
        return status;
    }

    if(sign_request->u16CurveType == EC_SECP256R1)
    {
        *signature_size = 64;
        status = atcab_sign(DEVICE_KEY_SLOT, hash, signature);
    }

    return status;
}

sint8 ecdh_derive_server_shared_secret(uint16 private_key_id, tstrECPoint *client_public_key, uint8 *ecdh_shared_secret)
{
    uint16 key_slot    = private_key_id;
    sint8 status = M2M_ERR_FAIL;
    uint8 atca_status = ATCA_STATUS_UNKNOWN;

    atca_status = atcab_ecdh(key_slot, client_public_key->X, ecdh_shared_secret);
    if(atca_status == ATCA_SUCCESS)
    {
        status = M2M_SUCCESS;
    }
    else
    {
        M2M_INFO("__SLOT = %u, Err = %X\n", key_slot, atca_status);
    }
    
    return status;
}

static const char* bin2hex(const void* data, size_t data_size)
{
    static char buf[256];
    static char hex[] = "0123456789abcdef";
    const uint8_t* data8 = data;
    
    if (data_size*2 > sizeof(buf)-1)
    return "[buf too small]";
    
    for (size_t i = 0; i < data_size; i++)
    {
        buf[i*2 + 0] = hex[(*data8) >> 4];
        buf[i*2 + 1] = hex[(*data8) & 0xF];
        data8++;
    }
    buf[data_size*2] = 0;
    
    return buf;
}

char read_slot (uint8_t *subj_key_id)
{
	uint8_t pubkey[64];
	
	atcab_get_pubkey(0,pubkey);
		atcacert_get_key_id(pubkey,subj_key_id);
	//printf ("Key ID read: %x %x %x %x %x %x %x %x %x %x\r\n",subj_key_id[0],subj_key_id[1],subj_key_id[2],subj_key_id[3],subj_key_id[4],subj_key_id[5],subj_key_id[6],subj_key_id[7],subj_key_id[8],subj_key_id[9]);
	return 0;
}
