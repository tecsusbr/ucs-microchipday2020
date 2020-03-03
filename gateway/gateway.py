# MQTT Gateway

import paho.mqtt.client as mqtt
from threading import Thread
import serial
import json

SERVER_MQTT = "mqtt.eclipse.org"
PORT_MQTT = 1883 # 1883 = Normal Port    8883 = Encrypted port TLS 1.0 to 1.2 x509 cert
TOPIC_CMD = "techday2020/strlight/cmd/"
TOPIC_REC = "techday2020/strlight/rec/"
SERIAL = "COM3"

ser = serial.Serial(SERIAL, 9600)

lastUID = 0

def on_connect(con, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    con.subscribe(TOPIC_REC)

# The callback for when a PUBLISH message is received from the server.


def on_message(con, userdata, msg):
    global ser, lastUID
    pay = json.loads(msg.payload)
    print(msg.topic+" "+str(pay))
    if 'uid' in pay and 'light' in pay:
        # @<UID><light>
        #s = '@{:0>2}{}'.format(pay['uid'], pay['light'])
        lastUID = pay['uid']
        s = 'AT+SENDGPIO={},6,{}\r\n'.format(pay['uid'], pay['light'])
        print(s)
        ser.write(s.encode())


def sub_loop():
    sub = mqtt.Client()
    sub.on_connect = on_connect
    sub.on_message = on_message
    sub.connect(SERVER_MQTT, PORT_MQTT, 60)
    sub.loop_forever()


def pub_loop():
    global lastUID
    pub = mqtt.Client()
    pub.connect(SERVER_MQTT, PORT_MQTT, 60)
    pub.loop_start()
    while True:
        # msg = raw_input()
        # msg = {"light": 1, "temp": 12, "hum": 41, "uid": 1}
        # pub.publish(TOPIC_CMD + "1", json.dumps(msg))
        msg = ser.readline()
        print(msg)
        # +RECVDATA:<UID>,<UID><light><temp><humildade>\n
        # ERROR:17 - NO ROUTE
        if msg:
            msg = msg.decode('utf-8')

            if ('ERROR' in msg):
                topico = '{}{}'.format(TOPIC_CMD, uid)
                pay = dict(light=0, temp=0, hum=0, uid=lastUID)
                payload = json.dumps(pay)
                pub.publish(topico, payload)
                lastUID = 0
		
            msg = msg.split(',')				
            msg = msg[1:]
            print(msg)
            if len(msg) == 4:
                uid, light, temp, humildade = [int(x) for x in msg]
                topico = '{}{}'.format(TOPIC_CMD, uid)
                pay = dict(light=light, temp=temp, hum=humildade, uid=uid)
                payload = json.dumps(pay)
                pub.publish(topico, payload)

sub_t = Thread(target=sub_loop, args=())
sub_t.start()

pub_t = Thread(target=pub_loop, args=())
pub_t.start()
