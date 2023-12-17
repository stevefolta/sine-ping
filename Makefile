PLUGIN := sine-ping.clap
OBJECTS_DIR := objects
CFLAGS += -Wall
LINK_FLAGS += -shared

-include Makefile.local

SOURCES := sine-ping.c
SOURCES += Plugin.c Voice.c Parameters.c
SOURCES += Stream.c
CFLAGS += -fPIC

OBJECTS = $(foreach source,$(SOURCES),$(OBJECTS_DIR)/$(source:.c=.o))

ifndef VERBOSE_MAKE
	QUIET := @
endif

CFLAGS += -MMD
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

.PHONY: edit-all
edit-all:
	@ $(EDITOR) $(filter-out sine-ping.h,$(foreach source,$(SOURCES),$(source:.c=.h) $(source)))


