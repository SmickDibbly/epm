TARGET    = epm
PROJDIR   = $(realpath $(CURDIR)/..)
SOURCEDIR = $(PROJDIR)/src
BUILDDIR  = $(PROJDIR)/build/linux

ZIGIL_H_DIR = $(PROJDIR)/zigil
ZIGIL_A_DIR = $(PROJDIR)/zigil
DIBLIB_H_DIR = $(ZIGIL_H_DIR)/diblib_local

VERBOSE = #TRUE

DIRS = \
draw \
draw/viewport \
draw/window \
entity \
input \
interface \
misc \
system \
world \
world/bsp \
world/gltf
SOURCEDIRS = $(foreach dir, $(DIRS), $(addprefix $(SOURCEDIR)/, $(dir)))
TARGETDIRS = $(foreach dir, $(DIRS), $(addprefix $(BUILDDIR)/, $(dir)))

INCLUDES = $(foreach dir, $(SOURCEDIRS), $(addprefix -I, $(dir))) \
-I$(ZIGIL_H_DIR) -I$(DIBLIB_H_DIR) -I$(PROJDIR)

SOURCES = $(foreach dir, $(SOURCEDIRS), $(wildcard $(dir)/*.c))
OBJS := $(subst $(SOURCEDIR), $(BUILDDIR), $(SOURCES:.c=.o))
DEPS = ${OBJS:.o=.d}

VPATH = $(SOURCEDIRS)

GPROF_CFLAGS  = #-pg
GPROF_LDFLAGS = #-pg

SANITIZE_CFLAGS  = -fsanitize=address,undefined
SANITIZE_LDFLAGS = -fsanitize=address,undefined

CC = gcc
CFLAGS = \
-Wall \
-Wextra \
-Wconversion \
-Wdouble-promotion \
-Wno-unused-parameter \
-Wno-unused-function \
-Wno-sign-conversion \
-std=c99 -pedantic \
-MMD \
-O3 \
$(SANITIZE_CFLAGS) \
$(GPROF_CFLAGS)

LDFLAGS = -L$(ZIGIL_A_DIR) $(SANITIZE_LDFLAGS) $(GPROF_LDFLAGS)
LDLIBS = -lzigil -lX11 -lXext -lm

RM = rm -rf
RMDIR = rm -rf
MKDIR = mkdir -p
ERRIGNORE = 2>/dev/null
SEP=/

PSEP = $(strip $(SEP))

ifeq ($(VERBOSE), TRUE)
	HIDE =
else
	HIDE = @
endif

define generateRules
$(1)/%.o: %.c
	$(HIDE)echo Building $$@
	$$(HIDE)$$(CC) $$(CFLAGS) -c $$(INCLUDES) -o $$(subst /,$$(PSEP),$$@) $$(subst /,$$(PSEP),$$<)
endef

.PHONY: all clean dirs

all: dirs $(TARGET)

$(TARGET): $(OBJS)
	$(HIDE)echo Linking $@
	$(HIDE)$(CC) $(OBJS) -o $@ ${LDFLAGS} ${LDLIBS}

-include $(DEPS)

$(foreach targetdir, $(TARGETDIRS), $(eval $(call generateRules, $(targetdir))))

dirs:
	$(HIDE)$(MKDIR) $(subst /,$(PSEP),$(TARGETDIRS)) $(ERRIGNORE)

clean:
	$(HIDE)$(RMDIR) $(BUILDDIR) $(ERRIGNORE)
	$(HIDE)$(RM) $(TARGET) $(ERRIGNORE)
	@echo Cleaning done!
