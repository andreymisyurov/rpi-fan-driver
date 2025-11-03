BRANCH ?= master

lint:
	clang-format -i src/rpifan.c
	@./checkpatch.pl -f --no-tree --terse src/rpifan.c || exit 1
	@make -C src clean && make -C src || exit 1
	@make -C src clean

push: lint
	@git push origin $(BRANCH)

.PHONY: push
