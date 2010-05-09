/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Author: Jim Faulkner <newspost@sdf.lonestar.org>
 *         and William McBrine <wmcbrine@users.sf.net>
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
 * 
 */

#include <getopt.h>
#include "../base/encode.h"
#include "options.h"
#include "ui.h"

/* Command-line short option keys */
#define help_option 'h'
#define subject_option 's'
#define newsgroup_option 'n'
#define from_option 'f'
#define name_option 'F'
#define organization_option 'o'
#define address_option 'i'
#define port_option 'z'
#define user_option 'u'
#define password_option 'p'
#define threads_option 'N'
#define lines_option 'l'
#define uuenc_option 'U'
#define sfv_option 'c'
#define par_option 'a'
#define parnum_option 'A'
#define filesperpar_option 'B'
#define reference_option 'r'
#define default_option 'd'
#define version_option 'V'
#define filenumber_option 'q'
#define verbose_option 'v'
#define delay_option 'T'
#define noarchive_option 'x'
#define followupto_option 'w'
#define replyto_option 'm'
#define tmpdir_option 'k'
#define disable_option 'D'
#define extraheader_option 'X'
#define text_option 't'

/* Command-line long option keys */
#define help_long_option "help"
#define subject_long_option "subject"
#define newsgroup_long_option "newsgroup"
#define from_long_option "from"
#define name_long_option "name"
#define organization_long_option "organization"
#define address_long_option "host"
#define port_long_option "port"
#define user_long_option "user"
#define password_long_option "password"
#define threads_long_option "threads"
#define lines_long_option "lines"
#define uuenc_long_option "uuenc"
#define sfv_long_option "sfv"
#define par_long_option "par"
#define parnum_long_option "parnum"
#define filesperpar_long_option "files-par"
#define reference_long_option "reference"
#define default_long_option "writedefaults"
#define version_long_option "version"
#define filenumber_long_option "filenumber"
#define verbose_long_option "verbose"
#define delay_long_option "delay"
#define noarchive_long_option "no-archive"
#define followupto_long_option "followupto"
#define replyto_long_option "replyto"
#define tmpdir_long_option "tmpdir"
#define disable_long_option "disable"
#define extraheader_long_option "extraheader"
#define text_long_option "text"

/* Option table for getopt() -- options which take parameters
   are followed by colons */

static const char short_options[] = {
	help_option,
	subject_option, ':',
	newsgroup_option, ':',
	from_option, ':',
	organization_option, ':',
	address_option, ':',
	port_option, ':',
	user_option, ':',
	password_option, ':',
	threads_option, ':',
	lines_option, ':',
	uuenc_option,
	sfv_option, ':',
	par_option, ':',
	parnum_option, ':',
	reference_option, ':',
	default_option,
	version_option,
	filenumber_option,
	delay_option, ':',
	verbose_option,
	noarchive_option,
	followupto_option, ':',
	replyto_option, ':',
	tmpdir_option, ':',
	disable_option, ':',
	filesperpar_option, ':',
	name_option, ':',
	extraheader_option,':',
	text_option,
	'\0'
};

/* long options struct for getopt_long */

static struct option long_options[] =
{
	{ help_long_option,               no_argument, NULL, help_option },
	{ subject_long_option,      required_argument, NULL, subject_option },
	{ newsgroup_long_option,    required_argument, NULL, newsgroup_option },
	{ from_long_option,         required_argument, NULL, from_option },
	{ organization_long_option, required_argument, NULL, organization_option },
	{ address_long_option,      required_argument, NULL, address_option },
	{ port_long_option,         required_argument, NULL, port_option },
	{ user_long_option,         required_argument, NULL, user_option },
	{ password_long_option,     required_argument, NULL, password_option },
	{ threads_long_option,      required_argument, NULL, threads_option },
	{ lines_long_option,        required_argument, NULL, lines_option },
	{ uuenc_long_option,              no_argument, NULL, uuenc_option },
	{ sfv_long_option,          required_argument, NULL, sfv_option },
	{ par_long_option,          required_argument, NULL, par_option },
	{ parnum_long_option,       required_argument, NULL, parnum_option },
	{ reference_long_option,    required_argument, NULL, reference_option },
	{ default_long_option,            no_argument, NULL, default_option },
	{ version_long_option,            no_argument, NULL, version_option },
	{ filenumber_long_option,         no_argument, NULL, filenumber_option },
	{ delay_long_option,        required_argument, NULL, delay_option },
	{ verbose_long_option,            no_argument, NULL, verbose_option },
	{ noarchive_long_option,          no_argument, NULL, noarchive_option },
	{ followupto_long_option,   required_argument, NULL, followupto_option },
	{ replyto_long_option,      required_argument, NULL, replyto_option },
	{ tmpdir_long_option,       required_argument, NULL, tmpdir_option },
	{ disable_long_option,      required_argument, NULL, disable_option },
	{ filesperpar_long_option,  required_argument, NULL, filesperpar_option },
	{ name_long_option,         required_argument, NULL, name_option },
	{ extraheader_long_option,  required_argument, NULL, extraheader_option },
	{ text_long_option,         required_argument, NULL, text_option },
	{ NULL,                           no_argument, NULL, 0 },
};		

/* Symbolic labels for .newspostrc keywords */

enum {
	newsgroup = 0,
	from,
	organization,
	address,
	port,
	user,
	password,
	threads,
	lines,
	uuenc,
	filenumber,
	noarchive,
	followupto,
	replyto,
	tmpdir,
	filesperpar,
	name,
	extraheader,
	number_of_defaults	/* Leave this at the end! */
};

/* The .newspostrc keywords themselves -- must be in the same order */

static const char *rc_keyword[number_of_defaults] = {
	"newsgroup",
	"from",
	"organization",
	"address",
	"port",
	"user",
	"password",
	"threads",
	"lines",
	"uuenc",
	"filenumber",
	"noarchive",
	"followupto",
	"replyto",
	"tmpdir",
	"filesperpar",
	"name",
	"extraheader"
};

/* Comment lines for the .newspostrc -- must be in the same order */

static const char *rc_comment[number_of_defaults] = {
	"newsgroup(s) to post to",
	"your e-mail address",
	"your organization",
	"address of news server",
	"port of news server",
	"username on news server",
	"password on news server",
	"number of threads to use",
	"lines per message",
	"0 to yencode, 1 to uuencode",
	"0 doesn't include filenumber in subject line, 1 does",
	"0 doesn't include X-No-Archive header, 1 does",
	"the Followup-To header",
	"the Reply-To header",
	"directory for storing temporary files",
	"create a par volume for every x files",
	"your full name",
	"any extra headers"
};

static void version_info();
static boolean parse_file_parts(newspost_data *data,
			file_entry *this_file_entry, const char *arg, int i);
static void parse_delay_option(const char *option);


/**
*** Public Routines
**/

SList *parse_input_files(int argc, char **argv, int optind,
		newspost_data *data) {
	FILE *testopen;
	file_entry *this_file_entry;
	SList *file_list = NULL;
	SList *tmplist, *tmplist2;
	file_entry *data1, *data2;
	Buff *tmpbuff = NULL;
	int i;
	boolean thisfilepart = FALSE;

	while (optind < argc) {
		this_file_entry = file_entry_alloc();

		i = strlen(argv[optind]);
		thisfilepart = FALSE;
		while ((i > 0) && (argv[optind][i] != ':'))
			i--;

		if (i < 2)
			this_file_entry->filename = 
				buff_create(this_file_entry->filename, "%s",
					    argv[optind]);
		else {
			/* cut of the colon and partnumbers */
			tmpbuff = buff_create(tmpbuff, "%s", argv[optind]);
			tmpbuff->data[i] = '\0';
			
			this_file_entry->filename = 
				buff_create(this_file_entry->filename, 
					     "%s", tmpbuff->data);
			thisfilepart = TRUE;
			tmpbuff = buff_free(tmpbuff);
		}

		if ( (stat(this_file_entry->filename->data,
			&this_file_entry->fileinfo) == 0) &&
		   (testopen = fopen(this_file_entry->filename->data, "rb")) ) {

			fclose(testopen);

			if (S_ISDIR(this_file_entry->fileinfo.st_mode)) {
				/* it's a directory */
				fprintf(stderr,
					"\nWARNING: %s is a directory"
					" - SKIPPING",
					this_file_entry->filename->data);
				if (data->text == TRUE) {
					printf("\nNothing to post!\n");
					exit(EXIT_NO_FILES);
				}
				/* only free() if we skip it */
				buff_free(this_file_entry->filename);
				free(this_file_entry);
			}
			else {
				/* it's a good file */
				boolean postany = TRUE;

				int numparts = get_number_of_encoded_parts(data, this_file_entry);
				this_file_entry->number_enc_parts = numparts;
				this_file_entry->parts_to_post = numparts;

				/* for file parts */
				if (thisfilepart == TRUE)
					postany = parse_file_parts(data, 
						this_file_entry,
						argv[optind], i);
				else
					this_file_entry->parts = NULL;

				if (postany == TRUE) {
					/* initialize the read/write lock for this file entry */
					this_file_entry->rwlock = (pthread_rwlock_t *) malloc(sizeof(pthread_rwlock_t));
					pthread_rwlock_init(this_file_entry->rwlock, NULL);

					/* add it to the list */
					file_list = slist_append(file_list,
							this_file_entry);
				}
			}
		}
		else {
			fprintf(stderr,
				"\nWARNING: %s %s - SKIPPING",
				strerror(errno), this_file_entry->filename->data);
			if (data->text == TRUE) {
				printf("\nNothing to post!\n");
				exit(EXIT_NO_FILES);
			}
			/* only free() if we skip it */
			file_entry_free(this_file_entry);
		}
		optind++;
	}

	/* make sure there are no duplicate entries */
	tmplist = file_list;
	while (tmplist != NULL) {
		tmplist2 = slist_next(tmplist);
		while (tmplist2 != NULL) {
			data1 = (file_entry *) tmplist->data;
			data2 = (file_entry *) tmplist2->data;
			if (strcmp(data1->filename->data, data2->filename->data) == 0) {
				fprintf(stderr,
					"\nWARNING: duplicate filename"
					" %s - IGNORING",
					data2->filename->data);
				tmplist2 = slist_next(tmplist2);
				/* only free() if we skip it */
				file_entry_free(data2);
				file_list = slist_remove(file_list, data2);
			}
			else
				tmplist2 = slist_next(tmplist2);
		}
		tmplist = slist_next(tmplist);
	}

	return file_list;
}

void parse_environment(newspost_data *data) {
	const char *envopt, *envopt2;

	envopt = getenv("NNTPSERVER");
	if (envopt != NULL)
		data->address = buff_create(data->address, "%s", envopt);

	envopt = getenv("USER");
	if (envopt != NULL) {
		envopt2 = getenv("HOSTNAME");
		if (envopt2 != NULL) {
			data->from = buff_create(data->from, "%s", envopt);
			data->from = buff_add(data->from, "@%s", envopt2);
		}
	}

	envopt = getenv("TMPDIR");
	if (envopt != NULL)
		data->tmpdir = buff_add(data->tmpdir, "%s", envopt);
	else {
		envopt = getenv("TMP");
		if (envopt != NULL)
			data->tmpdir = buff_add(data->tmpdir, "%s", envopt);
	}

	envopt = getenv("EDITOR");
	if (envopt != NULL)
		EDITOR = envopt;
}

void parse_defaults(newspost_data *data) {
	const char *envopt;
	char *setting;
	FILE *file;
	Buff *header = NULL;
	int i, linenum = 0;
	Buff *filename = NULL;
	Buff *line = NULL;

	envopt = getenv("HOME");
	if (envopt != NULL) {
		filename = buff_create(filename, "%s/.newspostrc", envopt);
		file = fopen(filename->data, "r");
		if (file != NULL) {
			while (!feof(file)) {
				line = buff_getline(line, file);
				linenum++;
				if(line == NULL) continue;

				/* ignore comment lines */
				for (i = 0; line->data[i] == ' ' ; i++){}
				if (line->data[i] == '#')
					continue;

				setting = strchr(line->data, '=');

				if (setting != NULL) {
				    *setting++ = '\0';

				    for (i = 0; i < number_of_defaults; i++)
					if (strcmp(line->data, rc_keyword[i]) == 0)
					    break;

				    switch (i) {
				    case newsgroup:
					data->newsgroup = buff_create(data->newsgroup, "%s", setting);
					break;
				    case from:
					data->from = buff_create(data->from, "%s", setting);
					break;
				    case organization:
					data->organization = buff_create(data->organization, "%s", setting);
					break;
				    case address:
					data->address = buff_create(data->address, "%s", setting);
					break;
				    case port:
					data->port = atoi(setting);
					break;
				    case user:
					data->user = buff_create(data->user, "%s", setting);
					break;
				    case password:
					data->password = buff_create(data->password, "%s", setting);
					break;
				    case threads:
					data->threads = atoi(setting);
					break;
				    case lines:
					data->lines = atoi(setting);
					break;
				    case uuenc:
					data->uuenc = atoi(setting);
					break;
				    case filenumber:
					data->filenumber = atoi(setting);
					break;
				    case noarchive:
					data->noarchive = atoi(setting);
					break;
				    case filesperpar:
					data->filesperpar = atoi(setting);
					break;
				    case followupto:
					data->followupto = buff_create(data->followupto, "%s", setting);
					break;
				    case replyto:
					data->replyto = buff_create(data->replyto, "%s", setting);
					break;
				    case tmpdir:
					data->tmpdir = buff_create(data->tmpdir, "%s", setting);
					break;
				    case name:
					data->name = buff_create(data->name, "%s", setting);
					break;
				    case extraheader:
					header = NULL;
					header = buff_create(header, "%s", setting);
					data->extra_headers =
					   slist_append(data->extra_headers,
							header);
					break;
				    default:
					fprintf(stderr,
					    "\nWARNING: invalid option in"
					    " %s: line %i",
					    filename->data, linenum);
				    }
				}
			}
			fclose(file);
		}
		else {
			if (errno != ENOENT)
				fprintf(stderr,
					"\nWARNING: %s %s", strerror(errno),
					filename->data);

			/* try to read the old .newspost */
			filename = buff_create(filename, "%s/.newspost", envopt);
			file = fopen(filename->data, "r");
			if (file != NULL) {
				linenum = 0;
				while (linenum < 8) {
					linenum++;
					line = buff_getline(line, file);
					if(line == NULL) continue;

					switch (linenum) {
					case 1:
						data->from = buff_create(data->from, "%s", line->data);
						break;
					case 2:
						data->organization = buff_create(data->organization,
							 "%s", line->data);
						break;
					case 3:
						data->newsgroup = buff_create(data->newsgroup,
							 "%s", line->data);
						break;
					case 4:
						data->address = buff_create(data->address, "%s", line->data);
						break;
					case 5:
						data->port = atoi(line->data);
						break;
					case 6:
						data->user = buff_create(data->user, "%s", line->data);
						break;
					case 7:
						data->password = buff_create(data->password,
							 "%s", line->data);
						break;
					case 8:
						data->lines = atoi(line->data);
					}
				}
			}
		}
	}
	else
		fprintf(stderr,
			"\nWARNING: Unable to determine your home directory"
			"\nPlease set your HOME environment variable");
	buff_free(filename);
	buff_free(line);
}

boolean set_defaults(newspost_data *data) {
	const char *envopt;
	FILE *file;
	SList *tmplist;
	Buff *filename = NULL;
	Buff * header = NULL;

	envopt = getenv("HOME");
	if (envopt != NULL) {
		filename = buff_create(filename, "%s/.newspostrc", envopt);
		file = fopen(filename->data, "w");
		if (file != NULL) {
			fprintf(file, "# %s\n%s=%s\n\n# %s\n%s=%s\n\n"
				"# %s\n%s=%s\n\n# %s\n%s=%s\n\n",
				rc_comment[newsgroup],
				rc_keyword[newsgroup], (data->newsgroup != NULL) ? data->newsgroup->data : "",
				rc_comment[from],
				rc_keyword[from], (data->from != NULL) ? data->from->data : "",
				rc_comment[organization],
				rc_keyword[organization], 
				(data->organization != NULL) ? data->organization->data : "",
				rc_comment[address],
				rc_keyword[address], (data->address != NULL) ? data->address->data : "");
			fprintf(file, "# %s\n%s=%i\n\n",
				rc_comment[port],
				rc_keyword[port], data->port);
			fprintf(file, "# %s\n%s=%s\n\n# %s\n%s=%s\n\n# %s\n%s=%i\n\n",
				rc_comment[user],
				rc_keyword[user], (data->user != NULL) ? data->user->data : "",
				rc_comment[password],
				rc_keyword[password], (data->password != NULL) ? data->password->data : "",
				rc_comment[threads],
				rc_keyword[threads], data->threads);
			fprintf(file, "# %s\n%s=%i\n\n# %s\n%s=%i\n\n"
				"# %s\n%s=%i\n\n# %s\n%s=%i\n\n",
				rc_comment[lines],
				rc_keyword[lines], data->lines,
				rc_comment[uuenc],
				rc_keyword[uuenc], data->uuenc,
				rc_comment[filenumber],
				rc_keyword[filenumber], data->filenumber,
				rc_comment[noarchive],
				rc_keyword[noarchive], data->noarchive);
			fprintf(file, "# %s\n%s=%s\n\n# %s\n%s=%s\n\n#"
				" %s\n%s=%s\n\n",
				rc_comment[followupto],
				rc_keyword[followupto], (data->followupto != NULL) ? data->followupto->data : "",
				rc_comment[replyto],
				rc_keyword[replyto], (data->replyto != NULL) ? data->replyto->data : "",
				rc_comment[tmpdir],
				rc_keyword[tmpdir], (data->tmpdir != NULL) ? data->tmpdir->data : "");
			fprintf(file, "# %s\n%s=%i\n\n",
				rc_comment[filesperpar],
				rc_keyword[filesperpar], data->filesperpar);
			fprintf(file, "# %s\n%s=%s\n\n",
				rc_comment[name],
				rc_keyword[name], (data->name != NULL) ? data->name->data : "");
			fprintf(file, "# %s\n",
				rc_comment[extraheader]);

			tmplist = data->extra_headers;
			while (tmplist != NULL) {
				header = (Buff *) tmplist->data;
				fprintf(file, "%s=%s\n",
					rc_keyword[extraheader],
					header->data);
				tmplist = slist_next(tmplist);
			}
			fclose(file);
			chmod(filename->data, S_IRUSR | S_IWUSR);
			buff_free(filename);
			return TRUE;
		}
		else {
			fprintf(stderr,
				"\nWARNING: %s %s", strerror(errno), filename->data);
			buff_free(filename);
			return FALSE;
		}
	}
	else {
		fprintf(stderr,
			"\nWARNING: Unable to determine your home directory"
			"\nPlease set your HOME environment variable");
		return FALSE;
	}
}

/* returns index of first non-option argument */
int parse_options(int argc, char **argv, newspost_data *data) {
	int flag, i, long_options_index;
	SList *listptr;
	Buff *header = NULL;
	boolean isflag = FALSE;

	opterr = 0;

	while (TRUE) {
		long_options_index = -1;
		flag = getopt_long(argc, argv, short_options, long_options, &long_options_index);
		if (flag == -1)
			break;
		else {
			switch (flag) {
				
			case help_option:
				print_help();
				break;

			case subject_option:
				data->subject = buff_create(data->subject, "%s", optarg);
				break;

			case newsgroup_option:
				data->newsgroup = buff_create(data->newsgroup, "%s", optarg);
				break;

			case from_option:
				data->from = buff_create(data->from, "%s", optarg);
				break;

			case organization_option:
				data->organization = buff_create(data->organization, "%s", optarg);
				break;

			case address_option:
				data->address = buff_create(data->address, "%s", optarg);
				break;

			case port_option:
				data->port = atoi(optarg);
				break;

			case user_option:
				data->user = buff_create(data->user, "%s", optarg);
				break;

			case password_option:
				data->password = buff_create(data->password, "%s", optarg);
				break;

			case threads_option:
				data->threads = atoi(optarg);
				break;

			case lines_option:
				data->lines = atoi(optarg);
				break;

			case uuenc_option:
				data->uuenc = TRUE;
				break;

			case sfv_option:
				data->sfv = buff_create(data->sfv, "%s", optarg);
				break;

			case par_option:
				data->par = buff_create(data->par, "%s", optarg);
				break;

			case parnum_option:
				data->parnum = atoi(optarg);
				break;

			case filesperpar_option:
				data->parnum = 0;
				data->filesperpar = atoi(optarg);
				break;

			case reference_option:
				data->reference = buff_create(data->reference, "%s", optarg);
				break;

			case default_option:
				writedefaults = TRUE;
				break;

			case version_option:
				version_info();
				printf("\n");
				exit(EXIT_NORMAL);

			case filenumber_option:
				data->filenumber = TRUE;
				break;

			case delay_option:
				parse_delay_option(optarg);
				break;

			case verbose_option:
				verbosity = TRUE;
				break;

			case noarchive_option:
				data->noarchive = FALSE;
				break;

			case followupto_option:
				data->followupto = buff_create(data->followupto, "%s", optarg);
				break;

			case replyto_option:
				data->replyto = buff_create(data->replyto, "%s", optarg);
				break;

			case tmpdir_option:
				data->tmpdir = buff_create(data->tmpdir, "%s", optarg);
				break;

			case name_option:
				data->name = buff_create(data->name, "%s", optarg);
				break;

			case extraheader_option:
				header = NULL; /* buff_create free()s memory */
				header = buff_create(header, "%s", optarg);
				data->extra_headers =
					slist_append(data->extra_headers,
						     header);
				break;

			case text_option:
				data->text = TRUE;
				break;

			case disable_option:
				switch (optarg[0]) {

				case user_option:
					data->user = buff_free(data->user);
					break;

				case password_option:
					data->password = buff_free(data->password);
					break;

				case name_option:
					data->name = buff_free(data->name);
					break;

				case organization_option:
					data->organization = 
						buff_free(data->organization);
					break;

				case followupto_option:
					data->followupto = buff_free(data->followupto);
					break;

				case replyto_option:
					data->replyto = buff_free(data->replyto);
					break;

				case noarchive_option:
					data->noarchive = TRUE;
					break;

				case extraheader_option:
					listptr = data->extra_headers;
					while (listptr != NULL) {
						buff_free(listptr->data);
						listptr = slist_next(listptr);
					}
					slist_free(data->extra_headers);
					data->extra_headers = NULL;
					break;

				case uuenc_option:
					data->uuenc = FALSE;
					break;

				case filenumber_option:
					data->filenumber = FALSE;
					break;

				default:
					fprintf(stderr,
						"\nUnknown argument to"
						" -%c option: %c\n",
						disable_option,optarg[0]);
					exit(EXIT_MISSING_ARGUMENT);
				}
				break;
	
			case '?':
				/* a bit of nastiness... my linux box
				   sends me to this case though the man
				   page says it should send me to the
				   next */
				for (i = 0 ; short_options[i] != '\0' ; i++) {
					while (short_options[i] == ':') {
						i++;
					}
					if (short_options[i] == '\0')
						break;
					if (optopt == short_options[i])
						isflag = TRUE;
				}
				if (!isflag) {
		     
					fprintf(stderr,
						"\nUnknown argument: -%c",
						optopt);
					print_help();
				}
				/* else continue to the next case */
			
			case ':':
				fprintf(stderr,
					"\nThe -%c option requires"
					" an argument.\n", optopt);
				exit(EXIT_MISSING_ARGUMENT);
				
			default:
				fprintf(stderr,
					"\nUnknown argument: -%c", optopt);
				print_help();	
				break;
			}
		}
	}
	return optind;
}

void check_options(newspost_data *data) {
	boolean goterror = FALSE;
	
	if (data->newsgroup == NULL) {
		fprintf(stderr,
			"\nThe newsgroup to post to is required.\n");
		goterror = TRUE;
	}
	else {
		const char *pi;
		int comma_count = 0;

		for (pi = data->newsgroup->data; *pi != '\0'; pi++)
			if (*pi == ',')
				comma_count++;

		if (comma_count > 4) {
			fprintf(stderr,
				"\nCrossposts are limited to 5 newsgroups.\n");
			goterror = TRUE;
		}
	}

	if (data->from == NULL) {
		fprintf(stderr,
			"\nYour e-mail address is required.\n");
		goterror = TRUE;
	}
	if (data->address == NULL) {
		fprintf(stderr,
			"\nThe news server's IP address or hostname"
			" is required.\n");
		goterror = TRUE;
	}
	if (data->threads <= 0 ) {
		fprintf(stderr,
			"\nAmount of threads to use for posting"
			" should be positive.\n");
		goterror = TRUE;
	}
#ifndef ALLOW_NO_SUBJECT
	if (data->subject == NULL) {
		fprintf(stderr,
			"\nThe subject line is required.\n");
		goterror = TRUE;
	}
#else
	if ((data->subject == NULL) && (data->text == TRUE)) {
		fprintf(stderr,
			"\nThe subject line is always required"
			" when posting text.\n");
		goterror = TRUE;
	}
#endif
	if (data->lines > 20000) {
		if (data->lines > 50000) {
			fprintf(stderr,
				"\nYour messages have too many lines!"
				"\nSet the maximum number of lines to 10000\n");
			goterror = TRUE;
		}
		else {
			fprintf(stderr,
				"\nWARNING: Most uuencoded messages are"
				" 5000 to 10000 lines");
		}
	}
	else if (data->lines < 3000) {
		if (data->lines < 500) {
			fprintf(stderr,
				"\nYour messages have too few lines!"
				"\nSet the maximum number of lines to 5000\n");
			goterror = TRUE;
		}
		else {
			fprintf(stderr,
				"\nWARNING: Most uuencoded messages are"
				" 5000 to 10000 lines");
		}
	}
	if (goterror == TRUE)
		exit(EXIT_BAD_HEADER_LINE);
}

void print_help() {
	version_info();
	printf("\n\nUsage: newspost [OPTIONS [ARGUMENTS]]"
		" file1 file2 file3...");
	printf("\nOptions:");
	printf("\n  --%-15s  -%c   <string> - hostname or IP of the news server", address_long_option, address_option);
	printf("\n  --%-15s  -%c   <int>    - port number on the news server", port_long_option, port_option);
	printf("\n  --%-15s  -%c   <string> - username on the news server", user_long_option, user_option);
	printf("\n  --%-15s  -%c   <string> - password on the news server", password_long_option, password_option);
	printf("\n  --%-15s  -%c   <int>    - amount of threads to use for posting", threads_long_option, threads_option);
	printf("\n  --%-15s  -%c   <string> - your e-mail address", from_long_option, from_option);
	printf("\n  --%-15s  -%c   <string> - your full name", name_long_option, name_option);
	printf("\n  --%-15s  -%c   <string> - your organization", organization_long_option, organization_option);
	printf("\n  --%-15s  -%c   <string> - newsgroups to post to", newsgroup_long_option, newsgroup_option);
	printf("\n  --%-15s  -%c   <string> - subject", subject_long_option, subject_option);
	printf("\n  --%-15s  -%c   <string> - newsgroup to put in the Followup-To header", followupto_long_option, followupto_option);
	printf("\n  --%-15s  -%c   <string> - e-mail address to put in the Reply-To header", replyto_long_option, replyto_option);
	printf("\n  --%-15s  -%c   <string> - reference these message IDs", reference_long_option, reference_option);
	printf("\n  --%-15s  -%c            - do NOT include \"X-No-Archive: yes\" header", noarchive_long_option, noarchive_option);
	printf("\n  --%-15s  -%c   <string> - a complete header line", extraheader_long_option, extraheader_option);
	printf("\n  --%-15s  -%c            - include \"File x of y\" in subject line", filenumber_long_option, filenumber_option);
	printf("\n  --%-15s  -%c            - uuencode instead of yencode", uuenc_long_option, uuenc_option);
	printf("\n  --%-15s  -%c   <string> - generate SFV file", sfv_long_option, sfv_option);
	printf("\n  --%-15s  -%c   <string> - generate PAR files",par_long_option, par_option);
	printf("\n  --%-15s  -%c   <int>    - number of PAR volumes to create", parnum_long_option, parnum_option);
	printf("\n  --%-15s  -%c   <int>    - number of files per PAR volume", filesperpar_long_option, filesperpar_option);
	printf("\n  --%-15s  -%c   <int>    - number of lines per message", lines_long_option, lines_option);
	printf("\n  --%-15s  -%c            - post one file as plain text", text_long_option, text_option);
	printf("\n  --%-15s  -%c   <int>    - time to wait before posting", delay_long_option, delay_option);
	printf("\n  --%-15s  -%c   <string> - use this directory for storing temporary files", tmpdir_long_option, tmpdir_option);
	printf("\n  --%-15s  -%c            - set current options as default", default_long_option, default_option);
	printf("\n  --%-15s  -%c   <char>   - disable or clear another option", disable_long_option, disable_option);
	printf("\n  --%-15s  -%c            - be verbose", verbose_long_option, verbose_option);
	printf("\n  --%-15s  -%c            - print version info and exit", version_long_option, version_option);
	printf("\n  --%-15s  -%c            - display this help screen and exit", help_long_option, help_option);
	printf("\nPlease see the newspost manpage for more"
		" information and examples.");
	printf("\n");
	exit(EXIT_NORMAL);
}

/**
*** Private Routines
**/

static boolean parse_file_parts(newspost_data *data,
		file_entry *this_file_entry, const char *arg, int i) {

	int numparts, thisint, prevint;
	boolean postany, postall;

	numparts = get_number_of_encoded_parts(data, this_file_entry);
	this_file_entry->parts = (boolean *) calloc((numparts + 1),
		sizeof(boolean));

	/* i is on the colon */
	i++;
	while (i < strlen(arg)) {
		if ((arg[i] < '0') || (arg[i] > '9')) {
			i++;
			continue;
		}
		thisint = atoi(arg + i);
		if (thisint > numparts) {
			fprintf(stderr,
				"\nWARNING: %s only has %i parts",
				this_file_entry->filename->data, numparts);
			while ((arg[i] != ',') && (arg[i] != '\0'))
				i++;
			i++;
			continue;
		}
		else if (thisint < 0) {
			fprintf(stderr,
				"\nWARNING: invalid part number %i",
				thisint);
			while ((arg[i] != ',') && (arg[i] != '\0'))
				i++;
			i++;
			continue;
		}
		while ((arg[i] >= '0') && (arg[i] <= '9'))
			i++;

		switch (arg[i]) {
		case '+':
			while (thisint <= numparts) {
				this_file_entry->parts[thisint] = TRUE;
				thisint++;
			}
			i += 2; /* i++ should be a comma or nothing */
			continue;

		case '-':
			prevint = thisint;
			i++;
			thisint = atoi(arg + i);

			if (prevint > thisint) {	/* backwards! */
				int swap = prevint;
				prevint = thisint;
				thisint = swap;
			}
			if (thisint > numparts) {
				fprintf(stderr,
					"\nWARNING: %s only has %i parts",
					this_file_entry->filename->data, numparts);

				thisint = numparts;
			}

			while (prevint <= thisint)
				this_file_entry->parts[prevint++] = TRUE;

			/* make sure i is in the right place */
			while ((arg[i] >= '0') && (arg[i] <= '9'))
				i++;

			continue;

		case '\0':
		case ',':
			this_file_entry->parts[thisint] = TRUE;
			i++;
			continue;

		default:
			/* encountered an unexpected character */
			fprintf(stderr,
				"\nWARNING: bad character %c in %s - IGNORING",
				arg[i], arg);
			i++;
		}
	}

	/* are we posting any parts? */
	postany = FALSE;
	postall = TRUE;

	for (i = 1; i <= numparts; i++) {
		if (this_file_entry->parts[i] == TRUE)
			postany = TRUE;
		else {
			postall = FALSE;
			this_file_entry->parts_to_post--;
		}
	}

	/* not posting any parts */
	if (postany == FALSE) {
		if(this_file_entry->parts[0] == FALSE){
			fprintf(stderr,
				"\nWARNING: Not posting %s: valid"
				" parts are 1 through %i",
				this_file_entry->filename->data, numparts);
			file_entry_free(this_file_entry);
		}
	}

	/* posting all parts despite colon */
	if (postall == TRUE) {
		if(this_file_entry->parts[0] == FALSE){
			free(this_file_entry->parts);
			this_file_entry->parts = NULL;
			
			fprintf(stderr,
				"\nWARNING: Posting ALL of %s (parts 1 - %i):"
				" is that what you meant to do?",
				this_file_entry->filename->data, numparts);
		}
	}

	if(this_file_entry->parts != NULL){
		if(this_file_entry->parts[0] == TRUE){
			printf("\nPretending to post %s",
			       this_file_entry->filename->data);
			fflush(stdout);
		}
	}

	if(this_file_entry->parts != NULL){
		return (postany | this_file_entry->parts[0]);
	}
	else return postany;
}

static void parse_delay_option(const char *option) {
	time_t hours = 0, minutes = 0, seconds = 0;

	seconds = atoi(option);
	while ((*option >= '0') && (*option <= '9'))
		option++;

	switch (*option) {

	case 'h':
	case 'H':
		hours = seconds;
		seconds = 0;
		break;

	case 'm':
	case 'M':
		minutes = seconds;
		seconds = 0;
		break;

	case ':':
		minutes = seconds;
		option++;
		seconds = atoi(option);
		while ((*option >= '0') && (*option <= '9'))
			option++;

		if (*option == ':') {
			hours = minutes;
			minutes = seconds;
			option++;
			seconds = atoi(option);
		}
	}

	post_delay = hours * 3600 + minutes * 60 + seconds;

	if (post_delay < 3)
		post_delay = 3;
}

static void version_info() {
	printf("\n" NEWSPOSTNAME " version " VERSION
		"\nCopyright (C) 2001 - 2010 Jim Faulkner"
		"\nThis is free software; see the source for"
		" copying conditions.  There is NO"
		"\nwarranty; not even for MERCHANTABILITY or"
		" FITNESS FOR A PARTICULAR PURPOSE.");
}
