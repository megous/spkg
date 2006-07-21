#/----------------------------------------------------------------------\#
#| spkg - The Unofficial Slackware Linux Package Manager                |#
#|                                      designed by Ond�ej Jirman, 2005 |#
#|----------------------------------------------------------------------|#
#|          No copy/usage restrictions are imposed on anybody.          |#
#\----------------------------------------------------------------------/#
#VERSION := $(shell git-describe --abbrev=4 | tr - _)
VERSION := rc0

DESTDIR :=
prefix := /usr/local
DEBUG := no
ASSERTS := yes
BENCH := no
RELEASE := no
STATIC := no

ifeq ($(RELEASE),yes)
DEBUG := no
ASSERTS := yes
BENCH := no
STATIC := yes
prefix := /usr
endif

sbindir = $(prefix)/sbin
mandir = $(prefix)/man
docdir = $(prefix)/doc/spkg-$(VERSION)

CC := gcc
AR := ar
CFLAGS := -pipe -Wall -Wformat
ifeq ($(STATIC),yes)
CPPFLAGS := -Iinclude -Ilibs/zlib -Ilibs/glib -Ilibs/popt -Ilibs/judy
LDFLAGS := libs/zlib/libz.a libs/glib/libglib-2.0.a libs/popt/libpopt.a libs/judy/libJudy.a
else
CPPFLAGS := -Iinclude $(shell pkg-config --cflags glib-2.0)
LDFLAGS := -lz $(shell pkg-config --libs glib-2.0) -lpopt -lJudy
endif
CPPFLAGS += -D_GNU_SOURCE -DSPKG_VERSION=$(VERSION)
ifeq ($(DEBUG),yes)
CFLAGS +=  -ggdb3 -O0
CPPFLAGS += -D__DEBUG=1
else
CFLAGS += -ggdb1 -O2 -march=i486 -mtune=i686 -fomit-frame-pointer
endif
ifeq ($(BENCH),yes)
CPPFLAGS += -D__BENCH=1
endif
ifeq ($(ASSERTS),no)
CPPFLAGS += -DG_DISABLE_ASSERT
endif

objs-spkg := misc.o error.o sys.o path.o untgz.o pkgdb.o \
  taction.o sigtrap.o message.o cmd-install.o cmd-remove.o \
  cmd-upgrade.o cmd-list.o main.o

export MAKEFLAGS += --no-print-directory -r

objs-spkg := $(addprefix .build/, $(objs-spkg))
objs-all := $(sort $(objs-spkg))
dep-files := $(addprefix .build/,$(addsuffix .d,$(basename $(notdir $(objs-all)))))

# main
############################################################################
vpath %.c src

spkg: .build/libspkg.a
	$(CC) $^ $(LDFLAGS) -o $@

.build/libspkg.a: $(objs-spkg)
	$(AR) rcs $@ $^

.build/%.o: %.c
	@mkdir -p .build
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# generate deps
.build/%.d: %.c
	@mkdir -p .build
	$(CC) -MM -MG -MP -MF $@ -MT ".build/$(<F:.c=.o) $@" $(CPPFLAGS) $<
ifneq ($(dep-files),)
-include $(dep-files)
endif

# include tests
include Makefile.tests

# installation
############################################################################
.PHONY: install install-docs install-spkg uninstall

install: install-spkg install-docs

install-spkg: spkg
	install -d -o root -g root -m 0755 $(DESTDIR)$(sbindir)
	install -o root -g bin -m 0755 spkg $(DESTDIR)$(sbindir)/
	ln -sf spkg $(DESTDIR)$(sbindir)/ipkg
	ln -sf spkg $(DESTDIR)$(sbindir)/rpkg
	ln -sf spkg $(DESTDIR)$(sbindir)/upkg
	ln -sf spkg $(DESTDIR)$(sbindir)/lpkg
	strip $(DESTDIR)$(sbindir)/spkg

install-docs: docs
	install -d -o root -g root -m 0755 $(DESTDIR)$(mandir)/man8/
	install -o root -g root -m 0644 docs/spkg.8 $(DESTDIR)$(mandir)/man8/
	gzip -f -9 $(DESTDIR)$(mandir)/man8/spkg.8
	ln -sf spkg.8.gz $(DESTDIR)$(mandir)/man8/ipkg.8.gz
	ln -sf spkg.8.gz $(DESTDIR)$(mandir)/man8/rpkg.8.gz
	ln -sf spkg.8.gz $(DESTDIR)$(mandir)/man8/upkg.8.gz
	ln -sf spkg.8.gz $(DESTDIR)$(mandir)/man8/lpkg.8.gz
	install -d -o root -g root -m 0755 $(DESTDIR)$(docdir)
	install -d -o root -g root -m 0755 $(DESTDIR)$(docdir)/html
	install -o root -g root -m 0644 LICENSE README INSTALL BENCHMARKS NEWS TODO $(DESTDIR)$(docdir)/
	install -o root -g root -m 0644 docs/html/* $(DESTDIR)$(docdir)/html/

uninstall:
	rm -f $(DESTDIR)$(sbindir)/{s,r,u,i,l}pkg
	rm -f $(DESTDIR)$(mandir)/man8/{s,r,u,i,l}pkg.8.gz
	rm -rf $(DESTDIR)$(docdir)

# distribution
############################################################################
.PHONY: slackpkg dist

PACKAGES := spkg-$(VERSION)-i486-1.tgz

slackpkg: $(PACKAGES)

spkg-$(VERSION)-i486-1.tgz:
	make clean
	rm -rf pkg
	make install RELEASE=yes DESTDIR=./pkg
	install -d -o root -g root -m 0755 ./pkg/install
	install -o root -g root -m 0644 docs/slack-desc.spkg ./pkg/install/slack-desc
	( cd pkg ; makepkg -l y -c n ../$@ )
	rm -rf pkg

dist: docs
	rm -rf spkg-$(VERSION)
	mkdir -p spkg-$(VERSION)
	git tar-tree HEAD spkg-$(VERSION) | tar x
	git log > spkg-$(VERSION)/ChangeLog
	sed -i 's/^VERSION :=.*$$/VERSION := $(VERSION)/' spkg-$(VERSION)/Makefile
	cp -a docs/html spkg-$(VERSION)/docs
	tar czf spkg-$(VERSION).tar.gz spkg-$(VERSION)
	rm -rf spkg-$(VERSION)

# documentation
############################################################################
.PHONY: docs

docs:
	rm -rf docs/html
	doxygen docs/Doxyfile
	rm -f docs/html/doxygen.png

# cleanup
############################################################################
.PHONY: clean mrproper 

clean: tests-clean
	-rm -rf .build/*.o .build/*.a spkg build

mrproper: clean
	-rm -rf .build docs/html
