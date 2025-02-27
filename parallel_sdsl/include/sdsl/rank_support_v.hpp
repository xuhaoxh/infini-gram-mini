/* sdsl - succinct data structures library
    Copyright (C) 2008-2013 Simon Gog

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
/*! \file rank_support_v.hpp
    \brief rank_support_v.hpp contains rank_support_v.
	\author Simon Gog
*/
#ifndef INCLUDED_SDSL_RANK_SUPPORT_V
#define INCLUDED_SDSL_RANK_SUPPORT_V

#include "sequence.hpp"
#include "rank_support.hpp"

//! Namespace for the succinct data structure library.
namespace sdsl
{


//! A rank structure proposed by Sebastiano Vigna
/*! \par Space complexity
 *  \f$ 0.25n\f$ for a bit vector of length n bits.
 *
 * The superblock size is 512. Each superblock is subdivided into 512/64 = 8
 * blocks. So absolute counts for the superblock add 64/512 bits on top of each
 * supported bit. Since the first of the 8 relative count values is 0, we can
 * fit the remaining 7 (each of width log(512)=9) in a 64bit word.  The relative
 * counts add another 64/512 bits on top of each supported bit.
 * In total this results in 128/512=25% overhead.
 *
 * \tparam t_b       Bit pattern `0`,`1`,`10`,`01` which should be ranked.
 * \tparam t_pat_len Length of the bit pattern.
 *
 * \par Reference
 *    Sebastiano Vigna:
 *    Broadword Implementation of Rank/Select Queries.
 *    WEA 2008: 154-168
 *
 * @ingroup rank_support_group
 */
template<uint8_t t_b=1, uint8_t t_pat_len=1>
class rank_support_v : public rank_support
{
    private:
        static_assert(t_b == 1u or t_b == 0u or t_b == 10u or t_b == 11, "rank_support_v: bit pattern must be `0`,`1`,`10` or `01`");
        static_assert(t_pat_len == 1u or t_pat_len == 2u , "rank_support_v: bit pattern length must be 1 or 2");
    public:
        typedef bit_vector                          bit_vector_type;
        typedef rank_support_trait<t_b, t_pat_len>  trait_type;
        enum { bit_pat = t_b };
        enum { bit_pat_len = t_pat_len };
    private:
        // basic block for interleaved storage of superblockrank and blockrank
        int_vector<64> m_basic_block;
    public:
        explicit rank_support_v(const bit_vector* v = nullptr)
        {
            set_vector(v);
            if (v == nullptr) {
                return;
            } else if (v->empty()) {
                m_basic_block = int_vector<64>(2,0);   // resize structure for basic_blocks
                return;
            }
            size_type basic_block_size = ((v->capacity() >> 9)+1)<<1;
            m_basic_block.resize(basic_block_size);   // resize structure for basic_blocks
            if (m_basic_block.empty())
                return;
	    // sometimes last value isnt used
            const uint64_t* data = m_v->data();
            //m_basic_block[0] = m_basic_block[1] = 0;

	    // We assume carry only spreads one block!
	    
	    // One pass only calculating carry and storing it in the first part of basic blocks
	    m_basic_block[0] = m_basic_block[1] = trait_type::init_carry();
	    parallel_for (size_type b = 1; b <= (m_v->capacity()>>9); b++) {
		    m_basic_block[2*b] = trait_type::init_carry();
		    trait_type::args_in_the_word(data[(b<<3) - 1], m_basic_block[2*b]);
	    }
	    // Second pass to calculate all the block sums
	    {parallel_for (size_type b = 0; b <= (m_v->capacity()>>9); b++) {
		uint64_t carry = m_basic_block[2*b];
		uint64_t sum = trait_type::args_in_the_word(data[b<<3], carry); 
		uint64_t second_level_cnt = 0;
		// Last word is covered by <= with the +1
		size_type end = std::min((b<<3) + 8, (m_v->capacity()>>6));
		size_type i;
	    	for (i = (b<<3)+1; i < end; i++) {
			second_level_cnt |= sum<<(63-9*(i&0x7));	
			sum += trait_type::args_in_the_word(data[i], carry);
		}	
		m_basic_block[2*b] = sum;
		if (i % 8 != 0)  // Special case for last block
			second_level_cnt |= sum<<(63-9*(i&0x7));	
		m_basic_block[2*b+1] = second_level_cnt;
	    }}
	    // exclusive prefix sum over the block_sums
	    sequence::skip1<int_vector<64>, uint64_t, size_type> skp(m_basic_block);
	    sequence::scan(skp, skp, (basic_block_size>>1), utils::addF<uint64_t>() ,   (uint64_t)0);
        }

        rank_support_v(const rank_support_v&)  = default;
        rank_support_v(rank_support_v&&) = default;
        rank_support_v& operator=(const rank_support_v&) = default;
        rank_support_v& operator=(rank_support_v&&) = default;


        size_type rank(size_type idx) const
        {
            assert(m_v != nullptr);
            assert(idx <= m_v->size());
            const uint64_t* p = m_basic_block.data()
                                + ((idx>>8)&0xFFFFFFFFFFFFFFFEULL); // (idx/512)*2
            if (idx&0x3F)  // if (idx%64)!=0
                return  *p + ((*(p+1)>>(63 - 9*((idx&0x1FF)>>6)))&0x1FF) +
                        trait_type::word_rank(m_v->data(), idx);
            else
                return  *p + ((*(p+1)>>(63 - 9*((idx&0x1FF)>>6)))&0x1FF);
        }

        inline size_type operator()(size_type idx)const
        {
            return rank(idx);
        }

        size_type size()const {
            return m_v->size();
        }

        size_type serialize(std::ostream& out, structure_tree_node* v=nullptr,
                            std::string name="")const
        {
            size_type written_bytes = 0;
            structure_tree_node* child = structure_tree::add_child(v, name,
                                         util::class_name(*this));
            written_bytes += m_basic_block.serialize(out, child,
                             "cumulative_counts");
            structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream& in, const int_vector<1>* v=nullptr)
        {
            set_vector(v);
            m_basic_block.load(in);
        }

        void set_vector(const bit_vector* v=nullptr)
        {
            m_v = v;
        }

        void swap(rank_support_v& rs)
        {
            if (this != &rs) { // if rs and _this_ are not the same object
                m_basic_block.swap(rs.m_basic_block);
            }
        }
};

}// end namespace sds

#endif // end file
