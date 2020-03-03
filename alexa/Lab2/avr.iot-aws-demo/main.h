/*
 * Main.h
 *
 * Created: 10/02/2020 16:18:09
 *  Author: Seiti
 */ 


#ifndef MAIN_H_
#define MAIN_H_

void umqtt_received_cb (struct umqtt_connection * conn,char *topic, uint8_t *data, int len);



#endif /* INCFILE1_H_ */