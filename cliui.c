#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cliui.h"
#include "clibase.h"

void cli_ui_init(cli_ui *ui)
{
	initscr();
	keypad(stdscr, true);
	noecho();

	cbreak();

	pthread_mutex_init(&ui->mutex, NULL);

	getmaxyx(stdscr, ui->h, ui->w);
// setscrreg(0, ui->h - 1);

	idlok(stdscr, true);
	scrollok(stdscr, true);

	ui->first = NULL;
	ui->cur = ui->first;
	ui->cpos = 0;
	ui->input_len = 0;
	ui->hwm = 0;
}

void cli_ui_transpose(cli_ui *ui)
{
	int pos = 0;
	struct input_key *s, *cur = ui->first;

	memset(ui->cmd, 0, 256);

	while (cur != NULL) {
		s = cur;
		cur = cur->n;

		ui->cmd[pos] = s->val;
		pos++;

		free(s);
	}

	ui->first = NULL;
	ui->input_len = 0;
	ui->hwm = 0;
	ui->cpos = 0;
}

int cli_ui_handle_keyleft(cli_ui *ui)
{
	int ret = 0;

	if (ui->cur != NULL) {
		ui->cur = ui->cur->p;
		ui->cpos--;
		ret = 1;
	}

	return ret;
}

int cli_ui_handle_keyright(cli_ui *ui)
{
	int ret = 0;

	if (ui->cur == NULL) {
		if (ui->first != NULL) {
			ui->cur = ui->first;
			ui->cpos++;
			ret = 1;
		}
	} else {
		if (ui->cur->n != NULL) {
			ui->cur = ui->cur->n;
			ui->cpos++;
			ret = 1;
		}
	}

	return ret;
}

void cli_ui_replace(cli_ui *ui, const char *cmd)
{
	int i;
	struct input_key *s, *cur = ui->first;

	memset(ui->cmd, 0, CLI_DEFAULT_BUFFER);

	while (cur != NULL) {
		s = cur;
		cur = cur->n;

		free(s);
	}
	ui->first = NULL;
	ui->input_len = 0;

	for (i = 0; i < CLI_DEFAULT_BUFFER; i++) {
		if (isprint(cmd[i])) {
			s = (struct input_key *)malloc(sizeof(struct input_key));
			s->val = cmd[i];
			s->n = NULL;

			if (ui->first == NULL) {
				s->p = NULL;
				ui->first = s;
			} else {
				s->p = cur;
				cur->n = s;
			}
			cur = s;
			ui->input_len++;
		}
	}

	i = 0;
	cur = ui->first;
	while (cur != NULL) {
		i++;
		if (i == ui->input_len) break;

		cur = cur->n;
	}
	ui->cur = cur;
	ui->cpos = ui->input_len;
	
	if (ui->input_len > ui->hwm) { ui->hwm = ui->input_len; }
}

void cli_ui_history_next(cli_ui *ui)
{
	char tmp[CLI_DEFAULT_BUFFER];

	if (ui->hoffset < ui->hsize) {
		ui->hoffset++;
		fseek(ui->history, ui->hoffset * CLI_DEFAULT_BUFFER, SEEK_SET);
		if (fgets(tmp, CLI_DEFAULT_BUFFER, ui->history) != NULL) {
			cli_ui_replace(ui, tmp);	
		}
	}
}

void cli_ui_history_prev(cli_ui *ui)
{
	char tmp[CLI_DEFAULT_BUFFER];

	if (ui->hoffset > 0) {
		ui->hoffset--;
		fseek(ui->history, ui->hoffset * CLI_DEFAULT_BUFFER, SEEK_SET);
		if (fgets(tmp, CLI_DEFAULT_BUFFER, ui->history) != NULL) {
			cli_ui_replace(ui, tmp);	
		}
	}
}

void cli_ui_handle_keypress(cli_ui *ui, int key)
{
	struct input_key *n, *p;

	switch (key) {
	case KEY_BACKSPACE:
		if (ui->cur != NULL) {
			p = ui->cur->p;

			if (p != NULL) {
				p->n = ui->cur->n;
			} else {
				ui->first = ui->cur->n;
			}
			if (ui->cur->n != NULL) { ui->cur->n->p = p; }
			
			free(ui->cur);
			
			ui->cur = p;
			ui->input_len--;
			ui->cpos--;
		}
		break;
	default:
		if (!isprint(key)) break;

		p = (struct input_key *)malloc(sizeof(struct input_key));
		p->val = key;
		p->n = NULL;
		p->p = NULL;

		if (ui->first == NULL) {
			ui->first = p;
			ui->cur = p;
		} else if (ui->cur == NULL) {
			p->n = ui->first;
			ui->first->p = p;
			ui->first = p;
			ui->cur = p;
		} else {
			n = ui->cur->n;

			p->p = ui->cur;
			p->n = ui->cur->n;
			if (n != NULL) { n->p = p; }
			ui->cur->n = p;
			ui->cur = p;
		}
		ui->input_len++;
		ui->cpos++;
		if (ui->input_len > ui->hwm) { ui->hwm = ui->input_len; }
		break;
	case KEY_LEFT:
		cli_ui_handle_keyleft(ui);
		break;
	case KEY_RIGHT:
		cli_ui_handle_keyright(ui);
		break;
	case KEY_END:
		while (cli_ui_handle_keyright(ui)) { }
		break;
	case KEY_HOME:
		while (cli_ui_handle_keyleft(ui)) { }
		break;
	case KEY_UP:
		cli_ui_history_prev(ui);
		break;
	case KEY_DOWN:
		cli_ui_history_next(ui);
		break;
	}

	if ((ui->cur == NULL) && (ui->input_len == 0)) { ui->first = NULL; }
}

void cli_ui_draw_cmd(cli_ui *ui, int x, int y)
{
	struct input_key *cur = ui->first;
	int i;

	move(y, x);
	for (i = 0; i < ui->hwm; i++) {
		addch(' ');
	}
	move(y, x);
	
	while (cur != NULL) {
		addch(cur->val);
		cur = cur->n;
	}
	move(y, x + ui->cpos);

	refresh();
}

void cli_ui_exit(cli_ui *ui)
{
	pthread_mutex_destroy(&ui->mutex);
	
	cli_ui_transpose(ui);

	endwin();
}

int cli_ui_capture(cli_ui *ui)
{
	int c;
	int x, y;
	refresh();

	getyx(stdscr, y, x);

	pthread_mutex_lock(&ui->mutex);
	ui->irq = 0;
	pthread_mutex_unlock(&ui->mutex);

	do {
		c = getch();

		if (c == ERR) continue;

		pthread_mutex_lock(&ui->mutex);
		if (ui->irq) {
			pthread_mutex_unlock(&ui->mutex);
			return 1;
		}

		cli_ui_handle_keypress(ui, c);
		cli_ui_draw_cmd(ui, x, y);

		pthread_mutex_unlock(&ui->mutex);
	} while ((c != '\n') && (c != '\r'));

	cli_ui_transpose(ui);
	addch('\n');

	return 0;
}

#ifdef CLIUI_STANDALONE
int main(int argc, char **argv)
{
	cli_ui ui;
	cli_ui_init(&ui);

	while (1) {
		printw("cli> ");
		cli_ui_capture(&ui);
	}

	cli_ui_exit(&ui);

	return 0;
}
#endif

