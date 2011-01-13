#pragma once

#include "config.h"
#include "clibase.h"

#ifdef HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>

void cli_archive_write(cli_ctx *ctx, const char *savefile);
void cli_archive_read(cli_ctx *ctx, const char *loadfile);
void cli_archive_entry(
   cli_ctx *ctx, const char *tmp, unsigned int corefile,
   struct archive *a, struct archive_entry *e);
#endif

