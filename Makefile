NAME := libuosdevicea
CFLAGS += -Wall -Wextra
SONAME := ${NAME}.so
WRAPPER := wechat-universal
STRIP ?= strip

all: ${SONAME} ${WRAPPER}

%: %.unstripped
	${STRIP} $< -o $@

${SONAME}.unstripped: ${NAME}.c
	${CC} ${CFLAGS} ${LDFLAGS} -fPIC -shared $< -o $@

${WRAPPER}.unstripped: ${WRAPPER}.c
	${CC} ${CFLAGS} ${LDFLAGS} -lsystemd $< -o $@

clean:
	rm -f ${SONAME} ${SONAME}.unstripped ${WRAPPER} ${WRAPPER}.unstripped

fresh: clean all

.PHONY: all clean fresh