mkdir -p ../out
gcc -c -o ../out/lua.o src/lua/onelua.c -DLUA_USE_LINUX
ar rcs ../out/liblua.a ../out/lua.o
g++ -o ../out/MinBoolFunc \
	src/main_linux.cpp src/MinBoolFunc.cpp src/imgui/imgui_unity_build.cpp \
	-Isrc/imgui -Isrc/lua -I/usr/include/freetype2 \
	-L../out \
	-lglfw -llua -lGL -lfreetype
