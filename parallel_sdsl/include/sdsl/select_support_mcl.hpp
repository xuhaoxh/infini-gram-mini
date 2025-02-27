/* sdsl - succinct data structures library
    Copyright (C) 2008 Simon Gog

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/ .
*/
/*! \file select_support_mcl.hpp
    \brief select_support_mcl.hpp contains classes that support a sdsl::bit_vector with constant time select information.
    \author Simon Gog
*/
#ifndef INCLUDED_SDSL_SELECT_SUPPORT_MCL
#define INCLUDED_SDSL_SELECT_SUPPORT_MCL

#include "int_vector.hpp"
#include "util.hpp"
#include "select_support.hpp"
#include "sequence.hpp"

//! Namespace for the succinct data structure library.
namespace sdsl
{

//! A class supporting constant time select queries.
/*!
 * \par Space usage
 *      The space usage of the data structure depends on the number of \f$ m \f$ of ones in the
 *      original bitvector $b$. We store the position of every $4096$th set bit
 *      (called L1-sampled bits) of $b$.
 *      This takes in the worst case \f$\frac{m}{4096} \log{n} \leq \frac{64}{n}\f$ bits.
 *      Next,
 *      (1) if the distance of two adjacent L1-sampled bits $b[i]$ and $b[j]$
 *      is greater or equal than $\log^4 n$, then
 *      we store each of the 4096 positions of the set $b$ in [i..j-1] with
 *      $\log{n}$ bits. This results in at most
 *      \$ \frac{4096\cdot \log n}{\log^4 n}=\frac{4096}{\log^3 n}\$ bits per bit.
 *      For a bitvector of 1GB, i.e. \f$ \log n = 35 \f$ we get about 0.01 bits per bit.
 *      If the $j-i+1 < \log^4 n$ then
 *      (2) we store the relative position of every $64$th set bit (called L2-sampled bits)
 *      in b[i..j-1] in at most $4\log\log n$ bits per L2-sampled bits.
 *      An pessimistic upper bound for the space would be
 *      \f$ \frac{4\log\log n}{64} \leq \frac{24}{64} = 0.375\f$ bit per
 *      bit (since $\log\log n\leq 6$. It is very pessimistic, since we store
 *      the relative position in $\log\log(j-i+1)\leq \log\log n$ bits.
 *
 * \tparam t_b       Bit pattern `0`,`1`,`10`,`01` which should be ranked.
 * \tparam t_pat_len Length of the bit pattern.
 *
 * The implementation is a practical variant of the following reference:
 *
 * \par Reference
 *      David Clark:
 *      PhD Thesis: Compact Pat Trees
 *      University of Waterloo, 1996 (Section 2.2.2).
 *      http://www.nlc-bnc.ca/obj/s4/f2/dsk3/ftp04/nq21335.pdf
 *
 * @ingroup select_support_group
 */
template<uint8_t t_b=1, uint8_t t_pat_len=1>
class select_support_mcl : public select_support
{
    private:
        static_assert(t_b == 1u or t_b == 0u or t_b == 10u or t_b == 11u, "select_support_mcl: bit pattern must be `0`,`1`,`10`, `01`, or `11`");
        static_assert(t_pat_len == 1u or t_pat_len == 2u , "select_support_mcl: bit pattern length must be 1 or 2");
    public:
        typedef bit_vector bit_vector_type;
        enum { bit_pat = t_b };
        enum { bit_pat_len = t_pat_len };
    private:
        uint32_t m_logn                 = 0,     // \f$ log(size) \f$
                 m_logn2                = 0,     // \f$ log^2(size) \f$
                 m_logn4                = 0;     // \f$ log^4(size) \f$
        // entry i of m_superblock equals the answer to select_1(B,i*4096)
        int_vector<0> m_superblock;
        int_vector<0>* m_longsuperblock = nullptr;
        int_vector<0>* m_miniblock      = nullptr;
        size_type m_arg_cnt             = 0;
        void copy(const select_support_mcl<t_b, t_pat_len>& ss);
        void initData();
        void init_fast(const bit_vector* v=nullptr);
	// Helper Functions
	size_type sum_args_serial(size_type s, size_type e);
	void init_superblock_serial(int_vector<64>& blockstarts, int_vector<64>& blockends, size_type arg_cnt, 
			std::pair<uint64_t, uint64_t>* sb_to_chunk, uint64_t chunk_id, 
			size_type s, size_type e, const size_type SUPER_BLOCK_SIZE);
	void init_longblock_serial(int_vector<64>& longblock, size_type s, size_type e, size_type offset);
	void init_miniblock_serial(int_vector<0>& miniblock, size_type s, size_type e);
    public:
        explicit select_support_mcl(const bit_vector* v=nullptr);
        select_support_mcl(const select_support_mcl<t_b,t_pat_len>& ss);
        select_support_mcl(select_support_mcl<t_b,t_pat_len>&& ss);
        ~select_support_mcl();
        void init_slow(const bit_vector* v=nullptr);
        //! Select function
        inline size_type select(size_type i) const;
        //! Alias for select(i).
        inline size_type operator()(size_type i)const;
        size_type serialize(std::ostream& out, structure_tree_node* v=nullptr, std::string name="")const;
        void load(std::istream& in, const bit_vector* v=nullptr);
        void set_vector(const bit_vector* v=nullptr);
        select_support_mcl<t_b, t_pat_len>& operator=(const select_support_mcl& ss);
        select_support_mcl<t_b, t_pat_len>& operator=(select_support_mcl&&);
        void swap(select_support_mcl<t_b, t_pat_len>& ss);
};


template<uint8_t t_b, uint8_t t_pat_len>
select_support_mcl<t_b,t_pat_len>::select_support_mcl(const bit_vector* f_v):select_support(f_v)
{
    if (t_pat_len>1 or(v!=nullptr and  v->size() < 100000))
        init_slow(v);
    else {
        init_fast(v);
   }
    return;
}

template<uint8_t t_b, uint8_t t_pat_len>
select_support_mcl<t_b,t_pat_len>::select_support_mcl(const select_support_mcl& ss):select_support(ss.m_v)
{
    copy(ss);
}

template<uint8_t t_b, uint8_t t_pat_len>
select_support_mcl<t_b,t_pat_len>::select_support_mcl(select_support_mcl&& ss) : select_support(ss.m_v)
{
    *this = std::move(ss);
}

template<uint8_t t_b, uint8_t t_pat_len>
select_support_mcl<t_b, t_pat_len>& select_support_mcl<t_b,t_pat_len>::operator=(const select_support_mcl& ss)
{
    if (this != &ss) {
        copy(ss);
    }
    return *this;
}

template<uint8_t t_b, uint8_t t_pat_len>
select_support_mcl<t_b, t_pat_len>& select_support_mcl<t_b,t_pat_len>::operator=(select_support_mcl&& ss)
{
    if (this != &ss) {
        m_logn       = ss.m_logn;      // copy log n
        m_logn2      = ss.m_logn2;      // copy (logn)^2
        m_logn4      = ss.m_logn4;      // copy (logn)^4
        m_superblock = std::move(ss.m_superblock); // move long superblock
        m_arg_cnt    = ss.m_arg_cnt;    // copy count of 1-bits
        m_v          = ss.m_v;          // copy pointer to the supported bit vector

        if (m_longsuperblock!=nullptr)
            delete [] m_longsuperblock;
        m_longsuperblock = ss.m_longsuperblock;
        ss.m_longsuperblock = nullptr;

        if (m_miniblock!=nullptr)
            delete [] m_miniblock;
        m_miniblock = ss.m_miniblock;
        ss.m_miniblock = nullptr;
    }
    return *this;
}

template<uint8_t t_b, uint8_t t_pat_len>
void select_support_mcl<t_b,t_pat_len>::swap(select_support_mcl& ss)
{
    std::swap(m_logn, ss.m_logn);
    std::swap(m_logn2, ss.m_logn2);
    std::swap(m_logn4, ss.m_logn4);
    m_superblock.swap(ss.m_superblock);
    std::swap(m_longsuperblock, ss.m_longsuperblock);
    std::swap(m_miniblock, ss.m_miniblock);
    std::swap(m_arg_cnt, ss.m_arg_cnt);
}

template<uint8_t t_b, uint8_t t_pat_len>
void select_support_mcl<t_b,t_pat_len>::copy(const select_support_mcl<t_b, t_pat_len>& ss)
{
    m_logn        = ss.m_logn;      // copy log n
    m_logn2      = ss.m_logn2;      // copy (logn)^2
    m_logn4      = ss.m_logn4;      // copy (logn)^4
    m_superblock = ss.m_superblock; // copy long superblock
    m_arg_cnt    = ss.m_arg_cnt;    // copy count of 1-bits
    m_v          = ss.m_v;          // copy pointer to the supported bit vector
    size_type sb = (m_arg_cnt+4095)>>12;
    if (m_longsuperblock!=nullptr)
        delete [] m_longsuperblock;
    m_longsuperblock = nullptr;
    if (ss.m_longsuperblock!=nullptr) {
        m_longsuperblock = new int_vector<0>[sb]; //copy longsuperblocks
        for (size_type i=0; i<sb; ++i) {
            m_longsuperblock[i] = ss.m_longsuperblock[i];
        }
    }
    if (m_miniblock!=nullptr)
        delete [] m_miniblock;
    m_miniblock = nullptr;
    if (ss.m_miniblock!=nullptr) {
        m_miniblock = new int_vector<0>[sb]; // copy miniblocks
        for (size_type i=0; i<sb; ++i) {
            m_miniblock[i] = ss.m_miniblock[i];
        }
    }
}

template<uint8_t t_b, uint8_t t_pat_len>
select_support_mcl<t_b,t_pat_len>::~select_support_mcl()
{
    if (m_longsuperblock!=nullptr)
        delete[] m_longsuperblock;
    if (m_miniblock!=nullptr)
        delete[] m_miniblock;
}

template<uint8_t t_b, uint8_t t_pat_len>
void select_support_mcl<t_b,t_pat_len>::init_slow(const bit_vector* v)
{
    set_vector(v);
    initData();
    if (m_v==nullptr)
        return;
    // Count the number of arguments in the bit vector
    m_arg_cnt = select_support_trait<t_b,t_pat_len>::arg_cnt(*v);

    const size_type SUPER_BLOCK_SIZE = 4096;

    if (m_arg_cnt==0) // if there are no arguments in the vector we are done...
        return;

    size_type sb = (m_arg_cnt+SUPER_BLOCK_SIZE-1)/SUPER_BLOCK_SIZE; // number of superblocks
    if (m_miniblock != nullptr) delete [] m_miniblock;
    m_miniblock = new int_vector<0>[sb];

    m_superblock = int_vector<0>(sb, 0, m_logn);


    size_type arg_position[SUPER_BLOCK_SIZE], arg_cnt=0;
    size_type sb_cnt=0;
    for (size_type i=0; i < v->size(); ++i) {
        if (select_support_trait<t_b,t_pat_len>::found_arg(i, *v)) {
            arg_position[ arg_cnt%SUPER_BLOCK_SIZE ] = i;
            assert(arg_position[arg_cnt%SUPER_BLOCK_SIZE] == i);
            ++arg_cnt;
            if (arg_cnt % SUPER_BLOCK_SIZE == 0 or arg_cnt == m_arg_cnt) { //
                assert(sb_cnt < sb);
                m_superblock[sb_cnt] = arg_position[0];

                size_type pos_diff = arg_position[(arg_cnt-1)%SUPER_BLOCK_SIZE]-arg_position[0];
                if (pos_diff > m_logn4) { // longblock
                    if (m_longsuperblock == nullptr) m_longsuperblock = new int_vector<0>[sb]; // create longsuperblock
                    m_longsuperblock[sb_cnt] = int_vector<0>(SUPER_BLOCK_SIZE, 0, bits::hi(arg_position[(arg_cnt-1)%SUPER_BLOCK_SIZE]) + 1);

                    for (size_type j=0; j <= (arg_cnt-1)%SUPER_BLOCK_SIZE ; ++j) m_longsuperblock[sb_cnt][j] = arg_position[j]; // copy argument positions to longsuperblock
                } else { // short block
                    m_miniblock[sb_cnt] = int_vector<0>(64, 0, bits::hi(pos_diff)+1);
                    for (size_type j=0; j <= (arg_cnt-1)%SUPER_BLOCK_SIZE; j+=64) {
                        m_miniblock[sb_cnt][j/64] = arg_position[j]-arg_position[0];
                    }
                }
                ++sb_cnt;
            }
        }
    }
}
template<uint8_t t_b, uint8_t t_pat_len>
typename select_support_mcl<t_b,t_pat_len>::size_type select_support_mcl<t_b,t_pat_len>::sum_args_serial(size_type s, size_type e) {
	size_type result = 0;
	const uint64_t* data = m_v->data();
	uint64_t carry = 0;
	// first partial block
	while (s % 64 != 0 && e < s) {
		if (select_support_trait<t_b, t_pat_len>::found_arg(s, *m_v)) 
			result++;
		s++;
	}
	// get carry
	if (s > 63) {
		select_support_trait<t_b, t_pat_len>::args_in_the_word(data[s/64-1], carry);	
	}
	// sum blockwise
	while (s/64 < e/64) {
		result += select_support_trait<t_b, t_pat_len>::args_in_the_word(data[s/64], carry);
		s += 64;
	}
	// last partial block
	while (s < e) {
		if (select_support_trait<t_b, t_pat_len>::found_arg(s, *m_v)) 
			result++;
		s++;
	}	
	return result;
}


template<uint8_t t_b, uint8_t t_pat_len>
void select_support_mcl<t_b,t_pat_len>::init_superblock_serial(
		int_vector<64>& blockstarts, 
		int_vector<64>& blockends, 
		size_type arg_cnt, 
		std::pair<uint64_t, uint64_t>* sb_to_chunk, 
		uint64_t chunk_id, 
		size_type s, size_type e, 
		const size_type SUPER_BLOCK_SIZE) {
	const uint64_t* data = m_v->data();
	uint64_t carry = 0;
	// first partial block
	while (s % 64 != 0 && s < e) {
	//while (s < e) {
		if (select_support_trait<t_b,t_pat_len>::found_arg(s, *m_v)) {
			// Set end
			if ((arg_cnt + 1) % SUPER_BLOCK_SIZE == 0) {
				blockends[arg_cnt / SUPER_BLOCK_SIZE] = s; 
				sb_to_chunk[arg_cnt / SUPER_BLOCK_SIZE].second = chunk_id;
			}
			// Set start
			if (arg_cnt % SUPER_BLOCK_SIZE == 0 ) {
				blockstarts[arg_cnt / SUPER_BLOCK_SIZE] = s; 
				sb_to_chunk[arg_cnt / SUPER_BLOCK_SIZE].first = chunk_id;
			}
			arg_cnt++;
			// special case for last block 
			if (arg_cnt == m_arg_cnt) {
				blockends[blockends.size()-1] = s; 
				sb_to_chunk[blockends.size()-1].second = chunk_id;
			}
		}
		s++;
	}
	if (arg_cnt == m_arg_cnt) return; 
	// get carry
	if (s > 63) {
		select_support_trait<t_b, t_pat_len>::args_in_the_word(data[s/64-1], carry);	
	}
	size_type arg_cnt_old = arg_cnt;
	uint64_t carry_old = carry;
	// Traverse blockwise
	while (s/64 < e/64) {
		arg_cnt += select_support_trait<t_b, t_pat_len>::args_in_the_word(data[s/64], carry);	
		// if start of a superblock is in the current block 
		if ((arg_cnt-1) / SUPER_BLOCK_SIZE != (arg_cnt_old-1)/SUPER_BLOCK_SIZE) {
			size_type block_num = (arg_cnt-1) / SUPER_BLOCK_SIZE;
			blockstarts[block_num] = s + select_support_trait<t_b, t_pat_len>::ith_arg_pos_in_the_word(data[s/64],
				       	block_num * SUPER_BLOCK_SIZE - arg_cnt_old+1, carry_old);			
			sb_to_chunk[block_num].first = chunk_id;
		}
		// if end of a superblock is in the current block
		if (arg_cnt / SUPER_BLOCK_SIZE != arg_cnt_old / SUPER_BLOCK_SIZE) {
			// id of block which just started
			size_type block_num = arg_cnt / SUPER_BLOCK_SIZE;
			blockends[block_num-1] = s + select_support_trait<t_b, t_pat_len>::ith_arg_pos_in_the_word(data[s/64],
				       	block_num * SUPER_BLOCK_SIZE - arg_cnt_old, carry_old);			
			sb_to_chunk[block_num-1].second = chunk_id;
		}
		// special case for last block
		if (arg_cnt == m_arg_cnt && arg_cnt_old < arg_cnt) {
			blockends[blockends.size()-1] = s + select_support_trait<t_b, t_pat_len>::ith_arg_pos_in_the_word(data[s/64],
					arg_cnt - arg_cnt_old, carry_old);
			sb_to_chunk[blockends.size()-1].second = chunk_id;
			break; // Important to stop after last argument
		}
		arg_cnt_old = arg_cnt;
		carry_old = carry;		
		s += 64;
	}	
	// last partial block
	while (s < e) {
		if (select_support_trait<t_b,t_pat_len>::found_arg(s, *m_v)) {
			// Set end
			if ((arg_cnt + 1) % SUPER_BLOCK_SIZE == 0) {
				blockends[arg_cnt / SUPER_BLOCK_SIZE] = s;
				sb_to_chunk[arg_cnt / SUPER_BLOCK_SIZE].second = chunk_id;
			}
			// Set start
			if (arg_cnt % SUPER_BLOCK_SIZE == 0 && arg_cnt < m_arg_cnt ) {
				blockstarts[arg_cnt / SUPER_BLOCK_SIZE] = s;
				sb_to_chunk[arg_cnt / SUPER_BLOCK_SIZE].first = chunk_id;
			}
			arg_cnt++;
			// special case for last block 
			if (arg_cnt == m_arg_cnt) {
				blockends[blockends.size()-1] = s;
				sb_to_chunk[blockends.size()-1].second = chunk_id;
			}
		}	
		s++;
	}
}

template<uint8_t t_b, uint8_t t_pat_len>
void select_support_mcl<t_b,t_pat_len>::init_longblock_serial(int_vector<64>& longblock, size_type s, size_type e, size_type offset) {
	size_type arg_cnt = offset;	
	const uint64_t* data = v->data();
	uint64_t carry = 0;
	// first partial block
	while (s % 64 != 0 && s < e) {
		if (select_support_trait<t_b, t_pat_len>::found_arg(s, *v)) {
			longblock[arg_cnt] = s;
			arg_cnt++;
		}	
		s++;
	}
	// init carry
	if (s > 63) {
		select_support_trait<t_b, t_pat_len>::args_in_the_word(data[s/64-1], carry);
	}
	uint64_t carry_old = carry;
	size_type arg_cnt_old = arg_cnt;
	while (s/64 < e/64) {	
		arg_cnt += select_support_trait<t_b, t_pat_len>::args_in_the_word(data[s/64], carry);
		for (size_type j = 1; j <= arg_cnt - arg_cnt_old; j++) {	
			longblock[arg_cnt_old+j-1] = s + select_support_trait<t_b, t_pat_len>::ith_arg_pos_in_the_word(data[s/64], j, carry_old);
		}
		arg_cnt_old = arg_cnt;
		carry_old = carry;
		s += 64;
	}
	// last partial block
	if ( s > e) s -= 64;
	while (s < e) {
		if (select_support_trait<t_b, t_pat_len>::found_arg(s, *v)) {
			longblock[arg_cnt] = s;
			arg_cnt++;
		}	
		s++;
	}
}
// Sample the delta to start of every 64 argument
template<uint8_t t_b, uint8_t t_pat_len>
void select_support_mcl<t_b,t_pat_len>::init_miniblock_serial(int_vector<0>& miniblock, size_type s, size_type e) {
const uint64_t* data = v->data();
uint64_t carry, carry_old;
carry = carry_old = 0;
size_type arg_cnt, arg_cnt_old;
arg_cnt = arg_cnt_old = 0;
size_type i = s;

// First partial block
while (i % 64 != 0 && i < e) {
	if (select_support_trait<t_b, t_pat_len>::found_arg(i, *v)) {
		if (arg_cnt % 64 == 0) {
			miniblock[arg_cnt/64] = i-s;	
		}	
		arg_cnt++;
	}
	i++;
}
arg_cnt_old = arg_cnt;
// blockwise
while (i/64 < e/64) {
	arg_cnt += select_support_trait<t_b, t_pat_len>::args_in_the_word(data[i/64], carry);
	if ((arg_cnt-1)/64 != (arg_cnt_old-1)/64) {
		miniblock[(arg_cnt-1)/64] = i-s + select_support_trait<t_b, t_pat_len>::ith_arg_pos_in_the_word(data[i/64], 64 - ((arg_cnt_old-1)%64) , carry_old);	
	}
	arg_cnt_old = arg_cnt;
	carry_old = carry;
	i += 64;
} 

// Rest of the interval
while (i < e) {
	if (select_support_trait<t_b, t_pat_len>::found_arg(i, *v)) {
		if (arg_cnt % 64 == 0) {
			miniblock[arg_cnt/64] = i-s;	
		}	
		arg_cnt++;
	}
	i++;
}
}

template<uint8_t t_b, uint8_t t_pat_len>
void select_support_mcl<t_b,t_pat_len>::init_fast(const bit_vector* v)
{
set_vector(v);
initData();
if (m_v==nullptr)
return;
//    m_arg_cnt = select_support_trait<t_b,t_pat_len>::arg_cnt(*v); 
// Calculate prefix sum blockwise over sum args 
size_type num_blocks = nblocks(v->size(), _SCAN_BSIZE<<3); 
bit_vector::size_type *block_sum_arg = new bit_vector::size_type[num_blocks];
bit_vector::size_type s = 0;
bit_vector::size_type e = v->size();
blocked_for (i, s, e, _SCAN_BSIZE<<3, 
	 block_sum_arg[i] = sum_args_serial(s, e););
m_arg_cnt =  sequence::scan<bit_vector::size_type>(block_sum_arg, block_sum_arg, num_blocks, utils::addF<bit_vector::size_type>(), 0);



const size_type SUPER_BLOCK_SIZE = 64*64;

if (m_arg_cnt==0) // if there are no arguments in the vector we are done...
return;

//    size_type sb = (m_arg_cnt+63+SUPER_BLOCK_SIZE-1)/SUPER_BLOCK_SIZE; // number of superblocks, add 63 as the last block could contain 63 uninitialized bits
size_type sb = (m_arg_cnt+SUPER_BLOCK_SIZE-1)/SUPER_BLOCK_SIZE; // number of superblocks
if (m_miniblock != nullptr) delete [] m_miniblock;
m_miniblock = new int_vector<0>[sb+1];
if (m_longsuperblock != nullptr) delete [] m_longsuperblock;
m_longsuperblock = new int_vector<0>[sb+1];

int_vector<64> superblockstart(sb+1, 0); 
int_vector<64> superblockend(sb, 0); 
// Assings start and end chunk to every superblock
std::pair<uint64_t, uint64_t>* sb_to_chunk = new std::pair<uint64_t, uint64_t>[sb];
// Init m_superblock with blocked for 
blocked_for (i, s, e, _SCAN_BSIZE<<3, 
	init_superblock_serial(superblockstart, superblockend, block_sum_arg[i], sb_to_chunk, i, s, e, SUPER_BLOCK_SIZE);
	);
// Calculate long/miniblocks 
parallel_for (uint64_t i = 0; i < sb; ++i) {
	size_type cnt_s = superblockstart[i];
	size_type cnt_e = superblockend[i];
	if (cnt_e - cnt_s > m_logn4) { // Long
		m_longsuperblock[i] = int_vector<0>(SUPER_BLOCK_SIZE, 0, bits::hi(cnt_e) + 1);
		int_vector<64> temp_longblock = int_vector<64>(SUPER_BLOCK_SIZE, 0);
		// Do chunks in parallel
		for (uint64_t c = sb_to_chunk[i].first; c <= sb_to_chunk[i].second; ++c) {
			size_type cur_start = std::max(cnt_s, (size_type)c * (_SCAN_BSIZE<<3));
			size_type cur_end = std::min(cnt_e +1, (size_type)(c+1)*(_SCAN_BSIZE<<3));
			size_type cur_offset = cur_start == cnt_s ? 0 : (block_sum_arg[c] - i*SUPER_BLOCK_SIZE); 
			init_longblock_serial(temp_longblock, cur_start, cur_end, cur_offset);
		}
		for (uint64_t j = 0; j < SUPER_BLOCK_SIZE; j++) m_longsuperblock[i][j] = temp_longblock[j];
	} else { // Miniblock
		m_miniblock[i] = int_vector<0>(64, 0, bits::hi(cnt_e-cnt_s)+1);
		init_miniblock_serial(m_miniblock[i], cnt_s, cnt_e+1);
	}
}
delete[] sb_to_chunk;
delete []block_sum_arg;
m_superblock = int_vector<0>(sb, 0, m_logn);
s = 0; e = sb;
blocked_for (i, s, e, 64, 
	for (bit_vector::size_type j = s; j < (bit_vector::size_type)e; j++) m_superblock[j] = superblockstart[j];);

bool empty = true;
{parallel_for (uint64_t i = 0; i < sb; i++) {
	if (m_longsuperblock[i].size() > 0)
		empty = false;
}}
if (empty) {
	delete []m_longsuperblock;
	m_longsuperblock = nullptr;
}
}


template<uint8_t t_b, uint8_t t_pat_len>
inline auto select_support_mcl<t_b,t_pat_len>::select(size_type i)const -> size_type
{
    assert(i > 0 and i <= m_arg_cnt);

    i = i-1;
    size_type sb_idx = i>>12;   // i/4096
    size_type offset = i&0xFFF; // i%4096
    if (m_longsuperblock!=nullptr and !m_longsuperblock[sb_idx].empty()) {
        return m_longsuperblock[sb_idx][offset];
    } else {
        if ((offset&0x3F)==0) {
            assert(sb_idx < m_superblock.size());
            assert((offset>>6) < m_miniblock[sb_idx].size());
            return m_superblock[sb_idx] + m_miniblock[sb_idx][offset>>6/*/64*/];
        } else {
            i = i-(sb_idx<<12)-((offset>>6)<<6);
            // now i > 0 and i <= 64
            assert(i > 0);
            size_type pos = m_superblock[sb_idx] + m_miniblock[sb_idx][offset>>6] + 1;

            // now pos is the position from where we search for the ith argument
            size_type word_pos = pos>>6;
            size_type word_off = pos&0x3F;
            const uint64_t* data = m_v->data() + word_pos;
            uint64_t carry = select_support_trait<t_b,t_pat_len>::init_carry(data, word_pos);
            size_type args = select_support_trait<t_b,t_pat_len>::args_in_the_first_word(*data, word_off, carry);

            if (args >= i) {
                return (word_pos<<6)+select_support_trait<t_b,t_pat_len>::ith_arg_pos_in_the_first_word(*data, i, word_off, carry);
            }
            word_pos+=1;
            size_type sum_args = args;
            carry = select_support_trait<t_b,t_pat_len>::get_carry(*data);
            uint64_t old_carry = carry;
            args = select_support_trait<t_b,t_pat_len>::args_in_the_word(*(++data), carry);
            while (sum_args + args < i) {
                sum_args += args;
                assert(data+1 < m_v->data() + (m_v->capacity()>>6));
                old_carry = carry;
                args = select_support_trait<t_b,t_pat_len>::args_in_the_word(*(++data), carry);
                word_pos+=1;
            }
            return (word_pos<<6) +
                   select_support_trait<t_b,t_pat_len>::ith_arg_pos_in_the_word(*data, i-sum_args, old_carry);
        }
    }
}

template<uint8_t t_b, uint8_t t_pat_len>
inline auto select_support_mcl<t_b,t_pat_len>::operator()(size_type i)const -> size_type
{
    return select(i);
}

template<uint8_t t_b, uint8_t t_pat_len>
void select_support_mcl<t_b,t_pat_len>::initData()
{
    m_arg_cnt = 0;
    if (nullptr == m_v) {
        m_logn = m_logn2 = m_logn4 = 0;
    } else {
        m_logn = bits::hi(m_v->capacity())+1; // TODO maybe it's better here to take a max(...,12)
        m_logn2 = m_logn*m_logn;
        m_logn4 = m_logn2*m_logn2;
    }
    if (nullptr != m_longsuperblock)
        delete[] m_longsuperblock;
    m_longsuperblock = nullptr;
    if (nullptr != m_miniblock)
        delete[] m_miniblock;
    m_miniblock = nullptr;
}

template<uint8_t t_b, uint8_t t_pat_len>
void select_support_mcl<t_b,t_pat_len>::set_vector(const bit_vector* v)
{
    m_v = v;
}

template<uint8_t t_b, uint8_t t_pat_len>
auto select_support_mcl<t_b,t_pat_len>::serialize(std::ostream& out, structure_tree_node* v, std::string name)const -> size_type
{
    structure_tree_node* child = structure_tree::add_child(v, name, util::class_name(*this));
    size_type written_bytes = 0;
    // write the number of 1-bits in the supported bit_vector
    out.write((char*) &m_arg_cnt, sizeof(size_type)/sizeof(char));
    written_bytes = sizeof(size_type)/sizeof(char);
    // number of superblocks in the data structure
    size_type sb = (m_arg_cnt+4095)>>12;

    if (m_arg_cnt) { // if there exists 1-bits to be supported
        written_bytes += m_superblock.serialize(out, child, "superblock"); // serialize superblocks
        bit_vector mini_or_long;// Helper vector: mini or long block?
        if (m_longsuperblock!=nullptr) {
            mini_or_long.resize(sb); // resize indicator bit_vector to the number of superblocks
            for (size_type i=0; i< sb; ++i)
                mini_or_long[i] = !m_miniblock[i].empty();
        }
        written_bytes += mini_or_long.serialize(out, child, "mini_or_long");
        size_type written_bytes_long = 0;
        size_type written_bytes_mini = 0;
        for (size_type i=0; i < sb; ++i)
            if (!mini_or_long.empty() and !mini_or_long[i]) {
                written_bytes_long += m_longsuperblock[i].serialize(out);
            } else {
                written_bytes_mini += m_miniblock[i].serialize(out);
            }
        written_bytes += written_bytes_long;
        written_bytes += written_bytes_mini;
        structure_tree_node* child_long = structure_tree::add_child(child, "longsuperblock", util::class_name(m_longsuperblock));
        structure_tree::add_size(child_long, written_bytes_long);
        structure_tree_node* child_mini = structure_tree::add_child(child, "minisuperblock", util::class_name(m_miniblock));
        structure_tree::add_size(child_mini, written_bytes_mini);
    }
    structure_tree::add_size(child, written_bytes);
    return written_bytes;
}

template<uint8_t t_b, uint8_t t_pat_len>
void select_support_mcl<t_b,t_pat_len>::load(std::istream& in, const bit_vector* v)
{
    set_vector(v);
    initData();
    // read the number of 1-bits in the supported bit_vector
    in.read((char*) &m_arg_cnt, sizeof(size_type)/sizeof(char));
    size_type sb = (m_arg_cnt+4095)>>12;

    if (m_arg_cnt) { // if there exists 1-bits to be supported
        m_superblock.load(in); // load superblocks

        if (m_miniblock!=nullptr) {
            delete[] m_miniblock;
            m_miniblock = nullptr;
        }
        if (m_longsuperblock!=nullptr) {
            delete[] m_longsuperblock;
            m_longsuperblock = nullptr;
        }

        bit_vector mini_or_long;// Helper vector: mini or long block?
        mini_or_long.load(in); // Load the helper vector
        m_miniblock = new int_vector<0>[sb]; // Create miniblock int_vector<0>
        if (!mini_or_long.empty())
            m_longsuperblock = new int_vector<0>[sb]; // Create longsuperblock int_vector<0>

        for (size_type i=0; i < sb; ++i)
            if (!mini_or_long.empty() and not mini_or_long[i]) {
                m_longsuperblock[i].load(in);
            } else {
                m_miniblock[i].load(in);
            }
    }
}

}

#endif
