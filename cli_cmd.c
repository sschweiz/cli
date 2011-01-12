#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <curses.h>
#include <arpa/inet.h>

#include "clibase.h"
#include "cli.h"
#include "cli_cmd.h"

void cli_cmd_if_set_type(cli_ctx *ctx, const char *value)
{
	cli_if *iface = ctx->ifs[ctx->ifsel];
	int tmp;

	if (iface != NULL) {
		if (strncmp(value, "tcp", 3) == 0) {
			memset(&iface->sock, 0, sizeof(struct sockaddr_in));
			iface->sock.sin_family = AF_INET;
			iface->sock.sin_addr.s_addr = 0x0100007fUL;
			iface->sock.sin_port = htons((short)80);
			tmp = socket(AF_INET, SOCK_STREAM, 0);
			if (tmp < 0) {
				cli_print_error("if set type: ");
				iface->active = 0;
			} else {
				iface->rxdev.fd = tmp;
            iface->rxopen = 1;
			}
			iface->type = CLI_TYPE_TCP;
		} else if (strncmp(value, "udp", 3) == 0) {
			memset(&iface->sock, 0, sizeof(struct sockaddr_in));
			iface->sock.sin_family = AF_INET;
			iface->sock.sin_addr.s_addr = 0x0100007fUL;
			iface->sock.sin_port = htons((short)80);
			tmp = socket(AF_INET, SOCK_DGRAM, 0);
			if (tmp < 0) {
				cli_print_error("if set type: ");
				iface->active = 0;
			} else {
				iface->rxdev.fd = tmp;
            iface->rxopen = 1;
			}
			iface->type = CLI_TYPE_UDP;
		} else if ((strncmp(value, "mem", 3) == 0) ||
				   (strncmp(value, "memory", 6) == 0)) {
			iface->type = CLI_TYPE_MEMORY;
			iface->rxdev.ptr = NULL;
			iface->active = 0;
		} else if (strncmp(value, "file", 3) == 0) {
			iface->type = CLI_TYPE_FILE;
			memset(iface->devname, 0, CLI_DEFAULT_BUFFER);
			sprintf(iface->devname, "stdout");
			iface->rxdev.fp = stdout;
		} else if ((strncmp(value, "bin", 3) == 0) ||
				   (strncmp(value, "exec", 4) == 0)) {
			iface->type = CLI_TYPE_EXEC;
			memset(iface->devname, 0, CLI_DEFAULT_BUFFER);
			sprintf(iface->devname, "/usr/bin/cat");
			iface->active = 0;
		} else if (strncmp(value, "serial", 6) == 0) {
			memset(iface->devname, 0, CLI_DEFAULT_BUFFER);
			sprintf(iface->devname, "/dev/ttyS0");
			iface->active = 0;
			iface->type = CLI_TYPE_SERIAL;
		} else {
			printw("Error: `if set' type `%s' unrecognized.\n", value);
		}
	}
}

void cli_cmd_if_set_buffersize(cli_ctx *ctx, const char *value)
{
   int i;
	cli_if *iface = ctx->ifs[ctx->ifsel];

	if (iface != NULL) {
		i = atoi(value);
      if (i < CLI_MIN_BUFFER) i = CLI_DEFAULT_BUFFER;
      iface->buffer_size = i;
   }
}

void cli_cmd_if_set_ipaddr(cli_ctx *ctx, const char *value)
{
	cli_if *iface = ctx->ifs[ctx->ifsel];

	if (iface != NULL) {
		if (inet_pton(AF_INET, value, &iface->sock.sin_addr) != 1) {
			printw("Error: `if set' could not parse ip address `%s'.\n", value);
		}
	}
}

void cli_cmd_if_set_ipport(cli_ctx *ctx, const char *value)
{
	int i;
	cli_if *iface = ctx->ifs[ctx->ifsel];

	if (iface != NULL) {
		i = atoi(value);
		iface->sock.sin_port = htons((short)i);
	}
}

void cli_cmd_if_set_devname(cli_ctx *ctx, const char *value)
{
	cli_if *iface = ctx->ifs[ctx->ifsel];
	
	if (iface != NULL) {
		switch (iface->type) {
		case CLI_TYPE_FILE:
			if (strncmp(iface->devname, "stdout", 6) != 0) {
				fflush(iface->rxdev.fp);
				fclose(iface->rxdev.fp);
			}
			
			if (strncmp(value, "stdout", 6) == 0) {
				iface->rxdev.fp = stdout;
			} else {
				iface->rxdev.fp = fopen(value, "wb");
			}
			break;
		default:
			break;
		}

		memcpy(iface->devname, value, CLI_DEFAULT_BUFFER);
	}
}

void cli_cmd_if_set_xmode(cli_if_mode *mode, const char *value)
{
	if ((strncmp(value, "zlib", 4) == 0) ||
	 	(value[0] == 'z')) {
		*mode = CLI_MODE_Z;
	} else if ((strncmp(value, "pt", 2) == 0) ||
			   (strncmp(value, "plaintext", 9) == 0) ||
			   (strncmp(value, "ascii", 5) == 0) ||
			   (value[0] == 'a')) {
		*mode = CLI_MODE_PLAINTEXT;
	} else if ((strncmp(value, "hex", 3) == 0) ||
			   (value[0] == 'h') ||
			   (value[0] == 'x')) {
		*mode = CLI_MODE_HEX;
   } else if ((strncmp(value, "binary", 6) == 0) ||
              (value[0] == 'b')) {
		*mode = CLI_MODE_BINARY;
   }
}

void cli_cmd_if_set(cli_ctx *ctx)
{
	int pos = 0, lpos = 0, rpos = 0;
	int seeneq = 0;
	char c;

	char var[CLI_DEFAULT_BUFFER];
	char val[CLI_DEFAULT_BUFFER];
	memset(val, 0, 256);
	memset(var, 0, 256);
  
	while ((ctx->buffer[pos]) && (ctx->buffer[pos] != 't')) { pos++; }
	pos++;

	while ((pos < CLI_MAX_BUFFER) && (ctx->buffer[pos])) {
		c = ctx->buffer[pos];

		if (c == '=') {
			pos++;
			seeneq = 1;
			continue;
		}

		if (c != ' ') {
			if (seeneq) {
				if (lpos > CLI_DEFAULT_BUFFER) break;
				
				val[lpos] = c;
				lpos++;
			} else {
				if (rpos < CLI_DEFAULT_BUFFER) {
					var[rpos] = c;
					rpos++;
				}
			}
			
		}

		pos++;
	}

	if (var[0] == 0) { printw("Error: `if set' must specify variable name.\n"); }
	else if (val[0] == 0) { printw("Error: `if set' must specify value.\n"); }
	else {
		if ((strncmp(var, "devname", 7) == 0) &&
			(ctx->ifs[ctx->ifsel]->type & CLI_DEVNAME_TYPES)) {
			cli_cmd_if_set_devname(ctx, val);
		} else if ((strncmp(var, "ipaddr", 6) == 0) &&
				   (ctx->ifs[ctx->ifsel]->type & CLI_IP_TYPES)) {
			cli_cmd_if_set_ipaddr(ctx, val);
		} else if ((strncmp(var, "ipport", 6) == 0) &&
				   (ctx->ifs[ctx->ifsel]->type & CLI_IP_TYPES)) {
			cli_cmd_if_set_ipport(ctx, val);
		} else if ((strncmp(var, "addr", 4) == 0) &&
				   (ctx->ifs[ctx->ifsel]->type == CLI_TYPE_MEMORY)) {
			//cli_cmd_if_set_addr(ctx, val);
		} else if (strncmp(var, "type", 4) == 0) {
			cli_cmd_if_set_type(ctx, val);
		} else if (strncmp(var, "buffer", 6) == 0) {
         cli_cmd_if_set_buffersize(ctx, val);
		} else if (strncmp(var, "rxmode", 6) == 0) {
			cli_cmd_if_set_xmode(&ctx->ifs[ctx->ifsel]->rxmode, val);
		} else if (strncmp(var, "txmode", 6) == 0) {
			cli_cmd_if_set_xmode(&ctx->ifs[ctx->ifsel]->txmode, val);
		} else {
			printw("Error: `if set' variable `%s' unknown for target interface.\n", var);
		}
	}
	
   fseek(ctx->ifs[ctx->ifsel]->offset, 0, SEEK_SET);
   fwrite(ctx->ifs[ctx->ifsel], 1,
      sizeof(cli_if), ctx->ifs[ctx->ifsel]->offset);
   fseek(ctx->ifs[ctx->ifsel]->offset, 0, SEEK_END);
}

/**
   static struct cli_options opts[] = {
      {"add", cli_cmd_add, 0, "add"},
      {"a", cli_cmd_add, 1, "add"},
      ...
      {0, 0, 0, 0}
   }
 */
int cli_command(cli_ctx *ctx, const struct cli_option *opts)
{
   int i = 0, ret = 0;
   int len;

   while (opts[i].name != 0) {
      len = strlen(opts[i].name);
      if (strncmp(ctx->buffer, opts[i].name, len) == 0) {
         opts[i].func(ctx);
         ret = 1;
         break;
      }

      i++;
   }

   return ret;
}

