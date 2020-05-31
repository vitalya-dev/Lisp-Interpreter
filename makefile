test: letsstart.exe  
	echo ((lambda `() `(+ 1 2 3))) | letsstart.exe

run: letsstart.exe	
	letsstart.exe


letsstart.exe: letsstart.c mpc.c
	cls
	dmc -e -lfoo letsstart.c mpc.c 


