TAR=cam
OBJ=camera_dev.o
CC:=gcc

$(TAR):$(OBJ)
	$(CC) $^ -o $@
%.o:%.c
	$(CC) -c $^ -o $@

# cam:camera_dev.o
# 	gcc camera_dev.o -o cam
# camera_dev.o:camera_dev.s
# 	gcc -c camera_dev.s -o camera_dev.o
# cammera_dev.s:cammera_dev.i
# 	gcc -S camera_dev.i -o camera_dev.s
# camera_dev.i:camera_dev.c
# 	gcc -E camera_dev.c -o camera_dev.i

.PHONY:
clean:
	rm -rf *.i *.s *.o
cleanall:
	rm -rf $(TAR) *.i *.s *.o
exe:
	sudo ./$(TAR)
