examples: snake.exe space_invaders.exe

snake.exe: snake.c tcanvas.h tcanvas.dll
	clang -o snake.exe snake.c

space_invaders.exe: space_invaders.c tcanvas.h tcanvas.dll
	clang -o space_invaders.exe space_invaders.c

tcanvas.dll: tcanvas.c tcanvas.h win_term.c term.h
	clang -shared -o tcanvas.dll tcanvas.c

clean:
	del /q *.exe *.dll *.obj *.exp *.lib *.pdb
