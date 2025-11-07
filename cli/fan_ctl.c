#include <stdio.h>

#define PROC_STATUS_PATH "/proc/rpifan/status"
#define PROC_THRESHOLD_TEMP_PATH "/proc/rpifan/threshold_temp"

int main(void)
{
	printf("%s\n", PROC_STATUS_PATH);
	printf("%s\n", PROC_THRESHOLD_TEMP_PATH);
	return 0;
}