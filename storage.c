#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "storage.h"
#include "blocks.h"
#include "bitmap.h"
#include "inode.h"
#include "directory.h"

static int min(int a, int b);
static void setup_hierarchy(const char* path, char* parent, char* child);

//returns the smallest of a b
static int min(int a, int b) {
  return a > b ? b : a;
}

//initializes storage
void storage_init(const char* path){
  blocks_init(path);
  
    if (!bitmap_get(get_blocks_bitmap(), 1))
      for (int i = 0; i < 3; i++){
	int new_block = alloc_block();
	printf("alloc inode block: %d\n", new_block);
      }
    
    if (!bitmap_get(get_blocks_bitmap(), 4)){
      printf("initializing root directory\n");
      directory_init();
    }
}

//sets up stat read dir for nufs
int storage_stat(const char *path, struct stat *st){
  printf("Debug: storage_stat(%s)\n", path);
  int inum = tree_lookup(path);
  if (inum > 0){
    inode_t* node = get_inode(inum);
    st->st_size = node->size;
        st->st_mode = node->mode;
        st->st_nlink = node->refs;
        return 0;
  }
  return -ENOENT; // No such directory, return -ENOENT
}

//reads from a file at a destination
int storage_read(const char *path, char *buf, size_t size, off_t offset){
  inode_t* node = get_inode(tree_lookup(path));
  
  int buf_index = 0;
    int src_index = offset;
    int read_size = size;
    
    while (read_size > 0){
      char* src = blocks_get_block(inode_get_bnum(node, src_index));
      src += src_index % BLOCK_SIZE;
      int copy_size = min(read_size, BLOCK_SIZE - (src_index % BLOCK_SIZE));
      memcpy(buf + buf_index, src, copy_size);
      buf_index += copy_size;
      src_index += copy_size;
      read_size -= copy_size;
    }

    printf("Debug: storage_read(%s, \"%s\", %ld, %ld)\n", path, buf, size, offset);
    return size;
}

//writes to a file ad a destination
int storage_write(const char *path, const char *buf, size_t size, off_t offset){
  inode_t* node = get_inode(tree_lookup(path));
  if (node->size < size + offset)
      storage_truncate(path, size + offset);
  
  int buf_index = 0;
  int dest_index = offset;
  int write_size = size;
  
    while (write_size > 0){
      char* dest = blocks_get_block(inode_get_bnum(node, dest_index));
      dest += dest_index % BLOCK_SIZE;
      int copy_size = min(write_size, BLOCK_SIZE - (dest_index % BLOCK_SIZE));
      memcpy(dest, buf + buf_index, copy_size);
      buf_index += copy_size;
      dest_index += copy_size;
      write_size -= copy_size;
    }
    
    printf("Debug: storage_write(%s, \"%s\", %ld, %ld)\n", path, buf, size, offset);
    return size;
}

//implements the truncate call
int storage_truncate(const char *path, off_t size){
    printf("Debug: storage_truncate(%s, %ld)\n", path, size);
    inode_t* node = get_inode(tree_lookup(path));
    if (node->size < size)
      grow_inode(node, size);
    else
      shrink_inode(node, size);
    return 0;
}

//implements the mknod system call
int storage_mknod(const char *path, int mode){
  printf("Debug: storage_mknod(%s, %d)\n", path, mode);
  // check to make sure the node doesn't already exist
  if (tree_lookup(path) != -ENOENT)
    return -EEXIST;
  
  char* child = (char*)malloc(DIR_NAME_LENGTH + 2);
  char* parent = (char*)malloc(strlen(path));
  setup_hierarchy(path, parent, child);
  
  int parent_inum = tree_lookup(parent);
  if (parent_inum < 0){
    free(child);
    free(parent);
    return -ENOENT;
  }
  inode_t* parent_node = get_inode(parent_inum);
  
  int new_inum = alloc_inode();
  inode_t* node = get_inode(new_inum);
  node->mode = mode;
  node->size = 0;
  node->refs = 1;
  
  directory_put(parent_node, child, new_inum);
  free(child);
  free(parent);
  return 0;
}

//implements the unlink call
int storage_unlink(const char *path){
  printf("Debug: storage_unlink(%s)\n", path);
  char* child = (char*)malloc(DIR_NAME_LENGTH + 2);
  char* parent = (char*)malloc(strlen(path));
  setup_hierarchy(path, parent, child);
  
  inode_t* parent_node = get_inode(tree_lookup(parent));
  int ret = directory_delete(parent_node, child);
  
  free(child);
  free(parent);
  
  return ret;
}

//implements the link call
int storage_link(const char *from, const char *to){
  printf("Debug: storage_link(%s, %s)\n", from, to);
  int inum = tree_lookup(to);
  if (inum < 0)
    return inum;
  
  char* child = (char*)malloc(DIR_NAME_LENGTH + 2);
  char* parent = (char*)malloc(strlen(from));
  setup_hierarchy(from, parent, child);
  
  inode_t* parent_node = get_inode(tree_lookup(parent));
  directory_put(parent_node, child, inum);
  get_inode(inum)->refs++;
  
  free(child);
  free(parent);
  
  return 0;
}

//implements rename call
int storage_rename(const char *from, const char *to){
  printf("Debug: storage_rename(%s, %s)\n", from, to);
  storage_link(to, from);
  storage_unlink(from);
  return 0;
}


slist_t *storage_list(const char *path){
  return directory_list(path);
}

//sets up parent and child files/directories relationship
static void setup_hierarchy(const char* path, char* parent, char* child){
  slist_t* path_slist = s_explode(path, '/');
  slist_t* pt_slist = path_slist;
  parent[0] = '\0';
  while (pt_slist->next){
    strncat(parent, "/", 2);
    strncat(parent, pt_slist->data, DIR_NAME_LENGTH);
    pt_slist = pt_slist->next;
  }
  memcpy(child, pt_slist->data, strlen(pt_slist->data));
  child[strlen(pt_slist->data)] = '\0';
  s_free(path_slist);
}
