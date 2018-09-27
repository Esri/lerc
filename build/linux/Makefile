FILES = src/LercLib/BitMask\
	src/LercLib/RLE\
	src/LercLib/BitStuffer2\
	src/LercLib/Huffman\
	src/LercLib/Lerc2\
	src/LercLib/Lerc\
	src/LercLib/Lerc_c_api_impl\
	src/LercLib/Lerc1Decode/BitStuffer\
	src/LercLib/Lerc1Decode/CntZImage

CFLAGS = -Wall -std=c++11 -fPIC -O2 -c
LDFLAGS = -shared -fvisibility=hidden -fvisibility-inlines-hidden

RM = rm -f
INSTALL ?= install
INSTALL_DATA ?= $(INSTALL) -m 644
INSTALL_LIB ?= $(INSTALL) -m 755

DESTDIR     ?=
PREFIX      ?= /usr/local
EXEC_PREFIX ?= $(PREFIX)
LIBDIR      ?= $(EXEC_PREFIX)/lib
INCLUDEDIR  ?= $(PREFIX)/include
LERC2SUFFIX ?= Lerc
LERC1SUFFIX ?= Lerc

SRCS = $(addsuffix .cpp,$(FILES))
OBJS = $(addsuffix .o,  $(FILES))

LIBVER_MAJOR := 2
LIBVER_MINOR := 4
LIBVER_PATCH := 0

SHARED_EXT := so
STATIC_EXT := a
SHARED_EXT_MAJOR := $(SHARED_EXT).$(LIBVER_MAJOR)
SHARED_EXT_VER := $(SHARED_EXT).$(LIBVER_MAJOR).$(LIBVER_MINOR).$(LIBVER_PATCH)
SHARED_LIB = libLerc.$(SHARED_EXT_VER)
STATIC_LIB = libLerc.$(STATIC_EXT)
SONAME_FLAGS = -Wl,-soname=libLerc.$(SHARED_EXT_MAJOR)

.PHONY: all
all: ${TARGET_LIB}

$(SHARED_LIB): $(OBJS)
	$(CXX) ${LDFLAGS} ${SONAME_FLAGS} -o $@ $^

libLerc.a: ARFLAGS = rcs
libLerc.a: $(OBJS)
	@echo compiling static library
	@$(AR) $(ARFLAGS) $@ $^

$(SRCS:.cpp=.o):%.o:%.cpp
	$(CXX) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	-$(RM) $(SHARED_LIB) $(STATIC_LIB) $(OBJS) 

install: $(SHARED_LIB) $(STATIC_LIB)
	@$(INSTALL) -d -m 755 $(DESTDIR)$(LIBDIR)/
	@$(INSTALL) -d -m 755 $(DESTDIR)$(INCLUDEDIR)/
	@$(INSTALL) -d -m 755 $(DESTDIR)$(INCLUDEDIR)/$(LERC2SUFFIX)
	@$(INSTALL) -d -m 755 $(DESTDIR)$(INCLUDEDIR)/Lerc1Decode/
	@echo Installing libraries
	@$(INSTALL_DATA) $(STATIC_LIB) $(DESTDIR)$(LIBDIR)
	@$(INSTALL_LIB) $(SHARED_LIB) $(DESTDIR)$(LIBDIR)
	@ln -sf $(SHARED_LIB) $(DESTDIR)$(LIBDIR)/libLerc.$(SHARED_EXT_MAJOR)
	@ln -sf $(SHARED_LIB) $(DESTDIR)$(LIBDIR)/libLerc.$(SHARED_EXT)
	@echo Installing includes
	@$(INSTALL_DATA) include/Lerc_c_api.h $(DESTDIR)$(INCLUDEDIR)/$(LERC2SUFFIX)
	@$(INSTALL_DATA) include/Lerc_types.h $(DESTDIR)$(INCLUDEDIR)/$(LERC2SUFFIX)
	@$(INSTALL_DATA) src/LercLib/Lerc.h $(DESTDIR)$(INCLUDEDIR)/$(LERC2SUFFIX)
	@$(INSTALL_DATA) src/LercLib/Lerc2.h $(DESTDIR)$(INCLUDEDIR)/$(LERC2SUFFIX)
	@$(INSTALL_DATA) src/LercLib/Defines.h $(DESTDIR)$(INCLUDEDIR)/$(LERC2SUFFIX)
	@$(INSTALL_DATA) src/LercLib/BitMask.h $(DESTDIR)$(INCLUDEDIR)/$(LERC2SUFFIX)
	@$(INSTALL_DATA) src/LercLib/BitSuffer2.h $(DESTDIR)$(INCLUDEDIR)/$(LERC2SUFFIX)
	@$(INSTALL_DATA) src/LercLib/Huffman.h $(DESTDIR)$(INCLUDEDIR)/$(LERC2SUFFIX)
	@$(INSTALL_DATA) src/LercLib/RLE.h $(DESTDIR)$(INCLUDEDIR)/$(LERC2SUFFIX)
	@$(INSTALL_DATA) src/LercLib/Lerc1Decode/CntZImage.h $(DESTDIR)$(INCLUDEDIR)/$(LERC1SUFFIX)/
	@$(INSTALL_DATA) src/LercLib/Lerc1Decode/TImage.hpp $(DESTDIR)$(INCLUDEDIR)/$(LERC1SUFFIX)/
	@$(INSTALL_DATA) src/LercLib/Lerc1Decode/Image.h $(DESTDIR)$(INCLUDEDIR)/$(LERC1SUFFIX)/
	@echo static and shared library installed

uninstall:
	@$(RM) $(DESTDIR)$(LIBDIR)/$(STATIC_LIB)
	@$(RM) $(DESTDIR)$(LIBDIR)/libLerc.$(SHARED_EXT)
	@$(RM) $(DESTDIR)$(LIBDIR)/libLerc.$(SHARED_EXT_MAJOR)
	@$(RM) $(DESTDIR)$(LIBDIR)/libLerc.$(SHARED_EXT_VER)
	@$(RM) $(DESTIDR)$(INCLUDEIDR)/Lerc_c_api.h
	@$(RM) $(DESTIDR)$(INCLUDEIDR)/Lerc_types.h
	@$(RM) $(DESTIDR)$(INCLUDEIDR)/$(LERC2SUFFIX)/Lerc2.h
	@$(RM) $(DESTIDR)$(INCLUDEIDR)/$(LERC2SUFFIX)/Defines.h
	@$(RM) $(DESTIDR)$(INCLUDEIDR)/$(LERC2SUFFIX)/BitMasC.h
	@$(RM) $(DESTIDR)$(INCLUDEIDR)/$(LERC2SUFFIX)/BitSuffer2.h
	@$(RM) $(DESTIDR)$(INCLUDEIDR)/$(LERC2SUFFIX)/Huffman.h
	@$(RM) $(DESTIDR)$(INCLUDEIDR)/$(LERC2SUFFIX)/RLE.h
	@$(RM) $(DESTIDR)$(INCLUDEIDR)/$(LERC1SUFFIX)/GntZImage.h
	@$(RM) $(DESTIDR)$(INCLUDEIDR)/$(LERC1SUFFIX)/TImage.hpp
	@$(RM) $(DESTIDR)$(INCLUDEIDR)/$(LERC1SUFFIX)/Image.h

