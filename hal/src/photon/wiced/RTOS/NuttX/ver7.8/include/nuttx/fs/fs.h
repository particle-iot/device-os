/****************************************************************************
 * include/nuttx/fs/fs.h
 *
 *   Copyright (C) 2007-2009, 2011-2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_FS_FS_H
#define __INCLUDE_NUTTX_FS_FS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <sys/types.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>

#ifdef CONFIG_FS_NAMED_SEMAPHORES
#  include <nuttx/semaphore.h>
#endif

#ifndef CONFIG_DISABLE_MQUEUE
#  include <nuttx/mqueue.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Stream flags for the fs_flags field of in struct file_struct */

#define __FS_FLAG_EOF   (1 << 0) /* EOF detected by a read operation */
#define __FS_FLAG_ERROR (1 << 1) /* Error detected by any operation */

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* This structure is provided by devices when they are registered with the
 * system.  It is used to call back to perform device specific operations.
 */

struct file;   /* Forward reference */
struct pollfd; /* Forward reference */
struct inode;  /* Forward reference */

struct file_operations
{
  /* The device driver open method differs from the mountpoint open method */

  int     (*open)(FAR struct file *filep);

  /* The following methods must be identical in signature and position because
   * the struct file_operations and struct mountp_operations are treated like
   * unions.
   */

  int     (*close)(FAR struct file *filep);
  ssize_t (*read)(FAR struct file *filep, FAR char *buffer, size_t buflen);
  ssize_t (*write)(FAR struct file *filep, FAR const char *buffer, size_t buflen);
  off_t   (*seek)(FAR struct file *filep, off_t offset, int whence);
  int     (*ioctl)(FAR struct file *filep, int cmd, unsigned long arg);

  /* The two structures need not be common after this point */

#ifndef CONFIG_DISABLE_POLL
  int     (*poll)(FAR struct file *filep, struct pollfd *fds, bool setup);
#endif
  int     (*unlink)(FAR struct inode *inode);
};

/* This structure provides information about the state of a block driver */

#ifndef CONFIG_DISABLE_MOUNTPOINT
struct geometry
{
  bool   geo_available;    /* true: The device is available */
  bool   geo_mediachanged; /* true: The media has changed since last query */
  bool   geo_writeenabled; /* true: It is okay to write to this device */
  size_t geo_nsectors;     /* Number of sectors on the device */
  size_t geo_sectorsize;   /* Size of one sector */
};

/* This structure is provided by block devices when they register with the
 * system.  It is used by file systems to perform filesystem transfers.  It
 * differs from the normal driver vtable in several ways -- most notably in
 * that it deals in struct inode vs. struct filep.
 */

struct inode;
struct block_operations
{
  int     (*open)(FAR struct inode *inode);
  int     (*close)(FAR struct inode *inode);
  ssize_t (*read)(FAR struct inode *inode, FAR unsigned char *buffer,
            size_t start_sector, unsigned int nsectors);
  ssize_t (*write)(FAR struct inode *inode, FAR const unsigned char *buffer,
            size_t start_sector, unsigned int nsectors);
  int     (*geometry)(FAR struct inode *inode, FAR struct geometry *geometry);
  int     (*ioctl)(FAR struct inode *inode, int cmd, unsigned long arg);
  int     (*unlink)(FAR struct inode *inode);
};

/* This structure is provided by a filesystem to describe a mount point.
 * Note that this structure differs from file_operations ONLY in the form of
 * the open method.  Once the file is opened, it can be accessed either as a
 * struct file_operations or struct mountpt_operations
 */

struct inode;
struct fs_dirent_s;
struct stat;
struct statfs;
struct mountpt_operations
{
  /* The mountpoint open method differs from the driver open method
   * because it receives (1) the inode that contains the mountpoint
   * private data, (2) the relative path into the mountpoint, and (3)
   * information to manage privileges.
   */

  int     (*open)(FAR struct file *filep, FAR const char *relpath,
            int oflags, mode_t mode);

  /* The following methods must be identical in signature and position
   * because the struct file_operations and struct mountp_operations are
   * treated like unions.
   */

  int     (*close)(FAR struct file *filep);
  ssize_t (*read)(FAR struct file *filep, FAR char *buffer, size_t buflen);
  ssize_t (*write)(FAR struct file *filep, FAR const char *buffer,
            size_t buflen);
  off_t   (*seek)(FAR struct file *filep, off_t offset, int whence);
  int     (*ioctl)(FAR struct file *filep, int cmd, unsigned long arg);

  /* The two structures need not be common after this point. The following
   * are extended methods needed to deal with the unique needs of mounted
   * file systems.
   *
   * Additional open-file-specific mountpoint operations:
   */

  int     (*sync)(FAR struct file *filep);
  int     (*dup)(FAR const struct file *oldp, FAR struct file *newp);

  /* Directory operations */

  int     (*opendir)(FAR struct inode *mountpt, FAR const char *relpath,
            FAR struct fs_dirent_s *dir);
  int     (*closedir)(FAR struct inode *mountpt,
            FAR struct fs_dirent_s *dir);
  int     (*readdir)(FAR struct inode *mountpt,
            FAR struct fs_dirent_s *dir);
  int     (*rewinddir)(FAR struct inode *mountpt,
            FAR struct fs_dirent_s *dir);

  /* General volume-related mountpoint operations: */

  int     (*bind)(FAR struct inode *blkdriver, FAR const void *data,
            FAR void **handle);
  int     (*unbind)(FAR void *handle, FAR struct inode **blkdriver);
  int     (*statfs)(FAR struct inode *mountpt, FAR struct statfs *buf);

  /* Operations on paths */

  int     (*unlink)(FAR struct inode *mountpt, FAR const char *relpath);
  int     (*mkdir)(FAR struct inode *mountpt, FAR const char *relpath,
            mode_t mode);
  int     (*rmdir)(FAR struct inode *mountpt, FAR const char *relpath);
  int     (*rename)(FAR struct inode *mountpt, FAR const char *oldrelpath,
            FAR const char *newrelpath);
  int     (*stat)(FAR struct inode *mountpt, FAR const char *relpath,
            FAR struct stat *buf);

  /* NOTE:  More operations will be needed here to support:  disk usage
   * stats file stat(), file attributes, file truncation, etc.
   */
};
#endif /* CONFIG_DISABLE_MOUNTPOINT */

/* Named OS resources are also maintained by the VFS.  This includes:
 *
 *   - Named semaphores:     sem_open(), sem_close(), and sem_unlink()
 *   - POSIX Message Queues: mq_open() and mq_close()
 *   - Shared memory:        shm_open() and shm_unlink();
 *
 * These are a special case in that they do not follow quite the same
 * pattern as the other file system types in that they have operations.
 */

/* These are the various kinds of operations that can be associated with
 * an inode.
 */

union inode_ops_u
{
  FAR const struct file_operations      *i_ops;    /* Driver operations for inode */
#ifndef CONFIG_DISABLE_MOUNTPOINT
  FAR const struct block_operations     *i_bops;   /* Block driver operations */
  FAR const struct mountpt_operations   *i_mops;   /* Operations on a mountpoint */
#endif
#ifdef CONFIG_FS_NAMED_SEMAPHORES
  FAR struct nsem_inode_s               *i_nsem;   /* Named semaphore */
#endif
#ifndef CONFIG_DISABLE_MQUEUE
  FAR struct mqueue_inode_s             *i_mqueue; /* POSIX message queue */
#endif
};

/* This structure represents one inode in the Nuttx pseudo-file system */

struct inode
{
  FAR struct inode *i_peer;     /* Link to same level inode */
  FAR struct inode *i_child;    /* Link to lower level inode */
  int16_t           i_crefs;    /* References to inode */
  uint16_t          i_flags;    /* Flags for inode */
  union inode_ops_u u;          /* Inode operations */
#ifdef CONFIG_FILE_MODE
  mode_t            i_mode;     /* Access mode flags */
#endif
  FAR void         *i_private;  /* Per inode driver private data */
  char              i_name[1];  /* Name of inode (variable) */
};
#define FSNODE_SIZE(n) (sizeof(struct inode) + (n))

/* This is the underlying representation of an open file.  A file
 * descriptor is an index into an array of such types. The type associates
 * the file descriptor to the file state and to a set of inode operations.
 */

struct file
{
  int               f_oflags;   /* Open mode flags */
  off_t             f_pos;      /* File position */
  FAR struct inode *f_inode;    /* Driver interface */
  void             *f_priv;     /* Per file driver private data */
};

/* This defines a list of files indexed by the file descriptor */

#if CONFIG_NFILE_DESCRIPTORS > 0
struct filelist
{
  sem_t   fl_sem;               /* Manage access to the file list */
  struct file fl_files[CONFIG_NFILE_DESCRIPTORS];
};
#endif

/* The following structure defines the list of files used for standard C I/O.
 * Note that NuttX can support the standard C APIs without or without buffering
 *
 * When buffering us used, the following described the usage of the I/O buffer.
 * The buffer can be used for reading or writing -- but not both at the same time.
 * An fflush is implied between each change in directionof access.
 *
 * The field fs_bufread determines whether the buffer is being used for reading or
 * for writing as fillows:
 *
 *              BUFFER
 *     +----------------------+ <- fs_bufstart Points to the beginning of the buffer.
 *     | WR: Buffered data    |                WR: Start of buffered write data.
 *     | RD: Already read     |                RD: Start of already read data.
 *     +----------------------+
 *     | WR: Available buffer | <- fs_bufpos   Points to next byte:
 *     | RD: Read-ahead data  |                WR: End+1 of buffered write data.
 *     |                      |                RD: Points to next char to return
 *     +----------------------+
 *     | WR: Available        | <- fs_bufread  Top+1 of buffered read data
 *     | RD: Available        |                WR: =bufstart buffer used for writing.
 *     |                      |                RD: Pointer to last buffered read char+1
 *     +----------------------+
 *                              <- fs_bufend   Points to end end of the buffer+1
 */

#if CONFIG_NFILE_STREAMS > 0
struct file_struct
{
  int                fs_fd;        /* File descriptor associated with stream */
#if CONFIG_STDIO_BUFFER_SIZE > 0
  sem_t              fs_sem;       /* For thread safety */
  pid_t              fs_holder;    /* Holder of sem */
  int                fs_counts;    /* Number of times sem is held */
  FAR unsigned char *fs_bufstart;  /* Pointer to start of buffer */
  FAR unsigned char *fs_bufend;    /* Pointer to 1 past end of buffer */
  FAR unsigned char *fs_bufpos;    /* Current position in buffer */
  FAR unsigned char *fs_bufread;   /* Pointer to 1 past last buffered read char. */
#endif
  uint16_t           fs_oflags;    /* Open mode flags */
  uint8_t            fs_flags;     /* Stream flags */
#if CONFIG_NUNGET_CHARS > 0
  uint8_t            fs_nungotten; /* The number of characters buffered for ungetc */
  unsigned char      fs_ungotten[CONFIG_NUNGET_CHARS];
#endif
};

struct streamlist
{
  sem_t               sl_sem;   /* For thread safety */
  struct file_struct sl_streams[CONFIG_NFILE_STREAMS];
};
#endif /* CONFIG_NFILE_STREAMS */

/* Callback used by foreach_mountpoints to traverse all mountpoints in the
 * pseudo-file system.
 */

#ifndef CONFIG_DISABLE_MOUNTPOINT
struct statfs;                    /* Forward reference */
typedef int (*foreach_mountpoint_t)(FAR const char *mountpoint,
                                    FAR struct statfs *statbuf,
                                    FAR void *arg);
#endif

/****************************************************************************
 * Global Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* fs_inode.c ***************************************************************/
/****************************************************************************
 * Name: fs_initialize
 *
 * Description:
 *   This is called from the OS initialization logic to configure the file
 *   system.
 *
 ****************************************************************************/

void fs_initialize(void);

/* fs_foreachmountpoint.c ***************************************************/
/****************************************************************************
 * Name: foreach_mountpoint
 *
 * Description:
 *   Visit each mountpoint in the pseudo-file system.  The traversal is
 *   terminated when the callback 'handler' returns a non-zero value, or when
 *   all of the mountpoints have been visited.
 *
 *   This is just a front end "filter" to foreach_inode() that forwards only
 *   mountpoint inodes.  It is intended to support the mount() command to
 *   when the mount command is used to enumerate mounts.
 *
 *   NOTE 1: Use with caution... The pseudo-file system is locked throughout
 *   the traversal.
 *   NOTE 2: The search algorithm is recursive and could, in principle, use
 *   an indeterminant amount of stack space.  This will not usually be a
 *   real work issue.
 *
 ****************************************************************************/

#ifndef CONFIG_DISABLE_MOUNTPOINT
int foreach_mountpoint(foreach_mountpoint_t handler, FAR void *arg);
#endif

/* fs_registerdriver.c ******************************************************/
/****************************************************************************
 * Name: register_driver
 *
 * Description:
 *   Register a character driver inode the pseudo file system.
 *
 * Input parameters:
 *   path - The path to the inode to create
 *   fops - The file operations structure
 *   mode - inmode priviledges (not used)
 *   priv - Private, user data that will be associated with the inode.
 *
 * Returned Value:
 *   Zero on success (with the inode point in 'inode'); A negated errno
 *   value is returned on a failure (all error values returned by
 *   inode_reserve):
 *
 *   EINVAL - 'path' is invalid for this operation
 *   EEXIST - An inode already exists at 'path'
 *   ENOMEM - Failed to allocate in-memory resources for the operation
 *
 ****************************************************************************/

int register_driver(FAR const char *path, FAR const struct file_operations *fops,
                    mode_t mode, FAR void *priv);

/* fs_registerblockdriver.c *************************************************/
/****************************************************************************
 * Name: register_blockdriver
 *
 * Description:
 *   Register a block driver inode the pseudo file system.
 *
 * Input parameters:
 *   path - The path to the inode to create
 *   bops - The block driver operations structure
 *   mode - inmode priviledges (not used)
 *   priv - Private, user data that will be associated with the inode.
 *
 * Returned Value:
 *   Zero on success (with the inode point in 'inode'); A negated errno
 *   value is returned on a failure (all error values returned by
 *   inode_reserve):
 *
 *   EINVAL - 'path' is invalid for this operation
 *   EEXIST - An inode already exists at 'path'
 *   ENOMEM - Failed to allocate in-memory resources for the operation
 *
 ****************************************************************************/

#ifndef CONFIG_DISABLE_MOUNTPOINT
int register_blockdriver(FAR const char *path,
                         FAR const struct block_operations *bops, mode_t mode,
                         FAR void *priv);
#endif

/* fs_unregisterdriver.c ****************************************************/
/****************************************************************************
 * Name: unregister_driver
 *
 * Description:
 *   Remove the character driver inode at 'path' from the pseudo-file system
 *
 ****************************************************************************/

int unregister_driver(const char *path);

/* fs_unregisterblockdriver.c ***********************************************/
/****************************************************************************
 * Name: unregister_blockdriver
 *
 * Description:
 *   Remove the block driver inode at 'path' from the pseudo-file system
 *
 ****************************************************************************/

int unregister_blockdriver(const char *path);

/* fs_open.c ****************************************************************/
/****************************************************************************
 * Name: inode_checkflags
 *
 * Description:
 *   Check if the access described by 'oflags' is supported on 'inode'
 *
 ****************************************************************************/

int inode_checkflags(FAR struct inode *inode, int oflags);

/* fs_files.c ***************************************************************/
/****************************************************************************
 * Name: files_initlist
 *
 * Description:
 *   Initializes the list of files for a new task
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
void files_initlist(FAR struct filelist *list);
#endif

/****************************************************************************
 * Name: files_releaselist
 *
 * Description:
 *   Release a reference to the file list
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
void files_releaselist(FAR struct filelist *list);
#endif

/****************************************************************************
 * Name: file_dup2
 *
 * Description:
 *   Assign an inode to a specific files structure.  This is the heart of
 *   dup2.
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
int file_dup2(FAR struct file *filep1, FAR struct file *filep2);
#endif

/* fs_filedup.c *************************************************************/
/****************************************************************************
 * Name: fs_dupfd OR dup
 *
 * Description:
 *   Clone a file descriptor 'fd' to an arbitray descriptor number (any value
 *   greater than or equal to 'minfd'). If socket descriptors are
 *   implemented, then this is called by dup() for the case of file
 *   descriptors.  If socket descriptors are not implemented, then this
 *   function IS dup().
 *
 *   This alternative naming is used when dup could operate on both file and
 *   socket descritors to avoid drawing unused socket support into the link.
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
int fs_dupfd(int fd, int minfd);
#endif

/****************************************************************************
 * Name: file_dup
 *
 * Description:
 *   Equivalent to the non-standard fs_dupfd() function except that it
 *   accepts a struct file instance instead of a file descriptor.  Currently
 *   used only by file_vfcntl();
 *
 ****************************************************************************/

int file_dup(FAR struct file *filep, int minfd);

/* fs_filedup2.c ************************************************************/
/****************************************************************************
 * Name: fs_dupfd2 OR dup2
 *
 * Description:
 *   Clone a file descriptor to a specific descriptor number. If socket
 *   descriptors are implemented, then this is called by dup2() for the
 *   case of file descriptors.  If socket descriptors are not implemented,
 *   then this function IS dup2().
 *
 *   This alternative naming is used when dup2 could operate on both file and
 *   socket descritors to avoid drawing unused socket support into the link.
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
#if defined(CONFIG_NET) && CONFIG_NSOCKET_DESCRIPTORS > 0
int fs_dupfd2(int fd1, int fd2);
#else
#  define fs_dupfd2(fd1, fd2) dup2(fd1, fd2)
#endif
#endif

/* fs_openblockdriver.c *****************************************************/
/****************************************************************************
 * Name: open_blockdriver
 *
 * Description:
 *   Return the inode of the block driver specified by 'pathname'
 *
 * Inputs:
 *   pathname - the full path to the block driver to be opened
 *   mountflags - if MS_RDONLY is not set, then driver must support write
 *     operations (see include/sys/mount.h)
 *   ppinode - address of the location to return the inode reference
 *
 * Return:
 *   Returns zero on success or a negated errno on failure:
 *
 *   EINVAL  - pathname or pinode is NULL
 *   ENOENT  - No block driver of this name is registered
 *   ENOTBLK - The inode associated with the pathname is not a block driver
 *   EACCESS - The MS_RDONLY option was not set but this driver does not
 *     support write access
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
int open_blockdriver(FAR const char *pathname, int mountflags,
                     FAR struct inode **ppinode);
#endif

/* fs_closeblockdriver.c ****************************************************/
/****************************************************************************
 * Name: close_blockdriver
 *
 * Description:
 *   Call the close method and release the inode
 *
 * Inputs:
 *   inode - reference to the inode of a block driver opened by open_blockdriver
 *
 * Return:
 *   Returns zero on success or a negated errno on failure:
 *
 *   EINVAL  - inode is NULL
 *   ENOTBLK - The inode is not a block driver
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
int close_blockdriver(FAR struct inode *inode);
#endif

/* fs/vfs/fs_ioctl.c ********************************************************/
/****************************************************************************
 * Name: fs_ioctl
 *
 * Description:
 *   Perform device specific operations.
 *
 * Parameters:
 *   fd       File/socket descriptor of device
 *   req      The ioctl command
 *   arg      The argument of the ioctl cmd
 *
 * Return:
 *   >=0 on success (positive non-zero values are cmd-specific)
 *   -1 on failure with errno set properly:
 *
 *   EBADF
 *     'fd' is not a valid descriptor.
 *   EFAULT
 *     'arg' references an inaccessible memory area.
 *   EINVAL
 *     'cmd' or 'arg' is not valid.
 *   ENOTTY
 *     'fd' is not associated with a character special device.
 *   ENOTTY
 *      The specified request does not apply to the kind of object that the
 *      descriptor 'fd' references.
 *
 ****************************************************************************/

#ifdef CONFIG_LIBC_IOCTL_VARIADIC
int fs_ioctl(int fd, int req, unsigned long arg);
#endif

/* fs_fdopen.c **************************************************************/
/****************************************************************************
 * Name: fs_fdopen
 *
 * Description:
 *   This function does the core operations for fopen and fdopen.  It is
 *   used by the OS to clone stdin, stdout, stderr
 *
 ****************************************************************************/

#if CONFIG_NFILE_STREAMS > 0
struct tcb_s; /* Forward reference */
FAR struct file_struct *fs_fdopen(int fd, int oflags, FAR struct tcb_s *tcb);
#endif

/* libc/stdio/lib_fflush.c *************************************************/
/****************************************************************************
 * Name: lib_flushall
 *
 * Description:
 *   Called either (1) by the OS when a task exits, or (2) from fflush()
 *   when a NULL stream argument is provided.
 *
 ****************************************************************************/

#if CONFIG_NFILE_STREAMS > 0
int lib_flushall(FAR struct streamlist *list);
#endif

/* libc/misc/lib_sendfile.c *************************************************/
/****************************************************************************
 * Name: lib_sendfile
 *
 * Description:
 *   Transfer a file
 *
 ****************************************************************************/

#ifdef CONFIG_NET_SENDFILE
ssize_t lib_sendfile(int outfd, int infd, off_t *offset, size_t count);
#endif

/* fs/fs_getfilep.c *********************************************************/
/****************************************************************************
 * Name: fs_getfilep
 *
 * Description:
 *   Given a file descriptor, return the corresponding instance of struct
 *   file.  NOTE that this function will currently fail if it is provided
 *   with a socket descriptor.
 *
 * Parameters:
 *   fd - The file descriptor
 *
 * Return:
 *   A point to the corresponding struct file instance is returned on
 *   success.  On failure,  NULL is returned and the errno value is
 *   set appropriately (EBADF).
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
FAR struct file *fs_getfilep(int fd);
#endif

/* fs/fs_read.c *************************************************************/
/****************************************************************************
 * Name: file_read
 *
 * Description:
 *   Equivalent to the standard read() function except that is accepts a
 *   struct file instance instead of a file descriptor.  Currently used
 *   only by net_sendfile() and aio_read();
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
ssize_t file_read(FAR struct file *filep, FAR void *buf, size_t nbytes);
#endif

/* fs/fs_write.c ************************************************************/
/****************************************************************************
 * Name: file_write
 *
 * Description:
 *   Equivalent to the standard write() function except that is accepts a
 *   struct file instance instead of a file descriptor.  Currently used
 *   only by aio_write();
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
ssize_t file_write(FAR struct file *filep, FAR const void *buf, size_t nbytes);
#endif

/* fs/fs_pread.c ************************************************************/
/****************************************************************************
 * Name: file_pread
 *
 * Description:
 *   Equivalent to the standard pread function except that is accepts a
 *   struct file instance instead of a file descriptor.  Currently used
 *   only by aio_read();
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
ssize_t file_pread(FAR struct file *filep, FAR void *buf, size_t nbytes,
                   off_t offset);
#endif

/* fs/fs_pwrite.c ***********************************************************/
/****************************************************************************
 * Name: file_pwrite
 *
 * Description:
 *   Equivalent to the standard pwrite function except that is accepts a
 *   struct file instance instead of a file descriptor.  Currently used
 *   only by aio_write();
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
ssize_t file_pwrite(FAR struct file *filep, FAR const void *buf,
                    size_t nbytes, off_t offset);
#endif

/* fs/fs_lseek.c ************************************************************/
/****************************************************************************
 * Name: file_seek
 *
 * Description:
 *   Equivalent to the standard lseek() function except that is accepts a
 *   struct file instance instead of a file descriptor.  Currently used
 *   only by net_sendfile()
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
off_t file_seek(FAR struct file *filep, off_t offset, int whence);
#endif

/* fs/fs_fsync.c ************************************************************/
/****************************************************************************
 * Name: file_fsync
 *
 * Description:
 *   Equivalent to the standard fsync() function except that is accepts a
 *   struct file instance instead of a file descriptor.  Currently used
 *   only by aio_fsync();
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
int file_fsync(FAR struct file *filep);
#endif

/* fs/fs_fcntl.c ************************************************************/
/****************************************************************************
 * Name: file_vfcntl
 *
 * Description:
 *   Similar to the standard vfcntl function except that is accepts a struct
 *   struct file instance instead of a file descriptor.  Currently used
 *   only by aio_fcntl();
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0
int file_vfcntl(FAR struct file *filep, int cmd, va_list ap);
#endif

/* drivers/dev_null.c *******************************************************/
/****************************************************************************
 * Name: devnull_register
 *
 * Description:
 *   Register /dev/null
 *
 ****************************************************************************/

void devnull_register(void);

/* crypto/cryptodev.c *******************************************************/
/****************************************************************************
 * Name: devcrypto_register
 *
 * Description:
 *   Register /dev/crypto
 *
 ****************************************************************************/

void devcrypto_register(void);

/* drivers/dev_zero.c *******************************************************/
/****************************************************************************
 * Name: devzero_register
 *
 * Description:
 *   Register /dev/zero
 *
 ****************************************************************************/

void devzero_register(void);

/* drivers/loop.c ***********************************************************/
/****************************************************************************
 * Name: losetup
 *
 * Description:
 *   Setup the loop device so that it exports the file referenced by 'filename'
 *   as a block device.
 *
 ****************************************************************************/

int losetup(FAR const char *devname, FAR const char *filename,
            uint16_t sectsize, off_t offset, bool readonly);

/****************************************************************************
 * Name: loteardown
 *
 * Description:
 *   Undo the setup performed by losetup
 *
 ****************************************************************************/

int loteardown(FAR const char *devname);

/* drivers/bch/bchdev_register.c ********************************************/
/****************************************************************************
 * Name: bchdev_register
 *
 * Description:
 *   Setup so that it exports the block driver referenced by 'blkdev' as a
 *   character device 'chardev'
 *
 ****************************************************************************/

int bchdev_register(FAR const char *blkdev, FAR const char *chardev,
                    bool readonly);

/* drivers/bch/bchdev_unregister.c ******************************************/
/****************************************************************************
 * Name: bchdev_unregister
 *
 * Description:
 *   Unregister character driver access to a block device that was created
 *   by a previous call to bchdev_register().
 *
 ****************************************************************************/

int bchdev_unregister(FAR const char *chardev);

/* Low level, direct access.  NOTE:  low-level access and character driver access
 * are incompatible.  One and only one access method should be implemented.
 */

/* drivers/bch/bchlib_setup.c ***********************************************/
/****************************************************************************
 * Name: bchlib_setup
 *
 * Description:
 *   Setup so that the block driver referenced by 'blkdev' can be accessed
 *   similar to a character device.
 *
 ****************************************************************************/

int bchlib_setup(FAR const char *blkdev, bool readonly, FAR void **handle);

/* drivers/bch/bchlib_teardown.c ********************************************/
/****************************************************************************
 * Name: bchlib_teardown
 *
 * Description:
 *   Setup so that the block driver referenced by 'blkdev' can be accessed
 *   similar to a character device.
 *
 ****************************************************************************/

int bchlib_teardown(FAR void *handle);

/* drivers/bch/bchlib_read.c ************************************************/
/****************************************************************************
 * Name: bchlib_read
 *
 * Description:
 *   Read from the block device set-up by bchlib_setup as if it were a
 *   character device.
 *
 ****************************************************************************/

ssize_t bchlib_read(FAR void *handle, FAR char *buffer, size_t offset,
                    size_t len);

/* drivers/bch/bchlib_write.c ***********************************************/
/****************************************************************************
 * Name: bchlib_write
 *
 * Description:
 *   Write to the block device set-up by bchlib_setup as if it were a
 *   character device.
 *
 ****************************************************************************/

ssize_t bchlib_write(FAR void *handle, FAR const char *buffer, size_t offset,
                     size_t len);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __INCLUDE_NUTTX_FS_FS_H */
