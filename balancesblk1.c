#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/ec.h>

#include "block.h"
#include "common.h"
#include "transaction.h"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
/* Usage: ./balances *.blk
 * Reads in a list of block files and outputs a table of public key hashes and
 * their balance in the longest chain of blocks. In case there is more than one
 * chain of the longest length, chooses one arbitrarily. */

/* If a block has height 0, it must have this specific hash. */
const hash_output GENESIS_BLOCK_HASH = {
	0x00, 0x00, 0x00, 0x0e, 0x5a, 0xc9, 0x8c, 0x78, 0x98, 0x00, 0x70, 0x2a, 0xd2, 0xa6, 0xf3, 0xca,
	0x51, 0x0d, 0x40, 0x9d, 0x6c, 0xca, 0x89, 0x2e, 0xd1, 0xc7, 0x51, 0x98, 0xe0, 0x4b, 0xde, 0xec,
};

struct blockchain_node {
	struct blockchain_node *parent;
	struct blockchain_node *next;
	struct blockchain_node *prev;
	struct block block;
	struct transaction prev_transaction;
	// hash_output hash;
	int height;
	int is_valid;
	int is_main;
};

/* A simple linked list to keep track of account balances. */
struct balance {
	struct ecdsa_pubkey pubkey;
	int balance;
	struct balance *next;
};

/* Add or subtract an amount from a linked list of balances. Call it like this:
 *   struct balance *balances = NULL;
 *
 *   // reward_tx increment.
 *   balances = balance_add(balances, &b.reward_tx.dest_pubkey, 1);
 *
 *   // normal_tx increment and decrement.
 *   balances = balance_add(balances, &b.normal_tx.dest_pubkey, 1);
 *   balances = balance_add(balances, &prev_transaction.dest_pubkey, -1);
 */
static struct balance *balance_add(struct balance *balances,
	struct ecdsa_pubkey *pubkey, int amount)
{
	struct balance *p;

	for (p = balances; p != NULL; p = p->next) {
		if ((byte32_cmp(p->pubkey.x, pubkey->x) == 0)
			&& (byte32_cmp(p->pubkey.y, pubkey->y) == 0)) {
			p->balance += amount;
			return balances;
		}
	}

	/* Not found; create a new list element. */
	p = malloc(sizeof(struct balance));
	if (p == NULL)
		return NULL;
	p->pubkey = *pubkey;
	p->balance = amount;
	p->next = balances;

	return p;
}

int compare(const void *block1, const void *block2) 
{
	struct block b1 = *((struct block *) block1);
	struct block b2 = *((struct block *) block2);
	if (b1.height < b2.height) {
		return -1;
	} 
	if (b1.height == b2.height) {
		return 0;
	}
	if (b1.height > b2.height) {
		return 1;
	}
	return 2; // should never reach here
}

int checkAncestorTx(struct blockchain_node *curr) {
	struct blockchain_node *ancestor = curr->parent;
	while (ancestor != 0) {
		hash_output reward_tx_hash;
		hash_output normal_tx_hash;
		transaction_hash(&ancestor->block.reward_tx, reward_tx_hash);
		transaction_hash(&ancestor->block.normal_tx, normal_tx_hash);
		if (byte32_cmp(curr->block.normal_tx.prev_transaction_hash, reward_tx_hash) == 1 || byte32_cmp(curr->block.normal_tx.prev_transaction_hash, normal_tx_hash) == 1) {
			return 1;
		} else {
			ancestor = ancestor->parent;
		}
	}
	return 0;
}

int checkAncestorPrevTx(struct blockchain_node *curr) {
	struct blockchain_node *ancestor = curr->parent;
	while (ancestor != 0) {
		if (byte32_cmp(curr->block.normal_tx.prev_transaction_hash, ancestor->block.normal_tx.prev_transaction_hash) == 0 ) {
			return 1;
		} else {
			ancestor = ancestor->parent;
		}
	}
	return 0;
}

int hasValidPath(struct blockchain_node *curr) {
	struct blockchain_node *ancestor = curr->parent;
	if (curr->is_valid == 0) {
		return 0;
	}
	while (ancestor != 0) {
		if (ancestor->is_valid == 1) {
			if (ancestor->height == 0) {
				return 1;
			} else {
				ancestor = ancestor->parent;
			}
		} else {
			return 0;
		}
	}
	return 1;
}

void markMainChain(struct blockchain_node *current) {
	while (current && current != 0) {
		current->is_main = 1;
		current = current->parent;
	}
}

void findPrevTx (struct block* block, struct block* blocks, struct blockchain_node *current, int argc) {
	
	int i;
	for (i=0; i<argc; i++) {
		struct block b = blocks[i];
		hash_output reward_tx_hash;
		hash_output normal_tx_hash;
		transaction_hash(&b.reward_tx, reward_tx_hash);
		transaction_hash(&b.normal_tx, normal_tx_hash);
		if (byte32_cmp(reward_tx_hash, block->normal_tx.prev_transaction_hash) == 0) {
			current->prev_transaction = b.reward_tx;
			//printf("%u/n", current->prev_transaction.height);
			break;
		} else if (byte32_cmp(normal_tx_hash, block->normal_tx.prev_transaction_hash) == 0) {
			current->prev_transaction = b.normal_tx;
			break;
		} 
	}
}













int main(int argc, char *argv[])
{
	int i;
	struct block* blocks = malloc(argc * sizeof(struct block));

	/* Read input block files. */
	for (i = 1; i < argc; i++) {
		char *filename;
		struct block b;
		int rc;

		filename = argv[i];
		rc = block_read_filename(&b, filename);
		if (rc != 1) {
			fprintf(stderr, "could not read %s\n", filename);
			exit(1);
		}
		/* TODO */
		/* Feel free to add/modify/delete any code you need to. */
		blocks[i] = b;
	}

	qsort(&blocks[0], argc, sizeof(struct block), compare); // sort block list w.r.t height

	/* Organize into a tree, check validity, and output balances. */
	/* TODO */
	
	// /* Make a linked list of sorted blocks*/
	struct blockchain_node *root;
	struct blockchain_node *current;
	struct blockchain_node *lastNode;
	root = malloc(sizeof (struct blockchain_node));
	lastNode = malloc(sizeof(struct blockchain_node));

	int x=0;
	hash_output currentHash;
	/*Find root*/
	while (x < argc) { 
		struct block b = blocks[x];
		block_hash(&b, currentHash);
		if (blocks[x].height == 0) {
			if (byte32_cmp(GENESIS_BLOCK_HASH, currentHash) == 0) {
				root->next = 0;
				root->block = blocks[x];
				root->height = 0;
				root->parent = 0;
				root->is_valid = 1;
				current = root;
				x = argc;
				break;
			} 
			else {
				root->is_valid = 0;
				x++;
			}
		} else {
			x++;
		}
	}

	/*Build linked list*/
	for (i = 1; i < argc; i ++) {
		if (blocks[i].height != 0) {
			current->next = malloc(sizeof(struct blockchain_node));
			current->next->is_valid = 1;
			current->next->block = blocks[i];
			current->next->height = blocks[i].height;
			current->next->prev = current;
			current->next->parent = 0;
			current->next->next = 0;
	
			findPrevTx(&blocks[i], blocks, current->next, argc);
			current = current->next;
		}
	}
	lastNode = current;

	/*Build tree*/
	current = root;
	while (current != 0) {
		struct blockchain_node *currentChild = current->next;
		hash_output currHash;
		block_hash(&current->block, currHash);
		while (currentChild != 0 && (currentChild->height == current->height + 1 || currentChild->height == current->height)) {
			if (byte32_cmp(currHash, currentChild->block.prev_block_hash) == 0)  {
				currentChild->parent = current;
			}
			if (currentChild->next != 0) {
				currentChild = currentChild->next;
			} else {
				break;
			}
		}
		current = current->next;
	}

	/*Check validity*/
	current = lastNode;
	struct transaction *prev_transaction;
	prev_transaction = malloc(sizeof (struct transaction));
	while (current && current != 0) {
		hash_output currentBlockHash;
		block_hash(&current->block, currentBlockHash);
		if (hash_output_is_below_target(currentBlockHash) == 0) {
			current->is_valid = 0;
		}
		else if (!current->parent) {
			if (current->height != 0) {
				current->is_valid = 0;
			}
		}
		else if ( (current->block.reward_tx.height != current->height) || (current->block.normal_tx.height != current->height) ) {
			current->is_valid = 0;
		}
		else if ( byte32_is_zero(current->block.reward_tx.prev_transaction_hash) == 0 || byte32_is_zero(current->block.reward_tx.src_signature.r) == 0 || byte32_is_zero(current->block.reward_tx.src_signature.s) == 0 ) {
			current->is_valid = 0;
		} 
		else if (byte32_is_zero(current->block.normal_tx.prev_transaction_hash) == 0 ) {
			if (checkAncestorTx(current) == 0) {
				current->is_valid = 0;
			}
			else if (transaction_verify(&current->block.normal_tx, &current->prev_transaction) != 1) {
				current->is_valid = 0;
			}
			else if (checkAncestorPrevTx(current) == 1) {
				current->is_valid = 0;
			}
		} 
		current = current->prev;
	}

	/*Find main chain*/
	current = lastNode;
	while (current && current != 0) {
		if (hasValidPath(current) == 1) {
			markMainChain(current);
			break;
		}
		current = current->prev;
	}

	/*Add up balances*/
	current = root;
	struct balance *balances = NULL;
	while (current && current != 0) {
		if (current->is_main == 1) {
			// reward_tx increment.
			//block_print(&current->block, stdout);
			balances = balance_add(balances, &current->block.reward_tx.dest_pubkey, 1);
			 // normal_tx increment and decrement.
			if (!byte32_is_zero(current->block.normal_tx.prev_transaction_hash)) {
				balances = balance_add(balances, &current->block.normal_tx.dest_pubkey, 1);
				balances = balance_add(balances, &current->prev_transaction.dest_pubkey, -1);
			}
		}
		current = current->next;
	}
  	
	struct balance *p, *next;
	/* Print out the list of balances. */
	for (p = balances; p != NULL; p = next) {
		next = p->next;
		printf("%s %d\n", byte32_to_hex(p->pubkey.x), p->balance);
		free(p);
	}

	/*Find leaf node (of the main chain)*/
	current = lastNode;
	struct blockchain_node *block4_node;
	struct blockchain_node *leafNode;
	while (current && current != 0) {
		if ((current->is_main)) {
			leafNode =current;
			break;
		} else {
		current = current->prev;
		}
	}
	current = lastNode;
	while (current && current != 0) {
		if ((current->is_main) && (current->height == 4)){
			block4_node =current;
			break;
		}else{
		current = current->prev;
		}
	}




	// EC_KEY *block5_key = key_read_filename("block5_key.priv");

	// // BLOCK 5
	// // mine a new block that transfers the reward of block 5 to mykey.priv. build off of myblock1.blk
	// block_init(&newblock, &headblock);
	// transaction_set_dest_privkey(&newblock.reward_tx, mykey);

	// /* The last transaction was in the previous block. */
	// transaction_set_prev_transaction(&newblock.normal_tx, &newblock.normal_tx);

	// //transaction_set_dest_privkey(&newblock.normal_tx, mykey);
	// int check = transaction_sign(&newblock.normal_tx, block5_key);
	// if (check == 0) {
	// 	printf("%s\n", "transaction sign failed");
	// }

	// /* Mine the new block and save to a file. */
	// block_mine(&newblock2);
	// block_write_filename(&newblock2, "myblock2.blk");



	struct block newblock; 
	struct block headblock; 
	struct block block4;

	headblock = leafNode->block;
	block4 = block4_node->block;

	block_init(&newblock, &headblock);
	
	const EC_KEY *mykey = key_read_filename("mykey.priv");
	if (mykey == NULL) {
		printf("%s\n", "FAIL");
	}
	
 	EC_KEY *weakkey = key_read_filename("weak_key.priv");
 	if (weakkey == NULL) {
		printf("%s\n", "FAIL");
	}

 	//block_print(&headblock,stdout);
 	printf("%s\n", "NEWBLOCK");
 	//
 	//block_print(&newblock, stdout);
 	transaction_set_dest_privkey(&newblock.reward_tx, mykey);

 	transaction_set_prev_transaction(&newblock.normal_tx, &block4.normal_tx);


 	// /* Send it to us and sign it with the guessed private key */
 	transaction_set_dest_privkey(&newblock.normal_tx, mykey);
 	int check = transaction_sign(&newblock.normal_tx, weakkey);
 	if (check == 0) {
 		printf("%s\n", "transaction sign failed");
 	}

 	// /* Mine the new block and save to a file. */
 	block_mine(&newblock);
 	block_write_filename(&newblock, "myblock1.blk");

	return 0;
}


