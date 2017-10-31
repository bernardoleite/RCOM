#ifndef SENDER_H
#define SENDER_H


/*Functions Responsible Of Application Layer*/
void read_data_into_packet();
int llwrite(int fd);
void llopen(int fd);
void llclose();
void setup_data_packet();
void setup_control_packet(char *argv);
void send_control_packet(unsigned char end_start, unsigned char control_byte_expected, unsigned char indice_trama);
/*Functions Responsible Of Application Layer*/



/*Functions Responsible Of Data Link Layer*/
int stuffing(int bufSize, unsigned char** buf);
void WriteControlPacket(int size, unsigned char bcc2, unsigned char * control, unsigned char * byte);
void writeData(int data_length, unsigned char * trama_informacao, unsigned char* data);
void sendDisc(int *fd);
void sendUA(int *fd);
void sendSet(int *fd);
/*Functions Responsible Of Data Link Layer*/



/*Auxiliary Function of Sender*/
void maquinaEstados(unsigned char ua, char buf[], unsigned char byteControl);
void atende();  
unsigned char receive_feedback(unsigned char control_byte_expected);
/*Auxiliary Function of Sender*/

#endif