BRANCH ?= master

lint:
	clang-format -i driver/rpifan.c
	@./checkpatch.pl -f --no-tree --terse driver/rpifan.c || exit 1
	@make -C driver clean && make -C driver || exit 1
	@make -C driver clean

push: lint
	@git push origin $(BRANCH)

.PHONY: push
