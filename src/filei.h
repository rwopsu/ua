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

// FILE COMPARISONS BY MD5 HASH VALUE - HEADER
//
 
#if !defined(_FILEI_H_)
#define _FILEI_H_


// decide whether to prefer hashed containers
// first you need gcc 3.x or higher 
// second __NOHASH should be undefined
//
#if defined(__GNUC__) && !defined(__NOHASH)
#if __GNUC__ >= 3
#define __UA_USEHASH
#endif
#endif

// default work buffer size
//
#if !defined(__UABUFFSIZE)
#define __UABUFFSIZE 32768
#endif

#include <string>

#if defined(__UA_USEHASH)
#include <ext/hash_set>
#include <ext/hash_map>
#endif

#include <set>
#include <map>

#include <vector>

#include <iostream>
#include <iomanip>

// Add OpenSSL hash sizes
#define FILEI_MD5_LEN 16
#define FILEI_SHA1_LEN 20
#define FILEI_SHA256_LEN 32
#define FILEI_BLAKE3_LEN 32
#define FILEI_XXHASH64_LEN 8

// Add enum for hash algorithm
enum class filei_hash_alg {
    MD5,
    SHA1,
    SHA256,
    BLAKE3,
    XXHASH64
};

/** File info.
 *
 * Contains the path name and the corresponding md5 hash. 
 * All calculations are performed during construction. Once 
 * constructed the object is "const"; there are only accessors.
 *
 * The calculation of MD5 requires a char buffer. By default
 * an internal buffer is used (filei::_buffer which is private).
 * For concurrent calculations, you can assign filei::_gbuff, filei::_buffc and 
 * filei::_relbuff to get a buffer, get its capacity and to release
 * the buffer. Eg. you could set
 * <pre>
 *    filei::_gbuff = &::malloc;
 *    filei::_relbuff = &::free;
 *    filei::_buffc = 0;
 * </pre>
 * If _buffc is 0, then it assumes that all requested memory is returned.
 * The code checks for _gbuff returning 0 (and catches exceptions) so it is
 * compatible with malloc and easy to wrap into an allocator as well.
 */
class filei {

   private:
      // a shared buffer
      // used for internal calculations, but can be overridden
      static char _buffer[__UABUFFSIZE];

      std::string _path; // path name
      unsigned char _hash[FILEI_SHA256_LEN]; // max size for SHA256
      size_t _h; // hash of hash :)
      filei_hash_alg _alg;
      int _hash_len;

      // calculate hash
      void calc(bool ic, bool iw, size_t bs, size_t m);

      // return buffer 
      static void* gbuff(size_t) { return _buffer; }

      // return buffer capacity
      static size_t buffc() { return __UABUFFSIZE; }

   public:

      /** Constructor.
       *
       * The constructor will open the file and calculate the hash.
       *
       * @param path file name
       * @param ic ignore case
       * @param iw ignore white space (in essence, remove it)
       * @param m consider at most these many bytes for the hash (0: ALL)
       * @param bs buffer size of internal work buffer (default 1024)
       * @param alg hash algorithm
       * @throws an error message if construction failed
       */
      filei(const std::string& path, bool ic, bool iw, 
         size_t m = 0ul, size_t bs=1024ul,
         filei_hash_alg alg = filei_hash_alg::MD5);

      /** Get an md5 hash char.
       * @param i index
       * @return md5 hash char at index
       */
      unsigned char operator[](int i) const { return _hash[i]; }

      /** Get the md5 hash characters.
       * @return the md5 hash characters. 
       */
      const unsigned char* hash() const { return _hash; }

      /** Get path name.
       * @return path name
       */
      const std::string& path() const { return _path; }

      /** Get hash code.
        * This is a hash of the _md5 value and meant to be
        * used in hashed data structures.
        * @return hash code
        */
      const size_t h() const { return _h; }

      /** Get hash algorithm.
       * @return hash algorithm
       */
      filei_hash_alg alg() const { return _alg; }

      /** Get hash length.
       * @return hash length
       */
      int hash_len() const { return _hash_len; }

      /** Get file size from the file system.
        * @param path absolute or relative path
        * @return file size in bytes
        * @throws an exception if status cannot be determined.
        */
      static off_t fsize(const std::string& path);

      /** Determine whether the two files are identical.
        * @param p1 path of one file
        * @param p2 path of the other
        * @param ic ignore letter case
        * @param iw ignore white spaces
        * @param m onlu consider these many bytes (0 all)
        * @param bs set the internal buffer size
        * @param alg hash algorithm
        * @return whether the files corresponding to p1 and p2 are identical
        * @throws an exception on any error
        */
      static bool eq(const std::string& p1, const std::string& p2,
         bool ic, bool iw, size_t m = 0ul, size_t bs = 1024ul,
         filei_hash_alg alg = filei_hash_alg::MD5);

      /** Functor for hashed containers.
       */
      struct hashfn {

         /** Calculate a hash of the filei object.
          * @param fi file info
          * @return hash value
          */
         size_t operator()(const filei& fi) const { return fi.h(); }
      };

      /** Functor for sorted containers.
       */
      struct hashcmp {

         /** Compare the hashes numerically.
           * @param fi1 file info 1
           * @param fi2 file info 2
           * @return true if fi1._hash < fi2._hash
           */
         bool operator()(const filei& fi1, const filei& fi2) const {
            int len = fi1.hash_len();
            for (int i = 0; i < len; ++i) {
                if (fi1._hash[i] < fi2._hash[i]) return true;
                if (fi1._hash[i] > fi2._hash[i]) return false;
            }
            return false;
         }
      };

      /** Functor for containers.
        */
      struct hasheq {
    
         /** Compare the hashes numerically.
           * @param fi1 file info 1
           * @param fi2 file info 2
           * @return true if fi1._hash == fi2._hash
           */
         bool operator()(const filei& fi1, const filei& fi2) const {
            int len = fi1.hash_len();
            for (int i = 0; i < len; ++i) {
                if (fi1._hash[i] != fi2._hash[i]) return false;
            }
            return true;
         }
      };

      /** Equality operator for filei objects.
       */
      bool operator==(const filei& other) const {
         return hasheq()(*this, other);
      }

      // assign the three plugins below differently 
      // if you want re-entrant calculations,
      // or different buffer allocation
      // by default, all calculations share the same buffer and
      // all calculations are performed at construction

      /** Function that gets work buffer.
       * The size_t argument is the requested capacity.
       * By default, it is set to filei::gbuff, which returns
       * a static (shared) buffer of size 32K (regardless of the
       * requested capacity).
       */
      static void* (*_gbuff)(size_t);

      /** Function that tells work buffer capacity.
        * The function should tell the capacity of the
        * buffer returned by (*_gbuff)(size_t).
        */
      static size_t (*_buffc)();

      /** Function that releases work buffer.
        * This function will be called when a calculation
        * has finished. The argument is the address of the work buffer
        * returned by (*_gbuff)(size_t).
        */
      static void (*_relbuff)(void*);
};


/** Vector of file names. */
typedef std::vector<std::string> fvec_t;

/** Set of file infos.
  */
typedef std::set<filei,filei::hashcmp> set_t;

/** Map from file name to file names. 
 * The key and the values together make up a set of 
 * identical files.
 */
typedef std::map<filei,fvec_t,filei::hasheq> map_t;


#if defined(__UA_USEHASH)

/** Hashed set of file infos.
  */
typedef __gnu_cxx::hash_set<filei,filei::hashfn,filei::hasheq> hset_t;

/** Hashed map from file name to file names. 
 * The key and the values together make up a set of 
 * identical files.
 */
typedef __gnu_cxx::hash_map<filei,fvec_t,filei::hashfn,filei::hasheq> hmap_t;
#endif

/** Sets of identical files.
 *
 * S: must be some set<filei>
 * M: must be some map<filei,std::vector<std::string> >
 */
template<typename S, typename M>
class fset {
   private:

      // "heads" are random representatives of sets of files
      // with the same _md5 

      S _files; // set of file "heads": each has unique _md5
      M _cmn;   // the subsets that are identical ("head" -> path names)

      bool _ic; // ignore case
      bool _iw; // ignore whitespace
      size_t _max; // max chars to consider
      size_t _bs;  // buffer size
      filei_hash_alg _alg;

      typedef typename M::const_iterator it_t; // subset iterator

      /** Add a file info.
        * @param fi file info
        */
      void add(const filei& fi) {
         typename S::const_iterator i = _files.find(fi);
         if (i != _files.end()) _cmn[*i].push_back(fi.path());
         else _files.insert(fi);
      }


   public:
 
      /** Constructor.
       *
       * @param ic ignore case
       * @param iw ignore white space
       * @param m consider at most these many bytes for hash (0: ALL)
       * @param bs internal buffer size (default 1024)
       * @param alg hash algorithm
       */
      fset(bool ic, bool iw, size_t m = 0, size_t bs = 1024, filei_hash_alg alg = filei_hash_alg::MD5):
         _ic(ic), _iw(iw), _max(m), _bs(bs), _alg(alg) {
      }

 
      /** Add a file.
        * @param path file
        * @throws a description if hash could not be constructed
        */
      void add(const std::string& path) {
         add(filei(path,_ic,_iw,_max,_bs,_alg));
      }

      /** Print the sets of identical files.
        *
        * Each set of identical files are printed on a single line.
        * @param cmn common files
        * @param os output stream
        * @param s separator (default " ")
        * @param ph print hash (if true, the first column is the hash)
        * @param quote quote file names with single quotes
        */
      static void produce(const M& cmn, std::ostream& os,
         const std::string& s = " ", bool ph = false, bool quote = false) {

         for(it_t it= cmn.begin(); it != cmn.end(); ++it) {
            if (ph) { // print hash
               for(int i=0; i< it->first.hash_len(); ++i) {
                  int hi = it->first[i] >> 4 & 0x0f;
                  int lo = it->first[i] & 0x0f;
                  os << std::hex << hi << lo;
               }
               os << s;
            }
            if (quote) os << "'";
            os << it->first.path();
            if (quote) os << "'";
            for(int i=0; i<(int)it->second.size();++i) {
               os << s;
               if (quote) os << "'";
               os << it->second[i];
               if (quote) os << "'";
            }
            os << std::endl;
         }
      }

      /** Identical sets of files.
        *
        * The result is a map from a file info to vectors of file paths.
        * The key and its corresponding values together make up one 
        * identical set of files. The key is not repeated in the values and
        * its designation as key is arbitrary. 
        *
        * @return a map of the identical sets
        */
      const M& common() const { return _cmn; }

      /** Calculate sets of identical files.
       *
       * This function can be used to implement a multi-stage
       * calculation of identical file sets. The param cmn
       * could be obtained by looking at only the prefix of the files
       * and thus files in its identical sets may not be totally identical
       * (only in prefix). However, files which are not in these sets can
       * be safely ignored.
       *
       * @param res resulting common set of files (returned)
       * @param cmn common set of files
       * @param ic ignore case
       * @param iw ignore white space
       * @param m consider at most these many bytes for hash (0: ALL)
       * @param bs internal buffer size (default 1024)
       */
      static void common(M& res, const M& cmn, 
         bool ic, bool iw, size_t m=0, size_t bs=1204) {
         for(it_t it=cmn.begin(); it != cmn.end(); ++it) {
            fset files(ic,iw,m,bs);
            files.add(it->first.path());
            for(int i=0; i<(int)it->second.size();++i) files.add(it->second[i]);

            const M& locmn = files.common();
            for(it_t lit = locmn.begin(); lit!=locmn.end(); ++lit) {
               res[lit->first] = lit->second;
            }
         }
      }
};

/** Choose preferred types for file set and result set.
 * fsetc_t: preferred type for the map from file size to file names
 * res_t:   preferred type for the map of identical subsets 
 * fset_t:  preferred type for the fset object
 *
 */
#if defined(__UA_USEHASH)
typedef fset<hset_t,hmap_t> fset_t;
typedef hmap_t res_t;
typedef __gnu_cxx::hash_map<size_t,fvec_t> fsetc_t;
#else
typedef std::map<size_t,fvec_t> fsetc_t;
typedef map_t res_t;
typedef fset<set_t,map_t> fset_t;
#endif

#endif
