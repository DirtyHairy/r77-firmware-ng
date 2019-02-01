#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h> // needed for memset
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


int main(int argc, char** argv){
	int cartridge_len = 4 * 1024;
	int ret = 0;
//        char cmd_to_run[] = "/usr/bin/killall stella; /usr/bin/killall r77_front_end; /bin/rm /dev/rom.bin; /bin/sleep 2; /usr/bin/r77_front_end&";
		char cmd_to_run[] = "/usr/bin/killall stella; /bin/rm /dev/rom.bin; /bin/sleep 2; /usr/bin/stella&";
        char cmd_to_run_factory[] = "/usr/bin/factory&";
	unsigned char status_cmd[] = { 0xa9, 0x02, 0x80, 0x00, 0x00 };//request status
	unsigned char send_cmd[] =   { 0xa9, 0x02, 0x40, 0x00, 0x00 };//request send data
	unsigned char reset_cmd[] =  { 0xa9, 0x02, 0x20, 0x00, 0x00 };//request reset
	unsigned char buf[128 * 1024];
	int factory_mode = 0;

	printf("Atari Dumper\n");
	OpenMCU();
	memset(buf, 0, 128 * 1024);

	if(access("/usr/bin/factory", F_OK) != -1){
		factory_mode = 1;
		system(cmd_to_run_factory);
	}else{
		factory_mode = 0;
	}

	write(tty_fd, status_cmd, 4); //send read status, for first time booting
	while(1){
		memset(buf, 0, 128 * 1024);
		read(tty_fd, buf, 4);//MCU will auto generate status data when plug/unplug

		if(*(buf + 2) == 0){
			printf("No Cartridge\n");
			if(factory_mode == 0)
	                        system(cmd_to_run);
			else
				system("/bin/rm -rf /dev/rom.bin");
		}else if(*(buf + 2) == 1){
			printf("Cartridge Type: ");
			switch(*(buf + 3)){
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
					if(factory_mode == 0)
						system(cmd_to_run);
					else
						system("/bin/rm -rf /dev/rom.bin");
                                        continue;
					break;
			}
			int receive_count = 0, buf_index = 0, write_count = 0;
			unsigned char* buf2 = buf;
			FILE* rom = fopen("/dev/rom.bin", "wb"); 

			memset(buf, 0, 128 * 1024);
			write(tty_fd, reset_cmd, 4); //send reset command
			write(tty_fd, send_cmd, 4); //send read data command
			while(1){
				ret = ReadMCU(buf2, 70);
				if(ret < 0) break;
				buf_index += ret;
				receive_count += ret;
				buf2 += ret;
				if(buf_index == 70){
					fwrite(buf + 4, 70 - 6, 1, rom);
					memset(buf, 0, 128 * 1024);
					buf_index = 0;
					buf2 = buf;
					write_count += 64;
				}
				if(write_count >= cartridge_len)
					break;
			}
			fflush(rom);
	  		fclose(rom);
			if(ret < 0){
       	                	system(cmd_to_run);
				printf("error occurs, drop execution stella\n");
				continue;
			}
			printf("Total receive data = %d, write_count = %d\n", receive_count, write_count);
			if( factory_mode == 0){
//	            system("/usr/bin/killall stella; /usr/bin/killall r77_front_end; /bin/sleep 1");
				system("/usr/bin/killall stella; /bin/sleep 1.5");
				system("/usr/bin/stella /dev/rom.bin&");
			}
		}
		system("/bin/sleep 1");
	}
	close(tty_fd);
}

