#ifndef _HPF_MD5_H_
#define _HPF_MD5_H_

#define MD5_DIGEST_LEN 16

void hpf_md5(const unsigned char *data, size_t len, unsigned char *md);

#endif

