all:
	make -C kmod
	make -C tools
install:
	make -C kmod install
	make -C tools install
clean:
	make -C kmod clean
	make -C tools clean
	rm -fr .tmp_versions
