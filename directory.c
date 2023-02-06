#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "directory.h"
#include "bitmap.h"
#include "inode.h"
#include "slist.h"

//initializes the directory by initializing a root node
void directory_init(){
  inode_t* root_node = get_inode(alloc_inode());
  root_node->mode = 040755; // directory
}

//attempts to find directory given by name
int directory_lookup(inode_t *dd, const char *name){
  // root directory, return 0;
  if (!strcmp(name, "")){
    return 0;
  }
  dirent_t* dirs = blocks_get_block(dd->block);
  for (int i = 0; i < 64; i++){
    if (!strcmp(name, dirs[i].name)){
      return dirs[i].inum;
    }
  }
  return -ENOENT; // No such directory, return -ENOENT
}

//looks up a tree of directories using *path, with / between directories
int tree_lookup(const char *path){
  slist_t* path_slist = s_explode(path, '/');
  slist_t* pt_slist = path_slist;
  int inum = 0;
  while (pt_slist){
    inum = directory_lookup(get_inode(inum), pt_slist->data);
    if (inum < 0){
      s_free(path_slist);
      printf("Debug: tree_lookup(%s), return %d\n", path, inum);
      return -ENOENT; // No such directory, return -ENOENT
    }
    pt_slist = pt_slist->next;
  }
  s_free(path_slist);
  
  printf("Debug: tree_lookup(%s), return %d\n", path, inum);
  return inum;
}

//puts a directory into another directory
int directory_put(inode_t *dd, const char *name, int inum){
  printf("Debug: directory_put(%s, %d)\n", name, inum);
  dirent_t new_dir;
  strncpy(new_dir.name, name, DIR_NAME_LENGTH);
  new_dir.inum = inum;
  int directories = dd->size / sizeof(dirent_t);
  dirent_t* dirs = blocks_get_block(dd->block);
  dirs[directories] = new_dir;
  dd->size += sizeof(dirent_t);
  return 0;
}

//prepares a directory for deletion, then deletes it
int directory_delete(inode_t *dd, const char *name){
  int directories = dd->size / sizeof(dirent_t);
  dirent_t* dirs = blocks_get_block(dd->block);
  for (int i = 0; i < directories; i++){
    if (!strcmp(dirs[i].name, name)){
      // decrease inode.refs
      inode_t* node = get_inode(dirs[i].inum);
      node->refs--;
      if (node->refs <= 0){
	free_inode(dirs[i].inum);
      }
      // shrink dirs
      for (int j = i; j < directories - 1; j++){
	dirs[j] = dirs[j + 1];
	dd->size -= sizeof(dirent_t);
	return 0;
      }
    }
  }
  return -ENOENT; // No such directory, return -ENOENT
}

//lists all directories in a path
slist_t *directory_list(const char *path){
  // search (and destroy)
  int inum = tree_lookup(path);
  inode_t* node = get_inode(inum);

  dirent_t* fulldirs = blocks_get_block(node->block);
  int directories = node->size / sizeof(dirent_t);  
  // iterate and add, then print/return
  slist_t* rv = NULL;
  for (int i = 0; i < directories; i++){
    rv = s_cons(fulldirs[i].name, rv);
  }
  return rv;
}

// prints out the directory info
void print_directory(inode_t *dd){
  int directories = dd->size / sizeof(dirent_t);
  dirent_t* dirs = blocks_get_block(dd->block);
  for (int i = 0; i < directories; i++){
    printf("Dir %d:\n", i);
    printf("Name: %s\n", dirs[i].name);
  }
}
