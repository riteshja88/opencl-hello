build:
	gcc opencl-hello.c -lOpenCL

run: build
	sudo ./a.out

clean:
	rm -f ./a.out
