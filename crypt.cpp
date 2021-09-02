#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <chrono>

using std::string;
using std::vector;
using std::cout;
using std::endl;
using namespace std;
using namespace chrono;


EVP_CIPHER_CTX * e_ctx = nullptr;
EVP_CIPHER_CTX *d_ctx = nullptr;

void aes_init()
{
	static int init = 0;
	if (init == 0)
	{
		e_ctx = EVP_CIPHER_CTX_new();
		d_ctx = EVP_CIPHER_CTX_new();

		//initialize openssl ciphers
		OpenSSL_add_all_ciphers();

		//initialize random number generator (for IVs)
		int rv = RAND_load_file("/dev/urandom", 32);

		init++;
	}
	
}

void aes_release()
{
	EVP_CIPHER_CTX_free(e_ctx);
    EVP_CIPHER_CTX_free(d_ctx);
}

int  inencrypt(unsigned char *plaintext, int plaintext_len, 
						// unsigned char *aad,int aad_len,
						unsigned char *tag, 
						unsigned char *key,
						unsigned char *iv,
   						unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;
	/* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) ;

    /* Initialise the encryption operation. */
    if(!EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL));

	/* Set IV length. Not necessary if this is 12 bytes (96 bits) */
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, NULL));

    /* Initialise key and IV */
    if(!EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) ;

	/* Provide any AAD data. This can be called zero or more times as
     * required
     */
    // if(!EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len)) ;

    /* Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if(!EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))  ;
    ciphertext_len = len;

 /* Finalise the decryption. A positive return value indicates success,
     * anything else is a failure - the plaintext is not trustworthy.
     */
EVP_EncryptFinal_ex(ctx, ciphertext+len, &len);
ciphertext_len += len;
    /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
       ;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
        return ciphertext_len;
}

int indecrypt(unsigned char *ciphertext, int ciphertext_len, 
							// unsigned char *aad,int aad_len, 
							unsigned char *tag, 
							unsigned char *key, 
							unsigned char *iv,
   							unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;
	/* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()));

    /* Initialise the decryption operation. */
    if(!EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL)) ;

/* Set IV length. Not necessary if this is 12 bytes (96 bits) */
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, NULL));

    /* Initialise key and IV */
    if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) ;


	/* Provide any AAD data. This can be called zero or more times as
     * required
     */
    // if(!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len)) ;

    /* Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) ;
    plaintext_len = len;

    /* Set expected tag value. Works in OpenSSL 1.0.1d and later */

    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag)) ;

    /* Finalise the decryption. A positive return value indicates success,
     * anything else is a failure - the plaintext is not trustworthy.
     */
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
	if(ret > 0)
    {
        /* Success */
        plaintext_len += len;
        return plaintext_len;
    }
    else
    {
        /* Verify failed */
		  plaintext_len += len;
        return plaintext_len;
        // return -1;
    }
}

void aes_128_gcm_encrypt0(const char* plaintext, size_t plaintext_len,
				const char * key,
				unsigned char ** out, size_t & out_len) {

	size_t output_length = AES_BLOCK_SIZE + AES_BLOCK_SIZE + plaintext_len;
	unsigned char *output = (unsigned char*)malloc(output_length);
	for (int i = 0; i < output_length; i++)
		output[i] = 0;
	//RAND_bytes(output+16, 16);

	int actual_size = 0, final_size = 0;
	
	EVP_EncryptInit(e_ctx, EVP_aes_128_gcm(), (const unsigned char*)key, output + 16);
	EVP_EncryptUpdate(e_ctx, &output[32], &actual_size, (const unsigned char*)plaintext, plaintext_len);
	EVP_EncryptFinal(e_ctx, &output[32 + actual_size], &final_size);
	EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_GET_TAG, 16, output);

	// EVP_EncryptInit_ex(e_ctx, EVP_aes_128_gcm(),nullptr,nullptr,nullptr,Mode::Encrypt);
	// EVP_EncryptUpdate(e_ctx, &output[32], &actual_size, (const unsigned char*)plaintext, plaintext_len);
	// EVP_EncryptFinal(e_ctx, &output[32 + actual_size], &final_size);
	// EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_GET_TAG, 16, output);

	*out = output;
	out_len = output_length;
}

void encrypt0(char *text, size_t text_len, char **cipher_text, char *mac)
{
	char key[16];
	for (int i = 0; i < 16; i++)
		key[i] = 0;
	size_t out_len;
	char* _cipher;
	aes_128_gcm_encrypt0(text, text_len, key, (unsigned char**)&_cipher, out_len);
	memcpy(mac, _cipher, 16);
	memcpy(*cipher_text, _cipher + 32, text_len);
	free(_cipher);
}

// void encrypt(char *text, size_t text_len, char **cipher_text, char *key, char *mac)
// {
// 	RAND_bytes((unsigned char*)key, 4);
// 	for (int i = 4; i < 16; i++)
// 		key[i] = 0;
// 	size_t out_len;
// 	char* _cipher;


// 	aes_128_gcm_encrypt0(text, text_len, key, (unsigned char**)&_cipher, out_len);
// 	memcpy(mac, _cipher, 16);
// 	memcpy(*cipher_text, _cipher + 32, text_len);
// 	free(_cipher);
// }
void encrypt(unsigned char* plain, size_t plain_len,
							unsigned char* cipher_text, 
							unsigned char * key, 
							unsigned char* mac)
{
	size_t out_len;
	char* _cipher;
	unsigned char* iv  = (unsigned char*)"000000000000";
	inencrypt( plain,plain_len,mac,key,iv,cipher_text);
}

void decrypt(unsigned char* plain, 
							unsigned char* cipher_text, size_t cipher_len,
							unsigned char * key, 
							unsigned char* mac)
{
	size_t out_len;
	char* _cipher;
	unsigned char* iv  = (unsigned char*)"000000000000";
	indecrypt( cipher_text,cipher_len,mac,key,iv,plain);
}

int aes_128_gcm_decrypt0(const char* ciphertext, size_t ciphertext_len,
				const char * key,
				unsigned char ** out, size_t & out_len) {

	unsigned char *plaintext = new unsigned char[ciphertext_len - 32];

	int actual_size = 0, final_size = 0;
	EVP_DecryptInit(d_ctx, EVP_aes_128_gcm(), (const unsigned char*)key, (unsigned char*)ciphertext+16);
	EVP_DecryptUpdate(d_ctx, plaintext, &actual_size, (const unsigned char*)&ciphertext[32], ciphertext_len - 32);
	EVP_CIPHER_CTX_ctrl(d_ctx, EVP_CTRL_GCM_SET_TAG, 16, (char*)ciphertext);
	int ret = EVP_DecryptFinal(d_ctx, &plaintext[actual_size], &final_size);

	*out = plaintext;
	out_len = ciphertext_len - 32;
	return ret;
}

// int decrypt(char **text, size_t text_len, char *cipher_text, char *key, char *mac)
// {
// 	size_t out_len;
// 	char* _cipher = (char *)malloc(text_len + 32);
// 	memcpy(_cipher, mac, 16);
// 	for (int i = 0; i < 16; i++)
// 		_cipher[i + 16] = 0;
// 	memcpy(_cipher + 32, cipher_text, text_len);
// 	int ret = aes_128_gcm_decrypt0(_cipher, text_len + 32, key, (unsigned char**)text, out_len);
// 	free(_cipher);
// 	return ret;
// }
