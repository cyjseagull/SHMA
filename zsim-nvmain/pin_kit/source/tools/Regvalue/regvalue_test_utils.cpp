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
#include <map>
#include <vector>
#include "regvalue_test_utils.h"
#include "regvalues.h"

using std::map;
using std::vector;

// In this file, we refer to the Active Register Set (ARS) as the set of registers being tested in a specific test run.
// The set is a subset of the TestReg enum (defined in regvalue_test_utils.h). It is defined by the knobs:
// KnobTestSt, KnobTestPartial and KnobTestSIMD.


/////////////////////
// TYPE DEFINITIONS
/////////////////////

class RegisterDB
{
public:
    static inline RegisterDB* Instance();
    inline const vector<TestReg>& GetTestRegs() const;
    inline const REGSET& GetTestRegset() const;
    inline REG GetRegFromTestReg(TestReg testReg) const;
    inline TestReg GetTestRegFromReg(REG reg) const;
    void PrintStoredRegisters(ostream& ost) const;
    inline REGVAL& GetRegval(TestReg testReg);
    inline const unsigned char* GetAppRegisterValue(TestReg testReg) const;
    inline const unsigned char* GetToolRegisterValue(TestReg testReg) const;

private:
    // DATA FIELDS

    static RegisterDB* _theInstance;

    vector<TestReg> _testRegs;  // A vector of all the ARS TestRegs.
    REGSET _testRegset;         // A REGSET of all the ARS REGs.
    int _numOfTestRegs;         // The number of registers in the ARS.

    // REG -> TestReg mapping. Contains only the registers in the ARS.
    map<REG, TestReg> _regToTestReg;

    // Application Registers:
    //
    // The application register values will be stored in the _regvalMap array by the tools.
    // They will be stored and printed before and after the application changes them.
    // In addition, after the application changes the registers, the values will be
    // compared to the expected values, obtained using the _GetAppRegisterValue() function.

    // TestReg -> Value saved by the tool. Contains all the registers in the TestReg enum.
    REGVAL _regvalMap[TESTREGSIZE];

private:
    inline RegisterDB();

    // DISABLED FUNCTIONS
    inline RegisterDB(const RegisterDB& src) { assert(false); } // disable copy constructor
    inline void operator=(const RegisterDB& src) { assert(false); } // disable assignment operator

    // MAPPING FUNCTIONS
    inline REG _GetRegFromTestReg(TestReg testReg) const;
    inline const unsigned char* _GetAppRegisterValue(TestReg testReg) const;
    inline const unsigned char* _GetToolRegisterValue(TestReg testReg) const;

    // UTILITY FUNCTIONS
    inline void Verify(TestReg testReg) const;
    inline void PrintRegval(REG reg, const REGVAL * regval, ostream& ost) const;
    void DefineActiveRegisterSet();
    void InitializeDB();
};

RegisterDB* RegisterDB::_theInstance = NULL;

RegisterDB* RegisterDB::Instance()
{
    if (_theInstance == NULL)
    {
        _theInstance = new RegisterDB;
    }
    return _theInstance;
}

const vector<TestReg>& RegisterDB::GetTestRegs() const
{
    return _testRegs;
}

const REGSET& RegisterDB::GetTestRegset() const
{
    return _testRegset;
}

REG RegisterDB::GetRegFromTestReg(TestReg testReg) const
{
    Verify(testReg);
    return _GetRegFromTestReg(testReg);
}

TestReg RegisterDB::GetTestRegFromReg(REG reg) const
{
    static map<REG, TestReg>::const_iterator end = _regToTestReg.end(); // only need to get this once
    map<REG, TestReg>::const_iterator it = _regToTestReg.find(reg);
    assert(it != end);
    return it->second;
}

void RegisterDB::PrintStoredRegisters(ostream& ost) const
{
    for (int r = 0; r < _numOfTestRegs; ++r)
    {
        TestReg testReg = _testRegs[r];
        PrintRegval(_GetRegFromTestReg(testReg), &_regvalMap[testReg], ost);
    }
}

REGVAL& RegisterDB::GetRegval(TestReg testReg)
{
    Verify(testReg);
    return _regvalMap[testReg];
}

const unsigned char* RegisterDB::GetAppRegisterValue(TestReg testReg) const
{
    Verify(testReg);
    return _GetAppRegisterValue(testReg);
}

const unsigned char* RegisterDB::GetToolRegisterValue(TestReg testReg) const
{
    Verify(testReg);
    return _GetToolRegisterValue(testReg);
}

RegisterDB::RegisterDB() : _numOfTestRegs(0)
{
    DefineActiveRegisterSet();
    InitializeDB();
}

REG RegisterDB::_GetRegFromTestReg(TestReg testReg) const
{
    // TestReg -> REG mapping. Contains all the registers in the TestReg enum.
    static REG const testRegToReg[TESTREGSIZE] =
    {
        REG_GBX,
#if defined(TARGET_IA32E) || defined(TARGET_MIC)
        REG_EBX,
#endif
        REG_BX, REG_BL, REG_BH, REG_ST2,
#ifdef TARGET_MIC
        REG_ZMM5, REG_K3
#else
        REG_XMM0, REG_YMM1
#endif // not TARGET_MIC
    };
    return testRegToReg[testReg];
}

const unsigned char* RegisterDB::_GetAppRegisterValue(TestReg testReg) const
{
    // TestReg -> Expected application register value (defined in ../Utils/regvalues.h).
    // Contains all the registers in the TestReg enum.
    static const unsigned char* const appRegisterValues[TESTREGSIZE] =
    {
        gprval,
#if defined(TARGET_IA32E) || defined(TARGET_MIC)
        gpr32val,
#endif
        gpr16val, gprlval, gprhval, stval,
#ifdef TARGET_MIC
        zmmval, kval
#else
        xmmval, ymmval
#endif // not TARGET_MIC
    };
    return appRegisterValues[testReg];
}

const unsigned char* RegisterDB::_GetToolRegisterValue(TestReg testReg) const
{
    // TestReg -> Value loaded by the tool (defined in ../Utils/regvalues.h).
    // Contains all the registers in the TestReg enum.
    static const unsigned char* const toolRegisterValues[TESTREGSIZE] =
    {
        tgprval,
#if defined(TARGET_IA32E) || defined(TARGET_MIC)
        tgpr32val,
#endif
        tgpr16val, tgprlval, tgprhval, tstval,
#ifdef TARGET_MIC
        tzmmval, tkval
#else
    txmmval, tymmval
#endif // not TARGET_MIC
    };
    return toolRegisterValues[testReg];
}

void RegisterDB::Verify(TestReg testReg) const
{
    assert(testReg >= 0 && testReg < TESTREGSIZE);
}

void RegisterDB::PrintRegval(REG reg, const REGVAL * regval, ostream& ost) const
{
    ost << REG_StringShort(reg) << " = ";
    PrintRawValue(regval, ost);
}

void RegisterDB::DefineActiveRegisterSet()
{
    // Define the ARS and initialize the _testRegs vector accordingly.
    _testRegs.push_back(GPRREG); // Always tested
    if (KnobTestPartial.Value())
    {
#if defined(TARGET_IA32E) || defined(TARGET_MIC)
        _testRegs.push_back(GPR32REG);
#endif
        _testRegs.push_back(GPR16REG);
        _testRegs.push_back(GPRLREG);
        _testRegs.push_back(GPRHREG);
    }
    if (KnobTestSt.Value())
    {
        _testRegs.push_back(STREG);
    }
    if (KnobTestSIMD.Value())
    {
#ifdef TARGET_MIC
        _testRegs.push_back(ZMMREG);
        _testRegs.push_back(KREG);
#else
        _testRegs.push_back(XMMREG);
        if (hasAvxSupport)
        {
            _testRegs.push_back(YMMREG);
        }
#endif
    }
    _numOfTestRegs = _testRegs.size();
}

void RegisterDB::InitializeDB()
{
    // Initialize the _testRegset REGSET and the _regToTestReg map.
    REGSET_Clear(_testRegset);
    for (int r = 0; r < _numOfTestRegs; ++r)
    {
        TestReg testReg = _testRegs[r];
        REG reg = _GetRegFromTestReg(testReg);
        REGSET_Insert(_testRegset, reg);
        _regToTestReg[reg] = testReg;
    }
}


/////////////////////
// GLOBAL VARIABLES
/////////////////////

// A knob for specifying whether x87 fpstate registers should be tested.
KNOB<bool> KnobTestSt(KNOB_MODE_WRITEONCE, "pintool", "test_st", "1",
                      "specify whether x87 fpstate registers should be tested");

// A knob for specifying whether partial registers should be tested.
KNOB<bool> KnobTestPartial(KNOB_MODE_WRITEONCE, "pintool", "test_partial", "1",
                           "specify whether partial registers should be tested");

// A knob for specifying whether the SIMD registers should be tested.
KNOB<bool> KnobTestSIMD(KNOB_MODE_WRITEONCE, "pintool", "test_simd", "1",
                           "specify whether the SIMD registers should be tested");


/////////////////////
// API FUNCTIONS IMPLEMENTATION
/////////////////////

const vector<TestReg>& GetTestRegs()
{
    return RegisterDB::Instance()->GetTestRegs();
}

const REGSET& GetTestRegset()
{
    return RegisterDB::Instance()->GetTestRegset();
}

REG GetRegFromTestReg(TestReg testReg)
{
    return RegisterDB::Instance()->GetRegFromTestReg(testReg);
}

TestReg GetTestRegFromReg(REG reg)
{
    return RegisterDB::Instance()->GetTestRegFromReg(reg);
}

void PrintStoredRegisters(ostream& ost)
{
    RegisterDB::Instance()->PrintStoredRegisters(ost);
}

void AssignNewPinRegisterValue(PIN_REGISTER* pinreg, const UINT64* newval, UINT qwords)
{
    static UINT maxQwords = sizeof(PIN_REGISTER) / sizeof(UINT64);
    assert(qwords <= maxQwords);
    for (UINT i = 0; i < qwords; ++i)
    {
        pinreg->qword[i] = newval[i];
    }
}

REGVAL& GetRegval(TestReg testReg)
{
    return RegisterDB::Instance()->GetRegval(testReg);
}

const unsigned char* GetAppRegisterValue(TestReg testReg)
{
    return RegisterDB::Instance()->GetAppRegisterValue(testReg);
}

const unsigned char* GetToolRegisterValue(TestReg testReg)
{
    return RegisterDB::Instance()->GetToolRegisterValue(testReg);
}

bool CheckAllExpectedValues(ostream& ost)
{
    bool success = true;
    const vector<TestReg>& regs = GetTestRegs();
    int numOfRegs = regs.size();
    for (int r = 0; r < numOfRegs; ++r)
    {
        TestReg reg = regs[r];
        success &= CheckRegval(&GetRegval(reg), GetAppRegisterValue(reg), REG_Size(GetRegFromTestReg(reg)), ost);
    }
    return success;
}
