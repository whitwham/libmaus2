/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
#if ! defined(LIBMAUS2_LZ_GZIPOUTPUTSTREAM_HPP)
#define LIBMAUS2_LZ_GZIPOUTPUTSTREAM_HPP

#include <libmaus2/lz/GzipOutputStreamBuffer.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct GzipOutputStream : public GzipOutputStreamBuffer, public std::ostream
		{
			typedef GzipOutputStream this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			GzipOutputStream(std::ostream & out, uint64_t const rbuffersize = 64*1024, int const level = Z_DEFAULT_COMPRESSION)
			: GzipOutputStreamBuffer(out,rbuffersize,level), std::ostream(this)
			{
				exceptions(std::ios::badbit);
			}
			~GzipOutputStream()
			{
				flush();
			}

			uint64_t terminate()
			{
				return GzipOutputStreamBuffer::terminate();
			}
		};
	}
}
#endif
