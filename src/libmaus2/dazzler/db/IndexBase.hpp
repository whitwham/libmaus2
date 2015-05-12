/*
    libmaus2
    Copyright (C) 2015 German Tischler

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#if ! defined(LIBMAUS2_DAZZLER_DB_HITSINDEXBASE_HPP)
#define LIBMAUS2_DAZZLER_DB_HITSINDEXBASE_HPP

#include <libmaus2/dazzler/db/InputBase.hpp>
#include <istream>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct IndexBase : public InputBase
			{
				// untrimmed number of reads
				int32_t ureads;
				// trimmed number of reads
				int32_t treads;
				// cut off
				int32_t cutoff;
				// multiple reads from each well?
				bool all;
				// frequency of bases A,C,G and T
				float freq[4];
				// maximum read length
				int32_t maxlen;
				// total number of bases in database
				int64_t totlen;
				// number of reads
				int32_t nreads;
				// is database trimmed?
				bool trimmed;
				// part this record refers to (0 for all of database, >0 for partial)
				int part;
				// first untrimmed record index
				int32_t ufirst;
				// first trimmed record index
				int32_t tfirst;
				// path to db
				std::string path;
				// are records loaded into memory
				bool loaded;
				

				IndexBase()
				{
				
				}
				
				IndexBase(std::istream & in)
				{
					deserialise(in);
				}
				
				void deserialise(std::istream & in)
				{
					uint64_t offset = 0;
				
					ureads  = getLittleEndianInteger4(in,offset);
					treads  = getLittleEndianInteger4(in,offset);
					cutoff  = getLittleEndianInteger4(in,offset);
					all     = getLittleEndianInteger4(in,offset);

					for ( size_t i = 0; i < sizeof(freq)/sizeof(freq[0]); ++i )
						freq[i] = getFloat(in,offset);

					maxlen  = getLittleEndianInteger4(in,offset);
					totlen  = getLittleEndianInteger8(in,offset);
					nreads  = getLittleEndianInteger4(in,offset);
					trimmed = getLittleEndianInteger4(in,offset);
					
					/* part = */   getLittleEndianInteger4(in,offset);
					/* ufirst = */ getLittleEndianInteger4(in,offset);
					/* tfirst = */ getLittleEndianInteger4(in,offset);
					
					/* path =   */ getLittleEndianInteger8(in,offset); // 8 byte pointer
					/* loaded = */ getLittleEndianInteger4(in,offset);
					/* bases =  */ getLittleEndianInteger8(in,offset); // 8 byte pointer
					
					/* reads =  */ getLittleEndianInteger8(in,offset); // 8 byte pointer
					/* tracks = */ getLittleEndianInteger8(in,offset); // 8 byte pointer				
					
					nreads = ureads;
				}
			};

			std::ostream & operator<<(std::ostream & out, IndexBase const & H);
		}
	}
}
#endif
// 40