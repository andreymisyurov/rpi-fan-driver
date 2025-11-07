#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#define PROC_STATUS_PATH "/proc/rpifan/status"
#define PROC_THRESHOLD_TEMP_PATH "/proc/rpifan/threshold_temp"

#define BUFFER_SIZE 64

enum {
	OPT_ON = 258,
	OPT_OFF = 259,
};

static struct option long_options[] = {
	{"status",    no_argument,       0, 's'},
	{"on",        no_argument,       0, OPT_ON},
	{"off",       no_argument,       0, OPT_OFF},
	{"threshold", required_argument, 0, 't'},
	{"help",      no_argument,       0, 'h'},
	{0, 0, 0, 0}
    };

static int read_proc_file(const char *path);
static int write_proc_file(const char *path, const char *value);

int main(int argc, char *argv[])
{

	if (argc < 2) {
		printf("Usage: fan_ctl [OPTIONS]\n");
		printf("Try 'fan_ctl --help' for more information.\n");
		return 1;
	}

	int opt;
	while ((opt = getopt_long(argc, argv, "st:h",
				  long_options, NULL)) != -1) {
	    switch (opt) {
		case 's':
			printf("Fan status:\n");
			if (read_proc_file(PROC_STATUS_PATH) != 0)
				return -1;
			break;
		case OPT_ON:
			if (write_proc_file(PROC_STATUS_PATH, "on") != 0)
				return -1;;
			break;
		case OPT_OFF:
			if (write_proc_file(PROC_STATUS_PATH, "off") != 0)
				return -1;
			break;
		case 't':
			if (write_proc_file(PROC_THRESHOLD_TEMP_PATH, optarg) != 0)
				return -1;
			break;
		default:
			printf("Usage: fan_ctl [options]\n");
			printf("Options:\n");
			printf("  -s, --status    Show fan status\n");
			printf("  -n, --on      Turn fan on\n");
			printf("  -f, --off     Turn fan off\n");
			printf("  -t, --threshold Set threshold temperature\n");
			printf("  -h, --help    Show help\n");
			return -1;
	    }
	}
	return 0;
}

static int read_proc_file(const char *path)
{
	FILE *file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "Error: Cannot open %s: %s\n",
			path, strerror(errno));
		return -1;
	}

	char buffer[BUFFER_SIZE];
	while (fgets(buffer, sizeof(buffer), file)) {
		printf("%s", buffer);
	}

	fclose(file);
	return 0;
}

static int write_proc_file(const char *path, const char *value)
{
	FILE *file = fopen(path, "w");
	if (!file) {
		fprintf(stderr, "Error: Cannot open %s: %s\n",
			path, strerror(errno));
		return -1;
	}

	if (fprintf(file, "%s", value) != (int)strlen(value)) {
		fprintf(stderr, "Error: Failed to write to %s: %s\n",
			path, strerror(errno));
		fclose(file);
		return -1;
	}

	fclose(file);
	return 0;
}
