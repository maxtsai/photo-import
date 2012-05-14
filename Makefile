CC=clang
CC1=gcc

TARGET = import

SRC = core_ops.c jpeg_ops.c main.c os_api.c

CFLAG = -std=c99 -Wall

all:
	@${CC} ${CFLAG} -O3 ${SRC} -o ${TARGET}
debug:
	@${CC} ${CFLAG} -g ${SRC} -o ${TARGET}

test_build:
	@${CC} -c ${CFLAG} -O3 ${SRC}
	@${CC1} -c ${CFLAG} -O3 ${SRC}
	@rm *.o

clean:
	@rm ${TARGET}
