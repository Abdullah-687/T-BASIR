DIRS=$(wildcard */.)
clean_DIRS=$(addprefix clean_,$(DIRS))
all: $(DIRS)
clean: $(clean_DIRS)
.PHONY: all $(DIRS)
$(DIRS):
	$(MAKE) -C $@
$(clean_DIRS):
	$(MAKE) -C $(patsubst clean_%,%,$@) clean



