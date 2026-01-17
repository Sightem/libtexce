# ----------------------------
# libtexce Autotest: %s
# ----------------------------

NAME = %s
CASE_NAME = %s
DESCRIPTION = "libtexce autotest case: %s"
COMPRESSED = NO
ARCHIVED = NO
LTO = NO

CFLAGS = -fno-addrsig -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

TEX_ROOT := $(shell cd ../../../.. && pwd)

EXTRA_C_SOURCES = \
    $(TEX_ROOT)/src/tex/tex_util.c \
    $(TEX_ROOT)/src/tex/tex_pool.c \
    $(TEX_ROOT)/src/tex/tex_symbols.c \
    $(TEX_ROOT)/src/tex/tex_metrics.c \
    $(TEX_ROOT)/src/tex/tex_fonts.c \
    $(TEX_ROOT)/src/tex/tex_token.c \
    $(TEX_ROOT)/src/tex/tex_parse.c \
    $(TEX_ROOT)/src/tex/tex_measure.c \
    $(TEX_ROOT)/src/tex/tex_layout.c \
    $(TEX_ROOT)/src/tex/tex_renderer.c \
    $(TEX_ROOT)/src/tex/tex_draw.c

CFLAGS += -I$(TEX_ROOT)/src -I$(TEX_ROOT)/src/tex -I$(TEX_ROOT)/include
CFLAGS += -I$(TEX_ROOT)/autotests
CFLAGS += -DTEX_USE_FONTLIB -DTEX_DIRECT_RENDER
CFLAGS += -Wframe-larger-than=126

include $(shell cedev-config --makefile)

all: copy_case

copy_case: bin/$(NAME).8xp
	@cp bin/$(NAME).8xp bin/$(CASE_NAME).8xp
