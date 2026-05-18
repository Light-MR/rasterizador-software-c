cl -Tc src/*.c -O2 -I".\include" -I"..\libosw\include" -link User32.lib Gdi32.lib Opengl32.lib Xinput9_1_0.lib ..\libosw\lib\Osw.lib -OUT:prog.exe
cl -Tc src/*.c -O2 -I".\include" -I"..\libosw\include" -DUSE_TRANSPARENCY -link User32.lib Gdi32.lib Opengl32.lib Xinput9_1_0.lib ..\libosw\lib\Osw.lib -OUT:prog_transp.exe
