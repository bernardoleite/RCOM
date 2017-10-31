#include "data_link_layer.c"



long long int calculate(int *size_stuffed, unsigned char *trama_informacao, unsigned char* data, long long int file_size_inc){

		if(indice_trama){
			trama_informacao[2] = I1;
			trama_informacao[3] = A^I1;
		}else{
			trama_informacao[2] = I0;
			trama_informacao[3] = A^I0;
		}

		read_data_into_packet();

		int i;
		int data_length = (data_packet.L2 << 8) + data_packet.L1;

		file_size_inc+=data_length;

		unsigned char* data = (unsigned char *)malloc(6 + data_length);
		data[0] = data_packet.C;
		data[1] = data_packet.N;
		data[2] = data_packet.L2;
		data[3] = data_packet.L1;

		unsigned char bcc2 = 0;
		for(i=0; i < 4; i++){
			bcc2 ^= data[i];			
		}

		for(i = 0; i < data_length; i++) {
			data[4 + i] = data_packet.P[i];
			bcc2 ^= data[4 + i];
		}	
		int x;
		data[i+4] = bcc2;

		*size_stuffed = stuffing(data_length + 5, &data);

		return file_size_inc;
}