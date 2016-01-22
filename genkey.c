#include <stdio.h>
#include <string.h>

#include <openssl/bn.h>
#include <openssl/ec.h>

#include "common.h"

/* Usage: genkey FILENAME
 * Generate a key and write it to the file FILENAME. */

/* Interpret the 256 bits in buf as a private key and return an EC_KEY *. */
static EC_KEY *generate_key_from_buffer(const unsigned char buf[32])
{
	EC_KEY *key;
	BIGNUM *bn;
	int rc;

	key = NULL;
	bn = NULL;

	key = EC_KEY_new_by_curve_name(EC_GROUP_NID);
	if (key == NULL)
		goto err;

	bn = BN_bin2bn(buf, 32, NULL);
	if (bn == NULL)
		goto err;

	rc = EC_KEY_set_private_key(key, bn);
	if (rc != 1)
		goto err;

	BN_free(bn);

	return key;

err:
	if (key != NULL)
		EC_KEY_free(key);
	if (bn != NULL)
		BN_free(bn);
	return NULL;
}

/* Generate a key using EC_KEY_generate_key. */
static EC_KEY *generate_key(void)
{
	EC_KEY *key;
	int rc;

	key = EC_KEY_new_by_curve_name(EC_GROUP_NID);
	if (key == NULL)
		printf("%s\n", "null key");
		return NULL;
	rc = EC_KEY_generate_key(key);
	if (rc != 1) {
		EC_KEY_free(key);
		return NULL;
	}

	return key;
}

// generate a matching key for block at height 4
static EC_KEY *find_key_block4(void) {
	unsigned char block4_buf[32];
	int x = 0;
	srand(1234);
	for (x = 0; x < 32; x++) {
		block4_buf[x] = rand() & 0xff;
	}
	EC_KEY *block4_key;
	//printf("%s\n", "made a new key");
	block4_key = generate_key_from_buffer(block4_buf);
	
	return block4_key;	
}

// find the key for block 5
static EC_KEY *find_key_block5(void){
	unsigned char key5_buf[32];
	int y;
	int curr_time = 1443701272;	//epoch time for a little before 10/01/2015
	srand(curr_time);
	for (y = 0; y <32; y++) {
		key5_buf[y] = rand() & 0xff;
	}
	EC_KEY *generated_key;
	generated_key = generate_key_from_buffer(key5_buf);

	// while (generated_key != block5_key) {
	// 	curr_time++;
	// 	srand(curr_time);
	// 	for (y = 0; y <32; y++) {
	// 		key5_buf[y] = rand() & 0xff;
	// 	}
	// 	generated_key = generate_key_from_buffer(key5_buf);

	return generated_key;
}




int main(int argc, char *argv[])
{
	// const char *filename;
	// EC_KEY *key;
	// int rc;

	// if (argc != 2) {
	// 	fprintf(stderr, "need an output filename\n");
	// 	exit(1);
	// }

	// filename = argv[1];

	// key = generate_key();
	// if (key == NULL) {
	// 	fprintf(stderr, "error generating key\n");
	// 	exit(1);
	// }

	// rc = key_write_filename(filename, key);
	// if (rc != 1) {
	// 	fprintf(stderr, "error saving key\n");
	// 	exit(1);
	// }	

	EC_KEY *weak_key = find_key_block4();
	int rc4 = key_write_filename("weak_key.priv", weak_key);
	if (rc4 != 1) {
		fprintf(stderr, "error saving weak key \n");
		exit(1);
	}

	EC_KEY *block5 = find_key_block5();
	int rc5 = key_write_filename("block5_key.priv", block5);
	if (rc5 != 1) {
		fprintf(stderr, "error saving key block 5 \n");
		exit(1);
	}

	printf("%s\n", "made it through genkey");

	return 0;
}
