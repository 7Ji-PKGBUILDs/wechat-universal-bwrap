NAME := libuosdevicea
SONAME := ${NAME}.so
STRIP ?= strip

${SONAME}: ${SONAME}.unstripped
	${STRIP} $< -o $@

${SONAME}.unstripped: ${NAME}.c
	${CC} ${CFLAGS} ${LDFLAGS} -fPIC -shared $< -o $@

