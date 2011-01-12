#pragma once

#include "clibase.h"

void cli_ctx_init(cli_ctx *ctx);
void cli_ctx_exit(cli_ctx *ctx);

void cli_ctx_display_info();
void cli_write_ctx(cli_ctx *ctx);

int cli_interpret(cli_ctx *ctx);
void cli_interpret_if(cli_ctx *ctx);
void cli_interpret_ip(cli_ctx *ctx);

int find_free_if_spot(cli_ctx *ctx);
int cli_strlen(const char *buffer, int cursize);
int cli_stripchars(cli_ctx *ctx);

void cli_print_type(cli_if_type type);
void cli_print_typel(cli_if_type type);
void cli_print_mode(cli_if_mode mode);
void cli_print_model(cli_if_mode mode);

void cli_print_error(const char *caller);

void cli_print_format_mode(cli_if_mode mode, const char *buffer, size_t len);
char *cli_format(cli_if_mode mode, char byte, int *size);

void cli_handle_rx(cli_ctx *ctx, cli_if *iface, char *buffer);
void *cli_rx_interrupt(void *pvctx);

void cli_if_rx(cli_ctx *ctx, const char *buffer);
void cli_if_tx(cli_ctx *ctx, cli_if *iface, char *buffer);

void cli_ctx_reload(cli_ctx *ctx, const char *ctxfile);
void cli_ctx_reload_iface(cli_ctx *ctx, const char *ifacefile);

