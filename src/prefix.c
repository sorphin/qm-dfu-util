/*
 * (C) 2011 - 2012 Stefan Schmidt <stefan@datenfreihafen.org>
 * (C) 2013 Hans Petter Selasky <hps@bitfrost.no>
 * (C) 2014 Uwe Bonnes <bon@elektron.ikp.physik.tu-darmstadt.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>

#include "portable.h"
#include "dfu_file.h"

enum mode {
	MODE_NONE,
	MODE_ADD,
	MODE_DEL,
	MODE_CHECK
};

static enum suffix_req dfu_has_prefix;
static uint8_t dfu_want_prefix;

int verbose;

static void help(void)
{
	fprintf(stderr, "Usage: dfu-prefix [options] ...\n"
		"  -h --help\t\t\tPrint this help message\n"
		"  -V --version\t\t\tPrint the version number\n"
		"  -c --check <file>\t\tCheck DFU prefix of <file>\n"
		"  -a --add <file>\t\tAdd DFU prefix to <file>\n"
		"  -D --delete <file>\t\tDelete DFU prefix from <file>\n"
		"  -L --lpc-prefix\t\tAdd device NXP LPC DFU prefix in <file>\n"
		);
	fprintf(stderr, "  -s --stellaris-address <address>  Add TI Stellaris address prefix to <file>,\n\t\t\t\t"
		"to be used in combination with -a\n"
		"  -T --stellaris\t\tAct on TI Stellaris address prefix of <file>, \n\t\t\t\t"
		"to be used in combination with -D or -c\n"
		);
	exit(EX_USAGE);
}

static void print_version(void)
{
	printf("dfu-prefix (%s) %s\n\n", PACKAGE, PACKAGE_VERSION);
	printf("(C) 2014, Uwe Bonnes, 2011-2012 Stefan Schmidt\n"
	       "This program is Free Software and has ABSOLUTELY NO WARRANTY\n"
	       "Please report bugs to %s\n\n", PACKAGE_BUGREPORT);

}

static struct option opts[] = {
	{ "help", 0, 0, 'h' },
	{ "version", 0, 0, 'V' },
	{ "check", 1, 0, 'c' },
	{ "add", 1, 0, 'a' },
	{ "delete", 1, 0, 'D' },
	{ "stellaris-address", 1, 0, 's' },
	{ "stellaris", 0, 0, 'T' },
	{ "LPC", 0, 0, 'L' },
};
int main(int argc, char **argv)
{
	struct dfu_file file;
	enum mode mode = MODE_NONE;
	enum prefix_type type = NO_PREFIX;
	uint32_t lmdfu_flash_address = 0;
	char *end;

	/* make sure all prints are flushed */
	setvbuf(stdout, NULL, _IONBF, 0);

	print_version();

	memset(&file, 0, sizeof(file));

	while (1) {
		int c, option_index = 0;
		c = getopt_long(argc, argv, "hVc:a:D:p:v:d:s:TL", opts,
				&option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			help();
			break;
		case 'V':
			exit(0);
			break;
		case 'D':
			dfu_has_prefix = NEEDS_SUFFIX;
			file.name = optarg;
			mode = MODE_DEL;
			break;
		case 'c':
			dfu_has_prefix = NEEDS_SUFFIX;
			file.name = optarg;
			mode = MODE_CHECK;
			break;
		case 'a':
			dfu_want_prefix = 1;
			file.name = optarg;
			mode = MODE_ADD;
			break;
		case 's':
			dfu_want_prefix = 1;
			lmdfu_flash_address = strtoul(optarg, &end, 0);
			if (*end) {
				errx(EX_IOERR, "Invalid lmdfu "
					"address: %s", optarg);
			}
			break;
		case 'T':
			dfu_has_prefix = MAYBE_SUFFIX;
			type = LMDFU_PREFIX;
			break;
		case 'L':
			dfu_has_prefix = MAYBE_SUFFIX;
			type = LPCDFU_UNENCRYPTED_PREFIX;
			break;
		default:
			help();
			break;
		}
	}

	if (!file.name) {
		fprintf(stderr, "You need to specify a filename\n");
		help();
	}

	switch(mode) {
	case MODE_ADD:
		dfu_load_file(&file, MAYBE_SUFFIX, dfu_has_prefix);
		file.lmdfu_address = lmdfu_flash_address;
		if (file.prefix_type == type) {
			printf("File already has Prefix\n");
			break;
		}
		file.prefix_type = type;
		dfu_store_file(&file, (file.size.suffix)?NEEDS_SUFFIX:NO_SUFFIX, dfu_want_prefix);
		if (dfu_want_prefix)
			printf("Prefix successfully added to file\n");
		break;

	case MODE_CHECK:
		dfu_load_file(&file, MAYBE_SUFFIX, MAYBE_SUFFIX);
		show_suffix_and_prefix(&file);
		break;

	case MODE_DEL:
		dfu_load_file(&file, MAYBE_SUFFIX, dfu_has_prefix);
		dfu_store_file(&file, 0, 0);
		if (dfu_has_prefix)
			printf("Prefix successfully removed from file\n");
		break;

	default:
		help();
		break;
	}
	return (0);
}