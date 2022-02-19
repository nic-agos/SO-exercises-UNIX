all:
	@echo "use either 'make posix' or 'make winapi' depending on the technology you are using for developing the program"

posix-A:
	@gcc esame_2018-07-18-A.c -o progA -lpthread -DPosix_compile

posix-B:
	@gcc esame_2018-07-18-B.c -o progB -lpthread -DPosix_compile

winapi-A:
	@winegcc esame_2018-07-18-A.c -o progA-win

winapi-B:
	@winegcc esame_2018-07-18-B.c -o progB-win

