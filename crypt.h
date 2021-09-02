#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

#include <chrono>

void aes_init();
void aes_release();
// void encrypt(char *text, size_t text_len, char **cipher_text, char *key, char *mac);
// int decrypt(char **text, size_t text_len, char *cipher_text, char *key, char *mac);
void encrypt(unsigned char* plain, size_t plain_len,
							unsigned char* cipher_text, 
							unsigned char * key, 
							unsigned char* mac);
void decrypt(unsigned char* plain, 
							unsigned char* cipher_text, size_t cipher_len,
							unsigned char * key, 
							unsigned char* mac);

int  inencrypt(unsigned char *plaintext, int plaintext_len, 
						// unsigned char *aad,int aad_len,
						unsigned char *tag, 
						unsigned char *key,
						unsigned char *iv,
   						unsigned char *ciphertext);
int indecrypt(unsigned char *ciphertext, int ciphertext_len, 
							// unsigned char *aad,int aad_len, 
							unsigned char *tag, 
							unsigned char *key, 
							unsigned char *iv,
   							unsigned char *plaintext)			;		   



void encrypt0(char *text, size_t text_len, char **cipher_text, char *mac);
int aes_128_gcm_decrypt0(const char* ciphertext, size_t ciphertext_len,
				const char * key,
				unsigned char ** out, size_t & out_len);