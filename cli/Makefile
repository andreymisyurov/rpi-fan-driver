all:
	gcc -Wall -Wextra -o fan_ctl fan_ctl.c

clean:
	rm -f fan_ctl

reinstall: all
	-sudo rm -f /usr/local/bin/fan_ctl
	sudo install -m 755 fan_ctl /usr/local/bin/

.PHONY: all clean reinstall
