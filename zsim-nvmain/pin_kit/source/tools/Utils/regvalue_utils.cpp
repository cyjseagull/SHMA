/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2013 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
#include <cassert>
#include <string>
#include "regvalue_utils.h"
//#include "regvalues.h"

using std::hex;
using std::endl;
using std::flush;
using std::string;


/////////////////////
// EXTERNAL FUNCTIONS
/////////////////////

extern "C" bool ProcessorSupportsAvx();


/////////////////////
// GLOBAL VARIABLES
/////////////////////

// Boolean indicating whether the system supports avx instructions and registers.
const bool hasAvxSupport = ProcessorSupportsAvx();


/////////////////////
// INTERNAL FUNCTIONS IMPLEMENTATION
/////////////////////

static inline UINT Bits2Uint64(UINT bits, UINT& extraBytes)
{
    assert(bits%8 == 0);
    UINT res = bits >> 6;
    extraBytes = (bits%64) >> 3;
    if (extraBytes) ++ res;
    return res;
}

static string Val2Str(const unsigned char * data, UINT bytes)
{
    string str(bytes<<1,0);
    UINT s = 0;
    // each byte in data translates to two chars (s) in the string
    while (bytes)
    {
        --bytes;
        char c = (data[bytes] & 0xf0) >> 4;
        str[s++] = (c>9) ? c-10+'a' : c+'0';
        c = data[bytes] & 0x0f;
        str[s++] = (c>9) ? c-10+'a' : c+'0';
    }
    return str;
}

template<typename SIZETYPE>
static bool CompareSizedWord(const unsigned char * value, const unsigned char * expected, UINT element,
                             UINT totalSize, ostream& ost)
{
    if (*((SIZETYPE*)(&value[element << 3])) != *((SIZETYPE*)(&expected[element << 3])))
    {
        ost << "WARNING: Expected value: 0x" << Val2Str(expected, totalSize) << endl << flush;
        ost << "WARNING: Received value: 0x" << Val2Str(value, totalSize) << endl << flush;
        return false;
    }
    return true;
}

static void FillBufferFromRegval(unsigned char * value, const REGVAL * regval, UINT bytes)
{
    UINT size = bytes >> 3; // number of UINT64 elements
    if (bytes % 8) ++ size;
    for (UINT i = 0; i < size; ++i)
    {
        // The last 64-bit chunk may be larger than the actual register size, for example with an 80-bit register.
        // This is fine since PIN_ReadRegValueQWord zero-extends to a 64-bit chunk and the 'value' buffer is a
        // multiple of UINT64.
        PIN_ReadRegvalQWord(regval, &(((UINT64*)value)[i]), i);
    }
}


/////////////////////
// API FUNCTIONS IMPLEMENTATION
/////////////////////

void PrintRawValue(const REGVAL * regval, ostream& ost)
{
    ost << "0x";
    UINT extraBytes = 0;
    UINT qw = Bits2Uint64(PIN_GetRegvalSize(regval), extraBytes); // number of UINT64 elements
    UINT64 val;
    --qw;
    PIN_ReadRegvalQWord(regval, &val, qw);
    ost.width((extraBytes == 0) ? 16 : extraBytes*2);
    ost.fill('0');
    ost << hex << val;
    while (qw)
    {
        --qw;
        UINT64 val;
        PIN_ReadRegvalQWord(regval, &val, qw);
        ost.width(16);
        ost << val;
    }
    ost << endl << flush;
}

bool CompareValues(const unsigned char * value, const unsigned char * expected, UINT size, ostream& ost)
{
    for (UINT bytes = size, i = 0; bytes > 0; bytes -= 8, ++i)
    {
        if (bytes == 1) return CompareSizedWord<UINT8>(value, expected, i, size, ost);
        if (bytes == 2) return CompareSizedWord<UINT16>(value, expected, i, size, ost);
        if (bytes == 4) return CompareSizedWord<UINT32>(value, expected, i, size, ost);
        if (bytes >= 8)
        {
            if (!CompareSizedWord<UINT64>(value, expected, i, size, ost))
            {
                return false;
            }
        }
        else
        {
            ost << "ERROR: Unexpected number of bytes left: " << bytes << endl << flush;
            assert(0);
        }
    }
    return true;
}

bool CheckRegval(const REGVAL * regval, const unsigned char * expected, UINT size, ostream& ost)
{
    // We use the size of PIN_REGISTER as our maximum register value size. This holds because PIN_REGISTER can
    // contain any architectural register value. It is a multiple of UINT64, which is important because we use
    // PIN_ReadRegValueQWord to fill the buffer.
    unsigned char value[sizeof(PIN_REGISTER)];
    UINT regvalSize = PIN_GetRegvalSize(regval);
    assert((regvalSize % 8) == 0);
    assert((regvalSize >> 3) == size);
    FillBufferFromRegval(value, regval, size);
    return CompareValues(value, expected, size, ost);
}
