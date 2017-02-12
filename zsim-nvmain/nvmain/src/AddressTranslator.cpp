/*******************************************************************************
* Copyright (c) 2012-2014, The Microsystems Design Labratory (MDL)
* Department of Computer Science and Engineering, The Pennsylvania State University
* All rights reserved.
* 
* This source code is part of NVMain - A cycle accurate timing, bit accurate
* energy simulator for both volatile (e.g., DRAM) and non-volatile memory
* (e.g., PCRAM). The source code is free and you can redistribute and/or
* modify it by providing that the following conditions are met:
* 
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
* 
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* Author list: 
*   Matt Poremba    ( Email: mrp5060 at psu dot edu 
*                     Website: http://www.cse.psu.edu/~poremba/ )
*   Tao Zhang       ( Email: tzz106 at cse dot psu dot edu
*                     Website: http://www.cse.psu.edu/~tzz106 )
*******************************************************************************/

#include <iostream>
#include <cstdlib>


#include "src/AddressTranslator.h"
#include "include/NVMHelpers.h"

#include "galloc.h"
#include "pad.h"

using namespace NVM;

AddressTranslator::AddressTranslator( )
{
    method = NULL;
    defaultField = NO_FIELD;

    /* the default bus width is 64 to comply with JEDEC-DDR */
    busWidth = 64; 
    /* the default burst length is 8 to comply with JEDEC-DDR */
    burstLength = 8; 

    lowColBits = 0;
	//bitWidths = new unsigned int[FIELD_NUM];
	//fieldOrder = new int[FIELD_NUM];
	//int* orders = gm_memalign<int>(CACHE_LINE_BYTES,1);
	//unsigned int* widths = gm_memalign<unsigned int>(CACHE_LINE_BYTES,1);
	//fieldOrder = new (orders) int[FIELD_NUM];
	//bitWidths = new (widths) unsigned int[FIELD_NUM];
	bitWidthsInited = false;
	fieldOrderInited = false;
}


AddressTranslator::~AddressTranslator( )
{
}


void AddressTranslator::SetTranslationMethod( TranslationMethod *m )
{
    method = m;
}


TranslationMethod *AddressTranslator::GetTranslationMethod( )
{
    return method;
}

/*
uint64_t AddressTranslator::InitBitWidths()
{
	if( method && !bitwidthsInited )
	{
		method->GetBitWidths();
		bitwidthsInited = true;
		return static_cast<uint64_t>(bitWidths[MEM_ROW] + bitWidths[MEM_COL] +    \
									bitWidths[MEM_BANK] + bitWidths[MEM_RANK] + bitWidths[MEM_CHANNEL]  \
									+ bitWidths[MEM_SUBARRAY]);
	}
	return static_cast<uint64_t>(-1);
}
*/

//modified on 2015,29,Jan
//in order to improve program's performance
uint64_t AddressTranslator::ReverseTranslate( const uint64_t& row, 
                                              const uint64_t& col, 
                                              const uint64_t& bank,
				              const uint64_t& rank, 
                                              const uint64_t& channel,
                                              const uint64_t& subarray )
{
    if( GetTranslationMethod( ) == NULL )
    {
        std::cerr << "Divider Translator: Translation method not specified!" << std::endl;
        exit(1);
    }

    uint64_t unitAddr = 1;
    uint64_t phyAddr = 0;
    MemoryPartition part = MEM_UNKNOWN;

    int busOffsetBits = mlog2( busWidth / 8 );
    int burstBits = mlog2( (busWidth * burstLength) / 8 );
    lowColBits = burstBits - busOffsetBits;

    /* first of all, add the bus width */
    unitAddr <<= busOffsetBits;

    /* then, add the lowest column bits */
    unitAddr <<= lowColBits;

    //unsigned channelBits, rankBits, bankBits, rowBits, colBits, subarrayBits;

   // method->GetBitWidths( &rowBits, &colBits, &bankBits, 
         //                 &rankBits, &channelBits, &subarrayBits );
	if(!bitWidthsInited)
	{
		for( int i=0; i<FIELD_NUM; i++)
		{
			bitWidths[i]= method->bitWidths[i];
		}
		bitWidthsInited = true;
	}
	
	if(!fieldOrderInited)
	{
		for( int i=0; i<FIELD_NUM; i++)
		{
			fieldOrder[i] = method->order[i];
		}
		fieldOrderInited = true;
	}

    for( int i = 0; i < 6 ; i++ )
    {
        /* 0->4, low to high, FindOrder() will find the correct one */
        FindOrder_modified( i, &part );
 
        switch( part )
        {
            case MEM_ROW:
                  phyAddr += ( row * unitAddr ); 
                  unitAddr <<= bitWidths[MEM_ROW];
                  break;

            case MEM_COL:
                  phyAddr += ( col * unitAddr ); 
                  unitAddr <<= (bitWidths[MEM_COL] /*- lowColBits*/ );
                  break;

            case MEM_BANK:
                  phyAddr += ( bank * unitAddr ); 
                  unitAddr <<= bitWidths[MEM_BANK];
                  break;

            case MEM_RANK:
                  phyAddr += ( rank * unitAddr ); 
                  unitAddr <<= bitWidths[MEM_RANK];
                  break;

            case MEM_CHANNEL:
                  phyAddr += ( channel * unitAddr ); 
                  unitAddr <<= bitWidths[MEM_CHANNEL];
                  break;

            case MEM_SUBARRAY:
                  phyAddr += ( subarray * unitAddr ); 
                  unitAddr <<= bitWidths[MEM_SUBARRAY];
                  break;

            case MEM_UNKNOWN:
            default:
                  std::cerr << "Address Translator: No partition found for address " << std::endl;
                  break;
        }
    }

    return phyAddr;
} 

/* 
 * SetBusWidth() provides the interface to set the data bus width. By
 * default, buswidth = 64, which stands for 64-bit bus. If the bus width
 * is different from 64-bit, then this function can be called
 */ 
void AddressTranslator::SetBusWidth( int bits )
{
    busWidth = bits;
}

/* 
 * SetburstLength() provides the interface to set the burst length. By
 * default, burstLength is 8, which comply with JEDEC-DDR. If the burst length
 * is different from 8, then this function can be called
 */ 
void AddressTranslator::SetBurstLength( int beat )
{
    burstLength = beat;
}

/*
 * Translate() translates the physical address and returns the corresponding
 * values of each memory domain 
 */
void AddressTranslator::Translate( NVMainRequest *request, uint64_t *row, uint64_t *col, uint64_t *bank,
				   uint64_t *rank, uint64_t *channel, uint64_t *subarray )
{
    return Translate( request->address.GetPhysicalAddress( ), row, col, bank, rank, channel, subarray );
}

/*
 * Translate() translates the physical address and returns the corresponding
 * values of each memory domain 
 */
//get row,col,bank,rank,channel,subarray address
void AddressTranslator::Translate( uint64_t address, uint64_t *row, uint64_t *col, uint64_t *bank,
				   uint64_t *rank, uint64_t *channel, uint64_t *subarray )
{
	//std::cout<<"address translator"<<std::endl;
    uint64_t refAddress;
    MemoryPartition part= MEM_UNKNOWN;

    uint64_t *partitions[6] = { row, col, bank, rank, channel, subarray };

    if( method == NULL )
    {
        std::cerr << "Divider Translator: Translation method not specified!" << std::endl;
        return;
    }

    int busOffsetBits = mlog2( busWidth / 8 );
    int burstBits = mlog2( (busWidth * burstLength) / 8 );
    lowColBits = burstBits - busOffsetBits;

    // first of all, truncate the bus offset bits 
    refAddress = address >> busOffsetBits;

    // then, truncate the lowest column bits 
    refAddress >>= lowColBits;
	
    //get order and bitWidths 
	if(!fieldOrderInited)
	{
		for( int i=0; i<FIELD_NUM; i++)
		{
			fieldOrder[i] = method->order[i];
		}
		fieldOrderInited = true;
	}
	
	if(!bitWidthsInited)
	{
		for( int i=0; i<FIELD_NUM; i++)
		{
			bitWidths[i] = method->bitWidths[i];
		}
		bitWidthsInited = true;
	}

    // 0->4, low to high, FindOrder() will find the correct one 
    for( int i = 0; i < 6; i++ )
    {
        FindOrder_modified( i, &part );
        /* 
         *  The new memsize does not include this partition, so dividing by the
         *  new memsize will give us the right channel/rank/bank/whatever.
         */
		if(part!=MEM_UNKNOWN)
			*partitions[part] = Modulo_modified(refAddress,part);
		else
		{
			std::cerr<<"Address Translator:unknown memory"<<std::endl;
			break;
		}	
        /*
         *  "Mask off" the first partition number we got. For example if memsize = 1000
         *  and the address is 8343, the partition would be 8, and we will look at 343 
         *  to determine the rest of the address now.
         */
        //refAddress = Divide_modified( refAddress, part ); 
    }
} 

uint64_t AddressTranslator::Translate( NVMainRequest *request )
{
    uint64_t rv = 0;
    if( request->address.IsTranslated( ) )
    {
        switch( defaultField )
        {
            case ROW_FIELD:
                rv = request->address.GetRow( );
                break;

            case COL_FIELD:
                rv = request->address.GetCol( );
                break;

            case BANK_FIELD:
                rv = request->address.GetBank( );
                break;

            case RANK_FIELD:
                rv = request->address.GetRank( );
                break;

            case CHANNEL_FIELD:
                rv = request->address.GetChannel( );
                break;

            case SUBARRAY_FIELD:
                rv = request->address.GetSubArray( );
                break;

            case NO_FIELD:
            default:
                rv = 0;
                break;
        }
    }
    else
    {
        rv = Translate( request->address.GetPhysicalAddress( ) );
    }
    return rv;
}

//return translated specified field according to defaultField
uint64_t AddressTranslator::Translate( uint64_t address )
{
    uint64_t row, col, bank, rank, channel, subarray;
    uint64_t rv;

    Translate( address, &row, &col, &bank, &rank, &channel, &subarray );

    switch( defaultField )
    {
        case ROW_FIELD:
            rv = row;
            break;

        case COL_FIELD:
            rv = col;
            break;

        case BANK_FIELD:
            rv = bank;
            break;

        case RANK_FIELD:
            rv = rank;
            break;

        case CHANNEL_FIELD:
            rv = channel;
            break;

        case SUBARRAY_FIELD:
            rv = subarray;
            break;

        case NO_FIELD:
        default:
            rv = 0;
            break;
    }

    return rv;
}

void AddressTranslator::SetDefaultField( TranslationField f )
{
    defaultField = f;
}
//added by YuJieChen on 2015,29,Jan in order to improve program's performance
//get rid of partition specified address,get the rest parts of addresses
uint64_t AddressTranslator::Divide_modified( uint64_t partSize, MemoryPartition partition )
{
    uint64_t retSize = partSize;
    
    if( partition == MEM_ROW )
        retSize >>= bitWidths[MEM_ROW];
    else if( partition == MEM_COL )
        retSize >>= ( bitWidths[MEM_COL] /*- lowColBits*/ );
    else if( partition == MEM_BANK )
        retSize >>= bitWidths[MEM_BANK];
    else if( partition == MEM_RANK )
        retSize >>= bitWidths[MEM_RANK];
    else if( partition == MEM_CHANNEL )
        retSize >>= bitWidths[MEM_CHANNEL];
    else if( partition == MEM_SUBARRAY )
        retSize >>= bitWidths[MEM_SUBARRAY];
    else
        std::cout << "Divider Translator: Warning: Invalid partition " << (int)partition << std::endl;

    return retSize;
}

/*
 * Divide() right shift the physical address for address translation
 */
uint64_t AddressTranslator::Divide( uint64_t partSize, MemoryPartition partition )
{
    uint64_t retSize = partSize;
    unsigned channelBits, rankBits, bankBits, rowBits, colBits, subarrayBits;

    method->GetBitWidths( &rowBits, &colBits, &bankBits, 
                          &rankBits, &channelBits, &subarrayBits );
    
    if( partition == MEM_ROW )
        retSize >>= rowBits;
    else if( partition == MEM_COL )
        retSize >>= ( colBits /*- lowColBits*/ );
    else if( partition == MEM_BANK )
        retSize >>= bankBits;
    else if( partition == MEM_RANK )
        retSize >>= rankBits;
    else if( partition == MEM_CHANNEL )
        retSize >>= channelBits;
    else if( partition == MEM_SUBARRAY )
        retSize >>= subarrayBits;
    else
        std::cout << "Divider Translator: Warning: Invalid partition " << (int)partition << std::endl;

    return retSize;
}

/*
 * Modulo() extract the corresponding bits for translation
 */
//Added by YuJieChen on 2015,29,JAN
//call GetBitWidths too many times will affect program's performance
//so I made rowBits,bankBits... as class's data member
uint64_t AddressTranslator::Modulo_modified( uint64_t &patialAddr,MemoryPartition partition)
{
	
    uint64_t retVal=0;
    uint64_t moduloSize = 1;

    if( partition == MEM_ROW )
        moduloSize <<= bitWidths[MEM_ROW];
    else if( partition == MEM_COL )
        moduloSize <<= ( bitWidths[MEM_COL]/*- lowColBits*/ );
    else if( partition == MEM_BANK )
        moduloSize <<= bitWidths[MEM_BANK];
    else if( partition == MEM_RANK )
        moduloSize <<= bitWidths[MEM_RANK];
    else if( partition == MEM_CHANNEL )
        moduloSize <<= bitWidths[MEM_CHANNEL];
    else if( partition == MEM_SUBARRAY )
        moduloSize <<= bitWidths[MEM_SUBARRAY];
    else
        std::cout << "Modulo Translator: Warning: Invalid partition " << (int)partition << std::endl;

    retVal = patialAddr % moduloSize;
	patialAddr /= moduloSize;

    return retVal;
}

//
uint64_t AddressTranslator::Modulo( uint64_t partialAddr, MemoryPartition partition )
{
    uint64_t retVal = partialAddr;
    unsigned channelBits, rankBits, bankBits, rowBits, colBits, subarrayBits;

    method->GetBitWidths( &rowBits, &colBits, &bankBits, 
                          &rankBits, &channelBits, &subarrayBits );

    uint64_t moduloSize = 1;

    if( partition == MEM_ROW )
        moduloSize <<= rowBits;
    else if( partition == MEM_COL )
        moduloSize <<= ( colBits /*- lowColBits*/ );
    else if( partition == MEM_BANK )
        moduloSize <<= bankBits;
    else if( partition == MEM_RANK )
        moduloSize <<= rankBits;
    else if( partition == MEM_CHANNEL )
        moduloSize <<= channelBits;
    else if( partition == MEM_SUBARRAY )
        moduloSize <<= subarrayBits;
    else
        std::cout << "Modulo Translator: Warning: Invalid partition " << (int)partition << std::endl;

    retVal = partialAddr % moduloSize;

    return retVal;
}



/*
 * FindOrder() finding the right memory domain that matches the input order
 */
//modified by YuJieChen On 2015,29,Jan
void AddressTranslator::FindOrder_modified( int order,MemoryPartition *p)
{
	if(order == fieldOrder[MEM_ROW])
		*p = MEM_ROW;
	else if(order == fieldOrder[MEM_COL])
		*p = MEM_COL;
	else if(order == fieldOrder[MEM_BANK])
		*p = MEM_BANK;
	else if(order == fieldOrder[MEM_RANK])
		*p = MEM_RANK;
	else if(order == fieldOrder[MEM_CHANNEL])
		*p = MEM_CHANNEL;
	else if(order == fieldOrder[MEM_SUBARRAY])
		*p = MEM_SUBARRAY;
	else
	{
		*p = MEM_UNKNOWN;
		std::cerr<<"AdressTranslation : NO order"<<order<<std::endl;
		
	}
}

void AddressTranslator::FindOrder( int order, MemoryPartition *p )
{
    int rowOrder, colOrder, bankOrder, rankOrder, channelOrder, subarrayOrder;

    method->GetOrder( &rowOrder, &colOrder, &bankOrder, 
                   &rankOrder, &channelOrder, &subarrayOrder );

    if( rowOrder == order )
        *p = MEM_ROW;
    else if( colOrder == order )
        *p = MEM_COL;
    else if( bankOrder == order )
        *p = MEM_BANK;
    else if( rankOrder == order )
        *p = MEM_RANK;
    else if( channelOrder == order )
        *p = MEM_CHANNEL;
    else if( subarrayOrder == order )
        *p = MEM_SUBARRAY;
    else
    {
        *p = MEM_UNKNOWN;
        std::cerr << "Address Translator: No order " << order << std::endl << "Row = " << rowOrder
	      << " Column = " << colOrder << " Bank = " << bankOrder << " Rank = " << rankOrder
	      << " Channel = " << channelOrder << " SubArray = " << subarrayOrder << std::endl;
    }
}


void AddressTranslator::SetStats( Stats *s )
{
    stats = s;
}


Stats *AddressTranslator::GetStats( )
{
    return stats;
}


void AddressTranslator::StatName( std::string name )
{
    statName = name;
}


std::string AddressTranslator::StatName( )
{
    return statName;
}

