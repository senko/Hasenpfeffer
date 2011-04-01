/*****************************************************************
 * Source file : DirectoryService.idl
 * Platform    : V4 IA32
 * Mapping     : CORBA C
 * 
 * Generated by IDL4 1.0.2 (roadrunner) on 13/01/2006 06:41
 * Report bugs to haeberlen@ira.uka.de
 *****************************************************************/

#define IDL4_ARCH ia32
#define IDL4_API v4
#define IDL4_OMIT_FRAME_POINTER 0

#include <l4io.h>

#include <stdlib.h>
#include <string.h>
#include <l4/kdebug.h>
#include <idl4/idl4.h>
#include <capability.h>
#include <dir.h>

#include "ext2fs.h"

#include <server/DirectoryService.h>
#include <server/File.h>

L4_Word_t my_secret;

/* Interface DirectoryService */

IDL4_INLINE CORBA_boolean DirectoryService_RegisterPath_implementation(CORBA_Object _caller, const Capability_t *cap, const CORBA_char *path, const Capability_t *object, Capability_t *newcap, idl4_server_environment *_env)

{
  CORBA_boolean __retval = 0;

  /* implementation of DirectoryService::RegisterPath */
  
  return __retval;
}

IDL4_PUBLISH_DIRECTORYSERVICE_REGISTERPATH(DirectoryService_RegisterPath_implementation);

IDL4_INLINE CORBA_boolean DirectoryService_UnregisterPath_implementation(CORBA_Object _caller, const Capability_t *cap, idl4_server_environment *_env)

{
  CORBA_boolean __retval = 0;

  /* implementation of DirectoryService::UnregisterPath */
  
  return __retval;
}

IDL4_PUBLISH_DIRECTORYSERVICE_UNREGISTERPATH(DirectoryService_UnregisterPath_implementation);

IDL4_INLINE CORBA_boolean DirectoryService_ResolvePath_implementation(CORBA_Object _caller, const Capability_t *cap, const CORBA_char *path, Capability_t *object, CORBA_long *resolved_chars, idl4_server_environment *_env)

{
  // FIXME - we don't check dir capabilities here!!
  
  uint32_t inode;

  if (!ext2_lookup_path(path, &inode)) return false;

  HpfCapability *hcap = (HpfCapability *) object;
  hpf_capability_new(hcap, L4_Myself(), inode, HPF_CAP_PERM_FULL, my_secret);

  return true;
}

IDL4_PUBLISH_DIRECTORYSERVICE_RESOLVEPATH(DirectoryService_ResolvePath_implementation);

IDL4_INLINE CORBA_boolean DirectoryService_LessenCapability_implementation(CORBA_Object _caller, const Capability_t *cap, const Word_t flags, Capability_t *newcap, idl4_server_environment *_env)

{
  CORBA_boolean __retval = 0;

  /* implementation of DirectoryService::LessenCapability */
  
  return __retval;
}

IDL4_PUBLISH_DIRECTORYSERVICE_LESSENCAPABILITY(DirectoryService_LessenCapability_implementation);

IDL4_INLINE CORBA_boolean DirectoryService_GetRootCapability_implementation(CORBA_Object _caller, Capability_t *newcap, idl4_server_environment *_env)

{
  CORBA_boolean __retval = 0;

  /* implementation of DirectoryService::GetRootCapability */
  
  return __retval;
}

IDL4_PUBLISH_DIRECTORYSERVICE_GETROOTCAPABILITY(DirectoryService_GetRootCapability_implementation);

void *DirectoryService_vtable[DIRECTORYSERVICE_DEFAULT_VTABLE_SIZE] = DIRECTORYSERVICE_DEFAULT_VTABLE;

/* Interface File */

#define MAX_OUTPUT_LEN 4096

/* max output + one block */
static char cache[2 * MAX_OUTPUT_LEN];
struct ext2_inode cached_inode;
uint32_t cached_inum = 0, cached_blk = 0, cached_len = 0;

IDL4_INLINE CORBA_boolean File_Read_implementation(CORBA_Object _caller, const Capability_t *cap, const CORBA_unsigned_long offset, byteseq_t *seq, CORBA_unsigned_long *size, idl4_server_environment *_env)

{
    HpfCapability *hcap = (HpfCapability *) cap;

    unsigned char **buffer = &seq->_buffer;

    if (!HPF_CAP_CAN_WRITE(*hcap) || !hpf_capability_check(hcap, my_secret)) {
        return 0;
    }

    // ugly - this one knows the block size is 1024 bytes

//    printf("got request for %ld %ld bytes at %lx\n", *size, seq->_length, offset);
    // FIXME - ugllyyyyy.... we really don't want hardcoded values :(
    if (*size >= MAX_OUTPUT_LEN) return 0;

    uint32_t first_blk = offset / 1024;
    uint32_t last_blk = (offset + *size) / 1024;
    uint32_t n_blk = last_blk - first_blk + 1;

    if (cached_inum != hcap->object) {
        if (!ext2_lookup_inode(hcap->object, &cached_inode)) return 0;
        cached_inum = hcap->object;
    }

    if (offset > cached_inode.i_size) return 0;
    if ((offset + *size) >= cached_inode.i_size) *size = cached_inode.i_size - offset;

    if ((cached_inum != hcap->object) || (cached_blk != first_blk) || (cached_len != n_blk)) {
        if (!ext2_read_file_block_multiple(&cached_inode, first_blk, n_blk, cache)) return 0;
    }

    seq->_length = *size;

    *buffer = cache + (offset % 1024);

    cached_blk = first_blk;
    cached_len = n_blk;

    return 1;


  CORBA_boolean __retval = 0;

  /* implementation of File::Read */

  return __retval;
}

IDL4_PUBLISH_FILE_READ(File_Read_implementation);

IDL4_INLINE CORBA_unsigned_long File_Write_implementation(CORBA_Object _caller, const Capability_t *cap, const CORBA_unsigned_long offset, const CORBA_char *buffer, const CORBA_unsigned_long size, idl4_server_environment *_env)

{
  CORBA_unsigned_long __retval = 0;

  /* implementation of File::Write */

  return __retval;
}

IDL4_PUBLISH_FILE_WRITE(File_Write_implementation);

IDL4_INLINE CORBA_unsigned_long File_Size_implementation(CORBA_Object _caller, const Capability_t *cap, idl4_server_environment *_env)

{
  CORBA_unsigned_long __retval = 0;

  /* implementation of File::Size */

  return __retval;
}

IDL4_PUBLISH_FILE_SIZE(File_Size_implementation);

void *File_vtable[FILE_DEFAULT_VTABLE_SIZE] = FILE_DEFAULT_VTABLE;

void server(void)

{
  L4_ThreadId_t partner;
  L4_MsgTag_t msgtag;
  idl4_msgbuf_t msgbuf;
  long cnt;

  idl4_msgbuf_init(&msgbuf);
  for (cnt = 0;cnt < DIRECTORYSERVICE_STRBUF_SIZE;cnt++)
    idl4_msgbuf_add_buffer(&msgbuf, malloc(8000), 8000);

  while (1)
    {
      partner = L4_nilthread;
      msgtag.raw = 0;
      cnt = 0;

      while (1)
        {
          idl4_msgbuf_sync(&msgbuf);

          idl4_reply_and_wait(&partner, &msgtag, &msgbuf, &cnt);

          if (idl4_is_error(&msgtag))
            break;

          if (idl4_get_interface_id(&msgtag) == 2) {
              idl4_process_request(&partner, &msgtag, &msgbuf, &cnt, DirectoryService_vtable[idl4_get_function_id(&msgtag) & DIRECTORYSERVICE_FID_MASK]);
          } else {
              idl4_process_request(&partner, &msgtag, &msgbuf, &cnt, File_vtable[idl4_get_function_id(&msgtag) & FILE_FID_MASK]);
          }
        }
    }
}

void DirectoryService_discard(void)
{
}

void File_discard(void)
{
}


bool service_init(HpfCapability *cap)
{
    my_secret = rand();

    uint32_t inode;
    if (!ext2_lookup_path("", &inode)) return false;

    hpf_capability_new(cap, L4_Myself(), inode, HPF_CAP_PERM_FULL, my_secret);
    return true;
}

