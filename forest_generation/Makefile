# Makefile simply to create and link jpeg's C prediction routine (a bit slow to test otherwise)

.PHONY: default
default: jpeg_ls

jpeg_ls:
	gcc -fPIC -shared -o jpeg-ls.so jpeg-ls.c

clean:
	bash clean.sh
