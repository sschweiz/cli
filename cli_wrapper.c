#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "cli.h"
#include "clibase.h"

#include <curses.h>

#if _USE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>


void cli_archive_entry(
   cli_ctx *ctx,
   const char *tmp,
   unsigned int corefile,
   struct archive *a,
   struct archive_entry *e)
{
	struct stat s;
	int fd, len;
   static char path[CLI_DEFAULT_BUFFER];
	static char buffer[CLI_MAX_BUFFER];

   memset(path, 0, CLI_DEFAULT_BUFFER);
   if (corefile) {
      sprintf(path, "%s/%s", ctx->pwd, tmp);
   } else {
      sprintf(path, "%s/%08x/%s", ctx->pwd, ctx->pid, tmp);
   }

	stat(path, &s);

	archive_entry_clear(e);

	archive_entry_set_pathname(e, tmp);
	archive_entry_copy_stat(e, &s);
	archive_entry_set_perm(e, 0644);

	archive_write_header(a, e);

	fd = open(path, O_RDONLY);
	len = read(fd, buffer, CLI_MAX_BUFFER);
	while (len > 0) {
		archive_write_data(a, buffer, len);
		len = read(fd, buffer, CLI_MAX_BUFFER);
	}
	close(fd);
}

void cli_archive_write(cli_ctx *ctx, const char *savefile)
{
	struct archive *a;
	struct archive_entry *e;
	char tmp[CLI_DEFAULT_BUFFER];
	int i;

	a = archive_write_new();
	archive_write_set_compression_gzip(a);
	archive_write_set_format_pax_restricted(a);
	archive_write_open_filename(a, savefile);

	e = archive_entry_new();
	
	memset(tmp, 0, CLI_DEFAULT_BUFFER);
	sprintf(tmp, "ctx");
	cli_archive_entry(ctx, tmp, 1, a, e);

	for (i = 0; i < CLI_DEFAULT_BUFFER; i++) {
		if (ctx->ifs[i] != NULL) {
			// if#-offset
			memset(tmp, 0, CLI_DEFAULT_BUFFER);
			sprintf(tmp, "if%02x-offset", ctx->ifs[i]->id);
			cli_archive_entry(ctx, tmp, 0, a, e);

			// if#-buffer
			memset(tmp, 0, CLI_DEFAULT_BUFFER);
			sprintf(tmp, "if%02x-buffer", ctx->ifs[i]->id);
			cli_archive_entry(ctx, tmp, 0, a, e);
		}
	}
	
	archive_entry_free(e);

	archive_write_close(a);
	archive_write_finish(a);
}

int cli_archive_copydata(cli_ctx *ctx, struct archive *ar, struct archive *aw)
{
   int r;
   const void *buf;
   size_t size;
   off_t off;

   while ((r = archive_read_data_block(ar, &buf, &size, &off)) == ARCHIVE_OK) {
      if (archive_write_data_block(aw, buf, size, off) != ARCHIVE_OK) {
         printw("Error: %s\n", archive_error_string(aw));
         break;
      }
   }

   return (r == ARCHIVE_EOF ? ARCHIVE_OK : r);
}

void cli_archive_read(cli_ctx *ctx, const char *loadfile)
{
   struct archive *a, *ext;
   struct archive_entry *e, *ctxe;
   int flags;
   int r;
   char tmp[CLI_DEFAULT_BUFFER];

   flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
      ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;

   a = archive_read_new();
   archive_read_support_format_tar(a);
   archive_read_support_compression_gzip(a);
   
   ext = archive_write_disk_new();
   archive_write_disk_set_options(ext, flags);
   archive_write_disk_set_standard_lookup(ext);

   if (!(r = archive_read_open_filename(a, loadfile, 16384))) {
      while ((r = archive_read_next_header(a, &e)) == ARCHIVE_OK) {
//         printw("%s\n", archive_entry_pathname(e));
         if (strcmp(archive_entry_pathname(e), "ctx") == 0) {
            // context file
            archive_read_data_skip(a);
            ctxe = e;
            printw("updating context... done\n");
         } else {
            printw("updating interface file %s... ", archive_entry_pathname(e));
            refresh();
            
            memset(tmp, 0, CLI_DEFAULT_BUFFER);
            sprintf(tmp, "%s/%08x/%s",
               ctx->pwd, ctx->pid, archive_entry_pathname(e));
            archive_entry_set_pathname(e, tmp);
            
            r = archive_write_header(ext, e);
            if (r == ARCHIVE_OK) {
               cli_archive_copydata(ctx, a, ext);
               if (archive_write_finish_entry(ext) != ARCHIVE_OK) {
                  printw("Error: %s\n", archive_error_string(ext));
               }
            } else { printw("Error: %s\n", archive_error_string(ext)); }
            printw("done.\n"); refresh();
         }
      }
   }

   archive_read_close(a);
   archive_read_finish(a);

   if (r == ARCHIVE_EOF) {
      cli_ctx_reload(ctx, archive_entry_pathname(ctxe));
   } else {
      printw("Error: %s\n", archive_error_string(a));
   }
   
}
#endif

