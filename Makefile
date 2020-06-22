build:
	gcc opencl-hello.c -lOpenCL

run: build
	sudo ./a.out 100

clean:
	rm -f ./a.out
	rm -f *_times
	rm -f *.png

plot_cpu: build
	for i in 1 2 3 4 5;do for i in 1 1 1 1 1 2 4 8 16 32 64 128 256 512;do sudo ./a.out $$i|grep cpu|awk -v idx=$$i '{print idx,$$3}';done|tail -10 ;done > cpu_times
	./plot_cpu

plot_gpu: build
	for i in 1 2 3 4 5;do for i in 1 1 1 1 1 2 4 8 16 32 64 128 256 512;do sudo ./a.out $$i|grep gpu|awk -v idx=$$i '{print idx,$$3}';done|tail -10 ;done > gpu_times
	./plot_gpu

plot: plot_cpu plot_gpu
