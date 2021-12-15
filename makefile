CC=gcc
flags=-c -Wall
executable_name=test
driver=driver

all: $(driver).o tlb_functions.o l1_cache_functions.o l2_cache_functions.o main_memory_functions.o
	$(CC)  $(driver).o kernel_functions.o tlb_functions.o cache_functions.o main_memory_functions.o -o $(executable_name)
	@echo "Executable generated -> test"

$(driver).o: $(driver).c
	$(CC) $(flags) $(driver).c

kernel_functions.o: kernel_functions.c
	$(CC) $(flags) kernel_functions.c

tlb_functions.o: tlb_functions.c
	$(CC) $(flags) tlb_functions.c

cache_functions.o: cache_functions.c
	$(CC) $(flags) cache_functions.c

main_memory_functions.o: main_memory_functions.c
	$(CC) $(flags) main_memory_functions.c 

clean:
	rm -f *.o $(executable_name) ./output_files/OUTPUT.txt
