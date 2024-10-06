# zlib
emcc zlib for web assembly 

original : https://github.com/emscripten-ports/libpng.git

how to ?

1. git clone https://github.com/emscripten-ports/libpng.git
2. 진짜 어케 했더라..?
3.emconfigure ./configure --prefix=$(pwd)/emscripten_build CFLAGS="-sUSE_ZLIB=1"
4.emmake make
5.emmake make install 

그리고 컴파일 할때는 

컴파일: emcc -o fitblk_custom.wasm fitblk_custom.c -I~/zlib/emscripten_build/include -L~/zlib/emscripten_build/lib -lz
실행: ~/gowon_re/wasm-micro-runtime/product-mini/platforms/linux/build/iwasm ./fitblk_custom.wasm 1000

C파일 안: #include "../emscripten_build/include/zlib.h" (지금은 일단 헤더파일 경로 이렇게 줌, 근데 아마 이렇게 안하는 방법이 있을 듯)


결과 : 
started
have:64

1000으로 주면 64까지 압축.


아니면 나중에 Makefile이나 이런데 경로를 잘 설정해줘서 바로바로 해줘도 될듯 
아니면 진짜 헤더파일들 많은 곳에 넣어서 <>  이런식으로 include 일단 모르겠고 되니깐 더 안 건드려야겠다.





gzappend.c 
컴파일 : clang --target=wasm32-wasi --sysroot=/opt/wasi-sdk/share/wasi-sysroot -I/home/cha/zlib/emscripten_build/include -L/home/cha/zlib/emscripten_build/lib -o gzappend.wasm gzappend.c -lz -nostdlib -Wl,--no-entry
실행: ~/gowon_re/wasm-micro-runtime/product-mini/platforms/linux/build/iwasm --dir=/home/cha/zlib/examples ./gzappend.wasm gowonisgood.gz ./file1.txt ./file2.txt
빅 이슈 : iwasm --dir=. test.wasm 이런식으로 하면 파일 시스템을 iwasm으로 사용할 수 있음.

------------------------------------------------------------------------------------
#ifdef __cplusplus
#define WASM_EXPORT __attribute__((visibility("default"))) extern "C"
#else
#define WASM_EXPORT __attribute__((visibility("default")))
#endif


WASM_EXPORT int main(int argc, char **argv)




