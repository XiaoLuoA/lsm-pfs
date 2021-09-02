#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <map>
#include <iostream>
#include <chrono>
#include "fs.h"
#include "disk.h"
#include "lsm.h"
#include "crypt.h"
#include "cache.h"

using namespace std;
using namespace chrono;

void test_cache(){
    init_cache();
    char *a = "123";
    put_to_cache(0, a, data_cache);
    printf("fff\n");
    char *b = find_in_cache(0, data_cache);
    printf("%s\n", b);
}

int main(int argc, char *arg0[]){
//    test_cache();
    //srand(0);
    auto begin = std::chrono::high_resolution_clock::now();
    aes_init();
    fs_mount("meta");
    int round = (1LL<<32) / BLOCK_SIZE;
//    printf("%d\n", round);
    int f = 1;
	init_cache();
    int arg=atoi(arg0[1]);
    int r=atoi(arg0[2]);
    int bs=atoi(arg0[3]);
/*    switch (arg){
        case 0: 
            test_write(round, r, f);
            break;
        case 1: 
            test_write_nocache(round, r, f);
            break;
        case 2: 
            test_read(round, r, f);
            break;
        case 3: 
            test_read_nocache(round, r, f);
            break;
        default: 
            break;
    }
    */
    if (arg == 0)
        test_write(r, bs);
    else
        test_read(r, bs);
    xx();
    auto end= std::chrono::high_resolution_clock::now();
	double elapsedTime = ((duration<double, std::milli>)(end - begin)).count();
	cout << "\ntotal elapsed time: " << elapsedTime << endl;
    double bd = (1LL<<34) / 1000.0 / elapsedTime;
    cout << "bandwidth " << bd << endl;
    aes_release();
    fs_umount("meta");
}
