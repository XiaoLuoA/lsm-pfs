#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "disk.h"
#include "lsm.h"
#include "crypt.h"
#include "fs.h"
#include "cache.h"

#define MAX_LAYER 3
using namespace std;


int xxx = 0;


int transform(lsm_kv *lv, int size, lsm_kv **mhbtv) {
	vector<lsm_kv> q[MAX_LAYER];
	vector<lsm_kv> m;
	for (int i = 0; i < MAX_LAYER; i++)
		q[i].clear();
	m.clear();

	int pos = 0;
	int len = BLOCK_SIZE / 16;

	for (int i = 0; i < size; i++) {
		q[0].push_back(lv[i]);

		int j = 0;
		while (q[j].size() == len){
			int key = -1, mac = -1;
			for (int k = 0; k < len; k++)
				m.push_back(q[j][k]);
			q[j + 1].push_back(lsm_kv(q[j][0].lba, key, mac, pos));
			pos++;
			q[j].clear();
			j++;
		}
	}

	for (int j = 0; j < MAX_LAYER; j++)
		if (q[j].size() != 0 ){
		        int key = -1, mac = -1;
			int padlen = len - q[j].size();
			for (int i = 0; i < padlen; i++)
				q[j].push_back(lsm_kv(-1, -1, -1, -1));
			if (j != MAX_LAYER - 1){
                q[j + 1].push_back(lsm_kv(q[j][0].lba, key, mac, pos));
			}
			for (int k = 0; k < len; k++)
				m.push_back(q[j][k]);
                pos++;
                q[j].clear();
		}

	int mhbt_size = sizeof(lsm_kv) * m.size();
	*mhbtv = new lsm_kv[m.size()];
	memcpy(*mhbtv, &m[0], sizeof(lsm_kv) * m.size());
	return mhbt_size;
}

void mhbt_write(lsm_kv *lv, int size, vector<char> &output) {
	vector<lsm_kv> q[MAX_LAYER];
	for (int i = 0; i < MAX_LAYER; i++)
		q[i].clear();
	output.clear();
	int pos = 0;
	int len = BLOCK_SIZE / 16;

	char *cipher_text = new char[BLOCK_SIZE];
	for (int i = 0; i < size; i++) {
		q[0].push_back(lv[i]);

		int j = 0;
		while (q[j].size() == len){
			int _key = -1, _mac = -1;

			char plain_text[BLOCK_SIZE];
			memcpy(plain_text, &q[j][0], BLOCK_SIZE);
			char key[16];
			char mac[16];
			encrypt(plain_text, BLOCK_SIZE, &cipher_text, key, mac);
			for (int i = 0; i < BLOCK_SIZE; i++)
				output.push_back(cipher_text[i]);
			memcpy(&_key, key, 4);

			q[j + 1].push_back(lsm_kv(q[j][0].lba, _key, _mac, pos));
			pos++;
			q[j].clear();
			j++;
		}
	}

	for (int j = 0; j < MAX_LAYER; j++) {
		int _key = -1, _mac = -1;
		int padlen = len - q[j].size();
		for (int i = 0; i < padlen; i++)
			q[j].push_back(lsm_kv(-1, -1, -1, -1));
		char plain_text[BLOCK_SIZE];
		memcpy(plain_text, &q[j][0], BLOCK_SIZE);
		char key[16];
		char mac[16];
		if (j != MAX_LAYER - 1)
			encrypt(plain_text, BLOCK_SIZE, &cipher_text, key, mac);
		else {
			encrypt0(plain_text, BLOCK_SIZE, &cipher_text, mac);
		}
		for (int i = 0; i < BLOCK_SIZE; i++)
			output.push_back(cipher_text[i]);

		memcpy(&_key, key, 4);

		if (j != MAX_LAYER - 1){
            q[j + 1].push_back(lsm_kv(q[j][0].lba, _key, _mac, pos));
		}

        pos++;
        q[j].clear();
	}
	delete[] cipher_text;
	
}

int read_mhbt(int num, int pos, int lba, lsm_kv *value){
	int j;
	lsm_kv *kv;
	int len = BLOCK_SIZE / 16;
	pos = pos / len - 1;
	int _key = 0;
	for (int layer = MAX_LAYER - 1; layer >= 0; layer--) {	
		kv = (lsm_kv*)find_in_cache(pos * 100 + num, mhbt_cache);
		if (kv == nullptr){
			//printf("sst block not find in cache! %d\n", xxx);
			xxx++;
			char *plain_text;
			char buf[BLOCK_SIZE];
			sst_read_pos(num, pos, buf);
			char key[16];
			char mac[16];
			memcpy(key, &_key, 4);
			for (int i = 4; i < 16; i++)
				key[i] = 0;
			decrypt(&plain_text, BLOCK_SIZE, buf, key, mac);
			kv = (lsm_kv*)plain_text;
			char *buf2 = put_to_cache(pos * 100 + num, (char *)kv, mhbt_cache);
			if (buf2 != nullptr)
				free(buf2);
			for (j = 0; j < len; j++)
			if (kv[j].lba > lba || kv[j].lba == -1)
				break;
			if (j == 0) {
				//printf("Not found in mhbt\n");
				return 0;
			}
			pos = kv[j - 1].pba;
			_key = kv[j - 1].key;
			
			if (layer == 0){
				if (kv[j - 1].lba == lba) {
					*value = kv[j - 1];
					//printf("Found in mhbt\n");
					return 1;
				}
				else {
					//printf("Not found in mhbt\n");
					return 0;
				}
			}
			free(plain_text);
		}
		else{
			//printf("sst block find in cache!\n");
		
		int head = 0, tail = len - 1;	
		while (head < tail) {
		int mid = (head + tail) / 2;
		
		//printf("%d %d %d %d\n", head, tail, mid, kv[mid].lba);
		if (kv[mid].lba < lba && kv[mid].lba != -1)
			head = mid + 1;
		else
			tail = mid;
		}
		j = head;


		for (; j < len; j++)
			if (kv[j].lba > lba || kv[j].lba == -1)
				break;

		if (j == 0) {
			//printf("Not found in mhbt\n");
			return 0;
		}
		pos = kv[j - 1].pba;
		_key = kv[j - 1].key;
		
		if (layer == 0){
			if (kv[j - 1].lba == lba) {
				*value = kv[j - 1];
				//printf("Found in mhbt\n");
				return 1;
			}
			else {
				//printf("Not found in mhbt\n");
				return 0;
			}
		}
		}
	}
}
