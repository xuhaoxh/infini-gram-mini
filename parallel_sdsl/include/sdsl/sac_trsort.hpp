// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//#include "sac_config.hpp"
#include "sac_divsufsort_private.hpp"
#include "parallel.hpp"
#include "select_support_mcl.hpp"
#include "rank_support_v.hpp"
#include "int_vector.hpp"
#include "sequence.hpp"

#include <iostream>
#include <algorithm>
#include "sac_blockRadixSort.hpp"
#include "sac_quickSort.hpp"

using namespace std;


//#define printInfo

#ifdef printInfo
#define nextTimeM(_str) nextTime(_str)
#else
#define nextTimeM(_str) 
#endif



template <class saidx_t>
struct cmp_offset {
	saidx_t* SA,*ISA;
	saidx_t n,offset;
	cmp_offset(saidx_t* SA_, saidx_t* ISA_, saidx_t n_, saidx_t offset_ ) : SA(SA_), ISA(ISA_), n(n_), offset(offset_) {}
	// Return rank of the suffix offset characters after the suffix at position pos
	saidx_t operator() (const saidx_t& pos)const {
		return (pos + offset >= n) ? (n-pos-offset) : ISA[pos+offset]; 
	}
	bool operator()(const saidx_t& a, const saidx_t& b)const {
		return (*this)(a) < (*this)(b);
	}
};


template <class saidx_t>
inline void setStartSeg(saidx_t i, sdsl::bit_vector& segBounds) {
	segBounds[i] = 1;
}
template <class saidx_t>
inline void setEndSeg(saidx_t i, sdsl::bit_vector& segBounds) {
	segBounds[i] = segBounds[i] & 1 ? 0 : 1;	
}
template <class saidx_t>
inline saidx_t getStartSeg(saidx_t i, sdsl::select_support_mcl<1,1>& ss) {
	return ss.select(2*i+1);
}
template <class saidx_t>
inline saidx_t getEndSeg(saidx_t i, sdsl::select_support_mcl<1,1>& ss) {
	return ss.select(2*i+2)+1;
}

const int32_t BLOCK_SIZE = 128*1024;

template <class saidx_t>
void splitSegmentsParallel(sdsl::bit_vector& segBounds, sdsl::select_support_mcl<1,1>& ss, sdsl::rank_support_v<1,1>& rs, saidx_t* ISA, saidx_t* SA, saidx_t n, saidx_t offset, cmp_offset<saidx_t> F) {
	uintT nSegs = rs.rank(n) / 2;
	sdsl::bit_vector segBoundsBuf(n,0);
	saidx_t num_blocks = n / BLOCK_SIZE +1;
	saidx_t* names = new saidx_t[num_blocks]; // Keep track of last name of segments spanning blocks
	bool* first_comp = new bool[num_blocks];

	// First round do all comparisons
	parallel_for (saidx_t b = 0; b < num_blocks; b++) {
		saidx_t bs = b * BLOCK_SIZE;
		saidx_t be = std::min(n, (b+1)*BLOCK_SIZE);
		// Get first/last segment in block
		saidx_t sb = rs.rank(bs)/2;
		saidx_t se = (rs.rank(be)+1) / 2;
		names[b] = -1;// -1 means not set
		for (saidx_t seg = sb; seg < se; seg++) { // Go over all segments
			saidx_t start = getStartSeg(seg,ss);
			saidx_t end = std::min(be, getEndSeg(seg,ss));
			if (start >= bs) {
				// By definition first value is 1
				segBoundsBuf[start] = 1;
				names[b] = start;
			} else {  // segments starts in previous block
				start = bs-1;
				first_comp[b] = F(SA[start]) != F(SA[start+1]); // Save comparison with element from previous block
			}
			for (saidx_t i = start + 1; i < end; i++) {
				if (F(SA[i-1]) != F(SA[i])) { segBoundsBuf[i] = 1; names[b] = i;}
			}
		}
	}
	// Pass names across multiple blocks TODO : in paralle
	for (saidx_t b = 1; b < num_blocks; b++) {
		if (names[b] == -1)  names[b] = names[b-1];
	}
	
	// Second round mark beginning end end of new segments, update ISA 
	{parallel_for (saidx_t b = 0; b < num_blocks; b++) {
		saidx_t bs = b * BLOCK_SIZE;
		saidx_t be = std::min(n, (b+1)*BLOCK_SIZE);
		// Get first/last segment in block
		saidx_t sb = rs.rank(bs)/2;
		saidx_t se = (rs.rank(be)+1) / 2;
		bool eval;
		saidx_t name, start, end;
		saidx_t tmp_seg_end;
		for (saidx_t seg = sb; seg < se; seg++) {
			start = std::max(bs, getStartSeg(seg,ss));
			tmp_seg_end = getEndSeg(seg,ss);
			end = std::min(be, tmp_seg_end);
			name = b == 0 ? start : names[b-1];
			for (saidx_t i = start; i < end-1; i++) {
				eval = segBoundsBuf[i] & 1;
				// Update name and isa
				if (eval) name = i;
				ISA[SA[i]] = name;
				// Is beginning of zero run
				if (eval && !(segBoundsBuf[i+1]&1)) segBoundsBuf[i] = 1;
				// End of zero run
				else if( !eval && (segBoundsBuf[i+1]&1)) segBoundsBuf[i] = 1;
				else if (eval) segBoundsBuf[i] = 0;
			}
			eval = segBoundsBuf[end-1];
			if (eval) name = end-1;
			ISA[SA[end-1]] = name;
			// Extra case for end
			if (end == tmp_seg_end) { // If really end of segment
				segBoundsBuf[end-1] = eval ? 0 : 1;
			} else { // if segment continues in next block
				if (eval && !first_comp[b+1]) segBoundsBuf[end-1] = 1;
				else if (!eval && first_comp[b+1]) segBoundsBuf[end-1] = 1;
				else if (eval) segBoundsBuf[end-1] = 0;
			}
		}
	}}
	swap(segBounds, segBoundsBuf);
	delete [] names;
	delete [] first_comp;
}

template <class saidx_t>
void sortSegmentsParallel(uintT nSegs, sdsl::select_support_mcl<1,1>& ss, saidx_t* ISA, saidx_t* SA, saidx_t n, saidx_t offset) {
	{parallel_for (uintT i=0; i < nSegs; i++) {
		saidx_t start = getStartSeg(i, ss);
		saidx_t *SAi = SA + start;
		saidx_t l = getEndSeg(i, ss) - start;
		if (l >= 256) 
			intSort::iSort(SAi, l, n , cmp_offset<saidx_t>(SA, ISA, n, offset));
		else
			quickSort(SAi,l,cmp_offset<saidx_t>(SA, ISA, n, offset));
	}}
}

template <class saidx_t>
bool brokenCilk(saidx_t *SA, saidx_t offset, saidx_t n, saidx_t* ISA, sdsl::bit_vector& segBounds, sdsl::select_support_mcl<1,1>& ss, sdsl::rank_support_v<1,1>& rs) {
  saidx_t nSegs = rs.rank(n) / 2;
  // Init select/rank support on segBounds
  sortSegmentsParallel(nSegs, ss, ISA, SA, n, offset);
  // Write new seg bounds to buffer
  splitSegmentsParallel(segBounds, ss, rs, ISA, SA, n, offset, cmp_offset<saidx_t>(SA, ISA, n, offset));
  // Rebuild rank and select support
  rs = sdsl::rank_support_v<1,1>(&segBounds);
  if (rs.rank(n) == 0) return false;
  ss = sdsl::select_support_mcl<1,1>(&segBounds);
  return true;
}

template <class saidx_t>
void paralleltrsort(saidx_t* ISA, saidx_t* SA, saidx_t n, saidx_t* buf, saidx_t buffer_len) {
	saidx_t offset = 1;
	sdsl::bit_vector segBounds(n,0);
	segBounds[0] = 1; segBounds[n-1] = 1;
	sdsl::select_support_mcl<1,1> ss(&segBounds);
	sdsl::rank_support_v<1,1> rs(&segBounds);
	// Init segBounds
	splitSegmentsParallel<saidx_t>(segBounds, ss, rs, ISA,  SA, n, 0, cmp_offset<saidx_t>(SA, ISA, n, 0));
	ss = sdsl::select_support_mcl<1,1>(&segBounds);
	rs = sdsl::rank_support_v<1,1>(&segBounds);
	while (true) {
		utils::myAssert(offset <= n,  "Suffix Array:  Too many rounds");
		if (!brokenCilk(SA, offset, n, ISA, segBounds, ss, rs)) break;
		// double offset		
	    	offset = 2 * offset;			
	}
}
