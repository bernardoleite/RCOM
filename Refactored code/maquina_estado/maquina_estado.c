


enum State {START, FLAG_RCV, A_RCV, C_RCV, BCC_RCV, STOP, DATA_RCV};

enum State  state = START;


void maquinaEstados(unsigned char tmp, char buf[], unsigned char byteControl) {

		switch(state) {
			case START:
				if(tmp == FLAG)	{
					state = FLAG_RCV;
					buf[state] = tmp;
				}
				break;
			case FLAG_RCV:
				if(tmp == A) {
					state = A_RCV;
					buf[state] = tmp;
				}
				else if (tmp == FLAG)
					state = FLAG_RCV;
				else
					state = START;
				break;
			case A_RCV:
				if(tmp == byteControl) {
					state = C_RCV;
					buf[state] = tmp;
				}
				else if(tmp == FLAG)
			  		state = FLAG_RCV;
				else
					state = START;
				break;
			case C_RCV:

				if (tmp == buf[1]^buf[2]) {
					state = BCC_RCV;
					buf[state] = tmp;
				}
				else if (tmp == FLAG)
					state = FLAG_RCV;
				else
					state = START;
				break;
			case BCC_RCV:

				if (tmp == FLAG) {
					state = STOP;
					buf[state] = tmp;
				}
				else
					state = START;
				break;
			case STOP:
				break;

		}


}

