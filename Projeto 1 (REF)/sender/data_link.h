
#pragma once

#ifndef WRITE_
#define WRITE_

#include "utilities.h"

struct Control_packet control_packet;
struct Data_packet data_packet;


int conta, flag;

void maquinaEstados(unsigned char ua, char buf[], unsigned char byteControl);

void sendSet(int *fd);
void sendDisc(int *fd);
void sendUA(int *fd);

void llopen(int fd);
int llwrite(int fd);
void llclose(int fd);

void send_control_packet(unsigned char end_start, unsigned char control_byte_expected, unsigned char indice_trama);
unsigned char receive_feedback(unsigned char control_byte_expected);
int stuffing(int bufSize, unsigned char** buf);

#endif
