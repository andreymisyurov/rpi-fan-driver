#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROC_STATUS "/proc/rpifan/status"
#define PROC_THRESHOLD "/proc/rpifan/threshold_temp"

#define BUFFER_SIZE 64
#define MIN_TEMP 20
#define MAX_TEMP 90

enum {
    OPT_ON = 258,
    OPT_OFF = 259,
};

static struct option long_options[] = {
    {"status",    no_argument,       0, 's'    },
    {"on",        no_argument,       0, OPT_ON },
    {"off",       no_argument,       0, OPT_OFF},
    {"threshold", required_argument, 0, 't'    },
    {"help",      no_argument,       0, 'h'    },
    {0,           0,                 0, 0      }
};

static int read_proc_file(const char *path);
static int write_proc_file(const char *path, const char *value);
static void print_help();
static inline int validate_temp(const char *temp);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    int opt;
    while ((opt = getopt_long(argc, argv, "st:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 's':
                if (read_proc_file(PROC_STATUS) != 0) return -1;
                if (read_proc_file(PROC_THRESHOLD) != 0) return -1;
                break;
            case OPT_ON:
                if (write_proc_file(PROC_STATUS, "on") != 0) return -1;
                ;
                break;
            case OPT_OFF:
                if (write_proc_file(PROC_STATUS, "off") != 0) return -1;
                break;
            case 't':
                if (validate_temp(optarg) != 0) return -1;
                if (write_proc_file(PROC_THRESHOLD, optarg) != 0) return -1;
                break;
            case 'h':
                print_help();
                break;
            default:
                print_help();
                return -1;
        }
    }
    return 0;
}

static int read_proc_file(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "err: Cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), file)) {
        printf("%s", buffer);
    }

    fclose(file);
    return 0;
}

static int write_proc_file(const char *path, const char *value) {
    FILE *file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "err: Cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }

    if (fprintf(file, "%s", value) != (int)strlen(value)) {
        fprintf(stderr, "err: write failed %s: %s\n", path, strerror(errno));
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

static inline int validate_temp(const char *temp) {
    int ret = 0;
    char *endptr;
    int temp_int = strtol(temp, &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "err: Invalid temperature: %s°C\n", temp);
        ret = -1;
    } else if (temp_int < MIN_TEMP || temp_int > MAX_TEMP) {
        fprintf(stderr, "err: Temperature %s°C out of range [%d-%d]°C\n", temp,
                MIN_TEMP, MAX_TEMP);
        ret = -1;
    }
    return ret;
}

static void print_help(void) {
    printf("Usage: fan_ctl [OPTIONS]\n");
    printf("Options:\n");
    printf("  -s, --status    Show fan status\n");
    printf("  -t, --threshold Set threshold temperature\n");
    printf("  -h, --help    Show help\n");
    printf("  --on      Turn fan on\n");
    printf("  --off     Turn fan off\n");
}
