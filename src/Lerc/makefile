FILES = Common/BitMask\
	Common/RLE\
	Lerc1Decode/BitStuffer\
	Lerc1Decode/CntZImage\
	Lerc2/BitStuffer2\
	Lerc2/Huffman\
	Lerc2/Lerc2\
	Lerc/Lerc

CC = g++
CFLAGS = -fPIC -Wall -O3
LDFLAGS = -shared -fvisibility=hidden -fvisibility-inlines-hidden
RM = rm -f
TARGET_LIB = libLerc.so
SRCS = $(addsuffix .cpp,$(FILES))
OBJS = $(addsuffix .o,  $(FILES))

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^

$(SRCS:.cpp=.d):%.d:%.cpp
	$(CC) $(CFLAGS) -MM $< >$@

include $(SRCS:.cpp=.d)

.PHONY: clean
clean:
	-${RM} ${TARGET_LIB} ${OBJS} $(SRCS:.cpp=.d)

