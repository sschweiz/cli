#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// 8 bits major, 8 bits minor, 16 bits build
#define CLI_VERSION  0x00010002

#define CLI_EXITING 0
#define CLI_NORMAL  1

#define CLI_MAX_BUFFER		16384
#define CLI_MIN_BUFFER		8
#define CLI_DEFAULT_BUFFER	256

#define CLI_FLAG_ECHO	0x01
#define CLI_FLAG_ASYNC	0x02
#define CLI_FLAG_SILENT 0x04
#define CLI_FLAG_ALF	0x08
#define CLI_FLAG_ACR	0x10
#define CLI_FLAG_AS		0x20 

typedef enum {
	CLI_TYPE_TCP		= 0x01,
	CLI_TYPE_UDP		= 0x02,
	CLI_TYPE_EXEC		= 0x04,
	CLI_TYPE_MEMORY		= 0x08,
	CLI_TYPE_FILE		= 0x10,
	CLI_TYPE_SERIAL 	= 0x20
	// planned:
	//CLI_TYPE_AUDIO = 0x40
	//CLI_TYPE_PCI = 0x80
	//CLI_TYPE_...
} cli_if_type;

#define CLI_DEVNAME_TYPES	0x34
#define CLI_IP_TYPES		0x03
#define CLI_FD_TYPES		0x23

typedef enum {
	CLI_MODE_PLAINTEXT,
	CLI_MODE_HEX,
	CLI_MODE_OCTAL,
	CLI_MODE_BINARY,
	CLI_MODE_Z
} cli_if_mode;

typedef struct __cli_if
{
	char header;
	int id;
	 
	unsigned int rx;
	unsigned int rx_count;
	unsigned int rx_size;
	unsigned int rx_last;
	unsigned int rx_offsetpos;
	unsigned int rx_bufferpos;
	
	FILE *offset;
	FILE *buffer;

	unsigned int buffer_size;
	unsigned int read_size;

	cli_if_type type;
	cli_if_mode rxmode, txmode;

	void *link;
	int active;
	unsigned int flags;

	union {
		FILE *fp;
		int fd;
		void *ptr;
	} rxdev;
	unsigned int rxopen;

	union {
		struct sockaddr_in sock;
		char devname[CLI_DEFAULT_BUFFER];
	};
} cli_if;

typedef struct __cli_line
{
	char header;
	int id;
	
	cli_if *tx, *rx;
	unsigned int txi, rxi;
} cli_line;

struct input_key {
	char val;
	struct input_key *n, *p;
};

typedef struct __cli_ui {
	struct input_key *first;
	struct input_key *cur;

	int hwm;
	int input_len;
	char cmd[CLI_DEFAULT_BUFFER];
	int cpos;

	int h, w;
	
	FILE *history;
	unsigned long hoffset;
	unsigned int hsize;

	unsigned int irq;
	pthread_mutex_t mutex;
} cli_ui;

typedef struct __cli_ctx
{
	unsigned int state;
	unsigned int flags;
	char buffer[CLI_MAX_BUFFER];
	char cmd[CLI_MAX_BUFFER];

	unsigned long version;

	unsigned int pid;
	uid_t uid;
	struct passwd *pw;

	unsigned int cmd_size;
	unsigned int hlen;

	char pwd[CLI_DEFAULT_BUFFER]; 

	cli_if *ifs[CLI_DEFAULT_BUFFER];
	unsigned int ifsel;
	
	FILE *context;

	int ins;
	char cr, lf;
	pthread_t thread;
	pthread_mutex_t mutex;
	cli_ui ui;
} cli_ctx;

