/*
 * memdump.c: Simple program to read/write from/to any location in memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>

#include "dmw96_gpio.h"
#include "dmw96_syscfg.h"
#include "dmw96_cmu.h"
#include "dmw96_spi.h"
#include "dmw96_i2c.h"
#include "video.h"
#include "common.h"

#define DSP_OSC_CLOCK	2
#define WIFI_OSC_CLOCK	3
#define PLL1 			4
#define PLL2			5

#define Reg12	0x06507430
#define Reg26	0x06507468
#define Reg35	0x0650748C
#define Reg36	0x06507490
#define Reg41	0x065074A4

#define DP
#ifdef DP
#include "dp_regs.h"



int pharse_command(int args_num, char **argv,int reserved);
static int dwt_host;
static int dwt_state;


unsigned int dp_rtc_get_ticks();
unsigned int dp_rtc_activate();
unsigned int dp_rtc_set_time();
unsigned int dp_rtc_set_alarm(unsigned int ticks);
//void DWT_PRINT(const char *string);
extern void dp_ldo_open(unsigned int ldo, unsigned int on_offn,
			float voltage);
extern void dp_vref(uint16 on_offn, uint16 voltage);
extern void dp_sdout(uint16 direction, uint16 on_offn);
extern void dp_out_gain(uint16 direction, uint16 gain);
extern void dp_afeout_connect(uint16 direction, uint16 input);
extern void dp_headset_en(unsigned short on_offn);
extern void dp_line_en(uint16 on_offn);
extern void dp_micpwr_en(uint16 on_offn);
extern void dp_handset_en(uint16 on_offn);
extern void dp_mic_en(uint16 on_offn);
extern void dp_handset_gain(uint16 gain);
extern void dp_mic_gain(uint16 gain);
//extern void dp_aux_en(uint16 module);
extern void dp_mux(uint16 module);
extern void dp_aux_sample();
extern void dp_aux_clk(uint16 clock_source);
extern int nand_readid();
extern void dp_hq_set(unsigned short mode);
extern void dp_pwm_on(unsigned short pwm_num, unsigned short pwm_width);
extern void dp_pwm_off(unsigned short pwm_num);
extern void dp_keypad_off();
extern void dp_keypad_on(unsigned short pwm_width);
extern void dp_headout_vol(unsigned short volume);
void dp_classd_on(void);
void dp_classd_off(void);
extern void dp_aux_en(void);
extern void dp_aux_mux(unsigned short mux);

unsigned short dp_read_aux();
extern void dp_charger_init(unsigned short type);

extern void dp_tp_init();
extern unsigned short dp_tp_measure_x1();
extern unsigned short dp_tp_measure_x2();
extern unsigned short dp_tp_measure_y1();
extern unsigned short dp_tp_measure_y2();

extern unsigned short dp_lqsd_en();
unsigned short dp_bandgap_tune();

char message_buffer[0xff];
extern void dw74_help_cmu();

void clr_clkrst(unsigned int reg, unsigned int bits);
void set_clkrst(unsigned int reg, unsigned int bits);
void i2c_write(unsigned int value, unsigned int interface);
void i2c_stop(unsigned int interface);
int wolfson_wr(volatile int RegAdd, unsigned short Data) ;
void unlock_clock();

#endif
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)
#define BUFSIZE 1024
extern int keypad_init();

unsigned long mem_read_virtual(unsigned long target, char access_type);
unsigned long mem_read(unsigned long target, char access_type);
unsigned long mem_write(unsigned long target, unsigned int writeval,
			char access_type);
void mem_setbits(unsigned long address, unsigned long bits);
void mem_clrbits(unsigned long address, unsigned long bits);
int spi_init(unsigned int interface);
int delay(unsigned int usec);

uint16 dp_read_reg(uint16 addr);
uint16 dp_write_reg(uint16 addr, uint16 data);
uint16 dp_clock_enable();
uint16 dp_setbits(uint16 addr, uint16 bits);
uint16 dp_clrbits(uint16 addr, uint16 bits);

static int i2c_base;
static int spi_base;

//#define EXPEDIBLUE
//#define EXPEDITOR
#define WAU

#define EXPEDIBLUE_HOST 0
#define EXPEDITOR_HOST 	1
#define WAU_HOST 		2

#define DEFAULT 0 
#define DEBUG 	1

int fd;
void *map_base, *virt_addr;

#define WM8976_CACHEREGNUM	58

static  uint16 wm8976_reg[WM8976_CACHEREGNUM] = {
    0x0000, 0x00ed, 0x0180, 0x01ef,
    0x0008, 0x0000, 0x014C, 0x0000,
    0x0000, 0x0000, 0x0004, 0x01ff,
    0x01ff, 0x0000, 0x0101, 0x0101,
    0x0000, 0x0000, 0x002c, 0x002c,
    0x002c, 0x002c, 0x002c, 0x0000,
    0x0132, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0038, 0x000b, 0x0032, 0x0008,
    0x0007, 0x0021, 0x161, 0x0026,
    0x0000, 0x0000, 0x0000, 0x0010,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0002, 0x0001, 0x0001,
    0x00, 0x00, 0x00, 0x00,
    0x0001, 0x0001
};

int verify_arguments(const char* func_name,int args_num,int req_args_num)
{
	if(args_num < req_args_num) {
       		DWT_PRINT("%s requires %d params\n",func_name,req_args_num); 
		return 1; 
	}
	return 0;
} 

int delay(unsigned int usec)
{
	usleep(usec);
	return 0;
}

int GetHex(char *String)
{
	int num = 0;
	int digit;
	unsigned int i;

	for (i = 0; i < strlen(String); i++) {
		if (String[i] >= 'A' && String[i] <= 'Z')
			String = String + 32;
	}

	for (i = 0; i < strlen(String); i++) {
		if ((String[i]) < '0' || String[i] > 'f')
			continue;
		num = num * 16;
		if (String[i] >= '0' && String[i] <= '9') {
			digit = String[i] - '0';
			num = num + digit;
		} else {
			digit = String[i] - 'a';
			num = num + digit + 10;
		}
	}
	return num;
}
/*void DWT_PRINT(const char *string)
{
	printf("%s",string);
}
*/
unsigned long mem_read(unsigned long target, char access_type)
{
	unsigned long read_result;
	
	if (dwt_state == DEBUG)
	{
		DWT_PRINT("mem_read 0x%lx\n", target);
	}

	map_base =
	    mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		 target & ~MAP_MASK);
	if (map_base == MAP_FAILED)
		FATAL;

	virt_addr = (unsigned char*) map_base + (target & MAP_MASK);

	switch (access_type) {
	case 'b':
		read_result = *((unsigned char *) virt_addr);
		break;
	case 'h':
		read_result = *((unsigned short *) virt_addr);
		break;
	case 'w':
		read_result = *((unsigned long *) virt_addr);
		break;
	default:
		DWT_ERROR("Illegal data type '%c'.\n", access_type);
		exit(2);
	}
	return read_result;
}

unsigned long mem_read_virtual(unsigned long target, char access_type)
{
	unsigned long read_result;
	
	if (dwt_state == DEBUG)
	{
		DWT_PRINT("mem_read 0x%lx\n", target);
	}

	map_base =
	    mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		 target & ~MAP_MASK);
	if (map_base == (void *) -1)
		FATAL;

	virt_addr = (unsigned char *)map_base + (target & MAP_MASK);

	switch (access_type) {
	case 'b':
		read_result = *((unsigned char *) virt_addr);
		break;
	case 'h':
		read_result = *((unsigned short *) virt_addr);
		break;
	case 'w':
		read_result = *((unsigned long *) virt_addr);
		break;
	default:
		DWT_ERROR("Illegal data type '%c'.\n", access_type);
		exit(2);
	}

	DWT_PRINT("virt_address %p\n", (unsigned long *) virt_addr);

	return read_result;
}

/* Setting clock or reset signal with protection register*/
void set_clkrst(unsigned int reg, unsigned int bits)
{
	unsigned int temp;

	temp = mem_read (reg,'w');
	temp |= bits;
	mem_write ( WRPR ,0x96,'w');
	mem_write (reg, temp,'w');
}

void clr_clkrst(unsigned int reg, unsigned int bits)
{
	unsigned int temp;

	temp = mem_read (reg,'w');
	temp &= ~bits;
	mem_write ( WRPR ,0x96,'w');
	mem_write (reg, temp,'w');
}


unsigned int get_cpu_speed(unsigned int pllval)
{
	unsigned int temp;
	unsigned int nf;
	unsigned int nr;
	unsigned int od;
	unsigned int cpu_speed ;
	char msg[0xff];
	int od_div;
	
	temp = mem_read (pllval,'w');
	DWT_PRINT("temp: 0x%x \r\n",temp );

	nf = (unsigned int)(temp/0x20);	


	nf = (nf & 0x3f) + 1;
	nr = (temp & 0x1f) +1;
	od = (temp>>12) & 0x3;

	DWT_PRINT("od: %d nf: %d, nr: %d\r\n",od, nf, nr );

	switch(od)
	{
		case 0:
			od_div =1 ;
			break;
		case 1:
			od_div =2 ;
			break;
		case 2:
			od_div =4 ;
			break;
		case 3:
			od_div =8 ;
			break;
		default:
			break;
	}
	delay(0x100);
	cpu_speed = nf ;
	cpu_speed = (12000000 * cpu_speed);
	cpu_speed = (float)cpu_speed/(float)nr ;
	cpu_speed = (float)cpu_speed/(float)od_div ;

	return cpu_speed;	
}

void set_cpu_speed(unsigned int speed)
{
	mem_clrbits (CPUCLKCNTRL, 0x100);
	switch(speed)
	{
		case 37:
			mem_write(PLL2CONTROL,0x3300,'w');
			break;
		case 99:
			mem_write(PLL2CONTROL,0x2400,'w');
			break;
		case 198:
			mem_write(PLL2CONTROL,0x1400,'w');
			break;
		case 240:
			mem_write(PLL2CONTROL,0x14e0,'w');
			break;
		case 300:
			mem_write(PLL2CONTROL,0x300,'w');
			break;
		case 360:
			mem_write(PLL2CONTROL,0x3a0,'w');
			break;
		case 480:
			mem_write(PLL2CONTROL,0x4e0,'w');
			break;
		case 576:
			mem_write(PLL2CONTROL,0x5e0,'w');
			break;
		case 600:
			mem_write(PLL2CONTROL,0x620,'w');
			break;
		case 660:
			mem_write(PLL2CONTROL,0x46c0,'w');
			break;
		case 720:
			mem_write(PLL2CONTROL,0x4760,'w');
			break;
		case 756:
			mem_write(PLL2CONTROL,0x47c0,'w');
			break;
		case 840:
			mem_write(PLL2CONTROL,0x48a0,'w');
			break;
		case 852:
			mem_write(PLL2CONTROL,0x48c0,'w');
			break;
		case 864:
			mem_write(PLL2CONTROL,0x48e0,'w');
			break;
		case 876:
			mem_write(PLL2CONTROL,0x4900,'w');
			break;
		case 888:
			mem_write(PLL2CONTROL,0x4920,'w');
			break;
		case 900:
			mem_write(PLL2CONTROL,0x4940,'w');
			break;
		case 912:
			mem_write(PLL2CONTROL,0x4960,'w');
			break;
		case 924:
			mem_write(PLL2CONTROL,0x4980,'w');
			break;
		case 936:
			mem_write(PLL2CONTROL,0x49a0,'w');
			break;
		case 948:
			mem_write(PLL2CONTROL,0x49c0,'w');
			break;
		case 960:
			mem_write(PLL2CONTROL,0x49e0,'w');
			break;
		case 972:
			mem_write(PLL2CONTROL,0x4a00,'w');
			break;
		case 984:
			mem_write(PLL2CONTROL,0x4a20,'w');
			break;
		case 996:
			mem_write(PLL2CONTROL,0x4a40,'w');
			break;
		case 1008:
			mem_write(PLL2CONTROL,0x4a60,'w');
			break;
		case 1020:
			mem_write(PLL2CONTROL,0x4a80,'w');
			break;
		case 1036:
			mem_write(PLL2CONTROL,0x4aa0,'w');
			break;
		case 1044:
			mem_write(PLL2CONTROL,0x4ad0,'w');
			break;
		case 1056:
			mem_write(PLL2CONTROL,0x4af0,'w');
			break;
		case 1068:
			mem_write(PLL2CONTROL,0x4b00,'w');
			break;
		case 1116:
			mem_write(PLL2CONTROL,0x4b80,'w');
			break;
			mem_write(PLL2CONTROL,0x4b80,'w');
			break;

		default:
			DWT_PRINT("Support just those speeds: 37, 99, 198, 240, 300, 360, 480, 576, 600, 660, 720, 756, 840, 864, 876, 888, 900, 912, 924, 936, 948, 960, 972, 988, 996, 1008, 1020, 1036, 1044, 1056, 1068,1116\r\n");
			break;
	}
	delay(0x20);
	mem_setbits (CPUCLKCNTRL, 0x100);
}


unsigned long mem_write(unsigned long target, unsigned int writeval,
			char access_type)
{
	unsigned long read_result = 0;

	if (dwt_state == DEBUG)
	{
		DWT_PRINT("mem_write 0x%lx 0x%x\n", target, writeval);
	}


	map_base =
	    mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		 target & ~MAP_MASK);
	if (map_base == (void *) -1)
		FATAL;
	virt_addr = (unsigned char*)map_base + (target & MAP_MASK);

	switch (access_type) {
	case 'b':
		*((unsigned char *) virt_addr) = writeval;
		read_result = *((unsigned char *) virt_addr);
		break;
	case 'h':
		*((unsigned short *) virt_addr) = writeval;
		read_result = *((unsigned short *) virt_addr);
		break;
	case 'w':
		*((unsigned long *) virt_addr) = writeval;
		read_result = *((unsigned long *) virt_addr);
		break;
	}
	return read_result;
}
void mem_setbits(unsigned long address, unsigned long bits)
{
	unsigned long value;

	if (dwt_state == DEBUG)
	{
		DWT_PRINT("mem_setbits 0x%lx 0x%lx\n", address, bits);
	}

	value = mem_read(address, 'w');
	value |= bits;
	mem_write(address, value, 'w');
}

void mem_clrbits(unsigned long address, unsigned long bits)
{
	unsigned long value;

	if (dwt_state == DEBUG)
	{
		DWT_PRINT("mem_clrbits 0x%lx 0x%lx\n", address, bits);
	}

	value = mem_read(address, 'w');
	value &= ~bits;
	mem_write(address, value, 'w');
}

void Overclocken()
{
            uint16 rstsr;

            rstsr = mem_read(0x5300040, 'w');
            rstsr = rstsr & 0xf;
     
       if ((rstsr == 0x4) || (rstsr  == 0x5)|| (rstsr == 0xC)|| (rstsr == 0x6))
            {
                        mem_write(0x5300000,0x1, 'w');
                        mem_write(0x5300080,0x0, 'w');
                        mem_write (0x5300060,0x94f0, 'w');
                        delay(0x1000);
                        mem_write (0x530002c ,0x6, 'w');
                        mem_write (0x5300080, 0x3, 'w');
                        mem_write (0x530007c,0x1100, 'w');
                        mem_write (0x5300070,0x26, 'w');
                        mem_write (0x5300074,0x103, 'w');
                        delay(1000);
                        mem_write (0x5300054, 0x1, 'w');
                        mem_write (0x5300000, 0x2, 'w');
                        delay(100);
                        DWT_PRINT("OverClocking set, speed now 240MHz\r\n");
            }
            else
            {
                        DWT_PRINT("Setting is not supported, work just in WIFI only straps\r\n");
            }
}
void  speed(uint16 speed)
{
            if (mem_read(0x5300074, 'w') != 0x103)
            {
                        DWT_PRINT("Can't overclock, open overclock mode by typing 'overclock_en'\r\n");
                        return ;
            }
            switch (speed)
            {
            case 264:
                        mem_write (0x5300070, 0x2a, 'w');
                        mem_write (0x5b00008, 0x1008b, 'w');
                        break;
            case 288:
                        mem_write (0x5300070, 0x2e, 'w');
                        mem_write (0x5b00008, 0x409c, 'w');
                        break;
            case 300:
                        mem_write (0x5300070, 0x31, 'w');
                        mem_write (0x5b00008, 0xc0a2, 'w');
                        break;
            case 312:
                        mem_write (0x5300070, 0x33, 'w');
                        mem_write (0x5b00008, 0x40a9, 'w');
                        break;

            case 324:
                        mem_write (0x5300070, 0x35, 'w');
                        mem_write (0x5b00008, 0xc0af, 'w');
                        break;
            default:
                        DWT_PRINT("Speed can be 268, 288, 300, 312, 324 \r\n");

                        break;
            }

}

 

 

//====================================================//
void dw240_en(void)
{
//   mem_write(0x530002c, 0x6,'w');
    mem_write(0x5300000, 0x0,'w');
    mem_write(0x5300080, 0x0,'w');
    mem_write(0x5300054, 0x0,'w');
    mem_write(0x5300074, 0x0,'w');
    mem_write(0x5300068, 0xC12,'w');
    mem_write(0x530006c, 0x101,'w');
    mem_write(0x5300080, 0x1,'w');
    mem_write(0x5300000, 0x2,'w');
}

void dw250_en(void)
{
    mem_write(0x5300000 ,0x0,'w');
    mem_write(0x530002c ,0x6,'w');
    mem_write(0x5300080 ,0x3,'w');
    mem_write(0x530007c ,0x1100,'w');
    mem_write(0x5300070 ,0x26,'w');
    mem_write(0x5300054 ,0x1,'w');
    mem_write(0x5300000 ,0x2,'w');
}
 
void dw260_en(void)
{
    mem_write(0x530002c, 0x6,'w');
    mem_write(0x5300068, 0xC14,'w');
    mem_write(0x530006c, 0x101,'w');
    mem_write(0x5300000, 0x0,'w');
    mem_write(0x5300080, 0x1,'w');
    mem_write(0x5300000, 0x2,'w');
}
void dw280_en(void)
{
    mem_write(0x530002c, 0x6,'w');
    mem_write(0x5300068, 0xC15,'w');
    mem_write(0x530006c, 0x101,'w');
    mem_write(0x5300000, 0x0,'w');
    mem_write(0x5300080, 0x1,'w');
    mem_write(0x5300000, 0x2,'w');
}

void dw300_en(void)
{
    mem_write(0x530002c, 0x6,'w');
    mem_write(0x5300068, 0xC15,'w');
    mem_write(0x530006c, 0x101,'w');
    mem_write(0x5300000, 0x0,'w');
    mem_write(0x5300080, 0x1,'w');
    mem_write(0x5300000, 0x2,'w');
}

void net_pharse(char *buffer)
{
	char main_buffer[BUFSIZE];
	char string1[0xff], string2[0xff], string3[0xff], string4[0xff];
	unsigned int value1, value2, value3;
	int i, j;

	sscanf(buffer, "%s %s %s %s", string1, string2, string3, string4);

	strcpy(main_buffer, "");
	if (strcmp(string1, "ip_mem_dump") == 0) {
		value1 = GetHex(string2);
		for (j = 0; j < 0x8; j++) {
			sprintf(string3, "%8x: ", (value1 + j * 0x10));
			strcat(main_buffer, string3);
			delay(1000);

			for (i = 0; i < 0x4; i++) {
				value2 =
				    mem_read(value1 + i * 4 + j * 0x10,
					     'w');
				sprintf(string3, "%8x ", value2);
				strcat(main_buffer, string3);
				delay(1000);
			}
			sprintf(string3, " \n");
			strcat(main_buffer, string3);
		}
		strcpy(buffer, main_buffer);
	} else if (strcmp(string1, "ip_mem_write") == 0) {
		value1 = GetHex(string2);
		value2 = GetHex(string3);
		value3 = mem_write(value1, value2, 'w');

		sprintf(string3, "0x%x ", value3);
		strcpy(buffer, string3);
	}
}

#define MEM_TRAN 0x401


void issue_address(unsigned int addr)
{
	unsigned int addr_inv =
	    ((addr & 0xff) << 16) | (addr & 0xff00) | ((addr & 0xff0000) >>
						       16);

	mem_write(DW_FC_PULSETIME, 0x47744, 'w');

	mem_write(DW_FC_SEQUENCE, 0x0631C1, 'w');	// change to CS3
	//mem_write(DW_FC_SEQUENCE,  0x0a31C1,'w'); // change to CS1, KEEP_CS
	mem_write(DW_FC_ADDR_COL, 0x0, 'w');
	mem_write(DW_FC_CMD, addr_inv, 'w');
	mem_write(DW_FC_DCOUNT, 0x0, 'w');
	mem_write(DW_FC_FBYP_CTL, 0x0, 'w');
	mem_write(DW_FC_CTL, MEM_TRAN, 'w');
	delay(1000);
//	volatile status;
//	volatile cnt = 0;
/*	while ((status =
		(mem_read(DW_FC_STATUS, 'w') & DW_FC_STATUS_TRANS_BUSY)) == 1) {
		cnt = status;
	}
*/
	delay(100);
}

void issue_address_16(unsigned int addr)
{
}


void prepare_write_data_byte_seq(void)
{
	mem_write(DW_FC_SEQUENCE, 0x060002, 'w');	// change to CS3
	mem_write(DW_FC_DCOUNT, 0x0, 'w');
}

void write_data_byte_seq(unsigned char data)
{	// Using ALE
	mem_write(DW_FC_ADDR_COL, data, 'w');
	mem_write(DW_FC_CTL, MEM_TRAN, 'w');
}
void write_data_byte(unsigned char data)
{	// Using ALE

	mem_write(DW_FC_SEQUENCE, 0x060002, 'w');	// change to CS3
	//mem_write(DW_FC_SEQUENCE,  0x0a0002, 'w'); // change to CS1, KEEP_CS
	mem_write(DW_FC_DCOUNT, 0x0, 'w');
	mem_write(DW_FC_ADDR_COL, data, 'w');
	mem_write(DW_FC_CTL, MEM_TRAN, 'w');
}

void write_data_word(unsigned int data)
{	// Using ALE

	mem_write(DW_FC_SEQUENCE, 0x060002 | NAND16_EN, 'w');	// change to CS3
	mem_write(DW_FC_DCOUNT, 0x0, 'w');
	mem_write(DW_FC_ADDR_COL, data, 'w');
	mem_write(DW_FC_CTL, MEM_TRAN, 'w');
}


void SSD_REGWB(unsigned int regNum, unsigned char value)
{
	issue_address(regNum);
	write_data_byte(value);
}

unsigned char read_data_byte_read_once(void)
{
	unsigned int result;

	mem_write(DW_FC_SEQUENCE, 0x070000, 'w');	// change to CS3
	//mem_write(DW_FC_SEQUENCE,  0x0b0000, 'w'); // change to CS1, KEEP_CS
	mem_write(DW_FC_DCOUNT, 0x0, 'w');
	mem_write(DW_FC_FBYP_CTL, 0x12, 'w');
	mem_write(DW_FC_CTL, MEM_TRAN, 'w');
	while ((mem_read(DW_FC_FBYP_CTL, 'w') & 0x1) != 0);	//Wait while BUSYSTS

	result = mem_read(DW_FC_FBYP_DATA, 'w');
	mem_write(DW_FC_FBYP_CTL, 0x0, 'w');

	result >>= 16;
	result &= 0xff;

	return result;
}


unsigned char SSD_REGRB(unsigned int regNum)
{

	unsigned char result;
	issue_address(regNum);
	delay(100);
	read_data_byte_read_once();	// First is void
	delay(100);
	result = read_data_byte_read_once();

	return result;
}

void lcd_reg_dump(unsigned int start_addr, unsigned int len)
{
	int i;
	for (i = 0; i < len; i++) {
		unsigned int cur_addr = start_addr + i;
		DWT_PRINT("DUMP_REG_LCD: address: %0#6x, value %0#4x\n",
		       cur_addr, SSD_REGRB(cur_addr));
	}
}

int ssd192x_init(void)
{

	lcd_reg_dump(0x120, 0x10);
//Removed, else it won't work with the external clock source
#if 0  
	SSD_REGWB(0x126, 0x0a);
	SSD_REGWB(0x127, 0xa0); // making it 64 MHz
	SSD_REGWB(0x04,  0x09); // making 6.4 MHz
#endif
	SSD_REGWB(0x12b, 0xae);
	SSD_REGWB(0x126, 0x8a);

	lcd_reg_dump(0x120, 0x10);

	return 0;
}


void issue_16address(unsigned int addr)
{
	unsigned int addr_inv =
	    ((addr & 0xff) << 16) | (addr & 0xff00) | ((addr & 0xff0000) >>
						       16);

	mem_write(DW_FC_PULSETIME, 0x47744, 'w');

	mem_write(DW_FC_SEQUENCE, 0x0631C1, 'w');	// change to CS3
	//mem_write(DW_FC_SEQUENCE,  0x0a31C1,'w'); // change to CS1, KEEP_CS
	mem_write(DW_FC_ADDR_COL, 0x0, 'w');
	mem_write(DW_FC_CMD, addr_inv, 'w');
	mem_write(DW_FC_DCOUNT, 0x0, 'w');
	mem_write(DW_FC_FBYP_CTL, 0x0, 'w');
	mem_write(DW_FC_CTL, MEM_TRAN, 'w');
	delay(1000);
//	volatile status;
//	volatile cnt = 0;
/*	while ((status =
		(mem_read(DW_FC_STATUS, 'w') & DW_FC_STATUS_TRANS_BUSY)) == 1) {
		cnt = status;
	}
*/
	delay(100);
}

void wm8976_init()
{
	int i=0;

	for (i=1;i<WM8976_CACHEREGNUM;i++)
	{
		wolfson_wr(i, wm8976_reg[i]);
		delay (0x1000);
	}
}





void dwt_server()
{
	char buffer[BUFSIZE];
	struct sockaddr_in addr;
	int sd, addr_size, bytes_read;

	sd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		abort();
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9999);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("bind");
		abort();
	}
	do {
		bzero(buffer, BUFSIZE);
		addr_size = BUFSIZE;
		bytes_read =
		    recvfrom(sd, buffer, BUFSIZE, 0,
			     (struct sockaddr *) &addr, &addr_size);
		if (bytes_read > 0) {
			DWT_PRINT("Connect: %s:%d \"%s\"\n",
			       inet_ntoa(addr.sin_addr),
			       ntohs(addr.sin_port), buffer);
			net_pharse(buffer);

			if (sendto
			    (sd, buffer, strlen(buffer), 0,
			     (struct sockaddr *) &addr, addr_size) < 0)
				perror("reply");
		} else
			perror("recvfrom");
	}
	while (bytes_read > 0);
	close(sd);
}

uint16 dp_set_bits(uint16 addr, uint16 bits)
{
	uint16 value = 0;

	value = dp_read_reg(addr);
	value |= bits;
	return dp_write_reg(addr, value);
}

uint16 dp_clr_bits(uint16 addr, uint16 bits)
{
	uint16 value;

	value = dp_read_reg(addr);
	value &= ~bits;
	return dp_write_reg(addr, value);
}


uint16 dp_clock_enable()
{
	mem_setbits(GCR0, 0x4000);
	mem_write(CDR1, 0x5, 'w');

	return 0;
}

void dp_dump(unsigned int address)
{
	int i, j;
	char string3[0xff];
	char main_buffer[BUFSIZE];
	int value;

	for (j = 0; j < 0x8; j++) {
		DWT_PRINT("%8x: ", (address + j * 0x4));
		delay(1000);

		for (i = 0; i < 0x4; i++) {
			value = dp_read_reg(address + i + j * 0x4);
			DWT_PRINT("%8x ", value);
			delay(1000);
		}
		DWT_PRINT(" \r\n");
	}
}

void test_keypad() 
{
	char string_keypad[0xff];
	int temp;
	int key_init_value;
	int l = 0;
	int j = 0;
	temp = 0;
	mem_clrbits(CSR1, 1 << 14);
	mem_setbits(0x500005c, 0xffff);
	
//      mem_setbits (0x5000064, 0xff);
	temp = mem_read(0x5000060, 'w');
	DWT_PRINT("kpint_row: 0x%x\r\n",temp);
	key_init_value = mem_read(0x5000060, 'w');

	DWT_PRINT("init val: 0x%x\n", key_init_value);
	while (1)  {
		for (j = 0; j < 0x5; j++) {
			mem_write(0x5000064, 1 << j, 'w');
			if (l == 0x200) {
				break;
			}

			delay(0x3000);
			if (temp != mem_read(0x5000060, 'w'))
			 {
				if (temp != key_init_value) {
					DWT_PRINT("kpint_row: 0x%x kpint_col: 0x%x\r\n",
						 temp, j);
				}
			}
			temp = mem_read(0x5000060, 'w');

		}
		l++;
	};
}

int main(int argc, char **argv)
{
	dwt_state = DEFAULT;

	if (argc < 2) {
		fprintf(stderr,
			"\nUsage:\t%s { address } [ type [ data ] ]\n"
			"\taddress : memory address to act upon\n"
			"\ttype    : access operation type : [br]byte read, [wr]word read, [b]yte, [h]alfword, [w]ord\n"
			"\tdata    : data to be written\n\n", argv[0]);
		exit(1);
	}

	if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
		FATAL;

#ifdef DEBUG
//	printf("/dev/mem opened.\n");
//	fflush(stdout);
#endif

	if (strcmp(argv[1],"-s")==0)
	{
		dwt_state = DEBUG;
		pharse_command(argc-2, &(argv[2]), 0);

	}
	else
	{
		pharse_command(argc-1,&(argv[1]) , 0);
	}

	/* Map one page */
	if (munmap(map_base, MAP_SIZE) == -1)
		FATAL;
	close(fd);
	return 0;
}

void I2CWait()
{
	delay(0x20);
}

int I2C_w_FIFO_Init_bus(int i2c_bus)
{
	if (i2c_bus ==0)
	{
		i2c_base = 0x6200000;
	}
	else
	{
		i2c_base = 0x6300000;
	}
	
	// set IIC bus clock to ~100Khz --> Thigh_o = Tlow_o = 0.000005  
	// IIC input clock is the APB clock @35Mhz --> Tlow_o*Fpclk = (5*10^-6)*(35*10^6) = 175 
	mem_write(IIC_CLKHI + i2c_base, 168,'w');	// 175 - 7
	mem_write(IIC_CLKLO + i2c_base, 174, 'w');	// 175 - 1
	mem_write(IIC_HOLDDAT + i2c_base, 20,'w');	

	// reset the registers (and SM?)
	mem_setbits(IIC_CTL + i2c_base,0x100);
	I2CWait();
	mem_clrbits(IIC_CTL + i2c_base,0x100);
	I2CWait();
	return 0;
}

void i2c_enable(int i2c_num)
{
	unsigned int temp;
	if (i2c_num == 0)
	{
		i2c_base = 0x6200000;
		mem_write(CGP_EN_CLR, 0x30000000,'w');
		set_clkrst(SWCLKENR2, 0x10);
		set_clkrst( SWCHWRST2, 0x10);
		clr_clkrst( SWCHWRST2, 0x10);
		mem_setbits ( SPCR1, 0x20000);
		
		I2C_w_FIFO_Init_bus(0);


	}
	else
	{
		i2c_base = 0x6300000;
		mem_write(CGP_EN_CLR,  0xc0000000,'w');
		set_clkrst(SWCLKENR2, 0x20);
		set_clkrst( SWCHWRST2, 0x20);
		clr_clkrst(SWCHWRST2, 0x20);
		mem_setbits ( SPCR1, 0x10000);

		I2C_w_FIFO_Init_bus(1);
	}
}

int wolfson_wr(volatile int RegAdd, unsigned short Data) 
{
            DWT_PRINT("Wolfson addr = %d   : val = 0x%x\n", RegAdd, Data);

	    i2c_write (0x34, 0);
            RegAdd = (RegAdd << 1) | ((Data & 0x100) >> 8);
	    i2c_write (RegAdd, 0);
	    i2c_write (Data & 0x1ff, 0);
	    i2c_stop(0);
	    return 0;
}
uint16 dp_read_reg(uint16 addr)

{
            uint16 spi_addr;
            uint16 value;
            volatile int temp=0, temp1=0, temp2=0;

	    spi_base = BASE_SPI1_ADDR;

            mem_write(spi_base + SPI_CFG_REG, 0x00060000,'w');  // TX & RX FIFO Flush (clear)
            mem_write(spi_base + SPI_CFG_REG, 0x80000072,'w');  // cs1 is not set to 1 at the end of transmit of each byte, Received data is ignored
            mem_write(spi_base + SPI_RATE_CNTL,0x4,'w');
            spi_addr = addr | 0x80;
            mem_setbits (BGP_DATA_CLR,0x40000000);
            mem_write (spi_base + SPI_TX_DAT, spi_addr,'w');
            delay(1);                                                          // time to finish sending 2 bytes
            mem_write(spi_base + SPI_TX_DAT, 0x0,'w');  // TX last byte transaction to assert cs1 ;
            delay(1);                                                         // time to finish sending 2 bytes
            temp1 = (mem_read(spi_base + SPI_RX_BUFF,'w') << 8);
            mem_write(spi_base + SPI_TX_DAT, 0x0,'w');
            delay(1);
            temp = mem_read(spi_base + SPI_RX_BUFF,'w') | temp1;
            mem_setbits (BGP_DATA_SET,0x40000000);

            return temp;

}

 

uint16 dp_write_reg(uint16 spi_addr, uint16 data)

{
            uint16 value;
	    volatile int temp=0, temp1=0, temp2=0;

	
	    spi_base = BASE_SPI1_ADDR;
            mem_write(spi_base + SPI_CFG_REG, 0x00060000,'w');  // TX & RX FIFO Flush (clear)
            mem_write(spi_base + SPI_CFG_REG, 0x80000172,'w');  // cs1 is not set to 1 at the end of transmit of each byte, Received data is ignored
            mem_write(spi_base + SPI_RATE_CNTL,0x4,'w');

			spi_addr &= ~0x80;
            mem_setbits (BGP_DATA_CLR,0x40000000);
            mem_write(spi_base + SPI_TX_DAT, spi_addr,'w');
            delay(1);                                                          // time to finish sending 2 bytes
            mem_write(spi_base + SPI_TX_DAT, (data>>8),'w');  // TX last byte transaction to assert cs1 ;
            delay(1);                                                         // time to finish sending 2 bytes
            mem_write(spi_base + SPI_TX_DAT, data, 'w');
            delay(1);
            mem_setbits (BGP_DATA_SET,0x40000000);
            mem_write(spi_base + SPI_TX_DAT, data, 'w');

	return 0;
}

void unlock_clock()
{
	mem_write ( WRPR ,0x96,'w');
}

int spi_init(unsigned int spi_num)

{

            unsigned int temp;
	    spi_base = BASE_SPI1_ADDR;

            if (spi_num == 0)

            {
                        mem_write (BGP_EN_CLR, 0xb0000000,'w');
                        mem_setbits (BGP_EN_SET, 0x40000000);
                        mem_setbits (BGP_DIR_OUT,0x40000000);
                        mem_setbits (BGP_DATA_SET,0x40000000);
                        temp = mem_read (SWCHWRST2,'w');
                        temp &= ~0x40;
                        unlock_clock();
                        mem_write (SWCHWRST2, temp,'w');
                        temp = mem_read ( SWCLKENR2 ,'w' );
                        temp |= 0x40;
                        unlock_clock();
                        mem_write(SWCLKENR2,temp,'w');
            }

            else

            {
                        mem_write (BGP_EN_CLR, 0xf000000,'w');
                        temp = mem_read (SWCHWRST2,'w');
                        temp &= ~0x80;
                        unlock_clock();
                        mem_write (SWCHWRST2, temp,'w');
                        temp = mem_read ( SWCLKENR2  ,'w');
                        temp |= 0x80;
                        unlock_clock();
                        mem_write        (SWCHWRST2,temp,'w');
            }
	return 0;
}



void i2c_scan(unsigned int i2c_interface)
{
	int i;
	unsigned int status;
	char messageBuffer[0xff];
	unsigned int Response=0;



	i2c_enable(i2c_interface);

	for (i = 0; i < 0x7f; i++) 
	{
		delay(0x10);
	   	mem_write(i2c_base + IIC_TX,(START_BIT | i*2),'w'); 
		delay(0x10);

		Response = mem_read (i2c_base + IIC_STS,'w');

		if((Response & NACK) ==0)
		{
			DWT_PRINT("Found address match at i2c address 0x%x\r\n",i << 1);
			delay(0x20);
			mem_write(i2c_base + IIC_TX, 0x0, 'w');	//Send byte other
			delay(0x20);
			mem_write(i2c_base + IIC_TX, 0x0, 'w');	//Send byte other
			delay(0x20);
			mem_write(i2c_base + IIC_TX, 0x100,'w');	//Send stop
			delay(0x20);
			mem_write(i2c_base + IIC_TX, 0x100, 'w');	//Send stop
			delay(0x20);
		}
	}
	
}

/*  Getting the I2C status */
void i2c_status(unsigned int interface)
{
	unsigned int Response=0;

	switch (interface)
	{
		case 0: 
		i2c_base = 0x6200000;
			break;
		case 1:
		i2c_base = 0x6300000;
			break;
		default:
		i2c_base = 0x6200000;
			break;
	}

	Response = mem_read(i2c_base + IIC_STS,'w');
	if((Response & IIC_BUS_BUSY) ==0)
	{
		DWT_PRINT("busy=0, ");
	}	
	else
	{
		DWT_PRINT("busy=1, ");
	}
	if((Response & NACK) ==0)
	{
		DWT_PRINT("nack=0, ");
	}	
	else
	{
		DWT_PRINT("nack=1, ");
	}
	DWT_PRINT("\r\n");
	
}

void i2c_read (unsigned int interface)
{
	unsigned int data;
	char message_buffer[0xff];

	switch (interface)
	{
		case 0: 
		i2c_base = 0x6200000;
			break;
		case 1:
		i2c_base = 0x6300000;
			break;
		default:
		i2c_base = 0x6200000;
			break;
	}

	mem_write (i2c_base + IIC_TX, mem_read (i2c_base + IIC_TX,'w'),'w');
	data=mem_read(i2c_base + IIC_RX,'w');		
	delay(50);
	data=mem_read(i2c_base + 0,'w');		
	DWT_PRINT("data: 0x%x", data);
	DWT_PRINT("\r\n");
	i2c_status(interface);
}

void i2c_write(unsigned int value, unsigned int interface)
{
	int i;
	unsigned int status;
	unsigned int Response=0;

	switch (interface)
	{
		case 0: 
		i2c_base = 0x6200000;
			break;
		case 1:
		i2c_base = 0x6300000;
			break;
		default:
		i2c_base = 0x6200000;
			break;
	}

	Response = mem_read (i2c_base + IIC_STS,'w');
	if((Response & IIC_BUS_BUSY) ==0)
	{
	   	mem_write(i2c_base + IIC_TX, START_BIT | value,'w'); 
	}
	else
	{
		mem_write(i2c_base + IIC_TX, value,'w');
	}
	delay(50);
	Response = mem_read(i2c_base + IIC_STS,'w');
	delay(50);
	i2c_status(interface);
}


void i2c_stop(unsigned int interface)
{
	int i;
	unsigned int status;
	unsigned int Response=0;

	switch (interface)
	{
		case 0: 
		i2c_base = 0x6200000;
			break;
		case 1:
		i2c_base = 0x6300000;
			break;
		default:
		i2c_base = 0x6200000;
			break;
	}

	mem_write(i2c_base + IIC_TX, 0x200,'w');
	delay(50);
	Response = mem_read(i2c_base + IIC_STS,'w');
	delay(50);
	i2c_status(interface);
}

void hdmi_write(unsigned int reg, unsigned int value)
{
	i2c_write (0x72,0);
	i2c_write (reg,0x0);
	i2c_write (value, 0x0);
	i2c_stop (0);
}

void hdmi_read(unsigned int reg)
{
	i2c_write (0x72,0x0);
	i2c_write (reg, 0x0);

	i2c_write (0x173,0);
	i2c_read (0);
	i2c_stop (0);
}

void acc_write(unsigned int reg, unsigned int value)
{
	i2c_write (0x32,0);
	i2c_write (reg,0x0);
	i2c_write (value, 0x0);
	i2c_stop (0);
}

void acc_read(unsigned int reg)
{
	i2c_write (0x32,0x0);
	i2c_write (reg, 0x0);

	i2c_write (0x133, 0x0);
	i2c_read (0);
	i2c_stop (0);
}


int ADV7525_powerup(){
   
        int i;
        int  add[]={0x94,0x41,0x98,0x9c,0x9d,0xa2,0xa3,0xde,0xe4,0xe5,0xe6,0xeb,0};
        int data[]={0x80, 0  , 3  ,0x38, 0x1,0xa0,0xa0,0x82,0xff,0xff,0xfc, 0x2};
  
//        mem_write (0x5300078, 0x502);

        //      printf(" Loading Power Up sequence \n\n");
        for (i=0;add[i];i++)
        hdmi_write(add[i],data[i]);
	return 0;
}


void hdmi_reset()
{
	hdmi_write (0xf8, 0x0);
	hdmi_write (0xf8, 0x80);
}


int sys_info()
{
	int temp;
	int nf;
	int nr;
	int od;
	float freq;

	temp = mem_read (PLL1CONTROL,'w');
	nf = temp >> 5;
	nf &= 0x7f;
	nf+=1;
	
	nr = (temp & 0x1f) +1;

	freq = nf/nr *12;
	DWT_PRINT("freq = %d\r\n", freq);
	return 0;
}

int pharse_command(int args_num, char **argv,int reserved)
{
	int value1, value2, value3;
	int i, j;
	unsigned long writeval;
	off_t target;
	int access_type = 'w';
	unsigned int value;

	if(args_num == 0)
	{
		return 0;
	}

	if (strcmp(argv[0], "help_shift_right") == 0) {
		VERIFY_ARGS("help_shift_right",3);
		value1 = GetHex(argv[1]);
		value2 = atoi(argv[2]);
		if (value2 > 31)
		{
			DWT_PRINT("Data can be shifted 0-31 bits\r\n");
			return -1;
		}
		DWT_PRINT("After shift: 0x%x\r\n",value1>>value2);
		
	} else if (strcmp(argv[0], "help_shift_left") == 0) {
		VERIFY_ARGS("help_shift_left",3);
		value1 = GetHex(argv[1]);
		value2 = atoi(argv[2]);
		if (value2 > 31)
		{
			DWT_PRINT("Data can be shifted 0-31 bits\r\n");
			return -1;
		}
		DWT_PRINT("After shift: 0x%x\r\n",value1<<value2);
		
	} else if (strcmp(argv[0], "swrst") == 0) {
		DWT_PRINT("swrst sent\r\n");
		mem_setbits(RSTSR, 0x20);
		mem_write(SWSYSRST, 0X7496398B,'w') ;
		mem_write(SWSYSRST, 0XB5C9C674,'w') ;	
	} else if (strcmp(argv[0], "sys_info") == 0) {
		sys_info();
	} else if (strcmp(argv[0], "mem_write") == 0) {
		VERIFY_ARGS("mem_write",3);
		value1 = GetHex(argv[1]);
		value2 = GetHex(argv[2]);
		value = mem_write(value1, value2, 'w');

		DWT_PRINT("0x%x\n", value);
	} else if (strcmp(argv[0], "mem_read") == 0) {
		VERIFY_ARGS("mem_read",2);
		value1 = GetHex(argv[1]);
		value2 = mem_read(value1, 'w');
		DWT_PRINT("0x%x\n", value2);
	} else if (strcmp(argv[0], "mem_read_virt") == 0) {
		VERIFY_ARGS("mem_read_virt",2);
		value1 = GetHex(argv[1]);
		value2 = mem_read_virtual(value1, 'w');
		DWT_PRINT("0x%x\n", value2);
	} else if (strcmp(argv[0], "mem_read16") == 0) {
		VERIFY_ARGS("mem_read16",2);
		value1 = GetHex(argv[1]);
		value2 = mem_read(value1, 'h');
		DWT_PRINT("0x%x\n", value2);
	} else if (strcmp(argv[0], "mem_setbits") == 0) {
		VERIFY_ARGS("mem_setbits",3);
		value1 = GetHex(argv[1]);
		value2 = GetHex(argv[2]);
		value = mem_read(value1, 'w');
		value = value | value2;
		value = mem_write(value1, value, 'w');
		DWT_PRINT("0x%x\n", value);
	} else if (strcmp(argv[0], "mem_clrbits") == 0) {
		VERIFY_ARGS("mem_clrbits",3);
		value1 = GetHex(argv[1]);
		value2 = GetHex(argv[2]);
		value = mem_read(value1, 'w');
		value &= (~value2);
		value = mem_write(value1, value, 'w');
		DWT_PRINT("0x%x\n", value);
	} else if (strcmp(argv[0], "mem_dump") == 0 || strcmp(argv[0], "md") == 0) {
		VERIFY_ARGS("mem_dump",2);
		value1 = GetHex(argv[1]);
		for (j = 0; j < 0x8; j++) {
			DWT_PRINT("%8x: ", (value1 + j * 0x10));
			delay(1000);

			for (i = 0; i < 0x4; i++) {
				value2 =
				    mem_read(value1 + i * 4 + j * 0x10,
					     'w');
				DWT_PRINT("%8x ", value2);
				delay(1000);
			}
			DWT_PRINT(" \r\n");
		}
	} else if (strcmp(argv[0], "mem_dump16") == 0) {
		VERIFY_ARGS("mem_dump16",2);
		value1 = GetHex(argv[1]);
		for (j = 0; j < 0x8; j++) {
			DWT_PRINT("%8x: ", (value1 + j * 0x10));
			delay(1000);

			for (i = 0; i < 0x8; i++) {
				value2 =
				    mem_read(value1 + i * 2 + j * 0x10,
					     'h');
				DWT_PRINT("%4x ", value2);
				delay(1000);
			}
			DWT_PRINT(" \r\n");
		}
	} else if (strcmp(argv[0], "mem_dump8") == 0) {
		VERIFY_ARGS("mem_dump8",2);
		value1 = GetHex(argv[1]);
		for (j = 0; j < 0x8; j++) {
			DWT_PRINT("%8x: ", (value1 + j * 0x10));
			delay(1000);

			for (i = 0; i < 0x16; i++) {
				value2 =
				    mem_read(value1 + i  + j * 0x10,
					     'b');
				DWT_PRINT("%4x ", value2);
				delay(1000);
			}
			DWT_PRINT(" \r\n");
		}
	} else if (strcmp(argv[0], "set")==0) {
		VERIFY_ARGS("set",3);
		if (strcmp(argv[1],"agpio")==0)
		{
			value1 =  AGP_DATA_SET;
			value2 =  AGP_DIR_OUT ;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else 
		if (strcmp(argv[1],"bgpio")==0)
		{
			value1 =  BGP_DATA_SET;
			value2 =  BGP_DIR_OUT ;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else
		if (strcmp(argv[1],"cgpio")==0)
		{
			value1 =  CGP_DATA_SET;
			value2 =  CGP_DIR_OUT ;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else
		if (strcmp(argv[1],"dgpio")==0)
		{
			value1 =  DGP_DATA_SET;
			value2 =  DGP_DIR_OUT ;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else
		if (strcmp(argv[1],"egpio")==0)
		{
			value1 =  EGP_DATA_SET;
			value2 =  EGP_DIR_OUT ;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 17) {
				DWT_PRINT("GPIO value can be 0-17 \r\n"); 
				return 1;
			}
		}
		else
		if (strcmp(argv[1],"fgpio")==0)
		{
			value1 =  FGP_DATA_SET;
			value2 =  FGP_DIR_OUT ;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else
		if (strcmp(argv[1],"ggpio")==0)
		{
			value1 =  GGP_DATA_SET;
			value2 =  GGP_DIR_OUT ;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 22) {
				DWT_PRINT("GPIO value can be 0-22 \r\n"); 
				return 1;
			}
		}

		mem_write (value2,(1<<value3),'w');
		mem_write ((value1), (1<<value3),'w');
	} else if (strcmp(argv[0], "clr")==0) {
		VERIFY_ARGS("clr",3);
		if (strcmp(argv[1],"agpio")==0)
		{
			value1 = AGP_DATA_CLR;
			value2 = AGP_DIR_OUT;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else 
		if (strcmp(argv[1],"bgpio")==0)
		{
			value1 = BGP_DATA_CLR;
			value2 = BGP_DIR_OUT;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else
		if (strcmp(argv[1],"cgpio")==0)
		{
			value1 = CGP_DATA_CLR;
			value2 = CGP_DIR_OUT;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else
		if (strcmp(argv[1],"dgpio")==0)
		{
			value1 = DGP_DATA_CLR;
			value2 = DGP_DIR_OUT;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}		
		else
		if (strcmp(argv[1],"egpio")==0)
		{
			value1 = EGP_DATA_CLR;
			value2 = EGP_DIR_OUT;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 17) {
				DWT_PRINT("GPIO value can be 0-17 \r\n"); 
				return 1;
			}
		}		
		else
		if (strcmp(argv[1],"fgpio")==0)
		{
			value1 = FGP_DATA_CLR;
			value2 = FGP_DIR_OUT;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}		

		else
		if (strcmp(argv[1],"ggpio")==0)
		{
			value1 = GGP_DATA_CLR;
			value2 = GGP_DIR_OUT;
			value3 = atoi(argv[2]);
			if (value3 < 0 || value3 > 22) {
				DWT_PRINT("GPIO value can be 0-22 \r\n"); 
				return 1;
			}
		}		
		mem_write (value2, (1<<value3),'w');
		mem_write (value1, (1<<value3),'w');

	} else if (strcmp(argv[0], "get")==0) {
		VERIFY_ARGS("get",3);
		value3 = atoi(argv[2]);
		if (strcmp(argv[1],"agpio")==0)
		{
			value1 =  AGP_DATA;
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}

		}
		else if (strcmp(argv[1],"bgpio")==0)
		{
			value1 =  BGP_DATA;
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else if (strcmp(argv[1],"cgpio")==0)
		{
			value1 =  CGP_DATA;
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}

		}
		else if (strcmp(argv[1],"dgpio")==0)
		{
			value1 =  DGP_DATA;
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}

		}
		else if (strcmp(argv[1],"egpio")==0)
		{
			value1 =  EGP_DATA;
			if (value3 < 0 || value3 > 17) {
				DWT_PRINT("GPIO value can be 0-17 \r\n"); 
				return 1;
			}

		}
		else if (strcmp(argv[1],"fgpio")==0)
		{
			value1 =  FGP_DATA;
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}

		}
		else if (strcmp(argv[1],"ggpio")==0)
		{
			value1 =  GGP_DATA;
			if (value3 < 0 || value3 > 26) {
				DWT_PRINT("GPIO value can be 0-26 \r\n"); 
				return 1;
			}

		}

		if (mem_read (value1,'w') & (1<<value3))
		{
			DWT_PRINT("1\r\n");
		}
		else
		{
			DWT_PRINT("0\r\n");
		}
	} else if (strcmp(argv[0], "setin")==0) {
		VERIFY_ARGS("setin",3);
		value3 = atoi(argv[2]);
		if (strcmp(argv[1],"agpio")==0)
		{
			value1 =  AGP_DIR_IN;
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}

		}
		else 
		if (strcmp(argv[1],"bgpio")==0)
		{
			value1 =  BGP_DIR_IN;
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else
		if (strcmp(argv[1],"cgpio")==0)
		{
			value1 =  CGP_DIR_IN;
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else
		if (strcmp(argv[1],"cgpio")==0)
		{
			value1 =  CGP_DIR_IN;
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}

		else
		if (strcmp(argv[1],"dgpio")==0)
		{
			value1 =  DGP_DIR_IN;
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else
		if (strcmp(argv[1],"egpio")==0)
		{

			value1 =  EGP_DIR_IN;
			if (value3 < 0 || value3 > 17) {
				DWT_PRINT("GPIO value can be 0-17 \r\n"); 
				return 1;
			}
		}
		else
		if (strcmp(argv[1],"fgpio")==0)
		{
			value1 =  FGP_DIR_IN;

			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		else
		if (strcmp(argv[1],"ggpio")==0)
		{
			value1 =  GGP_DIR_IN;

			if (value3 < 0 || value3 > 22) {
				DWT_PRINT("GPIO value can be 0-22 \r\n"); 
				return 1;
			}
		}
		mem_write (value1 , (1<<value3),'w');
	} else if (strcmp(argv[0], "disable") == 0) {
		VERIFY_ARGS("disable",3);
		value3 = atoi(argv[2]);
		if (strcmp(argv[1], "agpio") == 0) {
			value1 = 0x5000008;
			if (value3 < 0 || value3 > 33) {
				DWT_PRINT("GPIO value can be 0-33 \r\n"); 
				return 1;
			}
		} else if (strcmp(argv[1], "bgpio") == 0) {
			value1 = 0x5000020;
			if (value3 < 0 || value3 > 32) {
				DWT_PRINT("GPIO value can be 0-32 \r\n"); 
				return 1;
			}
		} else if (strcmp(argv[1], "cgpio") == 0) {
			value1 = 0x5000038;
			if (value3 < 0 || value3 > 26) {
				DWT_PRINT("GPIO value can be 0-26 \r\n"); 
				return 1;
			}
		} else if (strcmp(argv[1], "dgpio") == 0) {
			value1 = 0x5000080;
			if (value3 < 0 || value3 > 31) {
				DWT_PRINT("GPIO value can be 0-31 \r\n"); 
				return 1;
			}
		}
		value = mem_read(value1, 'w');
		value &= ~(1 << (value3));
		mem_write(value1, value, 'w');
	} else if (strcmp(argv[0], "overclock_en") == 0) {
		mem_write (GGP_EN_SET,1<<9,'w');
		mem_write (GGP_DIR_OUT,1<<9,'w');
		mem_write (GGP_DATA_SET,1<<9,'w');
	} else if (strcmp(argv[0], "set_cpu_speed") == 0) {
		VERIFY_ARGS("set_cpu_speed",2);
		value1 = atoi(argv[1]);

		set_cpu_speed(value1);
	} else if (strcmp(argv[0], "get_cpu_speed") == 0) {
		value1 = get_cpu_speed(PLL2CONTROL);
		DWT_PRINT("Cpu speed: %d\r\n", value1);
	} else if (strcmp(argv[0], "get_sys_speed") == 0) {
		value2 = get_cpu_speed(PLL1CONTROL);

		DWT_PRINT("PLL1 speed: %d\r\n", value2);

		value1 = mem_read(CPUCLKCNTRL,'w');
		value1 = value1>>4;
		value1 = value1 & 0xf;
		value2=value2/((value1+1)*2);
		DWT_PRINT("Sysclk: %d\r\n", value2);
	
	} else if (strcmp(argv[0], "get_dram_speed") == 0) {
		value2 = get_cpu_speed(PLL3CONTROL);
		DWT_PRINT("Dram speed: %d\r\n", value2);
	} else if (strcmp(argv[0], "get_arm_speed") == 0)  {
		value2 = get_cpu_speed(PLL4CONTROL);
		DWT_PRINT("ARM speed: %d\r\n", value2);
	} else if (strcmp(argv[0], "spi_init") == 0) {
		VERIFY_ARGS("spi_init",2);
		value1 = GetHex(argv[1]);
		spi_init(value1);
	} else if (strcmp(argv[0], "new_com") == 0x0) {
		DWT_PRINT("Hello Sanyo how r u!!!\n");
	} else if (strcmp(argv[0], "debug") == 0x0) {
		DWT_PRINT("Initiate Keypad\n");
	} else if (strcmp(argv[0], "server") == 0x0) {
		dwt_server();
	} else if (strcmp(argv[0], "dpw") == 0x0) {
		VERIFY_ARGS("dpw",3);
		value1 = GetHex(argv[1]);
		value2 = GetHex(argv[2]);
		value3 = dp_write_reg(value1, value2);

	} else if (strcmp(argv[0], "dp_clr_bits") == 0x0) {
		VERIFY_ARGS("dp_clr_bits",3);
		value1 = GetHex(argv[1]);
		value2 = GetHex(argv[2]);
		value3 = dp_clr_bits(value1, value2);
		DWT_PRINT("0x%x\n", value3);
	} else if (strcmp(argv[0], "dp_set_bits") == 0x0) {
		VERIFY_ARGS("dp_set_bits",3);
		value1 = GetHex(argv[1]);
		value2 = GetHex(argv[2]);
		value3 = dp_set_bits(value1, value2);
		DWT_PRINT("0x%x\n", value3);
	} else if (strcmp(argv[0], "dp_dump") == 0x0) {
		VERIFY_ARGS("dp_dump",2);
		value1 = GetHex(argv[1]);
		dp_dump(value1);
	} else if (strcmp(argv[0], "dp_magic") == 0x0) {
		dp_write_reg(0x7e, 0xdeaf);
		dp_write_reg(0x7e, 0xbabe);
		dp_write_reg(0x7e, 0xfeed);
		dp_write_reg(0x7e, 0xdcdc);
	} else if (strcmp(argv[0], "test_clk") == 0x0) {
		mem_write(0x5300034, 0x453a, 'w');
		delay(0x2000);
		mem_write(0x5300054, 0x5, 'w');
		delay(0x2000);
		mem_write(0x5300054, 0x4, 'w');
	}
#ifdef DP
	else if (strcmp(argv[0], "dp_lqsd_en") == 0x0) {
			dp_lqsd_en();
	}
	
	else if (strcmp(argv[0], "dp_bandgap_tune") == 0x0) {
		value1 = dp_bandgap_tune();
		DWT_PRINT("Tune value: 0x%x\n voltage: %f\n",value1, (float)dp_read_aux()*2/0x7fff);
	}
	else if (strcmp(argv[0], "dp_comparator_tune") == 0x0) {
		value1 = dp_comparator_tune();
		DWT_PRINT("Tune value: 0x%x\n  %f\n",value1); //RR ASK NIR
	}

	else if (strcmp(argv[0], "dp_tp_init") == 0x0) {
		dp_tp_init();
	}
	else if (strcmp(argv[0], "dp_tp_measure_x1") == 0x0) {
		value1 = dp_tp_measure_x1();
		DWT_PRINT("0x%x\n voltage: %f\n",value1, (float)value1*2/0x7fff);
	}
	else if (strcmp(argv[0], "dp_tp_measure_x1") == 0x0) {
			value1 = dp_tp_measure_x2();
			DWT_PRINT("0x%x\n voltage: %f\n",value1, (float)value1*2/0x7fff);
	}
	else if (strcmp(argv[0], "dp_tp_measure_y1") == 0x0) {
			value1 = dp_tp_measure_y1();
			DWT_PRINT("0x%x\n voltage: %f\n",value1, (float)value1*2/0x7fff);
	}
	else if (strcmp(argv[0], "dp_tp_measure_y2") == 0x0) {
			value1 = dp_tp_measure_y2();
			DWT_PRINT("0x%x\n voltage: %f\n",value1, (float)value1*2/0x7fff);
	}

	else if (strcmp(argv[0], "dp_charger_init") == 0x0) {
		dp_charger_init(1);
	}
	else if (strcmp(argv[0], "dp_aux_en") == 0x0) {
		dp_aux_en();
	}
	else if (strcmp(argv[0], "dp_aux_mux") == 0x0) {
		VERIFY_ARGS("dp_aux_mux",2);
		value1 = GetHex(argv[1]);
		dp_aux_mux(value1);
	}
	else if (strcmp(argv[0], "dp_aux_read") == 0x0) {
		value1 = dp_read_aux();
		DWT_PRINT("0x%x\n voltage: %f\n",value1, (float)value1*2/0x7fff);
	}
	else if (strcmp(argv[0], "dp_classd_on") == 0x0) {
		dp_classd_on();
	}
	else if (strcmp(argv[0], "dp_classd_off") == 0x0) {
		dp_classd_off();
	}
	else if (strcmp(argv[0], "dp_keypad_on") == 0x0) {
		VERIFY_ARGS("dp_keypad_on",2);
		value1 = GetHex(argv[1]);
		dp_keypad_on(value1);
	}
	else if (strcmp(argv[0], "dp_keypad_off") == 0x0) {
			dp_keypad_off();
	}
	else if (strcmp(argv[0], "dp_pwm_on") == 0x0) {
			VERIFY_ARGS("dp_pwm_on",3);
			value1 = GetHex(argv[1]);
			value2 = GetHex(argv[2]);
			
			if (value1!=0 && value1!=1&& value1!=0x2)
			{
				DWT_PRINT("PWM can be 0 or 1, format: dp_pwm_on <#pwm_num> <#pwm_width>");
				return -1;
			}
			dp_pwm_on(value1,value2 );
		}
	else if (strcmp(argv[0], "dp_headout_vol") == 0x0) {
			VERIFY_ARGS("dp_headout_vol",2);
			value1 = GetHex(argv[1]);
			dp_headout_vol(value1);
						
		}

	else if (strcmp(argv[0], "dp_pwm_off") == 0x0) {
			VERIFY_ARGS("dp_pmw_off",3);
			value1 = GetHex(argv[1]);
			value2 = GetHex(argv[2]);

			if (value1!=0 && value1!=1 && value1!=0x2)
			{
				DWT_PRINT("PWM can be 0 or 1, format: dp_pwm_off <#pwm_num>");
				return -1;
			}
			dp_pwm_off(value1);

		}
	else if (strcmp(argv[0], "dp_afeout_connect") == 0x0) {
		VERIFY_ARGS("dp_afeout_connect",3);
		if (strcmp(argv[1], "mute") == 0x0) {
			if (strcmp(argv[2], "r") == 0x0) {
				dp_afeout_connect(RIGHT, MUTE);
			} else if (strcmp(argv[2], "l") == 0x0) {
				dp_afeout_connect(LEFT, MUTE);
			} else {
				DWT_PRINT("Format is: dp_afeout_connect mute [r/l]\n");
			}
		} else if (strcmp(argv[1], "lqdac") == 0x0) {
			if (strcmp(argv[2], "r") == 0x0) {
				dp_afeout_connect(RIGHT, LQDAC);
			} else if (strcmp(argv[2], "l") == 0x0) {
				dp_afeout_connect(LEFT, LQDAC);
			} else {
				DWT_PRINT("Format is: dp_afeout_connect lqdac [r/l]\n");
			}
		} else if (strcmp(argv[1], "hqsdo_l") == 0x0) {
			if (strcmp(argv[2], "r") == 0x0) {
				dp_afeout_connect(RIGHT, LQSDO_L);
			} else if (strcmp(argv[2], "l") == 0x0) {
				dp_afeout_connect(LEFT, LQSDO_L);
			} else {
				DWT_PRINT("Format is: dp_afeout_connect hgsdo_l [r/l]\n");
			}
		} else if (strcmp(argv[1], "hqsdo_r") == 0x0) {
			if (strcmp(argv[2], "r") == 0x0) {
				dp_afeout_connect(RIGHT, LQSDO_R);
			} else if (strcmp(argv[2], "l") == 0x0) {
				dp_afeout_connect(LEFT, LQSDO_R);
			} else {
				DWT_PRINT("Format is: dp_afeout_connect hqsdo_r [r/l]\n");
			}
		} else if (strcmp(argv[1], "invmic") == 0x0) {
			if (strcmp(argv[2], "r") == 0x0) {
				dp_afeout_connect(RIGHT, INVMIC);
			} else if (strcmp(argv[2], "l") == 0x0) {
				dp_afeout_connect(LEFT, INVMIC);
			} else {
				DWT_PRINT("Format is: dp_afeout_connect invmic [r/l] \n");
			}
		}

		else if (strcmp(argv[1], "invhsmic") == 0x0) {
			if (strcmp(argv[2], "r") == 0x0) {
				dp_afeout_connect(RIGHT, INVHSMIC);
			} else if (strcmp(argv[2], "l") == 0x0) {
				dp_afeout_connect(LEFT, INVHSMIC);
			} else {
				DWT_PRINT("Format is: dp_afeout_connect invhsmic [r/l]\n");
			}
		} else if (strcmp(argv[1], "mic") == 0x0) {
			if (strcmp(argv[2], "r") == 0x0) {
				dp_afeout_connect(RIGHT, MIC);
			} else if (strcmp(argv[2], "l") == 0x0) {
				dp_afeout_connect(LEFT, MIC);
			} else {
				DWT_PRINT("Format is: dp_afeout_connect mic [r/l]\n");
			}
		}
		else if (strcmp(argv[1], "hsmic") == 0x0) {
			if (strcmp(argv[2], "r") == 0x0) {
				dp_afeout_connect(RIGHT, HSMIC);
			} else if (strcmp(argv[2], "l") == 0x0) {
				dp_afeout_connect(LEFT, HSMIC);
			} else {
				DWT_PRINT("Format is: dp_afeout_connect hsmic [r/l] \n");
			}
		}

	} else if (strcmp(argv[0], "dp_sdout") == 0x0) {
		VERIFY_ARGS("dp_sdout",3);
		value2 = GetHex(argv[2]);
		if (strcmp(argv[1], "r") == 0x0) {
			dp_sdout(RIGHT, value2);
		} else if (strcmp(argv[1], "l") == 0x0) {
			dp_sdout(LEFT, value2);
		} else {
			value2 = GetHex(argv[1]);
			dp_sdout(RIGHT, value2);
			dp_sdout(LEFT, value2);
		}
	} else if (strcmp(argv[0], "dp_out_gain") == 0x0) {
		VERIFY_ARGS("dp_out_gain",3);
		value2 = GetHex(argv[2]);
		if (strcmp(argv[1], "r") == 0x0) {
			dp_out_gain(RIGHT, value2);
		} else if (strcmp(argv[1], "l") == 0x0) {
			dp_out_gain(LEFT, value2);
		} else {
			value2 = GetHex(argv[1]);
			dp_out_gain(RIGHT, value2);
			dp_out_gain(LEFT, value2);
		}
	} else if (strcmp(argv[0], "dp_ldo") == 0x0) {
		VERIFY_ARGS("dp_ldo",3);
		value2 = GetHex(argv[2]);
		if (strcmp(argv[1], "sp1") == 0x0) {
			dp_ldo_open(LDO_SP1, value2, 0);
		} else if (strcmp(argv[1], "sp2") == 0x0) {
			dp_ldo_open(LDO_SP2, value2, 0);
		} else if (strcmp(argv[1], "sp3") == 0x0) {
			dp_ldo_open(LDO_SP3, value2, 0);
		} else if (strcmp(argv[1], "io") == 0x0) {
			dp_ldo_open(LDO_IO, value2, 0);
		} else if (strcmp(argv[1], "core0") == 0x0) {
			dp_ldo_open(LDO_CORE0, value2, 0);
		} else if (strcmp(argv[1], "mem") == 0x0) {
			dp_ldo_open(LDO_MEM, value2, 0);
		} else if (strcmp(argv[1], "dpio") == 0x0) {
			dp_ldo_open(LDO_DPIO, value2, 0);
		} else {
			DWT_PRINT("Usage is not right. Usage should be: dp_ldo [dpio,mem,core0,io,sp1,sp2,sp3] [0/1]\n");
		}
	} else if (strcmp(argv[0], "dp_vref") == 0x0) {
		VERIFY_ARGS("dp_vref",3);
		value2 = GetHex(argv[1]);
		value3 = GetHex(argv[2]);
		if (value2 <  0 || value2 > 0 || value3 < 0x0 || value3 > 0x7) {
			DWT_PRINT("Usage is not right, should be: dp_vref [0/1] [0-7]\n");
			return -1;
		}
		dp_vref(value2, value3);
	} else if (strcmp(argv[0], "dp_headset") == 0x0) {
		VERIFY_ARGS("dp_headset",2);
		value2 = GetHex(argv[1]);
		if (value2 != 0x0 && value2 != 0x1) {
			DWT_PRINT("Usage is: dp_headset [1/0]");
			return -1;
		}
		dp_headset_en(value2);
	} else if (strcmp(argv[0], "dp_mic") == 0x0) {
		VERIFY_ARGS("dp_mic",2);
		value2 = GetHex(argv[1]);
		if (value2 != 0x0 && value2 != 0x1) {
			DWT_PRINT("Usage is: dp_mic [1/0]");
			return -1;
		}
		dp_mic_en(value2);
	} else if (strcmp(argv[0], "dp_handset") == 0x0) {
		VERIFY_ARGS("dp_handset",2);
		value2 = GetHex(argv[1]);
		if (value2 != 0x0 && value2 != 0x1) {
			DWT_PRINT("Usage is: dp_mic [1/0]");
			return -1;
		}
		dp_handset_en(value2);
	} else if (strcmp(argv[0], "dp_mic_pwr") == 0x0) {
		VERIFY_ARGS("dp_mic_pwr",2);
		value2 = GetHex(argv[1]);
		if (value2 != 0x0 && value2 != 0x1) {
			DWT_PRINT("Usage is: dp_mic_pwr [1/0]");
			return -1;
		}
		dp_micpwr_en(value2);
	} else if (strcmp(argv[0], "dp_line") == 0x0) {
		VERIFY_ARGS("dp_line",2);
		value2 = GetHex(argv[1]);
		if (value2 != 0x0 && value2 != 0x1) {
			DWT_PRINT("Usage is: dp_line [1/0]");
			return -1;
		}
		dp_line_en(value2);
	} else if (strcmp(argv[0], "dp_mic_gain") == 0x0) {
		VERIFY_ARGS("dp_mic_gain",2);
		value2 = GetHex(argv[1]);
		dp_mic_gain(value2);
	} else if (strcmp(argv[0], "dp_handset_gain") == 0x0) {
		VERIFY_ARGS("dp_handset_gain",2);
		value2 = GetHex(argv[1]);
		dp_handset_gain(value2);
	}
	    else if (strcmp(argv[0], "dp_aux_mux") == 0x0) {
		VERIFY_ARGS("dp_aux_mux",2);
		if (strcmp(argv[1], "viled") == 0x0) {
			dp_mux(VILED);
		} else if (strcmp(argv[1], "vcc_div2") == 0x0) {
			dp_mux(VCC_DIV2);
		} else if (strcmp(argv[1], "vibat") == 0x0) {
			dp_mux(VIBAT);
		} else if (strcmp(argv[1], "vtemp0") == 0x0) {
			dp_mux(VTEMP0);
		} else if (strcmp(argv[1], "vtemp1") == 0x0) {
			dp_mux(VTEMP1);
		} else if (strcmp(argv[1], "vtemp2") == 0x0) {
			dp_mux(VTEMP2);
		} else if (strcmp(argv[1], "vbgp") == 0x0) {
			dp_mux(VBGP);
		} else if (strcmp(argv[1], "dcin0") == 0x0) {
			dp_mux(DCIN0);
		} else if (strcmp(argv[1], "dcin0_div2") == 0x0) {
			dp_mux(DCIN0_DIV2);
		} else if (strcmp(argv[1], "dcin1") == 0x0) {
			dp_mux(DCIN1);
		} else if (strcmp(argv[1], "dcin1_div2") == 0x0) {
			dp_mux(DCIN1_DIV2);
		} else if (strcmp(argv[1], "dcin2") == 0x0) {
			dp_mux(DCIN2);
		} else if (strcmp(argv[1], "dcin2_div2") == 0x0) {
			dp_mux(DCIN2_DIV2);
		} else if (strcmp(argv[1], "apdn_dcin3") == 0x0) {
			dp_mux(APDN_DCIN3);
		} else if (strcmp(argv[1], "apdn_dcin3_div2") == 0x0) {
			dp_mux(APDN_DCIN3_DIV2);
		} else if (strcmp(argv[1], "led_atten") == 0x0) {
			dp_mux(LED_ATTEN);
		} else if (strcmp(argv[1], "vdd") == 0x0) {
			dp_mux(VDD);
		} else if (strcmp(argv[1], "vibat_atten") == 0x0) {
			dp_mux(VIBAT_ATTEN);
		} else {
			DWT_PRINT("Format should be: dp_mux [viled, vcc_div2, vibat, vtemp0/1/2, vbgp,dcin0/_div2, dcin1/_div2, dcin2/_div2, apdn_dcin3/_div2, led_atten, vdd, vibat_atten\n");
		}
	} else if (strcmp(argv[0], "dp_aux_sample") == 0x0) {
		dp_aux_sample();
	} else if (strcmp(argv[0], "dp_aux_clk") == 0x0) {
		VERIFY_ARGS("dp_aux_clk",2);
		if (strcmp(argv[1], "1m") == 0x0) {
			dp_aux_clk(CLOCK_1M);
		} else if (strcmp(argv[1], "4m") == 0x0) {
			dp_aux_clk(CLOCK_4M);
		} else if (strcmp(argv[1], "100k") == 0x0) {
			dp_aux_clk(CLOCK_100K);
		} else {
			dp_aux_clk(CLOCK_4M);
		}
	}
	else if (strcmp(argv[0], "dp_rtc_get_ticks") == 0x0)
	{
		VERIFY_ARGS("dp_rtc_get_ticks",2);
		value1 = dp_rtc_get_ticks();
		DWT_PRINT("Number of ticks 0x%x  number of seconds %d\n",value1,value1/0x8000);
	}
	else if (strcmp(argv[0], "dp_rtc_set_alarm") == 0x0)
	{
		VERIFY_ARGS("dp_rtc_set_alarm",2);
		if (strcmp (argv[1],"-s")==0)
		{
			VERIFY_ARGS("dp_rtc_set_alarm -s",3);
			value1 = atoi(argv[2]);
			value1 = value1 * 0x8000;
		}
		else
		{
			value1 = atoi(argv[1]);
		}
		if (value1 == 0 )
		{
			DWT_PRINT("Alarm should be set to value greater then 0\r\n");
			return -1;
		}
		dp_rtc_set_alarm(value1);
	}
	else if (strcmp(argv[0],"dp_hq_set")==0x0)	
	{
		VERIFY_ARGS("dp_hq_set",2);
		value1 = atoi(argv[1]);
		dp_hq_set(value1);
	}
	else if (strcmp(argv[0], "dp_rtc_activate") == 0x0)
	{
		dp_rtc_activate();
	}
	else if (strcmp(argv[0], "dp_rtc_help") == 0x0)
	{
		DWT_PRINT("DRM_RTC_5: Address 0x6c\r\n");
		DWT_PRINT("15 SAMPLE,  13 ALRMSTAT,  12 ALRMINT_EN,  11 ALRMON_EN,  10-0 ALRM_10_0\r\n");
	}
	else if (strcmp(argv[0],"dp_help")==0x0)	
	{

		DWT_PRINT("Clock and PLL                          00-07\n");
		DWT_PRINT("Codecs I/F                             08-0F\n");
		DWT_PRINT("SPI                                    10-17\n");
		DWT_PRINT("ICU                                    18-1F\n");
		DWT_PRINT("SIM IF Register                        20-23\n");
		DWT_PRINT("D-Class Registers                      23-27\n");
		DWT_PRINT("Analog Front End (AFE)                 28-2F\n");
		DWT_PRINT("LQ Σ∆ CODECs Configuration and Testing 30-37\n");
		DWT_PRINT("HQ Σ∆ CODECs Configuration and Testing 38-3F\n");
		DWT_PRINT("AUX ADC Registers                      40-5F\n");
		DWT_PRINT("PWM Generators and Charge Timer        60-67\n");
		DWT_PRINT("DRM RTC                                68-6F\n");
		DWT_PRINT("PMU Registers                          70-7F\n");
	}
	else if (strcmp(argv[0],"dp_help_irq")==0x0)	
	{	
		DWT_PRINT("INT_MASK addr: 0x18, INT_STAT addr: 0x19\n");
		DWT_PRINT("11      TP_RDY_EN\n");
		DWT_PRINT("10      PMU_SEQ_EN\n");
		DWT_PRINT("9       SD_FAIL1_EN\n");
		DWT_PRINT("8       SD_FAIL0_EN\n");
		DWT_PRINT("7       CODIF_EN\n");
		DWT_PRINT("6       CHRGTMR _EN\n");
		DWT_PRINT("5       LQSD _EN\n");
		DWT_PRINT("4       RTC _EN\n");
		DWT_PRINT("3       AUXCOMP _EN\n");
		DWT_PRINT("2       AUXADC _EN\n");
		DWT_PRINT("1       PDN_EN\n");
		DWT_PRINT("0       PMUINT_EN\n");
	}
	else if (strcmp(argv[0],"dp_help_irq_pmu")==0x0)	
	{
		DWT_PRINT("PMUSTAT addr: 0x75, PMUINTMSK addr: 0x76, PMURAWSTAT: 0x77\n");
		DWT_PRINT("15      BAKFAIL\n");
		DWT_PRINT("14      DPFAIL\n");
		DWT_PRINT("13      DWFAIL\n");
		DWT_PRINT("10      MEMFAIL\n");
		DWT_PRINT("9       SP1FAIL\n");
		DWT_PRINT("8       DWIOFAIL\n");
		DWT_PRINT("7       RFFAIL\n");
		DWT_PRINT("6       DPIOFAIL\n");
		DWT_PRINT("5       SP2FAIL\n");
		DWT_PRINT("4       SP3FAIL\n");
		DWT_PRINT("3       POR\n");
		DWT_PRINT("2       OFF_DIBSTAT\n");
		DWT_PRINT("1       DCINS\n");
		DWT_PRINT("0       ON_OFF_z\n");
	}
	else if (strcmp(argv[0],"dp_help_irq_aux")==0x0)	
	{
		DWT_PRINT("AUXINTMSKN addr: 0x45, AUXINTPOL addr: 0x46, AUXINTSTAT addr: 0x47\n");
		DWT_PRINT("6 TPINTEN\n");
		DWT_PRINT("5HSDETINT\n");
		DWT_PRINT("3 DCIN3INTEN\n");
		DWT_PRINT("2 DCIN2INTEN\n");
		DWT_PRINT("1 DCIN1INTEN\n");
		DWT_PRINT("0 DCIN0INTEN\n");
	}

	
#endif


	else if (strcmp(argv[0],"wm8976_init")==0x0)
	{


	}
	else if (strcmp(argv[0], "acc_write") == 0)
	{
		VERIFY_ARGS("acc_write",3);
		value2 = GetHex(argv[1]);
		value3 = GetHex(argv[2]);
		hdmi_write(value2, value3);
	} 
	else if (strcmp(argv[0], "acc_read") == 0)
	 {
		VERIFY_ARGS("acc_read",2);
		value2 = GetHex(argv[1]);
		hdmi_read(value2);
	}

	else if (strcmp(argv[0], "hdmi_write") == 0)
	{
		VERIFY_ARGS("hdmi_write",3);
		value2 = GetHex(argv[1]);
		value3 = GetHex(argv[2]);
		hdmi_write(value2, value3);
	} 
	else if (strcmp(argv[0], "hdmi_read") == 0)
 	{
		VERIFY_ARGS("hdmi_read",2);
		value2 = GetHex(argv[1]);
		hdmi_read(value2);
	}
	else if (strcmp(argv[0], "hdmi_reset") == 0)
	 {
			hdmi_reset();
	}
	else if (strcmp(argv[0], "hdmi_powerup") == 0)
	 {
			ADV7525_powerup();
	}

	else if (strcmp(argv[0], "i2c_enable") == 0)
	{
		VERIFY_ARGS("i2c_enable",2);
		value2 = GetHex(argv[1]);
		i2c_enable(value2);
	} else if (strcmp(argv[0], "i2c_write") == 0)
	{
		VERIFY_ARGS("i2c_write",3);
		value2 = GetHex(argv[1]);
		value3 = GetHex(argv[2]);
		i2c_write(value3, value2);
	}
	else if (strcmp(argv[0], "i2c_read") == 0)
	{
		VERIFY_ARGS("i2c_read",2);
		value2 = GetHex(argv[1]);
		i2c_read(value2);
	}
	else if (strcmp(argv[0], "i2c_status") == 0)
	{
		VERIFY_ARGS("i2c_status",2);
		value2 = GetHex(argv[1]);
		i2c_status(value2);
	}
	else if (strcmp(argv[0], "i2c_stop") == 0)
	{
		VERIFY_ARGS("i2c_stop",2);
		value2 = GetHex(argv[1]);
		i2c_stop(value2);
	}
	else if (strcmp(argv[0],"dpw")==0)
	{
		VERIFY_ARGS("dpw",3);
		value1 = GetHex(argv[1]);
		value2 = GetHex(argv[2]);
		dp_write_reg (value1 ,value2);
		DWT_PRINT(" \r\n");
	}
	else
	if (strcmp(argv[0],"dpr")==0)
	{
		VERIFY_ARGS("dpr",2);
		value1 = GetHex(argv[1]);
		value1 = dp_read_reg(value1);

		DWT_PRINT("0x%x",value1);
		DWT_PRINT(" \r\n");
	}
	else
	if (strcmp(argv[0],"spi_init")==0)
	{
	}

	else if (strcmp(argv[0], "i2c_scan") == 0)
	{
		VERIFY_ARGS("i2c_scan",2);
		value2 = GetHex(argv[1]);
		i2c_scan(value2);
	}
	else if (strcmp(argv[0], "test_keypad") == 0)
	{
		test_keypad();
	}
	

	else if (strcmp(argv[0], "wolfson_wr") == 0)
	{
		VERIFY_ARGS("wolfson_wr",3);
		value2 = GetHex(argv[1]);
		value3 = GetHex(argv[2]);
		wolfson_wr(value2, value3);
	}

	else if (strcmp(argv[0],"sd_writeblock")==0x0)	
	{
		VERIFY_ARGS("sd_writeblock",2);
		value2 = GetHex(argv[1]); //num block
	}


	else if (strcmp(argv[0],"ssd_dump")==0x0)	
	{
		VERIFY_ARGS("ssd_dump",3);
		value2 = GetHex(argv[1]);
		value3 = GetHex(argv[2]);
		lcd_reg_dump(value2,value3);
	}
	else if (strcmp(argv[0],"dw74_help_cmu")==0x0)	
	{
		DWT_PRINT("Kaka\n\n");

		dw74_help_cmu();		
		DWT_PRINT("data");
		DWT_PRINT("Kaka");
	}
	else if (strcmp(argv[0],"dw74_grep_reg")==0x0)
	{
		VERIFY_ARGS("dw74_grep_reg",2);
		dw74_grep_reg(argv[1]);
	}
	else if (strcmp(argv[0],"tdm_en")==0x0)
	{
		mem_clrbits (AGPEN, 0x780020);
	}
	else if (strcmp(argv[0],"spi_en")==0x0)
	{
		mem_clrbits (AGPEN, 0x9800000);
	}

	else if (strcmp(argv[0],"dw74_grep")==0x0)
	{
		VERIFY_ARGS("dw74_grep",2);
		dw74_grep(argv[1]);
	}
	else {
		DWT_PRINT("Unknown command\n");
	}
	return 0;
}

