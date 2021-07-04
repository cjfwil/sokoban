@echo off
set debug_mode=1
set ignored_warnings=-wd4191 -wd5045
set compile_flags= -nologo -FC -Wall %ignored_warnings%
set link_flags= /link -incremental:no /out:game.exe
if %debug_mode%==1 (
	set debug_mode_flags= -Zi
	set release_mode_flags=
	pushd ..\debug
) else (
	set debug_mode_flags=
	set release_mode_flags= -O2
	pushd ..\release
)
del *.pdb > NUL 2> NUL
echo CODE IS COMPILING DO NOT INTERFERE WITH THIS FILE > lock.tmp
cl %compile_flags% %debug_mode_flags% %release_mode_flags% ..\code\dynamic_code.cpp /LD %link_flags% /PDB:"code_%time:~0,2%%time:~3,2%%time:~6,2%.pdb" /EXPORT:UpdateDraw /out:code.dll
del lock.tmp
cl %compile_flags% %debug_mode_flags% %release_mode_flags% ..\code\main.cpp %link_flags% /out:sokoban.exe
del *.obj
del *.exp
del *.lib
popd