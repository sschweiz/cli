#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "clibase.h"

#if _USE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>


void cli_archive_entry(
   cli_ctx *ctx,
   const char *tmp,
   struct archive *a,
   struct archive_entry *e)
{
	struct stat s;
	int fd, len;
   static char path[CLI_DEFAULT_BUFFER];
	static char buffer[CLI_MAX_BUFFER];

   memset(path, 0, CLI_DEFAULT_BUFFER);
   sprintf(path, "%s/%s", ctx->pwd, tmp);

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
	cli_archive_entry(ctx, tmp, a, e);

	for (i = 0; i < CLI_DEFAULT_BUFFER; i++) {
		if (ctx->ifs[i] != NULL) {
			// if#-offset
			memset(tmp, 0, CLI_DEFAULT_BUFFER);
			sprintf(tmp, "%08x/if%d-offset", ctx->pid, ctx->ifs[i]->id);
			cli_archive_entry(ctx, tmp, a, e);

			// if#-buffer
			memset(tmp, 0, CLI_DEFAULT_BUFFER);
			sprintf(tmp, "%08x/if%d-buffer", ctx->pid, ctx->ifs[i]->id);
			cli_archive_entry(ctx, tmp, a, e);
		}
	}
	
	archive_entry_free(e);

	archive_write_close(a);
	archive_write_finish(a);
}

void cli_archive_write(cli_ctx *ctx, const char *loadfile)
{
   struct archive *a, *ext;
   struct archive_entry *e;
   int flags;
   int r;

   flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
      ARCHIVE_EXTRACE_ACL | ARCHIVE_EXTRACE_FFLAGS;

   a = archive_read_new();
   archive_read_support_formal_tar(a);
   archive_read_support_compresion_gzip(a);
   
   ext = archive_write_disk_new();
   archive_write_disk_set_options(ext, flags);
   archive_write_disk_set_standard_lookup(ext);

   if (!(ra = archive_read_open_filename(a, loadfile, 16384))) {

   }
}
#endif

