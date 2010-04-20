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

#include "../base/encode.h"
#include "options.h"
#include "ui.h"

/* Command-line option keys */

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
#define lines_option 'l'
#define prefix_option 'e'
#define alternate_prefix_option '0'
#define edit_prefix_option 'E'
#define yenc_option 'y'
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

/* Option table for getopt() -- options which take parameters
   are followed by colons */

static const char valid_flags[] = {
	help_option,
	subject_option, ':',
	newsgroup_option, ':',
	from_option, ':',
	organization_option, ':',
	address_option, ':',
	port_option, ':',
	user_option, ':',
	password_option, ':',
	lines_option, ':',
	prefix_option, ':',
	alternate_prefix_option, ':',
	yenc_option,
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
	edit_prefix_option,
	filesperpar_option, ':',
	name_option, ':',
	extraheader_option,':',
	text_option,
	'\0'
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
	lines,
	yenc,
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
	"lines",
	"yenc",
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
	"lines per message",
	"0 to uuencode, 1 to yencode",
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

				/* for file parts */
				if (thisfilepart == TRUE)
					postany = parse_file_parts(data, 
						this_file_entry,
						argv[optind], i);
				else
					this_file_entry->parts = NULL;

				if (postany == TRUE)
					/* add it to the list */
					file_list = slist_append(file_list,
							this_file_entry);
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
				    case lines:
					data->lines = atoi(setting);
					break;
				    case yenc:
					data->yenc = atoi(setting);
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
			fprintf(file, "# %s\n%s=%s\n\n# %s\n%s=%s\n\n",
				rc_comment[user],
				rc_keyword[user], (data->user != NULL) ? data->user->data : "",
				rc_comment[password],
				rc_keyword[password], (data->password != NULL) ? data->password->data : "");
			fprintf(file, "# %s\n%s=%i\n\n# %s\n%s=%i\n\n"
				"# %s\n%s=%i\n\n# %s\n%s=%i\n\n",
				rc_comment[lines],
				rc_keyword[lines], data->lines,
				rc_comment[yenc],
				rc_keyword[yenc], data->yenc,
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
	int flag, i;
	SList *listptr;
	Buff *header = NULL;
	boolean isflag = FALSE;

	opterr = 0;

	while (TRUE) {
		flag = getopt(argc, argv, valid_flags);
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

			case lines_option:
				data->lines = atoi(optarg);
				break;

			case alternate_prefix_option:
			case prefix_option:
				data->prefix = buff_create(data->prefix, "%s", optarg);
				break;

			case yenc_option:
				data->yenc = TRUE;
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

			case edit_prefix_option:
				temporary_prefix = TRUE;
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

				case yenc_option:
					data->yenc = FALSE;
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
				for (i = 0 ; valid_flags[i] != '\0' ; i++) {
					while (valid_flags[i] == ':') {
						i++;
					}
					if (valid_flags[i] == '\0')
						break;
					if (optopt == valid_flags[i])
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
	FILE *testfile;
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
	if (data->prefix != NULL) {
		testfile = fopen(data->prefix->data, "rb");
		if (testfile == NULL) {
			fprintf(stderr,
				"\nWARNING: %s %s -  NO PREFIX WILL BE POSTED",
				strerror(errno), data->prefix->data);
			data->prefix = buff_free(data->prefix);
		}
		else
			fclose(testfile);
	}
	if (goterror == TRUE)
		exit(EXIT_BAD_HEADER_LINE);
}

void print_help() {
	version_info();
	printf("\n\nUsage: newspost [OPTIONS [ARGUMENTS]]"
		" file1 file2 file3...");
	printf("\nOptions:");
	printf("\n  -%c ARG - hostname or IP of the news server",
		address_option);
	printf("\n  -%c ARG - port number on the news server", port_option);
	printf("\n  -%c ARG - username on the news server", user_option);
	printf("\n  -%c ARG - password on the news server", password_option);
	printf("\n  -%c ARG - your e-mail address", from_option);
	printf("\n  -%c ARG - your full name",name_option);
	printf("\n  -%c ARG - your organization", organization_option);
	printf("\n  -%c ARG - newsgroups to post to", newsgroup_option);
	printf("\n  -%c ARG - subject", subject_option);
	printf("\n  -%c ARG - newsgroup to put in the Followup-To header",
		followupto_option);
	printf("\n  -%c ARG - e-mail address to put in the Reply-To header",
		replyto_option);
	printf("\n  -%c ARG - reference these message IDs", reference_option);
	printf("\n  -%c     - do NOT include \"X-No-Archive: yes\" header",
		noarchive_option);
	printf("\n  -%c ARG - a complete header line", extraheader_option);
	printf("\n  -%c     - include \"File x of y\" in subject line",
		filenumber_option);
	printf("\n  -%c     - yencode instead of uuencode", yenc_option);
	printf("\n  -%c ARG - text prefix", prefix_option);
	printf("\n  -%c     - write text prefix in text editor set by $EDITOR",
		edit_prefix_option);
	printf("\n  -%c ARG - generate SFV file", sfv_option);
	printf("\n  -%c ARG - generate PAR files",par_option);
	printf("\n  -%c ARG - number of PAR volumes to create", parnum_option);
	printf("\n  -%c ARG - number of files per PAR volume",
		filesperpar_option);
	printf("\n  -%c ARG - number of lines per message", lines_option);
	printf("\n  -%c     - post one file as plain text", text_option);
	printf("\n  -%c ARG - time to wait before posting", delay_option);
	printf("\n  -%c ARG - use this directory for storing temporary files",
		tmpdir_option);
	printf("\n  -%c     - set current options as default", default_option);
	printf("\n  -%c ARG - disable or clear another option", disable_option);
	printf("\n  -%c     - be verbose", verbose_option);
	printf("\n  -%c     - print version info and exit", version_option);
	printf("\n  -%c     - display this help screen and exit", help_option);
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

	for (i = 1; i <= numparts; i++)
		if (this_file_entry->parts[i] == TRUE)
			postany = TRUE;
		else
			postall = FALSE;

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
		"\nCopyright (C) 2001 - 2003 Jim Faulkner"
		"\nThis is free software; see the source for"
		" copying conditions.  There is NO"
		"\nwarranty; not even for MERCHANTABILITY or"
		" FITNESS FOR A PARTICULAR PURPOSE.");
}
