INSTALL_DIR=$(HOME)/.local/bin

LIBS=

CFLAGS=-Wall -Wextra -std=c99 -pedantic -fshort-enums -Werror \
	   -Wshadow -Wpointer-arith -Wcast-qual -Wmissing-prototypes \
	   -Wdeclaration-after-statement -Wno-sign-compare -Wstrict-prototypes \
	   -Wold-style-definition -Wvla \
	   $(foreach p,$(LIBS),$(shell pkg-config --cflags $(p)))

LDFLAGS=$(foreach p,$(LIBS),$(shell pkg-config --libs $(p))) -lm

SRCS=main.c ast.c vm.c token.c parse.c value.c
OBJS=$(SRCS:.c=.o)
HDRS=ast.h common.h vm.h token.h parse.h value.h
EXE=bread

#
# Release variables
#
RELDIR=release
RELEXE=$(RELDIR)/$(EXE)
RELOBJS=$(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS=-O3 -flto -march=native -mtune=native

#
# Debug variables
#
DBGDIR=debug
DBGEXE=$(DBGDIR)/$(EXE)
DBGOBJS=$(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS=-g -Og -DDEBUG

.PHONY: all
all: prep release debug

.PHONY: prep
prep:
	@mkdir -p $(DBGDIR) $(RELDIR)

#
# Debug rules
#
.PHONY: debug
debug: $(DBGEXE)

$(DBGEXE): $(DBGOBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(DBGCFLAGS) $^ -o $(DBGEXE)

$(DBGDIR)/%.o: %.c $(HDRS)
	$(CC) -c $(CFLAGS) $(DBGCFLAGS) $< -o $@

#
# Release rules
#
.PHONY: release
release: $(RELEXE)

$(RELEXE): $(RELOBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(RELCFLAGS) $^ -o $(RELEXE)
	strip $(RELEXE)

$(RELDIR)/%.o: %.c $(HDRS)
	$(CC) -c $(CFLAGS) $(RELCFLAGS) $< -o $@

.PHONY: clean
clean:
	-rm $(RELEXE) $(RELOBJS)
	-rm $(DBGEXE) $(DBGOBJS)

.PHONY: remake
remake: clean all

.PHONY: install
install: $(RELEXE)
	install $(RELEXE) $(INSTALL_DIR)/$(EXE)

.PHONY: uninstall
uninstall:
	rm $(INSTALL_DIR)/$(EXE)
