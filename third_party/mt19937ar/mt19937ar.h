/* 
 A C-program for MT19937, with initialization improved 2002/1/26.
 Coded by Takuji Nishimura and Makoto Matsumoto.
 
 Before using, initialize the state by using init_genrand(seed)  
 or init_by_array(init_key, key_length).
 
 Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
 All rights reserved.                          
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 
 3. The names of its contributors may not be used to endorse or promote 
 products derived from this software without specific prior written 
 permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 
 Any feedback is very welcome.
 http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
 email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
 */

#ifndef THIRD_PARTY_MT19937AR_MT19937AR_H_
#define THIRD_PARTY_MT19937AR_MT19937AR_H_

#include <vector>

#include "base/basictypes.h"

class MersenneTwister {
 public:
  MersenneTwister();
  ~MersenneTwister();

  void init_genrand(uint32 s);
  void init_by_array(uint32 init_key[], int key_length);
  uint32 genrand_int32(void);

 private:
  std::vector<uint32> mt; /* the array for the state vector  */
  int mti; /* mti==N+1 means mt[N] is not initialized */
};

#endif  // THIRD_PARTY_MT19937AR_MT19937AR_H_
