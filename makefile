test: letsstart.exe  
	cmd \/C cat foo.lisp | letsstart.exe 

run: letsstart.exe	
	cmd \/C letsstart.exe


letsstart.exe: letsstart.c mpc.c
	cls
	cmd \/C dmc -e -lfoo letsstart.c mpc.c 


