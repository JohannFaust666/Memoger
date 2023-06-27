CC=gcc
LIBS=monitor conf tree
cleanCom=rm -rf
oFlags= o a
extraLibs= libconfig-dev
linkPath=/usr/bin/memoger
manPath=/usr/share/man/man1
base:
	$(CC) main.c -lconfig -L. $(addprefix -l,$(LIBS)) -o main.o -w

full: clean libs base

libs: $(LIBS)

$(LIBS):
	$(CC) -c $@.c
	ar rc lib$@.a $@.o
	ranlib lib$@.a

clean:
	$(cleanCom) $(addprefix *.,$(oFlags))

installExtra:
	sudo apt-get install -y $(extraLibs)

uninstallExtra:
	sudo apt-get remove -y $(extraLibs)

install: installExtra full
	sudo cp -s $$PWD/main.o $(linkPath)
	sudo cp $$PWD/Resources/memoger.1.gz $(manPath)

uninstall: uninstallExtra
	sudo $(cleanCom) $(linkPath)
	sudo $(cleanCom) $(manPath)
	sudo $(cleanCom) $$PWD

