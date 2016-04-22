#ifndef TFS_I_H
#define TFS_I_H

#include <linux/fs.h>

struct tfs_inode {
        struct inode inode;
        int is_directory;
        int is_tagfile;
        char *filename;
        u64 nodeid;
        
        char **tag;
        int ntag;
};

struct tfs_file {
        u64 kh;
        u64 fh;

        u64 true_kh;
};


static inline struct tfs_inode *get_tfs_inode(struct inode *inode)
{
	return container_of(inode, struct tfs_inode, inode);
}

static inline u64 get_node_id(struct inode *inode)
{
	return get_tfs_inode(inode)->nodeid;
}


#endif //TFS_I_H
