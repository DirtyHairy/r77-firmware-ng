#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <wait.h>
#include <sys/stat.h>

#include "md5.h"

enum State {
	STATE_NO_STELLA,
	STATE_STELLA_WITHOUT_ROM,
	STATE_STELLA_WITH_ROM
};

const unsigned char status_cmd[] = { 0xa9, 0x02, 0x80, 0x00, 0x00 };//request status
const unsigned char send_cmd[] =   { 0xa9, 0x02, 0x40, 0x00, 0x00 };//request send data
const unsigned char reset_cmd[] =  { 0xa9, 0x02, 0x20, 0x00, 0x00 };//request reset
const char* pathToStella = "/usr/bin/stella";

enum State state = STATE_NO_STELLA;
pid_t stellaPid;
int tty_fd = 0;

int ReadMCU(unsigned char *buf, int len){
	fd_set fdset;
	struct timeval timeout = {2, 0};//set the timeout as 1 sec for each read
	int maxfd = tty_fd + 1;
	int rev_num = 0;
	unsigned char* buf2 = buf;
	int total_len = 0;
	while(total_len < len){
		FD_ZERO(&fdset);
		FD_SET(tty_fd, &fdset);
		if(select(maxfd, &fdset, NULL, NULL, &timeout) > 0){
			if(FD_ISSET(tty_fd, &fdset)){
				rev_num = read(tty_fd, buf2, 1);
				total_len += rev_num;
				buf2 += rev_num;
				if(total_len >= len){
					return total_len;
				}
			}
		}else{
			printf("timeout or error\n");
			return -1;//timeout or error occurs return -1;
		}
	}
	return total_len;
}

void OpenMCU(){
	struct termios options;

	bzero(&options, sizeof(options));

	tty_fd = open("/dev/ttyS2", O_RDWR);
	if(tty_fd < 0) {printf("OpenMCU Failed\n"); return; }

	cfsetispeed(&options, B115200); //set input baud rates speed
	cfsetospeed(&options, B115200); //set output baud rates speed
	options.c_cflag |= (CS8 | CREAD | CLOCAL);
	options.c_iflag = 0;
	options.c_oflag = 0;
	options.c_lflag = 0;
        options.c_cc[VMIN]=1;   //set the min of characters to read in each read call
        options.c_cc[VTIME]=5; //read returns either when the specified number of characters are received or the timeout occurs
        tcsetattr(tty_fd,TCSANOW,&options);

	return ;
}

void runStella(const char* romfile) {
	if (!romfile && state == STATE_STELLA_WITHOUT_ROM) return;

	if (state != STATE_NO_STELLA) {
		printf("killing stella...\n");

		kill(stellaPid, SIGTERM);

		int exitStatus;
		waitpid(stellaPid, &exitStatus, 0);
	}

	pid_t pid = fork();

	if (pid < 0) {
		fprintf(stderr, "failed to fork\n");
		state = STATE_NO_STELLA;
	} else if (pid == 0) {
		printf("launching stella...\n");

		romfile ? execlp(pathToStella, pathToStella, romfile, NULL) : execlp(pathToStella, pathToStella, NULL);

		fprintf(stderr, "failed to execute stella\n");
		exit(0);
	} else {
		stellaPid = pid;
		state = romfile ? STATE_STELLA_WITH_ROM : STATE_STELLA_WITHOUT_ROM;
	}
}

const char* romfile_path(unsigned char* rom, int size, const char* destination) {
	struct stat s;
	if (stat(destination, &s) != 0 || (s.st_mode & S_IFMT) != S_IFDIR) return destination;

	MD5_CTX md5_ctx;
	MD5_Init(&md5_ctx);
	MD5_Update(&md5_ctx, rom, size);

	unsigned char md5[16];
	MD5_Final(md5, &md5_ctx);

	unsigned char md5_string[33];
	static unsigned const char HEX[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	for (int i = 0; i < 16; i++) {
		md5_string[2*i] = HEX[md5[i] >> 4];
		md5_string[2*i + 1] = HEX[md5[i] & 0x0f];
	}
	md5_string[32] = 0;

	static char* path = NULL;
	static size_t path_maxlen;
	if (path == NULL)
	{
		path_maxlen = strlen(destination) + 64;
		path = malloc(path_maxlen);
	}

	snprintf(path, path_maxlen, "%s/rom_%s.bin", destination, md5_string);

	return path;
}

int main(int argc, char** argv){
	if (argc != 2) {
		fprintf(stderr, "usage: dumper <destination>\n");
		exit(1);
	}

	const char* destination = argv[1];

	int cartridge_len = 4 * 1024;
	int ret = 0;

	unsigned char buf[32 * 1024];

	printf("Atari Dumper\n");
	OpenMCU();
	memset(buf, 0, 32 * 1024);

	write(tty_fd, status_cmd, 4); //send read status, for first time booting

	while (1)
	{
		memset(buf, 0, 32 * 1024);
		read(tty_fd, buf, 4);//MCU will auto generate status data when plug/unplug

		if(*(buf + 2) == 0){
			printf("No Cartridge\n");

			runStella(NULL);
			continue;
		}

		if (*(buf + 2) != 1) {
			continue;
		}

		printf("Cartridge Type: ");
		switch (*(buf + 3)) {
			case 0:
				cartridge_len = 4 * 1024;
				printf(" 4K\n");
				break;
			case 1:
				cartridge_len = 8 * 1024;
				printf(" 8K\n");
				break;
			case 2:
				cartridge_len = 16 * 1024;
				printf("16K\n");
				break;
			case 3:
				cartridge_len = 32 * 1024;
				printf("32K\n");
				break;
			default://can not get correct type
				runStella(NULL);
				continue;

				break;
		}

		int receive_count = 0, buf_index = 0;
		unsigned char receive_buf[70];

		memset(buf, 0, 32 * 1024);
		write(tty_fd, reset_cmd, 4); //send reset command
		write(tty_fd, send_cmd, 4); //send read data command

		while (1) {
			ret = ReadMCU(receive_buf + buf_index, 70);
			if(ret < 0) break;

			buf_index += ret;

			if(buf_index == 70){
				memcpy(buf + receive_count, receive_buf + 4, 64);
				receive_count += 64;

				buf_index = 0;
			}

			if(receive_count >= cartridge_len)
				break;
		}

		if(ret < 0){
			printf("failed to receive ROM, dropping to Stella\n");

			runStella(NULL);
			continue;
		}

		const char* romfile = romfile_path(buf, receive_count, destination);

		FILE* file = fopen(romfile, "w");
		fwrite(buf, 1, receive_count, file);
		fclose(file);
		sync();

		runStella(romfile);
	}

	close(tty_fd);
}

