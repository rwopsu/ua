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

// FILE COMPARISONS BY MD5 HASH VALUE - PROGRAM
//
// THE NAME ua COMES FROM UgyanAz (HUNGARIAN FOR SAME)
//
// BUILD:
// THIS TOOL REQUIRES THE OPENSSL C LIBRARIES (libcrypto).
//
// g++ -O3 -o ua filei.cc ua.cc -I . -lcrypto
// 
// or
//
// g++ -O3 -o ua filei.cc ua.cc -I . -D__NOHASH -lcrypto
// if you prefer to use tree based containers.
//
// once compiled,
//
// $ ua -vh
//
// will provide help on using the program

#if !defined(__UA_VERSION)
#define __UA_VERSION "1.0"
#endif

#include <filei.h>
#include <cstring>
#include <thread>
#include <future>
#include <vector>
#include <mutex>
#include <map>

extern "C" {
#include <stdio.h>
#include <getopt.h>
}

static char __help[] = 
"ua [OPTION]... [FILE]...\n\n"
"where OPTION is\n" 
"  -i:         ignore case\n"
"  -w:         ignore white space\n"
"  -n:         do not ask the FS for file size\n"
"  -v:         verbose output (prints stuff to stderr), verbose help\n" 
"  -m <max>:   consider only the first <max> bytes\n"
"  -2:         perform two stage hashing\n"
"  -s <sep>:   separator (default SPACE)\n"
"  -p:         also print the hash value\n"
"  -b <bsize>: set internal buffer size (default 1024)\n"
"  -a <alg>:   hash algorithm: md5, sha1, sha256, b3, xxh64\n"
"  -q:         quote file names with single quotes\n"
"  -t <num>:   number of threads (default: auto-detect)\n"
"  -M:         disable adaptive milestone comparison\n"
"  -h:         this help (-vh more verbose help)\n"
"  -           read file names from stdin\n";

static char __vhelp[] =
"The algorithm performs the following steps:\n\n"
"   1. Ask the FS for file size and throw away files with unique counts\n"
"   2. If so requested, calculate a fast hash on a fixed-size prefix\n"
"      of the files with the same byte count and throw away the ones\n"
"      with unique prefix hash\n"
"   3. The still matching files will go through a full MD5 hash\n\n"
"Adaptive milestone comparison (enabled by default, disable with -M):\n"
"   For files with the same size, this feature performs progressive chunk\n"
"   comparisons starting with small chunks and adaptively increasing to\n"
"   larger chunks (up to 256MB for multi-gigabyte files). This eliminates\n"
"   non-identical files early using fast xxHash64, reducing the number of\n"
"   files that need full hashing. Chunk sizes are automatically adjusted\n"
"   based on file size for optimal performance.\n\n"
"-w implies -n, since the byte count is irrelevant information.\n"
"The two-stage hashing algorithm first calculates identical sets\n"
"considering only the first <max> bytes (thus the -2 option requires -m)\n"
"and then from these sets calculates the final result.\n"
"This can be much faster when there are many files with the same size\n"
"or when comparing files with whitespaces ignored. When -w and -m are\n"
"both set, <max> refers to the first <max> non-white characters.\n\n"
"The program returns (to the shell) 0 on success and 1 otherwise.\n\n"
"Files that cannot be processed are simply skipped (-v reports these).\n\n"
"Examples.\n\n"
"  Get help on usage.\n\n"
"    $ ua -h\n"
"    $ ua -vh\n\n"
"  Find identical files in the current directory.\n\n"
"    $ ua *\n"
"    $ ls | ua -p -\n\n"
"    In the first case, the files are read from the command line, while in\n"
"    the second the file names are read from the standard input. The letter\n"
"    one also prints the hashcode.\n\n"
"  Compare text files.\n\n"
"    $ ua -iwvb256 f1.txt f2.txt f3.txt\n\n"
"    Compares the three files ignoring letter case and white spaces.\n"
"    Intermediate steps will be reported on stderr (-v). The -w implies\n"
"    -n, thus file sizes are not grouped. The internal buffer size is\n"
"    reduced to 256, since the whitespaces will cause data to be moved\n"
"    in the buffer.\n\n"
"  Calculate the number of identical files under home.\n\n"
"    $ find ~ -type f | ua -2m256 - | wc -l\n\n"
"    Considering the large number of files, the calculation will be\n"
"    performed with a two stage hash (-2).  Only files that pass the\n"
"    256 byte prefix hash will be fully hashed.\n\n" 
"    Depending on what you compare, the fastest\n"
"    running times probably correspond to one of these option sets:\n\n"
"      (nothing):  there are many files most having unique size\n"
"      -2m256:     lots of files, many of the same size\n"
"      -2nm256:    files of the same size, or comparing files with white\n"
"                  spaces ignored\n\n"
"  Find identical header files.\n\n"
"    $ find /usr/include -name '*.h' | ua -b256 -wm256 -2s, -\n\n"
"    Ignore white spaces -w (and thus use a smaller buffer -b256). Perform\n"
"    the calculation in two stages (-2), first cluster based on the\n"
"    whitespace-free first 256 characters (-m256). Also, separate the\n"
"    identical files in the output by commas (-s,).\n\n"
"Output\n\n"
"  Each line of the output represents one set of identical files. The columns\n"
"  are the path names separated by <sep> (-s). When -p set, the first column\n"
"  will be the hash value. Remember that if -i or -w are set the hash value\n"
"  will likely be different from what md5sum would give.\n\n"
"Blame\n\n"
"  istvan.hernadvolgyi@gmail.com\n\n";


static void __phelp(bool v) {
   if (v) {
      std::cout << "Find identical sets of files." << std::endl << std::endl
                << __help << std::endl << __vhelp 
                << "Version: " << __UA_VERSION 
#if defined(__UA_USEHASH)
                << "_hash"
#else
                << "_tree"
#endif
                << std::endl << std::endl;
   } else {
      std::cout << __help << std::endl
                << "version: " << __UA_VERSION 
#if defined(__UA_USEHASH)
                << "_hash"
#else
                << "_tree"
#endif
                << std::endl << std::endl
                << "Type ua -vh for more help. If in doubt, one of " 
                << std::endl << std::endl
                << "$ find ... | ua -" << std::endl
                << "$ find ... | ua -2m256 -" << std::endl << std::endl;
   }
   std::cout.flush();
}

// Function to process a batch of files in parallel
void process_file_batch(const std::vector<std::string>& files, fsetc_t& files_by_size, 
                       bool count, bool verbose, std::mutex& mtx) {
   for (const auto& file : files) {
      try {
         size_t s = count ? filei::fsize(file) : 0;
         
         std::lock_guard<std::mutex> lock(mtx);
         files_by_size[s].push_back(file);
         if (verbose) std::cerr << (count ? "Counting " : "Spooling ") 
                               << file << std::endl;
      } catch(const char* e) {
         if (verbose) std::cerr << "Skipping " << file << ", " << e << std::endl;
         continue;
      }
   }
}

// Adaptive milestone chunk comparison
std::vector<std::string> adaptive_milestone_compare(const std::vector<std::string>& candidates,
                                                   bool ic, bool iw, int thread_count, bool verbose) {
   if (candidates.size() < 2) return candidates;
   
   std::vector<std::string> remaining = candidates;
   
   // Determine file size to choose appropriate chunk sizes
   size_t file_size = 0;
   try {
      file_size = filei::fsize(candidates[0]);
   } catch(const char*) {
      file_size = 0;
   }
   
   // Adaptive chunk sizes based on file size
   std::vector<size_t> chunk_sizes;
   if (file_size > 1024ULL * 1024ULL * 1024ULL) { // > 1GB
      chunk_sizes = {1048576, 4194304, 16777216, 67108864, 268435456}; // 1MB, 4MB, 16MB, 64MB, 256MB
   } else if (file_size > 100ULL * 1024ULL * 1024ULL) { // > 100MB
      chunk_sizes = {262144, 1048576, 4194304, 16777216, 67108864}; // 256KB, 1MB, 4MB, 16MB, 64MB
   } else if (file_size > 10ULL * 1024ULL * 1024ULL) { // > 10MB
      chunk_sizes = {65536, 262144, 1048576, 4194304, 16777216}; // 64KB, 256KB, 1MB, 4MB, 16MB
   } else {
      chunk_sizes = {1024, 4096, 16384, 65536, 262144, 1048576, 4194304, 16777216, 67108864}; // 1KB, 4KB, 16KB, 64KB, 256KB, 1MB, 4MB, 16MB, 64MB
   }
   
   if (verbose) {
      std::cerr << "File size: " << file_size << " bytes, using " << chunk_sizes.size() 
                << " milestone chunks (max: " << chunk_sizes.back() / (1024*1024) << "MB)" << std::endl;
   }
   
   for (size_t chunk_size : chunk_sizes) {
      if (remaining.size() < 2) break;
      
      // Skip chunks larger than file size
      if (file_size > 0 && chunk_size > file_size) break;
      
      if (verbose) {
         std::cerr << "Comparing " << remaining.size() << " candidates with " 
                   << chunk_size << " byte chunks" << std::endl;
      }
      
      // Group files by their chunk hash
      std::mutex chunk_mtx;
      std::map<std::string, std::vector<std::string>> chunk_groups;
      std::vector<std::future<void>> chunk_futures;
      
      for (const auto& file : remaining) {
         chunk_futures.push_back(std::async(std::launch::async, [&chunk_groups, &chunk_mtx, &file, chunk_size, ic, iw]() {
            try {
               // Create a temporary filei object just for the chunk
               filei fi(file, ic, iw, chunk_size, 1024, filei_hash_alg::XXHASH64); // Use fast xxHash for chunks
               std::string chunk_hash(reinterpret_cast<const char*>(fi.hash()), fi.hash_len());
               
               std::lock_guard<std::mutex> lock(chunk_mtx);
               chunk_groups[chunk_hash].push_back(file);
            } catch(const char*) {
               // Skip files that can't be read
            }
         }));
      }
      
      for (auto& f : chunk_futures) f.wait();
      
      // Update remaining candidates to only those with matching chunk hashes
      remaining.clear();
      for (const auto& pair : chunk_groups) {
         if (pair.second.size() >= 2) {
            remaining.insert(remaining.end(), pair.second.begin(), pair.second.end());
         }
      }
      
      if (verbose && remaining.size() < candidates.size()) {
         std::cerr << "Eliminated " << (candidates.size() - remaining.size()) 
                   << " candidates with " << chunk_size << " byte comparison" << std::endl;
      }
   }
   
   return remaining;
}

int main(int argc, char* const * argv) {

   
   fsetc_t files;

   bool ic = false; // ignore case
   bool iw = false; // ignore white space
   bool v = false; // verbose
   bool stage = false; // two stage
   int BN = 1024; // buffer size
   bool ph = false; // print hash
   bool count = true; // take size into account
   bool quote = false; // quote file names with single quotes
   bool milestone = true; // use adaptive milestone comparison

   int max = 0; // max chars to consider, ALL
   int thread_count = std::thread::hardware_concurrency(); // number of threads

   bool comm = true; // from command line

   std::string sep(" "); // default sep

   filei_hash_alg alg = filei_hash_alg::MD5;

   if (argc <= 1) {
      __phelp(false);
      return 1;
   }

   int opt;
   while((opt = ::getopt(argc,argv,"hb:viws:m:2pna:qt:M")) != -1) {
      switch(opt) {
         case 'b':
            BN = ::atoi(::optarg);
            if (!BN) {
               std::cerr << "Invalid buffer size " << ::optarg << std::endl;
               return 1;
            }
            break;
         case 'm':
            max = ::atoi(::optarg);
            break;
         case 't':
            thread_count = ::atoi(::optarg);
            if (thread_count <= 0) {
               std::cerr << "Invalid thread count " << ::optarg << std::endl;
               return 1;
            }
            break;
         case 'i':
            ic = true;
            break;
         case 'v':
            v = true;
            break;
         case 'w':
            iw = true;
            break;
         case 's':
            sep = std::string(::optarg);
            break;
         case '2':
            stage = true;
            break;
         case 'p':
            ph = true;
            break;
         case 'n':
            count = false;
            break;
         case 'q':
            quote = true;
            break;
         case 'M':
            milestone = false;
            break;
         case 'h':
            __phelp(v);
            return 0;
         case 'a':
            if (strcmp(::optarg, "md5") == 0) alg = filei_hash_alg::MD5;
            else if (strcmp(::optarg, "sha1") == 0) alg = filei_hash_alg::SHA1;
            else if (strcmp(::optarg, "sha256") == 0) alg = filei_hash_alg::SHA256;
            else if (strcmp(::optarg, "b3") == 0) alg = filei_hash_alg::BLAKE3;
            else if (strcmp(::optarg, "xxh64") == 0) alg = filei_hash_alg::XXHASH64;
            else {
               std::cerr << "Unknown algorithm: " << ::optarg << std::endl;
               return 1;
            }
            break;
         case '?':
            std::cerr << "Type " << argv[0] << " -h for options." << std::endl;
            return 1;
      }
   }

   if (stage && !max) {
      std::cerr << "The two stage algorithm requires -m set!" << std::endl;
      return 1;
   }

   if (count && iw) count = false;

   if (count && max && !stage) count = false;

   if (v) {
      std::cerr << "Using " << thread_count << " threads" << std::endl;
   }

   if (argc > ::optind) { 
      if (argc >= ::optind +1 && *argv[::optind] == '-') {
         if (argc > ::optind + 1) {
            std::cerr << "Spurious arguments!" << std::endl;
            return 1;
         }
         ++optind;
         comm = false; // read files names from stdin
      }
   }

   char fileb[1024];

   // Collect all file names first
   std::vector<std::string> all_files;
   
   for(int i = ::optind;;) {
      char* file;
      if (comm) {
         if (i == argc) break;
         file = argv[i++];
      } else {
         std::cin.getline(fileb,1024);
         if (std::cin.eof()) break;
         file = fileb;
      }
      all_files.push_back(file);
   }

   // Process files in parallel batches
   if (thread_count > 1 && all_files.size() > thread_count) {
      std::mutex mtx;
      std::vector<std::future<void>> futures;
      
      size_t batch_size = all_files.size() / thread_count;
      size_t remainder = all_files.size() % thread_count;
      
      size_t start = 0;
      for (int i = 0; i < thread_count; ++i) {
         size_t end = start + batch_size + (i < remainder ? 1 : 0);
         std::vector<std::string> batch(all_files.begin() + start, all_files.begin() + end);
         
         futures.push_back(std::async(std::launch::async, 
                                    process_file_batch, 
                                    std::move(batch), 
                                    std::ref(files), 
                                    count, v, std::ref(mtx)));
         
         start = end;
      }
      
      // Wait for all threads to complete
      for (auto& future : futures) {
         future.wait();
      }
   } else {
      // Fall back to sequential processing for small file lists
      for (const auto& file : all_files) {
         try {
            size_t s = count ? filei::fsize(file) : 0;
            files[s].push_back(file);
            if (v) std::cerr << (count ? "Counting " : "Spooling ") 
                             << file << std::endl;
         } catch(const char* e) {
            if (v) std::cerr << "Skipping " << file << ", " << e << std::endl;
            continue;
         }
      }
   }

   // iterate over size groups
   for(fsetc_t::const_iterator fct= files.begin(); fct != files.end(); ++fct) {
      // less than two in set
      if (fct->second.size() < 2) continue;
      // exactly two in set, and don't care about printing hash
      else if (fct->second.size() == 2 && !ph) {
         if (filei::eq(fct->second[0],fct->second[1],ic,iw,0,BN,alg)) {
            if (quote) {
               std::cout << "'" << fct->second[0] << "'" << sep << "'" << fct->second[1] << "'" << std::endl;
            } else {
               std::cout << fct->second[0] << sep << fct->second[1] << std::endl;
            }
         } 
         continue;
      }

      // Adaptive milestone comparison first
      std::vector<std::string> remaining_candidates;
      if (milestone) {
         remaining_candidates = adaptive_milestone_compare(fct->second, ic, iw, thread_count, v);
         
         if (remaining_candidates.size() < 2) continue;
         
         if (v) {
            std::cerr << "After milestone comparison: " << remaining_candidates.size() 
                      << " candidates remain from " << fct->second.size() << " files" << std::endl;
         }
      } else {
         remaining_candidates = fct->second;
      }

      // Parallel hashing for remaining candidates
      std::mutex hash_mtx;
      std::map<std::string, std::vector<std::string>> hash_to_files;
      std::vector<std::future<void>> hash_futures;
      for (const auto& file : remaining_candidates) {
         hash_futures.push_back(std::async(std::launch::async, [&hash_to_files, &hash_mtx, &file, ic, iw, max, BN, alg, v, count]() {
            try {
               filei fi(file, ic, iw, max, BN, alg);
               std::string hash_str(reinterpret_cast<const char*>(fi.hash()), fi.hash_len());
               {
                  std::lock_guard<std::mutex> lock(hash_mtx);
                  hash_to_files[hash_str].push_back(file);
               }
               if (v && !count) {
                  std::lock_guard<std::mutex> lock(hash_mtx);
                  std::cerr << "Processed " << file << std::endl;
               }
            } catch(const char* e) {
               std::lock_guard<std::mutex> lock(hash_mtx);
               if (v && !count) std::cerr << "Skipping " << file << ", " << e << std::endl;
            }
         }));
      }
      for (auto& f : hash_futures) f.wait();

      // Now, for each group of files with the same hash, add them to cands
      fset_t cands(ic,iw,max,BN,alg);
      for (const auto& pair : hash_to_files) {
         if (pair.second.size() < 2) continue; // skip unique hashes
         for (const auto& file : pair.second) {
            try {
               cands.add(file);
            } catch(const char*) { /* already reported */ }
         }
      }

      const res_t* resp = 0;
      res_t fres;
      if (stage) { // if -2
         try {
            fset_t::common(fres,cands.common(),ic,iw,0,BN);
            resp = &fres;
         } catch(const char* e) {
            if (v && !count) std::cerr << e <<  std::endl;
            continue;
         }
      } else resp = & cands.common();

      fset_t::produce(*resp,std::cout,sep,ph,quote);
   }

   return 0;

}
