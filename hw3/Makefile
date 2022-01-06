CC?=gcc-11
CFLAGS=-g
LDFLAGS=-lpcap
SRC=$(wildcard *.c)
OBJECTS=$(patsubst %.c, ${BUILD_DIR}/%.o, ${SRC})
DEP=$(patsubst %.c, ${BUILD_DIR}/%.d, ${SRC})
BUILD_DIR=./build
all: ${BUILD_DIR}/bin/parse


${BUILD_DIR}/bin/parse: ${OBJECTS}
	@mkdir -p ${BUILD_DIR}/bin
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJECTS}

${BUILD_DIR}/%.o: %.c
	@mkdir -p ${BUILD_DIR}
	${CC} ${CFLAGS} -c -o $@ $<

