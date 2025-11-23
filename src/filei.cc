/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * The Original Code was developed for an EU.EDGE internal project and
 * is made available according to the terms of this license.
 * 
 * The Initial Developer of the Original Code is Istvan T. Hernadvolgyi,
 * EU.EDGE LLC.
 *
 * Portions created by EU.EDGE LLC are Copyright (C) EU.EDGE LLC.
 * All Rights Reserved.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License (the "GPL"), in which case the
 * provisions of GPL are applicable instead of those above.  If you wish
 * to allow use of your version of this file only under the terms of the
 * GPL and not to allow others to use your version of this file under the
 * License, indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by the GPL.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under either the License or the GPL.
 */

// FILE COMPARISONS BY MD5 HASH VALUE - IMPLEMENTATION
//

#include <filei.h>

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/evp.h>
#include "blake3.h"
#include "xxhash.h"
}

#include <fstream>

void* (*filei::_gbuff)(size_t) = &filei::gbuff;
size_t (*filei::_buffc)() = &filei::buffc;
void (*filei::_relbuff)(void*) = 0;
char filei::_buffer[__UABUFFSIZE];
     
filei::filei(const std::string& path, bool ic, bool iw, size_t m, size_t bs, filei_hash_alg alg)
:_path(path),_h(0),_alg(alg)  {
   memset(_hash, 0, FILEI_SHA256_LEN);
   switch (_alg) {
      case filei_hash_alg::MD5: _hash_len = FILEI_MD5_LEN; break;
      case filei_hash_alg::SHA1: _hash_len = FILEI_SHA1_LEN; break;
      case filei_hash_alg::SHA256: _hash_len = FILEI_SHA256_LEN; break;
      case filei_hash_alg::BLAKE3: _hash_len = FILEI_BLAKE3_LEN; break;
      case filei_hash_alg::XXHASH64: _hash_len = FILEI_XXHASH64_LEN; break;
   }
   calc(ic,iw,bs,m);
}

// in-place turn buffer into lower case
static void __lower_case(char* buffer, size_t n) {
   static int diff = 'a' - 'A';
   for(char *p = buffer; p < buffer + n; ++p) 
      if (*p <= 'Z' && *p >= 'A') *p += diff;
}

// white spaces
static bool __whitec(char c) {
   switch(c) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
         return true;
   }
   return false;
}

// count contiguous white spaces from pointer
static size_t __countw(const char* p, const char* e) {
   size_t w = 0;
   for(;p<e; ++p, ++w) if (!__whitec(*p)) break;
   return w;
}

// in-place remove contiguous white space, return number of chars removed
static int __remove_white(char* buffer, int n) {
   int r = 0;

   for(char *p = buffer; p < buffer + n; ++p) {
      int k = __countw(p,buffer+n);
      if (k) {
         // shift by k
         for(char* pk = p; k && pk < buffer + n - k; ++pk) *pk = *(pk+k);
         n -= k;
         r += k;
      }
   }

   return r;
}

void filei::calc(bool ic, bool iw, size_t bn, size_t m) {
   const char* error = 0;
   char* buffer = 0;
   size_t tot = 0;
   bool ok = false;
   EVP_MD_CTX* evp_ctx = 0;
   const EVP_MD* evp_md = 0;
   blake3_hasher blake3_ctx;
   std::ifstream is(_path.c_str());
   if (!is.good()) { error = "Could not open file"; goto FINALLY; }
   try {
      buffer= static_cast<char*>((*_gbuff)(bn));   // get buffer
      if (!buffer) throw 1;
   } catch(...) {
      error = "Could not allocate memory";
      goto FINALLY;
   }
   bn = _buffc ? std::min(bn,(*_buffc)()) : bn;  // get buffer size
   switch (_alg) {
      case filei_hash_alg::MD5:
         evp_md = EVP_md5();
         break;
      case filei_hash_alg::SHA1:
         evp_md = EVP_sha1();
         break;
      case filei_hash_alg::SHA256:
         evp_md = EVP_sha256();
         break;
      case filei_hash_alg::BLAKE3:
         blake3_hasher_init(&blake3_ctx);
         ok = true;
         break;
      case filei_hash_alg::XXHASH64:
         ok = true; // handled as one-shot below
         break;
   }
   if (evp_md) {
      evp_ctx = EVP_MD_CTX_new();
      if (!evp_ctx) { error = "Could not allocate hash context"; goto FINALLY; }
      ok = EVP_DigestInit_ex(evp_ctx, evp_md, nullptr) == 1;
   }
   if (!ok) { error = "Could not init hash"; goto FINALLY; }
   if (_alg == filei_hash_alg::XXHASH64) {
      // One-shot hash for xxHash64
      std::vector<char> all_data;
      while (is) {
         is.read(buffer, bn);
         all_data.insert(all_data.end(), buffer, buffer + is.gcount());
      }
      uint64_t xxh = XXH64(all_data.data(), all_data.size(), 0);
      memcpy(_hash, &xxh, FILEI_XXHASH64_LEN);
      _h = xxh;
      goto FINALLY;
   }
   for(bool done=false;!done;) {
      is.read(buffer,bn);
      size_t n = is.gcount();
      if (!n) break;
      if (ic) __lower_case(buffer,n);
      if (iw) {
         n -=  __remove_white(buffer,n);
         if (!n) continue;
      }
      if (m) {
         if (tot + n > m) {
            n = m - tot;
            done = true;
         } else tot += n;
      }
      switch (_alg) {
         case filei_hash_alg::MD5:
         case filei_hash_alg::SHA1:
         case filei_hash_alg::SHA256:
            ok = EVP_DigestUpdate(evp_ctx, buffer, n) == 1;
            break;
         case filei_hash_alg::BLAKE3:
            blake3_hasher_update(&blake3_ctx, buffer, n);
            ok = true;
            break;
         case filei_hash_alg::XXHASH64:
            // handled above
            ok = true;
            break;
      }
      if (!ok) { error = "Hash calc error"; goto FINALLY; }
      if (is.eof()) break;
   }
   switch (_alg) {
      case filei_hash_alg::MD5:
      case filei_hash_alg::SHA1:
      case filei_hash_alg::SHA256:
         ok = EVP_DigestFinal_ex(evp_ctx, _hash, nullptr) == 1;
         break;
      case filei_hash_alg::BLAKE3:
         blake3_hasher_finalize(&blake3_ctx, _hash, FILEI_BLAKE3_LEN);
         ok = true;
         break;
      case filei_hash_alg::XXHASH64:
         // handled above
         break;
   }
   if (!ok) { error= "Hash calc error (final)"; goto FINALLY; }
   for(int i = 0, s = 0; i < _hash_len; ++i, ++s) {
      if (s >= (int)sizeof(size_t)) s = 0;
      _h ^= ((size_t)_hash[i]) << (s << 3);
   }
FINALLY:
   is.close();
   if (evp_ctx) EVP_MD_CTX_free(evp_ctx);
   if (_relbuff) (*_relbuff)(buffer);
   if (error) throw error;
}

off_t filei::fsize(const std::string& path) {
   struct stat fsi;

   if (::stat(path.c_str(),&fsi)) throw "Could not stat file.";
   if (!S_ISREG(fsi.st_mode) && !S_ISLNK(fsi.st_mode)) throw "Not a file.";
   return fsi.st_size;
}

static bool __bytesame(
   std::istream& is1, std::istream& is2,
   char* buff1, char* buff2, 
   size_t c1, size_t c2, size_t m) {

   size_t tot1 = 0, tot2 = 0;

   for(;;) {
      is1.read(buff1,c1);
      is2.read(buff2,c2);

      size_t n1 = is1.gcount();
      size_t n2 = is2.gcount();

      if (m) {
         if (tot1 + n1 > m) n1 = m - tot1;
         if (tot2 + n2 > m) n2 = m - tot2;
      }

      if (n1 != n2) return false;

      for(const char* p1 = buff1, * p2 = buff2; p1 < buff1 + n1; ++p1, ++p2)
         if (*p1 != *p2) return false;

      if (m) {
         tot1 += n1;
         tot2 += n2;
      }

      if (is1.eof()) return is2.eof();
   }

   return true;
}

static size_t __reload(std::istream& is, char* buff, size_t c, char*& p) {
   is.read(buff,c);
   p = buff;
   return is.gcount();
}

static void __tolower(char& c) {
   static int diff = 'a' - 'A';
   if (c >= 'A' && c <= 'Z')  c += diff;
}

static void __skipws(char*& p, const char* e) {
   for(;p < e; ++p) if (!__whitec(*p)) return;
}

static bool __same(
   std::istream& is1, std::istream& is2,
   char* buff1, char* buff2, 
   size_t c1, size_t c2, size_t m,
   bool ic, bool iw) {

   is1.read(buff1,c1);
   is2.read(buff2,c2);

   size_t n1 = is1.gcount();
   size_t n2 = is2.gcount();

   char* p1 = buff1;
   char* p2 = buff2;

   for(;;) {
      if (p1 == buff1+n1 && !(n1 = __reload(is1,buff1,c1,p1))) break;
      if (p2 == buff2+n2 && !(n2 = __reload(is2,buff2,c2,p2))) break;

      if (iw) { 
         __skipws(p1,buff1+n1);
         __skipws(p2,buff2+n2);
         if ((p1 == buff1+n1) || (p2 == buff2+n2)) continue;
      }

      if (ic) { __tolower(*p1), __tolower(*p2); }
      if (*p1 != *p2) return false;
      ++p1, ++p2;
   }

   if (iw) {
      for(;p1 < buff1 + n1; ++p1) if (!__whitec(*p1)) return false;
      for(;p2 < buff2 + n2; ++p2) if (!__whitec(*p2)) return false;
   }

   return true;
}

bool filei::eq(
   const std::string& p1, const std::string& p2,
   bool ic, bool iw, size_t m, size_t bn, filei_hash_alg alg) {

   const char* error = 0;
   char* buffer = 0;
   bool res = false;

   std::ifstream is1(p1.c_str());
   std::ifstream is2(p2.c_str());

   if (!is1.good() || !is2.good()) { 
      error = "Could not open file";
      goto FINALLY;
   }
   

   try {
      bn <<=1;
      buffer = static_cast<char*>((*_gbuff)(bn)); // get buffer
      if (!buffer) throw 1;

   } catch(...) {
      error = "Could not allocate memory";
      goto FINALLY;
   }

   bn = _buffc ? std::min(bn,(*_buffc)()) : bn; // get buffer size

   try {
      size_t h = bn >> 1;
      res = !iw && !ic ? __bytesame(is1,is2,buffer,buffer + h,h,bn-h,m) :
         __same(is1,is2,buffer,buffer + h,h,bn-h,m,ic,iw);
   } catch(const char* e) {
      error = e;
      goto FINALLY;
   }
      

FINALLY:

   // clean-up
   is1.close(), is2.close();
   if (_relbuff) (*_relbuff)(buffer);

   if (error) throw error;

   return res;
}
