#pragma once

#include "clibase.h"

void cli_ui_init(cli_ui *ui);
void cli_ui_exit(cli_ui *ui);

int cli_ui_capture(cli_ui *ui);
void cli_ui_transpose(cli_ui *ui);
void cli_ui_draw_cmd(cli_ui *ui, int x, int y);

int cli_ui_handle_keyleft(cli_ui *ui);
int cli_ui_handle_keyright(cli_ui *ui);
void cli_ui_handle_keypress(cli_ui *ui, int key);


