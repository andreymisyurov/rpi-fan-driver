BRANCH ?= master

lint:
	clang-format -i driver/rpifan.c --style=file:driver/.clang-format
	clang-format -i cli/fan_ctl.c --style=file:cli/.clang-format

check:
	@./checkpatch.pl -f --no-tree --terse driver/rpifan.c || exit 1
	@make -C driver clean && make -C driver || exit 1
	@make -C driver clean
	@make -C cli || exit 1
	@make -C cli clean

reinstall:
	@make -C driver reinstall
	@make -C cli reinstall
	make clean

clean:
	@make -C driver clean
	@make -C cli clean

push: check
	@git push origin $(BRANCH)

.PHONY: push lint check reinstall clean
