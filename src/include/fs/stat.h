#ifndef XJOS_STAT_H
#define XJOS_STAT_H

#include <xjos/types.h>

// File types
#define IFMT 00170000 // File type mask (Octal representation)
#define IFREG 0100000 // Regular file
#define IFBLK 0060000 // Block special (device) file, e.g., disk dev/fd0
#define IFDIR 0040000 // Directory file
#define IFCHR 0020000 // Character device file
#define IFIFO 0010000 // FIFO special file
#define IFSYM 0120000 // Symbolic link

// File mode flags:
// ISUID: Tests if the set-user-ID flag is set.
// If set, the process's effective user ID is set to the file owner's user ID upon execution.
// ISGID: Performs the same action for the group ID.
#define ISUID 0004000 // Set user ID upon execution (set-user-ID)
#define ISGID 0002000 // Set group ID upon execution (set-group-ID)

// If set on a directory, restricts deletion for non-file owners.
#define ISVTX 0001000 // Restricted deletion flag (sticky bit)

#define ISREG(m) (((m)&IFMT) == IFREG)  // Is a regular file
#define ISDIR(m) (((m)&IFMT) == IFDIR)  // Is a directory file
#define ISCHR(m) (((m)&IFMT) == IFCHR)  // Is a character device file
#define ISBLK(m) (((m)&IFMT) == IFBLK)  // Is a block device file
#define ISFIFO(m) (((m)&IFMT) == IFIFO) // Is a FIFO special file
#define ISSYM(m) (((m)&IFMT) == IFSYM)  // Is a symbolic link file

// File access permissions
#define IRWXU 00700 // Owner Read, Write, Execute/Search
#define IRUSR 00400 // Owner Read permission
#define IWUSR 00200 // Owner Write permission
#define IXUSR 00100 // Owner Execute/Search permission

#define IRWXG 00070 // Group Read, Write, Execute/Search
#define IRGRP 00040 // Group Read permission
#define IWGRP 00020 // Group Write permission
#define IXGRP 00010 // Group Execute/Search permission

#define IRWXO 00007 // Others Read, Write, Execute/Search
#define IROTH 00004 // Others Read permission
#define IWOTH 00002 // Others Write permission
#define IXOTH 00001 // Others Execute/Search permission

typedef struct stat_t
{
    dev_t dev;    // Device number containing the file
    idx_t nr;     // File inode number
    u16 mode;     // File type and mode bits
    u8 nlinks;    // Number of hard links to the file
    u16 uid;      // User ID of the file owner
    u8 gid;       // Group ID of the file owner
    dev_t rdev;   // Device ID (if file is a special char or block file)
    size_t size;  // File size in bytes (if file is a regular file)
    time_t atime; // Time of last access
    time_t mtime; // Time of last modification
    time_t ctime; // Time of last status change (inode change)
} stat_t;

#endif