PLUGIN := sine-ping.so
OBJECTS_DIR := objects
CFLAGS += -Wall
LINK_FLAGS += -shared

-include Makefile.local

SOURCES := sine-ping.c

OBJECTS = $(foreach source,$(SOURCES),$(OBJECTS_DIR)/$(source:.c=.o))

ifndef VERBOSE_MAKE
	QUIET := @
endif

CFLAGS += $(foreach switch,$(SWITCHES),-D$(switch))
CFLAGS += $(foreach switch,$(DEFINES),-D$(switch))
LINK_FLAGS += $(foreach lib,$(LIBRARIES),-l$(lib))

all: $(PLUGIN)

$(OBJECTS_DIR)/%.o: %.c
	@echo Compiling $<...
	$(QUIET) $(CC) -c $< -g $(CFLAGS) -o $@

$(OBJECTS): | $(OBJECTS_DIR)

$(PLUGIN): $(OBJECTS)
	@echo "Linking $@..."
	$(QUIET) $(CC) $(filter-out $(OBJECTS_DIR),$^) -g $(LINK_FLAGS) -o $@

$(OBJECTS_DIR):
	@echo "Making $@..."
	$(QUIET) mkdir -p $(OBJECTS_DIR) $(OBJECTS_SUBDIRS)

-include $(OBJECTS_DIR)/*.d

.PHONY: clean
clean:
	rm -rf $(OBJECTS_DIR)

.PHONY: tags
tags:
	ctags -R .


