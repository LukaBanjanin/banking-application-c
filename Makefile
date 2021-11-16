CC=gcc

make:
	$(CC) Assignment3.c -lpthread -o asn3.out

clean:
	rm asn3.out
	rm assignment_3_output_file.txt