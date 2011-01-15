#pragma once

#include <stdio.h>
#include "clibase.h"

typedef void (*cli_cmd)(cli_ctx *);

#define CLI_CMD_UPDATE_CTX    0x01
#define CLI_CMD_UPDATE_IFACE  0x02
#define CLI_CMD_ALIAS         0x04

struct cli_option {
	const char *name;
	cli_cmd func;
	unsigned int flags;
	const char *help_file;
};

int cli_command(cli_ctx *ctx, const struct cli_option *opts);

void cli_cmd_add(cli_ctx *ctx);
void cli_cmd_tie(cli_ctx *ctx);
void cli_cmd_cd(cli_ctx *ctx);
void cli_cmd_tx(cli_ctx *ctx);
void cli_cmd_if(cli_ctx *ctx);
void cli_cmd_ls(cli_ctx *ctx);
void cli_cmd_session(cli_ctx *ctx);
void cli_cmd_cwd(cli_ctx *ctx);
void cli_cmd_history(cli_ctx *ctx);
void cli_cmd_save(cli_ctx *ctx);
void cli_cmd_load(cli_ctx *ctx);
void cli_cmd_rx(cli_ctx *ctx);
void cli_cmd_flush(cli_ctx *ctx);

void cli_cmd_if_set(cli_ctx *ctx);
void cli_cmd_if_set_type(cli_ctx *ctx, const char *value);
void cli_cmd_if_set_ipaddr(cli_ctx *ctx, const char *value);
void cli_cmd_if_set_ipport(cli_ctx *ctx, const char *value);
void cli_cmd_if_set_devname(cli_ctx *ctx, const char *value);
void cli_cmd_if_set_xmode(cli_if_mode *mode, const char *value);

void cli_cmd_ip_connect(cli_ctx *ctx);
void cli_cmd_ip_close(cli_ctx *ctx);
// bind, listen, accept not currently supported

