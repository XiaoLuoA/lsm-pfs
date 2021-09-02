#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <hash_map>
#include <iostream>
#include <algorithm>
#include <vector>
#include "disk.h"
#include "lsm.h"
#include "crypt.h"
#include "mhbt.h"
#include "cache.h"
#include <list>

using namespace std;
using namespace __gnu_cxx;


class LRUCache2 {  
public:
    lsm_kv h[20000000];
    bool yh[20000000];

    LRUCache2()
    {  
        m_capacity = 0;
        //cache_Map.resize(50000000);
    }

    LRUCache2(int capacity)
    {  
        m_capacity = capacity;  
    }

    void clear()
    {
        //cache_Map.clear();
        memset(yh, 0, sizeof(yh));
		memset(h, -1, sizeof(h));
    }

    int hash_f(int key){
        return key % 20000000;
    }
	
	lsm_kv get(int key) 
    {  
        lsm_kv retValue = lsm_kv(-1, -1, -1, -1);
		int _hash = hash_f(key);
        while (yh[_hash]) {
			if (h[_hash].lba == key){
				retValue = h[_hash];
				break;
			}
			else 
				_hash++;
        }
		//printf("get %d  %d %d  %d  %d\n", _hash, key, h[_hash].lba, h[_hash].pba, h[_hash].key);
        return retValue;          
    }

	lsm_kv set(int key, lsm_kv value, int &pop_key) 
    {  
        lsm_kv retValue = lsm_kv(-1, -1, -1, -1);
		int _hash = hash_f(key);
		while (yh[_hash] && h[_hash].lba != key)
			_hash++;
        h[_hash] = value;
		//printf("k %d  %d\n", _hash, value.key);
		yh[_hash] = true;
        return retValue;
    }

    int m_capacity;
} ss;

hash_map<int, lsm_kv> lsm_C0;
int *sst_file_count;
int *sst_file_size;

void lsm_init(int *_sst_file_count, int *_sst_file_size)
{
	sst_file_count = _sst_file_count;
	sst_file_size = _sst_file_size;
	//printf("sst_count %d %d\n", *sst_file_count, sst_file_count);
	ss.clear();
//	lsm_C0.resize(2000000);
}

int sst_find(int num, int lba, lsm_kv *value)
{
	char sst_file_name[20];
	snprintf(sst_file_name, sizeof(sst_file_name), "sst_%d", num);

	int head = 0, tail = sst_file_size[num] - 1;
	while (head < tail) {
		int mid = (head + tail) / 2;
		sst_read_index(sst_file_name, mid, (char *)value);
		if (value->lba < lba)
			head = mid + 1;
		else
			tail = mid;
	}
	sst_read_index(sst_file_name, head, (char *)value);
	if (value->lba == lba) {
		printf("sst find\n");
		return 1;
	}
	else 
		return 0;

}

int small_sst_find(int num, int lba, lsm_kv *value)
{
	char sst_file_name[20];
	snprintf(sst_file_name, sizeof(sst_file_name), "sst_%d", num);

	char buf[BLOCK_SIZE];
//	int ret = file_read(sst_file_name, 0, BLOCK_SIZE, buf);
	int ret = small_sst_read(sst_file_name, sst_file_size[num], buf);
	if (ret < 0)
		return -1;

	sst sst_file;
	sst_file.kv_array = (lsm_kv*)buf;
	sst_file.sst_size = sst_file_size[num];

	int head = 0, tail = sst_file.sst_size - 1;	
	while (head < tail) {
		int mid = (head + tail) / 2;
		if (sst_file.kv_array[mid].lba < lba)
			head = mid + 1;
		else
			tail = mid;
	}
	if (sst_file.kv_array[head].lba == lba) {
		*value = sst_file.kv_array[head];
		printf("sst find\n");
		ret = 1;
	}
	else
		ret = 0;
	return ret;
}

lsm_kv lsm_find(int lba)
{
	//hash_map<int, lsm_kv>::iterator iter;
	//iter = lsm_C0.find(lba);
//	if (iter != lsm_C0.end()) {
//		return iter->second;	
//	}
	lsm_kv x = ss.get(lba);
	if (x.key != -1) {
		return x;
	}
	else {
		//printf("ffff\n");
		lsm_kv kv(-1, -1, -1, -1);
		for (int i = *sst_file_count - 1; i >= 0; i--) {
			if (read_mhbt(i, sst_file_size[i], lba, &kv)) {
				return kv;
			}
		}
		return kv;
	}
}

bool cmp(lsm_kv a, lsm_kv b){
	return a.lba < b.lba;
}

int C0_to_C1()
{
//	if (lsm_C0.size() == 0) return 0;
	sst sst_file;
//	sst_file.sst_size = ss.cache.size();
//	if (sst_file.sst_size <= 0)
//		return 0;
	sst_file.sst_size = 0;
	for (int j = 0; j < 20000000; j++)
		if (ss.yh[j]) 
			sst_file.sst_size++;
	sst_file.kv_array = new lsm_kv[sst_file.sst_size];
	int i = 0;
//	hash_map<int, lsm_kv>::iterator iter;

/*	for (iter = lsm_C0.begin(); iter != lsm_C0.end(); iter++) {
		sst_file.kv_array[i] = iter->second;
		i++;
	}

	list<pair<int, lsm_kv>>::iterator iter;
	for (iter = ss.cache.begin(); iter != ss.cache.end(); iter++) {
		sst_file.kv_array[i] = iter->second;
		i++;
		//printf("fn");
	}*/
	for (int j = 0; j < 20000000; j++)
		if (ss.yh[j]) {
			sst_file.kv_array[i] = ss.h[j];
			i++;
		}
	sort(sst_file.kv_array, sst_file.kv_array+sst_file.sst_size, cmp);
	lsm_C0.clear();

	char sst_name[20];
    snprintf(sst_name, sizeof(sst_name), "sst_%d", *sst_file_count);

	vector<char> buf;
	mhbt_write(sst_file.kv_array, sst_file.sst_size, buf);
	sst_file_size[*sst_file_count] = buf.size() / 16;
	(*sst_file_count)++;

	sst_write(sst_name, buf.size() / 16, (char *)&buf[0]);
	delete[] sst_file.kv_array;
}

int lsm_insert(lsm_kv kv){
/*	lsm_C0[kv.lba] = kv;
	if (lsm_C0.size() > 4000000) {
		C0_to_C1();
	}*/
	int x;
	ss.set(kv.lba, kv, x);
}

int data_write(int lba, char *buf, int pba){
    int ret;
	char *data_file_name = "data";
	char key[16] ;
	memset(key,2,16);
	char mac[16];
	memset(key,0,16);
	unsigned char *cipher_text = new unsigned char[BLOCK_SIZE];
	encrypt((unsigned char*)buf, BLOCK_SIZE, cipher_text, (unsigned char*)key,(unsigned char*) mac);
    ret = data_disk_write(data_file_name, pba,(char*) cipher_text);
    //ret = data_disk_write(data_file_name, pba, buf);
	int ki;
	memcpy(&ki, key, 4);
	//printf("%d\n", ki);
    lsm_kv kv(lba, ki, -1, pba);
    lsm_insert(kv);
	delete[] cipher_text;
    return ret;
}

int data_read(int lba, char *buf){
    char *data_file_name = "data";
    lsm_kv kv = lsm_find(lba);
    //printf("%d %d %d\n", kv.lba, kv.pba, kv.key);
	if (kv.lba == -1) {
		printf("Block %d not found!\n", lba);
		return -1;
	}
	else {
		//lsm_insert(kv);
       	data_disk_read(data_file_name, kv.pba, buf);
		char key[16];
		char mac[16];
		unsigned char *plain_text = (unsigned char *)malloc(BLOCK_SIZE);
memset(key,2,16);
		// memcpy(key, &kv.key, 4);
		// for (int i = 4; i < 16; i++)
		// 	key[i] = 0;
	memset(mac,0,16);
		decrypt(plain_text,  (unsigned char *)buf,BLOCK_SIZE,  (unsigned char*)key, (unsigned char*) mac);
		memcpy(buf, plain_text, BLOCK_SIZE);
		free(plain_text);
	}
}

int compaction(){          
	//to do
}
