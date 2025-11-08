all:
	gcc -Wall -Wextra -o fan_ctl fan_ctl.c

install: all
	sudo install -m 755 fan_ctl /usr/local/bin/

clean:
	rm -f fan_ctl

.PHONY: all clean install