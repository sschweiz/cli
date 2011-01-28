/*
 * cli.c - main functions and control routines for cli
 * (C) 2010-2011 Stephen Schweizer <schweizer@alumni.cmu.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <curses.h>
#include <pthread.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/errno.h>
#include <signal.h>

#include <pwd.h>
#include <dirent.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "config.h"

#include "clibase.h"
#include "cliui.h"
#include "cli.h"
#include "cli_cmd.h"
#include "cli_wrapper.h"

int find_free_if_spot(cli_ctx *ctx)
{
	int i;
	for (i = 0; i < CLI_DEFAULT_BUFFER; i++) {
		if (ctx->ifs[i] == NULL) { break; }
	}

	return i;
}

void cli_print_type(cli_if_type type)
{
	switch (type) {
	case CLI_TYPE_TCP: printw("tcp"); break;
	case CLI_TYPE_UDP: printw("udp"); break;
	case CLI_TYPE_EXEC: printw("bin"); break;
	case CLI_TYPE_MEMORY: printw("mem"); break;
	case CLI_TYPE_FILE: printw("fp "); break;
	case CLI_TYPE_SERIAL: printw("ser"); break;
	default: printw("gen"); break;
	}
}

void cli_print_typel(cli_if_type type)
{
	switch (type) {
	case CLI_TYPE_TCP: printw("tcp    "); break;
	case CLI_TYPE_UDP: printw("udp	   "); break;
	case CLI_TYPE_EXEC: printw("binary "); break;
	case CLI_TYPE_MEMORY: printw("memory "); break;
	case CLI_TYPE_FILE: printw("file   "); break;
	case CLI_TYPE_SERIAL: printw("serial "); break;
	default: printw("generic"); break;
	}
}

void cli_print_mode(cli_if_mode mode)
{
	switch (mode) {
	case CLI_MODE_PLAINTEXT: printw("a"); break;
	case CLI_MODE_HEX: printw("x"); break;
	case CLI_MODE_OCTAL: printw("o"); break;
	case CLI_MODE_BINARY: printw("b"); break;
	case CLI_MODE_Z: printw("z"); break;
	}
}

void cli_print_model(cli_if_mode mode)
{
	switch (mode) {
	case CLI_MODE_PLAINTEXT: printw("plaintext"); break;
	case CLI_MODE_HEX: printw("hex"); break;
	case CLI_MODE_OCTAL: printw("octal"); break;
	case CLI_MODE_BINARY: printw("binary"); break;
	case CLI_MODE_Z: printw("zlib"); break;
	}
}

void cli_print_format_mode(cli_if_mode mode, const char *buffer, size_t len)
{
	int i;

	switch (mode) {
	case CLI_MODE_PLAINTEXT:
		for (i = 0; i < len; i++) {
			if ((isprint(buffer[i])) || (buffer[i] == '\n')) addch(buffer[i]);
		}
		break;
	case CLI_MODE_Z:
	case CLI_MODE_HEX:
		for (i = 0; i < len; i++) {
			printw("%02x ", buffer[i]);
			if (((i + 1) % 24) == 0) {
				printw("\n");
			}
		}
		break;
	case CLI_MODE_OCTAL:
		break;
	case CLI_MODE_BINARY:
		break;
	}

// fflush(stdout);
	refresh();
}

char *cli_format(cli_if_mode mode, char byte, int *size)
{
	static char ret[8];
	memset(ret, 0, 8);

	*size = 1;

	switch (mode) {
	case CLI_MODE_PLAINTEXT:
		ret[0] = byte;
		break;
	case CLI_MODE_Z:
	case CLI_MODE_HEX:
		sprintf(ret, "%02x", byte);
		*size = 2;
		break;
	case CLI_MODE_OCTAL:
		sprintf(ret, "%03o", byte);
		*size = 3;
		break;
	case CLI_MODE_BINARY:
		ret[0] = '0' + ((byte >> 7) & 0x01);
		ret[0] = '0' + ((byte >> 6) & 0x01);
		ret[0] = '0' + ((byte >> 5) & 0x01);
		ret[0] = '0' + ((byte >> 4) & 0x01);
		ret[0] = '0' + ((byte >> 3) & 0x01);
		ret[0] = '0' + ((byte >> 2) & 0x01);
		ret[0] = '0' + ((byte >> 1) & 0x01);
		ret[0] = '0' + (byte & 0x01);
		*size = 8;
		break;
	}

	return ret;
}

void cli_print_error(const char *caller)
{
	char *sz = strerror(errno);

	if (sz == NULL) {
		printw("Error: %s: An unknown error occured.\n", caller);
	} else {
		printw("Error: %s: %s.\n", caller, sz);
	}
}

void cli_cmd_add(cli_ctx *ctx)
{
	int i;
	char fname[6] = "stdout";
	char tmp[CLI_DEFAULT_BUFFER];

	memset(tmp, 0, CLI_DEFAULT_BUFFER);


	cli_if *iface = (cli_if *)malloc(sizeof(cli_if));
	iface->header = 'i';
	iface->active = 1;
	iface->buffer_size = CLI_DEFAULT_BUFFER;
	iface->rxdev.fp = stdout;
	iface->type = CLI_TYPE_FILE;
	iface->flags = 0;
	iface->rx = 0;
	iface->rx_last = 0;
	iface->rx_count = 0;
	iface->rx_bufferpos = 0;
	iface->rx_size = 0;
	memset(iface->devname, 0, CLI_DEFAULT_BUFFER);
	memcpy(iface->devname, fname, 6);
	
	if ((i = find_free_if_spot(ctx)) < CLI_DEFAULT_BUFFER) {
		sprintf(tmp, "%s/%08x/if%02x-offset", ctx->pwd, ctx->pid, i);
		iface->offset = fopen(tmp, "wb+");
		// eventually, we will need to make sure that these are unique  in the
		// event we allow moving of ifaces (at present however I don't see a need
		// for this and it adds extra unncessary complexity)
		iface->id = i;
		iface->link = (void *)iface;

		fwrite(iface, 1, sizeof(cli_if), iface->offset);
		// write initial offset
		iface->rx_offsetpos = 0;
		fwrite(&iface->rx_offsetpos, 1, sizeof(unsigned int), iface->offset);

		sprintf(tmp, "%s/%08x/if%02x-buffer", ctx->pwd, ctx->pid, i);
		iface->buffer = fopen(tmp, "ab+");

		// aquire mutex
		pthread_mutex_lock(&ctx->mutex);
		ctx->ifs[i] = iface;
		ctx->ifsel = i;
		// release mutex
		pthread_mutex_unlock(&ctx->mutex);
	} else {
		free(iface);
	}
}

int cli_add_line(cli_ctx *ctx, cli_line *l)
{
	int i, ret = 0;

	if ((ctx->ifs[l->txi] != NULL) &&
		(ctx->ifs[l->rxi] != NULL) &&
		(l->rxi != l->txi)) {
		l->rx = ctx->ifs[l->rxi];
		l->tx = ctx->ifs[l->txi];
			
		cli_if *iface = (cli_if *)malloc(sizeof(cli_if));

		if ((i = find_free_if_spot(ctx)) < CLI_DEFAULT_BUFFER) {
			memset(iface, 0, sizeof(cli_if));
			memcpy(iface, l, sizeof(cli_line));

			// aquire mutex
			pthread_mutex_lock(&ctx->mutex);
			ctx->ifs[i] = iface;
			// release mutex
			pthread_mutex_unlock(&ctx->mutex);
			
			ret = 1;
		} else {
			free(iface);
		}
	}
	
	return ret;
}

void cli_cmd_ex(cli_ctx *ctx)
{
	int pos = 2;
	
	cli_line tie;
	tie.header = 'e';
	
	if (sscanf(ctx->buffer + pos, "%d %d", &tie.txi, &tie.rxi) < 2) {
		printw("Error: malformed `ex' command.\n");
	} else {
		if (cli_add_line(ctx, &tie) == 0) {
			printw("Error: `ex' command must specify valid rx and tx interfaces.\n");
		}
	}
}

void cli_cmd_tie(cli_ctx *ctx)
{
	int pos = 3;

	cli_line tie;
	tie.header = 't';

	if (sscanf(ctx->buffer + pos, "%d %d", &tie.txi, &tie.rxi) < 2) {
		printw("Error: malformed `tie' command.\n");
	} else {
		if (cli_add_line(ctx, &tie) == 0) {
			printw("Error: `tie' command must specify valid rx and tx interfaces.\n");
		}
	}
}

void cli_handle_rx(cli_ctx *ctx, cli_if *iface, char *buffer)
{
	int i;
	
	// add to offset file
	fseek(iface->offset, 0, SEEK_END);
	iface->rx_offsetpos += iface->read_size;
	fwrite(&iface->rx_offsetpos, 1, sizeof(unsigned int), iface->offset);
	
	// add to buffer file
	fwrite(buffer, 1, iface->read_size, iface->buffer);
	iface->rx_count++;	
	iface->rx_size += iface->read_size;

	// perform interface specific actions
	if ((iface->flags & CLI_FLAG_ASYNC) && (buffer)) {
		addch('\n');
		cli_print_format_mode(iface->rxmode, buffer, iface->read_size);
		refresh();
	}
	
	// update exchange line (if we have one selected and it applies to us)
	i = ctx->ifsel;
	if ((ctx->ifs[i] != NULL) && (ctx->ifs[i]->header == 'e')) {
		cli_line *t = (cli_line *)ctx->ifs[i];

		if (t->rx == iface) {
			cli_if_tx(ctx, t->tx, buffer);
		}
	}
}

void *cli_rx_interrupt(void *pvctx)
{
	cli_ctx *ctx = (cli_ctx *)pvctx;
	
	int fdmax;
	int i, ret;

	fd_set rxset;
	static char rx_buffer[CLI_MAX_BUFFER];
	struct timeval t;

	while (ctx->state == CLI_NORMAL) {
		fdmax = 0;
		FD_ZERO(&rxset);

		memset(&t, 0, sizeof(struct timeval));

		// aquire mutex
		pthread_mutex_lock(&ctx->mutex);
		for (i = 0; i < CLI_DEFAULT_BUFFER; i++) {
			if ((ctx->ifs[i] != NULL) && (ctx->ifs[i]->type & CLI_FD_TYPES) &&
				(ctx->ifs[i]->active != 0)) {
				FD_SET(ctx->ifs[i]->rxdev.fd, &rxset);
				if (ctx->ifs[i]->rxdev.fd > fdmax) {
					fdmax = ctx->ifs[i]->rxdev.fd;
				}
			}
		}
		// release mutex
		pthread_mutex_unlock(&ctx->mutex);

		ret = select(fdmax + 1, &rxset, NULL, NULL, &t);

		if (ret == -1) {
			perror("cli_rx_interrupt: ");
		} else if (ret) {
			for (i = 0; i < CLI_DEFAULT_BUFFER; i++) {
				if ((ctx->ifs[i] != NULL) &&
					(ctx->ifs[i]->type & CLI_FD_TYPES) &&
					(ctx->ifs[i]->active != 0) &&
					(FD_ISSET(ctx->ifs[i]->rxdev.fd, &rxset))) {
					// read the socket
					ret = read(ctx->ifs[i]->rxdev.fd, rx_buffer, ctx->ifs[i]->buffer_size);

					if (ret > 0) {
						pthread_mutex_lock(&ctx->ui.mutex);
						pthread_mutex_lock(&ctx->mutex);

						// update interrupt counter only if we are planning to redraw
						// the screen
						if (ctx->ifs[i]->flags & CLI_FLAG_ASYNC) { ctx->ui.irq++; }
						
						// handle rx
						ctx->ifs[i]->read_size = ret;
						cli_handle_rx(ctx, ctx->ifs[i], rx_buffer);
						
						pthread_mutex_unlock(&ctx->mutex);
						pthread_mutex_unlock(&ctx->ui.mutex);
					}
				}
			}
		}
	}

	pthread_exit(NULL);
}

void cli_cmd_cd(cli_ctx *ctx)
{
	int i, pos = 0;
	pos += 2;

	if (sscanf(ctx->buffer + pos, "%d", &i) < 1) {
		printw("Error: `cd' must specify a valid interface.\n");
	} else {
		if (ctx->ifs[i] == NULL) {
			printw("Error: `cd' must specify a valid interface.\n");
		} else {
			if (i != ctx->ifsel) {
				ctx->ifsel = i;
				printw("Selecting iterface %d.\n", i);
			}
		}
	}
}

void cli_rx_modify(cli_if *iface, int newrx)
{
	if (newrx < 0) {
		newrx = 0;
	} else if (newrx > iface->rx_count) {
		newrx = iface->rx_count;
	}

	iface->rx_last = iface->rx;
	iface->rx = newrx;
}

void cli_cmd_rx(cli_ctx *ctx)
{
	int pos = 2;
	int x;
	unsigned int start = 0, size;
	cli_if *iface = ctx->ifs[ctx->ifsel];

	static char rx_buffer[CLI_MAX_BUFFER];
	
	if (iface != NULL) {
		if (iface->header == 't') { iface = ((cli_line *)iface)->rx; }

		if (ctx->buffer[pos] == '?') {
			printw("  %d / %d  %d byte(s)\n",
				iface->rx,
				iface->rx_count,
				iface->rx_size);
		} else if (iface->rx_count > 0) {
			/** try to read an argument:
				rx	  = move to next entry
				rx -	 = move to previous entry
				rx .	 = move to last displayed entry
				rx -x  = move x queue entries backward
				rx +x  = move x queue entries forward
				rx $	 = move to last queue entry
				rx ^	 = move to first queue entry
			 */
			while ((ctx->buffer[pos]) && (ctx->buffer[pos] == ' ')) { pos++; }

			if ((ctx->buffer[pos] != '>') && (iface->rx < iface->rx_count)) {
				memset(rx_buffer, 0, CLI_MAX_BUFFER);
				// if rx is specified with a '>' character, we just move the
				// rx pointer and do not read anything

				// get the offset
				if (iface->rx != 0) {
					fseek(iface->offset,
						sizeof(cli_if) + (iface->rx * sizeof(unsigned int)),
						SEEK_SET);
				} else {
					fseek(iface->offset, sizeof(cli_if), SEEK_SET);
				}
				fread(&start, 1, sizeof(unsigned int), iface->offset);
				fread(&size, 1, sizeof(unsigned int), iface->offset);
				
				// move in and read
				fseek(iface->buffer, start, SEEK_SET);
				size = fread(rx_buffer, 1, size - start, iface->buffer);
				
				// print in formatted mode
				cli_print_format_mode(iface->rxmode, rx_buffer, size);
				addch('\n');
				refresh();
			} else if (ctx->buffer[pos] == '>') { pos++; }
			

			switch (ctx->buffer[pos]) {
			case '-':
				if (sscanf(ctx->buffer + pos + 1, "%d", &x) < 1) {
					x = 1;
				}
				cli_rx_modify(iface, iface->rx - x);
				break;
			case '.': cli_rx_modify(iface, iface->rx_last); break;
			case '+':
				if (sscanf(ctx->buffer + pos + 1, "%d", &x) < 1) {
					x = 1;
				}	  
				cli_rx_modify(iface, iface->rx + x);
				break;
			case '$': iface->rx = iface->rx_count; break;
			case '^': iface->rx = 0; break;
			default:
				cli_rx_modify(iface, iface->rx + 1);
				break;
			}
		} else {
			printw("Error: Nothing to read!\n");
		}
	}
}

void cli_cmd_tx(cli_ctx *ctx)
{
	cli_if *iface = ctx->ifs[ctx->ifsel];

	if (iface != NULL) {
		if (iface->header == 't') { iface = ((cli_line *)iface)->tx; }

		cli_if_tx(ctx, iface, ctx->cmd);
	}
}

int cli_strlen(const char *buffer, int cursize)
{
	int i;
	for (i = cursize; i > 0; i--) {
		if (buffer[i - 1] != 0) break;
	}

	return i;
}

void cli_if_tx(cli_ctx *ctx, cli_if *iface, char *buffer)
{
	char *tmp;
	int s, i;
	int trunc = iface->buffer_size;

	if (iface->header == 'i') {
		if ((iface->flags & CLI_FLAG_AS) &&
			(iface->txmode == CLI_MODE_PLAINTEXT)) {
			trunc = cli_strlen(buffer, iface->buffer_size); 
		}

		if (trunc > (CLI_MAX_BUFFER - 2)) { trunc = (CLI_MAX_BUFFER - 2); }
		if (iface->flags & CLI_FLAG_ACR) { 
			buffer[trunc] = ctx->cr;
			trunc++;
		}
		if (iface->flags & CLI_FLAG_ALF) {
			buffer[trunc] = ctx->lf;
			trunc++;
		}

		switch (iface->type) {
		case CLI_TYPE_FILE:
			// for files, txmode is preserved when writing (as opposed to just
			// being used to decipher the user input)
			for (i = 0; i < trunc; i++) {
				tmp = cli_format(iface->rxmode, buffer[i], &s);
				fwrite(tmp, 1, s, iface->rxdev.fp);
			}

			fflush(iface->rxdev.fp);
			// add to rx queue records immediately
			iface->read_size = trunc;
			cli_handle_rx(ctx, iface, buffer);
			break;
		case CLI_TYPE_TCP:
		case CLI_TYPE_UDP:
		case CLI_TYPE_SERIAL: 
			write(iface->rxdev.fd, buffer, trunc);
			break;
		default:
			break;
		}
	} else {
			
	}
}

void cli_cmd_if(cli_ctx *ctx)
{
	int pos = 2;

	memset(ctx->cmd, 0, CLI_MAX_BUFFER);
	if (sscanf(ctx->buffer + pos, "%s", ctx->cmd) > 0) {
		if (strncmp(ctx->cmd, "set", 3) == 0) {
			cli_cmd_if_set(ctx);
		} else {
			printw("Error: unknown `if' command.\n");
		}
	} else {
		printw("Interface %d status\n", ctx->ifsel);
		// show interface stats
	}
}

void cli_cmd_ip_connect(cli_ctx *ctx)
{
	int ret;
	cli_if *iface = ctx->ifs[ctx->ifsel];

	if (iface != NULL) {
		ret = connect(iface->rxdev.fd,
			(struct sockaddr *)&iface->sock,
			sizeof(struct sockaddr_in));
		if (ret == -1) {
			cli_print_error("cli_connect");
		} else {
			printw("Connected to attached socket.\n");
			iface->active = 1;
		}
	}
}

void cli_cmd_ip_close(cli_ctx *ctx)
{
	cli_if *iface = ctx->ifs[ctx->ifsel];

	if (iface != NULL) {
		if (close(iface->rxdev.fd) == -1) {
			cli_print_error("cli_close");
		}
		iface->active = 0;
	}
}

void cli_ctx_init(cli_ctx *ctx)
{
	int i;
	char tmp[CLI_DEFAULT_BUFFER];
	char tmp2[CLI_DEFAULT_BUFFER];

	memset(tmp, 0, CLI_DEFAULT_BUFFER);
	memset(tmp2, 0, CLI_DEFAULT_BUFFER);

	cli_ui_init(&ctx->ui);
	
	srand(time(NULL));
	
	ctx->state = CLI_NORMAL; 
	ctx->flags = 0;
	ctx->cmd_size = CLI_DEFAULT_BUFFER;
	ctx->pid = ((getpid() & 0xffff) << 16) | (rand() % 0xffff);

	for (i = 0; i < CLI_DEFAULT_BUFFER; i++) {
		ctx->ifs[i] = NULL;
	}

	ctx->uid = geteuid();
	ctx->pw = getpwuid(ctx->uid);
	ctx->version = CLI_VERSION;
	
	// create directories
	memset(ctx->pwd, 0, CLI_DEFAULT_BUFFER);
	sprintf(ctx->pwd, "/tmp/cli-%s", ctx->pw->pw_name);

	mkdir(ctx->pwd, S_IRWXU);

	sprintf(tmp, "%s/%08x", ctx->pwd, ctx->pid);
	mkdir(tmp, S_IRWXU);
	sprintf(tmp2, "%s/latest", ctx->pwd);
	symlink(tmp, tmp2);

	// create history and ctx files
	memset(tmp, 0, CLI_DEFAULT_BUFFER);
	sprintf(tmp, "%s/history", ctx->pwd);
	ctx->ui.history = fopen(tmp, "ab+");
	fseek(ctx->ui.history, 0, SEEK_END);
	ctx->ui.hsize = ftell(ctx->ui.history) / CLI_DEFAULT_BUFFER;
	ctx->ui.hoffset = ctx->ui.hsize;

	memset(tmp, 0, CLI_DEFAULT_BUFFER);
	sprintf(tmp, "%s/ctx", ctx->pwd);
	ctx->context = fopen(tmp, "wb");

	// create threads
	pthread_mutex_init(&ctx->mutex, NULL);
	pthread_create(&ctx->thread, NULL, cli_rx_interrupt, (void *)ctx);
	
	cli_cmd_add(ctx);
	ctx->ifsel = 0;

	ctx->cr = '\r';
	ctx->lf = '\n';

	cli_write_ctx(ctx);
}

void cli_write_ctx(cli_ctx *ctx)
{
	fseek(ctx->context, 0, SEEK_SET);
	fwrite(ctx, 1, sizeof(ctx), ctx->context);
}

void cli_cmd_history(cli_ctx *ctx)
{
	printw("histfile: %s/history\n", ctx->pwd);
	printw(" offset  % 8d\n", ctx->ui.hoffset);
	printw(" records % 8d\n", ctx->ui.hsize);
}

void cli_cmd_ls(cli_ctx *ctx)
{
	int i;
	char tmp[CLI_DEFAULT_BUFFER];
	
	for (i = 0; i < CLI_DEFAULT_BUFFER; i++) {
		if (ctx->ifs[i] != NULL) {
			if (i == ctx->ifsel) { printw("*"); } else { printw(" "); }
			if (ctx->ifs[i]->header == 'i') {
				if (ctx->ifs[i]->active) { printw(" "); } else { printw("#"); }
				printw("%02x", ctx->ifs[i]->flags);
				printw(" % 3d  if  ", i);
				cli_print_typel(ctx->ifs[i]->type);
				printw(" m:");
				cli_print_mode(ctx->ifs[i]->rxmode);
				cli_print_mode(ctx->ifs[i]->txmode);
				switch (ctx->ifs[i]->type) {
				case CLI_TYPE_TCP:
				case CLI_TYPE_UDP:
					memset(tmp, 0, CLI_DEFAULT_BUFFER);
					inet_ntop(AF_INET,
						&ctx->ifs[i]->sock.sin_addr,
						tmp,
						CLI_DEFAULT_BUFFER);
					printw("  %s:%d\n", tmp, ntohs(ctx->ifs[i]->sock.sin_port));
					break;
				case CLI_TYPE_MEMORY:
					printw("  %p\n", ctx->ifs[i]->rxdev.ptr);
					break;
				default:
					printw("  %s\n", ctx->ifs[i]->devname);
					break;
				}
			} else if (ctx->ifs[i]->header == 't') {
				cli_line *t = (cli_line *)ctx->ifs[i];
				printw("    % 3d  tie %d -> %d\n", i, t->txi, t->rxi);
			} else if (ctx->ifs[i]->header == 'e') {
				cli_line *t = (cli_line *)ctx->ifs[i];
				printw("    % 3d  ex  %d -> %d\n", i, t->txi, t->rxi);
			} else if (ctx->ifs[i]->header == 'm') {
				printw("    % 3d  mul\n");
			} else {
				printw("    % 3d  ??\n");
			}
		}
	}
	printw("\n");
}

void cli_cmd_session(cli_ctx *ctx)
{
/*
	struct dirent *dp;
	DIR *dir = opendir(ctx->pwd);
	int i = 0;

	printw("Available sessions:\n");
	while ((i < CLI_DEFAULT_BUFFER) && ((dp = readdir(dir)) != NULL)) {
		if ((dp->d_name[0] != '.') &&
			 (strncmp(dp->d_name, "history", 7) != 0) &&
			 (strncmp(dp->d_name, "latest", 6) != 0) &&
			 (strncmp(dp->d_name, "ctx", 3) != 0)) {
			printw(" [%02x] %s  ", i, dp->d_name);
			if (((i + 1) % 6) == 0) {
				printw("\n");
			}
			i++;
		}
	}
	printw("\n");

	if (i == CLI_DEFAULT_BUFFER) {
		printw("\nThe number of available sessions is more than %d.  It is recommended\n",
			CLI_DEFAULT_BUFFER);
		printw("that you delete some old sessions located in:\n");
		printw("  %s\n", ctx->pwd);
	}

	closedir(dir);
*/
	printw("  sessionid: 0x%08x\n", ctx->pid);
}

void cli_ctx_free_ifaces(cli_ctx *ctx)
{
	int i;
	
	for (i = 0; i < CLI_DEFAULT_BUFFER; i++) {
		if (ctx->ifs[i] != NULL) {
			if (ctx->ifs[i]->header == 'i') {
				ctx->ifs[i]->active = 0;
				fclose(ctx->ifs[i]->offset);
				fclose(ctx->ifs[i]->buffer);

				if (ctx->ifs[i]->rxopen) {
					switch (ctx->ifs[i]->type) {
					case CLI_TYPE_FILE:
						fclose(ctx->ifs[i]->rxdev.fp);
						break;
					default: break;
					}
				}
			}
			
			free(ctx->ifs[i]);
			ctx->ifs[i] = NULL;
		}
	}
}

void cli_ctx_reload(cli_ctx *ctx, const char *ctxfile)
{
	char tmp[CLI_DEFAULT_BUFFER];
	int tf;
	DIR *dir;
	struct dirent *dp;
	cli_if *iface;
	int i = 0;

	memset(tmp, 0, CLI_DEFAULT_BUFFER);
	sprintf(tmp, "%s/%08x", ctx->pwd, ctx->pid);

	// clear ifaces
	printw("freeing old interfaces... "); refresh();
	cli_ctx_free_ifaces(ctx);
	printw("done.\n"); refresh();
	
	printf("reloading interfaces from filesystem... "); refresh();
	dir = opendir(tmp);
	while ((dp = readdir(dir)) != NULL) {
		if (dp->d_name[0] != '.') {
			cli_ctx_reload_iface(ctx, dp->d_name);
		}
	}
	closedir(dir);
	printw("done.\n"); refresh();
	
	printw("restoring settings... "); refresh();
	for (i = 0; i < CLI_DEFAULT_BUFFER; i++) {
		iface = ctx->ifs[i];
		if (iface != NULL) {
			printw("  restoring %d\n", i);
			ctx->ifsel = i;
			sprintf(tmp, "%s/%08x/if%02x-offset", ctx->pwd, ctx->pid, i);
			iface->offset = fopen(tmp, "wb+");
			fseek(iface->offset, 0, SEEK_END);
			
			iface->id = i;
			iface->link = (void *)iface;

			sprintf(tmp, "%s/%08x/if%02x-buffer", ctx->pwd, ctx->pid, i);
			iface->buffer = fopen(tmp, "ab+");

			switch (iface->type) {
			case CLI_TYPE_TCP:
			tf = socket(AF_INET, SOCK_DGRAM, 0);
			if (tf >= 0) {
				iface->rxdev.fd = tf;
					iface->rxopen = 1;
				}
				break;
			default: break;
			}
		}
	}
	printw("done.\n"); refresh();
}

void cli_ctx_reload_iface(cli_ctx *ctx, const char *ifacefile)
{
	int x;
	FILE *fp;
	cli_if *iface;
	char tmp[CLI_DEFAULT_BUFFER];
	
	memset(tmp, 0, CLI_DEFAULT_BUFFER);

	if (sscanf(ifacefile, "if%02x-%s", &x, tmp) == 2) {
		if ((x >= 0) && (x < CLI_DEFAULT_BUFFER) &&
			 (tmp[0] != 'b')) {
			iface = ctx->ifs[x];
			if (iface == NULL) { iface = (cli_if *)malloc(sizeof(cli_if)); }

			sprintf(tmp, "%s/%08x/%s", ctx->pwd, ctx->pid, ifacefile);
			fp = fopen(tmp, "rb");
			rewind(fp);
			fread(iface, 1, sizeof(cli_if), fp);
			iface->rx = 0;
			iface->active = 0;

			ctx->ifs[x] = iface;
		} else if (tmp[0] == 'b') { // nothing
		} else {
			// TODO:
			printw("Error: interface file out-of-bounds at %d.\n", x);
		}
	}
}

int cli_stripchars(cli_ctx *ctx)
{
	int i, j = 0;
	static char swap[256];
	memset(swap, 0, 256);
	
	for (i = 0; i < 256; i++) {
		if (!iscntrl(ctx->buffer[i])) {
			swap[j] = ctx->buffer[i];
			j++;
		}
	}

	memcpy(ctx->buffer, swap, 256);

	return j;
}

void cli_cmd_cwd(cli_ctx *ctx)
{
	char tmp[CLI_DEFAULT_BUFFER];

	getcwd(tmp, CLI_DEFAULT_BUFFER);

	printw("%s\n", tmp);
}

void cli_cmd_save(cli_ctx *ctx)
{
	char buf[13];
	
	cli_write_ctx(ctx);
	fseek(ctx->ifs[ctx->ifsel]->offset, 0, SEEK_SET);
	fwrite(ctx->ifs[ctx->ifsel], 1,
		sizeof(cli_if), ctx->ifs[ctx->ifsel]->offset);
	fseek(ctx->ifs[ctx->ifsel]->offset, 0, SEEK_END);

	memset(buf, 0, 13);
	sprintf(buf, "%08x.tgz", ctx->pid);
#ifdef HAVE_LIBARCHIVE
	cli_archive_write(ctx, buf);
#endif
}

void cli_cmd_load(cli_ctx *ctx)
{
	char tmp[CLI_DEFAULT_BUFFER];
	int pos = 4;

	// eventually, we want to check if the input filename is found, and if it
	// is not, we should check whether a session existed by that name and restore
	// that session.  But for now, just list the contents of the tar
	memset(tmp, 0, CLI_DEFAULT_BUFFER);
	if (sscanf(ctx->buffer + pos, "%s", tmp) > 0) {
#ifdef HAVE_LIBARCHIVE
		cli_archive_read(ctx, tmp);
#endif
	} else {
		printw("Error: `load' command must specify session or filename.\n");
	}
}

void cli_cmd_clear(cli_ctx *ctx)
{
	clear();
	refresh();
}

void cli_cmd_flush(cli_ctx *ctx)
{
	memset(ctx->cmd, 0, CLI_MAX_BUFFER);
	ctx->cmd[0] = ctx->cr;
	ctx->cmd[1] = ctx->lf;

	cli_cmd_tx(ctx);
}

int cli_interpret(cli_ctx *ctx)
{
	int pos = 0;
	int ret = 0, len, i;

	static struct cli_option opts[] = {
		{"ls", cli_cmd_ls, 0, "ls"}, 
		{"cd", cli_cmd_cd, 0, "cd"},
		{"if", cli_cmd_if, 0, "if"},
		{"add", cli_cmd_add, CLI_CMD_UPDATE_CTX, "add"},
		{"save", cli_cmd_save, 0, "save"},
		{"load", cli_cmd_load, 0, "load"},
		{"cwd", cli_cmd_cwd, 0, "cwd"},
		{"history", cli_cmd_history, 0, "history"},
		{"tie", cli_cmd_tie, CLI_CMD_UPDATE_CTX, "tie"},
		{"ex", cli_cmd_ex, CLI_CMD_UPDATE_CTX, "ex"},
		{"sess", cli_cmd_session, 0, "session"},
		{"rx", cli_cmd_rx, 0, "rx"},
		{"flush", cli_cmd_flush, 0, "flush"},
		{"clear", cli_cmd_clear, 0, "clear"},
		{ 0, 0, 0, 0 }
	};
	
	if ((strncmp(ctx->buffer, "exit", 4) == 0) ||
		 (strncmp(ctx->buffer, "quit", 4) == 0) ||
		 (ctx->buffer[0] == 'q')) {
		ctx->state = CLI_EXITING;
		cli_write_ctx(ctx);
	} else if (strncmp(ctx->buffer, "echo", 4) == 0) {
		if (ctx->buffer[4] != '?') {
			ctx->flags ^= CLI_FLAG_ECHO;
		}
		printw("cli command echo is %s\n",
			(ctx->flags & CLI_FLAG_ECHO ? "ON" : "OFF"));
	} else if (strncmp(ctx->buffer, "tx:", 3) == 0) {
		pos += 3;
		memset(ctx->cmd, 0, CLI_MAX_BUFFER);
		memcpy(ctx->cmd, ctx->buffer + pos, CLI_MAX_BUFFER - pos);
		cli_cmd_tx(ctx);
	} else if (strncmp(ctx->buffer, "tx<", 3) == 0) {
		// load a file and send it over tx
	} else if (strncmp(ctx->buffer, "tx", 2) == 0) {
		pos += 2;
		while ((ctx->buffer[pos]) && (ctx->buffer[pos] == ' ')) { pos++; }

		memset(ctx->cmd, 0, CLI_MAX_BUFFER);
		memcpy(ctx->cmd, ctx->buffer + pos, CLI_MAX_BUFFER - pos);
		cli_cmd_tx(ctx);
	} else if (ctx->buffer[0] == 0) {
		// nothing
	} else {
		ret = 1;
		i = 0;

		while (opts[i].name != 0) {
			len = strlen(opts[i].name);
			if (strncmp(ctx->buffer, opts[i].name, len) == 0) {
				opts[i].func(ctx);

				if (opts[i].flags & CLI_CMD_UPDATE_CTX) { cli_write_ctx(ctx); }
				if (opts[i].flags & CLI_CMD_UPDATE_IFACE) { }

				ret = 0;
				break;
			}
			i++;
		}
	}

	return ret;
}

void cli_interpret_ip(cli_ctx *ctx)
{
	static struct cli_option ip_opts[] = {
		{"connect", cli_cmd_ip_connect, 0, "ip_connect"},
		{"close", cli_cmd_ip_close, 0, "ip_close"},
		{0, 0, 0, 0}
	};

	int i = 0;
	int len;

	while (ip_opts[i].name != 0) {
		len = strlen(ip_opts[i].name);
		if (strncmp(ctx->buffer, ip_opts[i].name, len) == 0) {
			ip_opts[i].func(ctx);
			break;
		}

		i++;
	}
}

void cli_interpret_if(cli_ctx *ctx)
{
	cli_if *iface = ctx->ifs[ctx->ifsel];

	if (iface != NULL) {
		if (strncmp(ctx->buffer, "async", 5) == 0) {
			if (ctx->buffer[5] != '?') {
				iface->flags ^= CLI_FLAG_ASYNC;
			}
			printw("cli asynchronous rx is %s\n",
				(iface->flags & CLI_FLAG_ASYNC ? "ON" : "OFF"));
		} else if (strncmp(ctx->buffer, "as", 2) == 0) {
			if (ctx->buffer[2] != '?') {
				iface->flags ^= CLI_FLAG_AS;
			}
			printw("cli auto sizing of plaintext is %s\n",
				(iface->flags & CLI_FLAG_AS ? "ON" : "OFF"));
		} else if (strncmp(ctx->buffer, "alf", 3) == 0) {
			if (ctx->buffer[3] != '?') {
				iface->flags ^= CLI_FLAG_ALF;
			}
			printw("cli auto line-feed is %s\n",
				(iface->flags & CLI_FLAG_ALF ? "ON" : "OFF"));
		} else if (strncmp(ctx->buffer, "acr", 3) == 0) {
			if (ctx->buffer[3] != '?') {
				iface->flags ^= CLI_FLAG_ACR;
			}
			printw("cli auto carriage return is %s\n",
				(iface->flags & CLI_FLAG_ACR ? "ON" : "OFF"));

		} else {
			switch (iface->type) {
			case CLI_TYPE_TCP:
			case CLI_TYPE_UDP:
				cli_interpret_ip(ctx);
				break;
			default: break;
			}
		}
	}
}

void cli_readcmd(cli_ctx *ctx)
{
	printw("cli> ");
	refresh();

	memset(ctx->buffer, 0, CLI_DEFAULT_BUFFER);

	if (cli_ui_capture(&ctx->ui) == 0) {
		memcpy(ctx->buffer, ctx->ui.cmd, CLI_DEFAULT_BUFFER);

		if (ctx->ui.cmd[0] != 0) {
			fwrite(ctx->ui.cmd, 1, CLI_DEFAULT_BUFFER, ctx->ui.history);
			ctx->ui.hsize++;
			ctx->ui.hoffset = ctx->ui.hsize;
		}

		cli_stripchars(ctx);
		if (cli_interpret(ctx) == 1) {
			cli_interpret_if(ctx);
		}

		if (ctx->flags & CLI_FLAG_ECHO) {
			printw("`%s'\n", ctx->cmd);
		}
	} else {
		refresh();
	}
}

void cli_ctx_exit(cli_ctx *ctx)
{
	char tmp[CLI_DEFAULT_BUFFER];
	memset(tmp, 0, CLI_DEFAULT_BUFFER);

	refresh();
	cli_ui_exit(&ctx->ui);

	cli_ctx_free_ifaces(ctx);

	pthread_mutex_destroy(&ctx->mutex);
	pthread_cancel(ctx->thread);
}

void cli_ctx_display_info()
{
	printw("cli - generic device command-line interface\n");
	printw("Copyright (C) 2010-2011 Stephen Schweizer <schweizer@alumni.cmu.edu>\n");
	printw("\nThis program is free software and can be redistributed under the terms\n");
	printw("of the GNU General Public License.  This program comes with ABSOLUTELY\n");
	printw("NO WARRANTY.\n\n");
}

int main(int argc, char **argv)
{
	cli_ctx ctx;
	
	cli_ctx_init(&ctx);
	cli_ctx_display_info();
	
	while (ctx.state == CLI_NORMAL) {
		cli_readcmd(&ctx);
	}

	cli_ctx_exit(&ctx);
	
	return 0;
}

