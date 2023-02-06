#include <stdio.h>
#include <math.h>

#include "inode.h"
#include "blocks.h"
#include "bitmap.h"

int block_list[256];

//prints the information in the given node
void print_inode(inode_t* node){
  // data dump
  printf("inode.refs = %d\n", node->refs);
  printf("inode.mode = %d\n", node->mode);
  printf("inode.size = %d\n", node->size);
  printf("inode.block = %d\n", node->block);
  int block = node->block;
  while (block != block_list[block]) {
    block = block_list[block];
    printf("\tblock_list = %d\n", block);
  }
}

//gets the inode that corresponds to the number inum
inode_t* get_inode(int inum){
  // get all data, pull specific idx
  inode_t* all_nodes = get_inode_bitmap() + BLOCK_BITMAP_SIZE;
  return &all_nodes[inum];
}

//creates and returns a new node
int alloc_inode(){
  // hello bmp
  void *bmp = get_inode_bitmap();

  // get inum
  int inum = 0;
  for (int i = 0; i < BLOCK_COUNT; ++i) {
    if (!bitmap_get(bmp, i)) {
      bitmap_put(bmp, i, 1);
      inum = i;
      break;
    }
  }

  // create node
  inode_t* node = get_inode(inum);
  node->refs = 1;
  node->mode = 0;
  node->size = 0;
  node->block = alloc_block();
  
  return inum;
}

//frees the inode represented by inum from use
void free_inode(int inum){
  // same idea as allocator free
  inode_t* node = get_inode(inum);
  shrink_inode(node, 0);
  free_block(node->block);
  
  void *bmp = get_inode_bitmap();
  bitmap_put(bmp, inum, 0);
}

//increases the size of an inode node
int grow_inode(inode_t* node, int size){
  int end = node->block;
  while (end != block_list[end])
    end = block_list[end];
  
  int grow_num = (size - 1) / BLOCK_SIZE - (node->size - 1) / BLOCK_SIZE;
  for (int i = 0; i < grow_num; i++){
      int inum = alloc_block();
      block_list[end] = inum;
      end = inum;
    }
  
  node->size = size;
  return 0;
}

//decreases the size of an inode node
int shrink_inode(inode_t* node, int size){
  // num of blocks
  int bnum = size / BLOCK_SIZE;

  // iterate
  int i = 0;
  int block = node->block;
  while (block != block_list[block]){
    if (i > bnum){
      // free
      free_block(block);
      int t = block_list[block];
      block = t;
    }
    i++;
  }

  // update
  node->size = size;
  return 0;
}

//gets the block number of an node
int inode_get_bnum(inode_t* node, int fpn){
  // num of blocks
  int bnum = fpn / BLOCK_COUNT;
  // iterate and repopulate
  int block = node->block;
  for (int i = 0; i < bnum; i++){ 
    block = block_list[block];
  }
  return block;
}
