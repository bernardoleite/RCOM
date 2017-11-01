
#include "utilities.h"
#include "data_link.h"

void atende()                   
{
	flag = 1;		
	conta++;
	printf("TIMEOUT\n");
}

FILE *log_;
FILE *imagem;
int fd;

static struct termios oldtio,newtio;

void read_data_into_packet(){
	static char sequence_number = 0; 
	data_packet.N = ++sequence_number;
	if(sequence_number == 255)	
		sequence_number = 0;

	int read_n_bytes = fread(data_packet.P, 1, NUMBER_BYTES_EACH_PACKER, imagem);
	
	fwrite(data_packet.P,1,10,log_);


	if(read_n_bytes != NUMBER_BYTES_EACH_PACKER){
		
		data_packet.L2 = read_n_bytes >> 8;
		data_packet.L1 = (unsigned char)read_n_bytes;
	}
}


void calculate_packet(unsigned char indice_trama, long long int *file_size_inc, unsigned char* trama_informacao, unsigned char* data){
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
	(*file_size_inc)+=data_length;
	
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
}

int calculate_control_packet(unsigned char indice_trama, unsigned char end_start,unsigned char* byte,unsigned char* control){
	byte[0] = FLAG;
	byte[1] = A;
	byte[2] = (indice_trama ? I1:I0);
	byte[3] = A^byte[2];


	control[0] 	= (end_start ? START_PACKET_CONTROL : END_PACKET_CONTROL);
	control[1]	= control_packet.T1;
	control[2]	= control_packet.L1;
			
	control[3]	= ((char*)&control_packet.V1)[3];
	control[4]	= ((char*)&control_packet.V1)[2];
	control[5]	= ((char*)&control_packet.V1)[1];
	control[6] 	= ((char*)&control_packet.V1)[0];

	control[7]	= control_packet.T2;
	control[8]	= control_packet.L2;

	byte[4] = 0;

	int i;
	for(i = 0; i < control_packet.L2; i++){
		control[9+i] = control_packet.V2[i];

	}
	int size = 9+i;
	for(i = 0; i < size; i++) {
		byte[4] ^= control[i];

	}

	return stuffing(size, &control);
}

void setup_data_packet(){

	data_packet.N = 0;
	data_packet.C = 0x01;
	data_packet.L2 = (unsigned char)(NUMBER_BYTES_EACH_PACKER >> 8);
	data_packet.L1 = (unsigned char)NUMBER_BYTES_EACH_PACKER;

	data_packet.P = (unsigned char*)malloc((short)NUMBER_BYTES_EACH_PACKER);
	
}

void setup_control_packet(char *argv){

	long long int file_size;
	fseek(imagem, 0, SEEK_END);
	file_size=ftell(imagem);
	rewind(imagem);

	control_packet.T1 = 0x00;
	control_packet.T2 = 0x01;
	control_packet.C = 0x02;
	control_packet.L1 = sizeof(long long int);
	control_packet.V1 = file_size;

	control_packet.L2 = strlen(argv);
	control_packet.V2 = (char *)malloc(control_packet.L2);
	int i;
	for(i = 0; i < control_packet.L2; i++){
		control_packet.V2[i] = argv[i];
	}
}



void setup_serial_port(char* serialPort){

    fd = open(serialPort, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(serialPort); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { 
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 10;   
    newtio.c_cc[VMIN]     = 0;   


    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
}

void progress_bar(long long int progress, unsigned int total){
	static unsigned char NUMBER_PROGRESS_BAR = 20;
	float percentage = (progress*1.0)/(total*1.0);
	printf("\e[1;1H\e[2J");
	int i;
	printf("\n [");
	unsigned char completed = NUMBER_PROGRESS_BAR*percentage;
	for(i = 0; i < NUMBER_PROGRESS_BAR; i++){
		if(i <= completed)
			printf("-");
		else
			printf(" ");
	}
	printf("] %0.1f%%\n\n", percentage*100);
}


void open_image_file(char* filename){

    imagem = fopen(filename, "rb");

    if(!imagem){
	printf("Image could not be found!\n");
        exit(3);
    }
}

int main(int argc, char** argv){
    
    if ( (argc < 3) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

    setup_serial_port(argv[1]);

    log_ = fopen("log.txt", "w");

    (void) signal(SIGALRM, atende);  


    open_image_file(argv[2]);


    setup_control_packet(argv[2]);
    setup_data_packet();


    llopen(fd);
 
    llwrite(fd);

    llclose(fd);


    free(data_packet.P);
    free(control_packet.V2);

   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }


    fclose(log_);

    fclose(imagem);

    close(fd);
    return 0;
}
