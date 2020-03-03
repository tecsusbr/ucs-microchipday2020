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

#ifndef ECC_FUNCTIONS_TLS_H_
#define ECC_FUNCTIONS_TLS_H_

#include <stdint.h>
#include "cryptoauthlib/lib/cryptoauthlib.h"
#include "winc/bsp/include/nm_bsp.h"
#include "winc/driver/include/ecc_types.h"


sint8 ecdsa_process_sign_verify_request(uint32 number_of_signatures);
sint8 ecdh_derive_key_pair(tstrECPoint *server_public_key);
sint8 ecdh_derive_client_shared_secret(tstrECPoint *server_public_key, uint8 *ecdh_shared_secret, tstrECPoint *client_public_key);
sint8 ecdsa_process_sign_gen_request(tstrEcdsaSignReqInfo *sign_request, uint8 *signature, uint16 *signature_size);
sint8 ecdh_derive_server_shared_secret(uint16 private_key_id, tstrECPoint *client_public_key, uint8 *ecdh_shared_secret);
char read_slot (uint8_t *subj_key_id);
#endif /* ECC_FUNCTIONS_TLS_H_ */