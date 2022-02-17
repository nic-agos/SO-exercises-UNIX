all:
	@echo "use either 'make posix' or 'make winapi' depending on the technology you are using for developing the program"

posix:
	@gcc esame_2019-01-23.c -o prog -lpthread -DPosix_compile

winapi:
	@winegcc esame_2019-09-18.c -o prog