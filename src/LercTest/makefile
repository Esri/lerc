INCL = /home/john/Code/Lerc/include
LIBS = /home/john/Code/Lerc/src/Lerc/libLerc.so

CC = g++
CFLAGS = -c -Wall -O2
RM = rm -f
SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)
EXEC = $(SRCS:.cpp=.out)

$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(LIBS)

$(OBJS): %.o : %.cpp
	$(CC) $(CFLAGS) -I$(INCL) $< -o $@

.PHONY: clean
clean:
	-${RM} ${EXEC} ${OBJS}


# or type directly in the cmd shell:
#
#$ g++ -c main.cpp -I/home/john/Code/Lerc/include
#$ g++ -o main.out main.o /home/john/Code/Lerc/src/Lerc/libLerc.so
#$ /home/john/Code/Lerc/src/LercTest/main.out

