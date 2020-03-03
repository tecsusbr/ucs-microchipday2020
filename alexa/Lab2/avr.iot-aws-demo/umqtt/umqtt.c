/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2015 Josef Gajdusek
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * */

#include <string.h>

#include "umqtt.h"

static inline uint16_t htons(uint16_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((x & 0xff) << 8) | (x >> 8);
#else
    return x;
#endif
}

#define umqtt_insert_messageid(conn, ptr) \
    do { \
        ptr[0] = conn->message_id >> 8;        \
        ptr[1] = conn->message_id & 0xff;    \
        conn->message_id++;                    \
    } while (0)

#define umqtt_build_header(type, dup, qos, retain) \
    (((type) << 4) | ((dup) << 3) | ((qos) << 1) | (retain))

#define umqtt_header_type(h) \
    ((h) >> 4)

static int umqtt_decode_length(uint8_t *data)
{
    int mul = 1;
    int val = 0;
    int i;

    for (i = 0; i == 0 || (data[i - 1] & 0x80); i++) {
        val += (data[i] & 0x7f) * mul;
        mul *= 128;
    }

    return val;
}

static int umqtt_encode_length(int len, uint8_t *data)
{
    int digit;
    int i = 0;

    do {
        digit = len % 128;
        len /= 128;
        if (len > 0)
            digit |= 0x80;
        data[i] = digit;
        i++;
    } while (len);

    return i; /* Return the amount of bytes used */
}

void umqtt_circ_init(struct umqtt_circ_buffer *buff)
{
    buff->pointer = buff->start;
    buff->datalen = 0;
}

int umqtt_circ_push(struct umqtt_circ_buffer *buff, uint8_t *data, int len)
{
    uint8_t *bend = buff->start + buff->length - 1;
    uint8_t *dend = (buff->pointer - buff->start + buff->datalen)
        % buff->length + buff->start; /* This points to new byte */

    for (; len > 0; len--) {
        if (dend > bend)
            dend = buff->start;
        if (buff->datalen != 0 && dend == buff->pointer)
            break;
        *dend = *data;
        dend++;
        data++;
        buff->datalen++;
    }

    return len; /* Return amount of bytes left */
}

int umqtt_circ_peek(struct umqtt_circ_buffer *buff, uint8_t *data, int len)
{
    uint8_t *ptr = buff->pointer;
    uint8_t *bend = buff->start + buff->length - 1;
    int i;

    for (i = 0; i < len && i < buff->datalen; i++) {
        data[i] = ptr[i];
        if (ptr > bend)
            ptr = buff->start;
    }

    return i; /* Return the amount of bytes actually peeked */
}

int umqtt_circ_pop(struct umqtt_circ_buffer *buff, uint8_t *data, int len)
{
    uint8_t *bend = buff->start + buff->length - 1;
    int i;

    for (i = 0; i < len && buff->datalen > 0; i++) {
        data[i] = *buff->pointer;
        buff->pointer++;
        buff->datalen--;
        if (buff->pointer > bend)
            buff->pointer = buff->start;
    }

    return i; /* Return the amount of bytes actually popped */
}

void umqtt_connect(struct umqtt_connection *conn)
{
    int cidlen = strlen(conn->clientid);
    uint8_t fixed;
    uint8_t remlen[4];
    uint8_t variable[10];
    uint16_t paylen;

    umqtt_circ_init(&conn->rxbuff);
    umqtt_circ_init(&conn->txbuff);
    conn->state = UMQTT_STATE_INIT;
    conn->nack_ping = 0;
    conn->nack_publish = 0;
    conn->nack_subscribe = 0;
    conn->message_id = 1;

    fixed = umqtt_build_header(UMQTT_CONNECT, 0, 0, 0);

    variable[0] = 0; /* UTF Protocol name */
    variable[1] = 4;
    variable[2] = 'M';
    variable[3] = 'Q';
    variable[4] = 'T';
    variable[5] = 'T';
    

    variable[6] = 4; /* Protocol version */

    variable[7] = 0b00000010; /* Clean session flag */

    variable[8] = conn->kalive >> 8; /* Keep Alive timer */
    variable[9] = conn->kalive & 0xff;

    paylen = htons(cidlen);

    umqtt_circ_push(&conn->txbuff, &fixed, 1);
    umqtt_circ_push(&conn->txbuff, remlen,
            umqtt_encode_length(sizeof(variable) + sizeof(paylen) + cidlen, remlen));
    umqtt_circ_push(&conn->txbuff, variable, sizeof(variable));
    umqtt_circ_push(&conn->txbuff, (uint8_t *) &paylen, sizeof(paylen));
    umqtt_circ_push(&conn->txbuff, conn->clientid, cidlen);

    conn->state = UMQTT_STATE_CONNECTING;
    if (conn->new_packet_callback)
        conn->new_packet_callback(conn);
}

void umqtt_subscribe(struct umqtt_connection *conn, char *topic)
{
    uint16_t topiclen = strlen(topic);
    uint8_t fixed;
    uint8_t remlen[4];
    uint8_t messageid[2];
    uint16_t paylen;
    uint8_t qos = 0;

    fixed = umqtt_build_header(UMQTT_SUBSCRIBE, 0, 1, 0);

    umqtt_insert_messageid(conn, messageid);

    paylen = htons(topiclen);

    umqtt_circ_push(&conn->txbuff, &fixed, 1);
    umqtt_circ_push(&conn->txbuff, remlen,
            umqtt_encode_length(
                sizeof(messageid) + sizeof(paylen) + topiclen + sizeof(qos), remlen));
    umqtt_circ_push(&conn->txbuff, messageid, sizeof(messageid));
    umqtt_circ_push(&conn->txbuff, (uint8_t *) &paylen, sizeof(paylen));
    umqtt_circ_push(&conn->txbuff, topic, topiclen);
    umqtt_circ_push(&conn->txbuff, &qos, sizeof(qos));

    conn->nack_subscribe++;
    if (conn->new_packet_callback)
        conn->new_packet_callback(conn);
}

void umqtt_publish(struct umqtt_connection *conn, char *topic,
        uint8_t *data, int datalen)
{
    int toplen = strlen(topic);
    uint8_t fixed;
    uint8_t remlen[4];
    uint8_t len[2];

    fixed = umqtt_build_header(UMQTT_PUBLISH, 0, 0, 0);

    umqtt_circ_push(&conn->txbuff, &fixed, 1);
    umqtt_circ_push(&conn->txbuff, remlen,
            umqtt_encode_length(2 + toplen + datalen, remlen));

    len[0] = toplen >> 8;
    len[1] = toplen & 0xff;
    umqtt_circ_push(&conn->txbuff, len, sizeof(len));
    umqtt_circ_push(&conn->txbuff, (uint8_t *) topic, toplen);

    umqtt_circ_push(&conn->txbuff, data, datalen);
    if (conn->new_packet_callback)
        conn->new_packet_callback(conn);
}

void umqtt_ping(struct umqtt_connection *conn)
{
    uint8_t packet[] = { umqtt_build_header(UMQTT_PINGREQ, 0, 0, 0), 0 };

    umqtt_circ_push(&conn->txbuff, packet, sizeof(packet));
    conn->nack_ping++;
    if (conn->new_packet_callback)
        conn->new_packet_callback(conn);
}

static void umqtt_handle_publish(struct umqtt_connection *conn,
        uint8_t *data, int len)
{
    uint16_t toplen = (data[0] << 8) | data[1];
    char topic[toplen + 1];
    uint8_t payload[len - 2 - toplen + 1];

    memcpy(topic, data + 2, sizeof(topic));
    topic[sizeof(topic) - 1] = 0;
    memcpy(payload, data + 2 + toplen, sizeof(payload));
    payload[sizeof(payload) - 1] = 0;

    conn->message_callback(conn, topic, payload, sizeof(payload));
}

static void umqtt_packet_arrived(struct umqtt_connection *conn,
        uint8_t header, int len)
{
    uint8_t data[len];

    umqtt_circ_pop(&conn->rxbuff, data, len);

    switch (umqtt_header_type(header)) {
    case UMQTT_CONNACK:
        if (data[1] == 0x00) {
            conn->state = UMQTT_STATE_CONNECTED;
            if (conn->connected_callback)
                conn->connected_callback(conn);
        }
        else {
            conn->state = UMQTT_STATE_FAILED;
        }
        break;
    case UMQTT_SUBACK:
        conn->nack_subscribe--;
        break;
    case UMQTT_PINGRESP:
        conn->nack_ping--;
        break;
    case UMQTT_PUBLISH:
        umqtt_handle_publish(conn, data, len);
        break;

    }
}

void umqtt_process(struct umqtt_connection *conn)
{
    uint8_t buf[5];
    unsigned int i = 2;

    while (conn->rxbuff.datalen >= 2) { /* We do have the fixed header */
        umqtt_circ_pop(&conn->rxbuff, buf, 2);
        for (i = 2; buf[i - 1] & 0x80 && i < sizeof(buf); i++)
            umqtt_circ_pop(&conn->rxbuff, &buf[i], 1);

        umqtt_packet_arrived(conn, buf[0],
                umqtt_decode_length(&buf[1]));
    }
}