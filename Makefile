CC = cc
CFLAGS = -Wall -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lmosquitto -linih
OBJ = config.o  mqttasker.o
TARGETS = mqttasker


mqttasker: ${OBJ}
	${CC} ${CFLAGS} ${OBJ} -o $@ ${LDFLAGS}

clean:
	rm -f ${OBJ} ${TARGETS}

%.o: %.c
	${CC} ${CFLAGS} -c $< -o $@
