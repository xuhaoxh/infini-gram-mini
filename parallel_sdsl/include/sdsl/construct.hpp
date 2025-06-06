/* sdsl - succinct data structures library
    Copyright (C) 2012 Simon Gog

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
/*! \file construct.hpp
    \brief construct.hpp contains methods to construct indexes (compressed suffix arrays and trees).
	\author Simon Gog
*/

#ifndef INCLUDED_SDSL_CONSTRUCT
#define INCLUDED_SDSL_CONSTRUCT

#include "sdsl_concepts.hpp"
#include "int_vector.hpp"
#include "construct_lcp.hpp"
#include "construct_bwt.hpp"
#include "construct_sa.hpp"
#include "csa_sampling_strategy.hpp"
#include <string>

namespace sdsl
{

// forward declarations
template<class t_csa, uint8_t t_width>
class _sa_order_sampling;
template<class t_csa, uint8_t t_width>
class _isa_sampling;

template<class int_vector>
bool contains_no_zero_symbol(const int_vector& text, const std::string& file)
{
    for (int_vector_size_type i=0; i < text.size(); ++i) {
        if ((uint64_t)0 == text[i]) {
            throw std::logic_error(std::string("Error: File \"")+file+"\" contains zero symbol.");
            return false;
        }
    }
    return true;
}

template<class int_vector>
void append_zero_symbol(int_vector& text)
{
    text.resize(text.size()+1);
    text[text.size()-1] = 0;
}


template<class t_index>
void construct(t_index& idx, std::string file, uint8_t num_bytes=0)
{
    tMSS file_map;
    cache_config config;
    if (is_ram_file(file)) {
        config.dir = "@";
    }
    construct(idx, file, config, num_bytes);
}

template<class t_index, class t_data>
void construct_im(t_index& idx, t_data data, uint8_t num_bytes=0)
{
    std::string tmp_file = ram_file_name(util::to_string(util::pid())+"_"+util::to_string(util::id()));
    store_to_file(data, tmp_file);
    construct(idx, tmp_file, num_bytes);
    ram_fs::remove(tmp_file);
}

//! Constructs an index object of type t_index for a text stored on disk.
/*!
 * \param idx       	t_index object.  Any sdsl suffix array of suffix tree.
 * \param file      	Name of the text file. The representation of the file
 *                  	is dependent on the next parameter.
 * \
 * \param num_bytes 	If `num_bytes` equals 0, the file format is a serialized
 *				    	int_vector<>. Otherwise the file is interpreted as sequence
 *                  	of `num_bytes`-byte integer stored in big endian order.
 */
template<class t_index>
void construct(t_index& idx, const std::string& file, cache_config& config, uint8_t num_bytes=0)
{
    // delegate to CSA or CST construction
    typename t_index::index_category 		index_tag;
    construct(idx, file, config, num_bytes, index_tag);
}

// Specialization for WTs
template<class t_index>
void construct(t_index& idx, const std::string& file, cache_config& config, uint8_t num_bytes, wt_tag)
{
    auto event = memory_monitor::event("construct wavelet tree");
    int_vector<t_index::alphabet_category::WIDTH> text;
    load_vector_from_file(text, file, num_bytes);
    std::string tmp_key = util::to_string(util::pid())+"_"+util::to_string(util::id());
    std::string tmp_file_name = cache_file_name(tmp_key, config);
    store_to_file(text, tmp_file_name);
    util::clear(text);
    {
        int_vector_buffer<t_index::alphabet_category::WIDTH> text_buf(tmp_file_name);
        t_index tmp(text_buf, text_buf.size());
        idx.swap(tmp);
    }
    sdsl::remove(tmp_file_name);
}

// Specialization for CSAs
template<class t_index>
void construct(t_index& idx, const std::string& file, cache_config& config, uint8_t num_bytes, csa_tag)
{
    auto event = memory_monitor::event("construct CSA");
    const char* KEY_TEXT = key_text_trait<t_index::alphabet_category::WIDTH>::KEY_TEXT;
    typedef int_vector<t_index::alphabet_category::WIDTH> text_type;
    {
        auto event = memory_monitor::event("parse input text");
        // (1) prepare text
        if (!cache_file_exists(KEY_TEXT, config)) {
            text_type text;
            load_vector_from_file(text, file, num_bytes);
            if (contains_no_zero_symbol(text, file)) {
                append_zero_symbol(text);
                store_to_cache(text,KEY_TEXT, config);
            }
        }
        register_cache_file(KEY_TEXT, config);
    }
    {
        // (2) construct SA
        auto event = memory_monitor::event("SA");
        if (!cache_file_exists(conf::KEY_SA, config)) {
            construct_sa<t_index::alphabet_category::WIDTH>(config);
        }
        register_cache_file(conf::KEY_SA, config);
    }
    {
        // (3) construct BWT
        auto event = memory_monitor::event("BWT");
        if (!cache_file_exists(conf::KEY_BWT, config)) {
            construct_bwt<t_index::alphabet_category::WIDTH>(config);
        }
        register_cache_file(conf::KEY_BWT, config);
    }
    {
        // (6, 7) sample SA and ISA
        if (!(cache_file_exists(conf::KEY_SAMPLED_SA, config) && cache_file_exists(conf::KEY_SAMPLED_ISA, config))) {
            _sa_order_sampling<t_index, 0> sa_sample;
            {
                auto start_time = std::chrono::high_resolution_clock::now();
                auto event = memory_monitor::event("sample SA");
                _sa_order_sampling<t_index, 0> tmp_sa_sample(config);
                sa_sample.swap(tmp_sa_sample);
                store_to_cache(sa_sample, conf::KEY_SAMPLED_SA, config);
                register_cache_file(conf::KEY_SAMPLED_SA, config);
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
                std::cout << "Step 6 (sampling SA): Done. Took " << duration.count() << " seconds" << std::endl;
            }
            {
                auto start_time = std::chrono::high_resolution_clock::now();
                auto event = memory_monitor::event("sample ISA");
                _isa_sampling<t_index, 0> tmp_isa_sample(config, &sa_sample); // this one has 64 bits per element

                // adjust to the appropriate ptr size
                int_vector_buffer<>  sa_buf(cache_file_name(conf::KEY_SA, config));
                auto n = sa_buf.size();
                _isa_sampling<t_index, 0> isa_sample;
                isa_sample.width(bits::hi(n)+1);
                isa_sample.resize((n-1)/t_index::isa_sample_dens+1);
                for (size_t i=0; i < isa_sample.size(); ++i) {
                    static_cast<int_vector<0>&>(isa_sample)[i] = static_cast<int_vector<0>&>(tmp_isa_sample)[i];
                }

                store_to_cache(isa_sample, conf::KEY_SAMPLED_ISA, config);
                register_cache_file(conf::KEY_SAMPLED_ISA, config);
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
                std::cout << "Step 7 (sample ISA): Done. Took " << duration.count() << " seconds" << std::endl;
            }
        }
    }
    {
        // put things together into .fm9
        auto event = memory_monitor::event("construct CSA");
        t_index tmp(config);
        idx.swap(tmp);
    }
    if (config.delete_files) {
        auto event = memory_monitor::event("delete temporary files");
        util::delete_all_files(config.file_map);
    }
}

// Specialization for CSTs
template<class t_index>
void construct(t_index& idx, const std::string& file, cache_config& config, uint8_t num_bytes, cst_tag)
{
    auto event = memory_monitor::event("construct CST");
    const char* KEY_TEXT = key_text_trait<t_index::alphabet_category::WIDTH>::KEY_TEXT;
    const char* KEY_BWT  = key_bwt_trait<t_index::alphabet_category::WIDTH>::KEY_BWT;
    csa_tag csa_t;
    {
        // (1) check, if the compressed suffix array is cached
        typename t_index::csa_type csa;
        if (!cache_file_exists(std::string(conf::KEY_CSA)+"_"+util::class_to_hash(csa), config)) {
            cache_config csa_config(false, config.dir, config.id, config.file_map);
            construct(csa, file, csa_config, num_bytes, csa_t);
            auto event = memory_monitor::event("store CSA");
            config.file_map = csa_config.file_map;
            store_to_cache(csa,std::string(conf::KEY_CSA)+"_"+util::class_to_hash(csa), config);
        }
        register_cache_file(std::string(conf::KEY_CSA)+"_"+util::class_to_hash(csa), config);
    }
    {
        // (2) check, if the longest common prefix array is cached
        auto event = memory_monitor::event("LCP");
        register_cache_file(KEY_TEXT, config);
        register_cache_file(KEY_BWT, config);
        register_cache_file(conf::KEY_SA, config);
        if (!cache_file_exists(conf::KEY_LCP, config)) {
            if (t_index::alphabet_category::WIDTH==8) {
                construct_lcp_semi_extern_PHI(config);
            } else {
                construct_lcp_PHI<t_index::alphabet_category::WIDTH>(config);
            }
        }
        register_cache_file(conf::KEY_LCP, config);
    }
    {
        auto event = memory_monitor::event("CST");
        t_index tmp(config);
        tmp.swap(idx);
    }
    if (config.delete_files) {
        auto event = memory_monitor::event("delete temporary files");
        util::delete_all_files(config.file_map);
    }
}

} // end namespace sdsl
#endif
