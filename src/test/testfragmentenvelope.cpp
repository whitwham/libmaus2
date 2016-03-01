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
#define LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG

#include <libmaus2/lcs/EnvelopeFragment.hpp>
#include <libmaus2/lcs/FragmentEnvelope.hpp>
#include <vector>
#include <algorithm>

void testbasic()
{
	libmaus2::lcs::FragmentEnvelope FE;

	#if 1
	FE.insert(1,2,10,0);
	FE.insert(3,4,6,1);
	FE.insert(3,4,4,2);
	FE.insert(3,4,7,3);

	FE.insert(2,5,10,4);

	std::cerr << FE.find(11,6) << std::endl;
	std::cerr << FE.find(100,102) << std::endl;
	#endif
}

int main()
{
	try
	{
		testbasic();

		typedef libmaus2::lcs::EnvelopeFragment fragment_type;
		std::vector<fragment_type> Vfrag;

		Vfrag.push_back(fragment_type(0,0,20));
		Vfrag.push_back(fragment_type(1,1,20));
		Vfrag.push_back(fragment_type(21,22,20));
		Vfrag.push_back(fragment_type(101,102,20));

		typedef std::vector<fragment_type> container_type;
		typedef container_type::iterator iterator;
		typedef container_type::const_iterator const_iterator;

		iterator rlow = Vfrag.begin();
		iterator rtop = Vfrag.end();

		iterator rout = fragment_type::mergeOverlappingAndSort(rlow,rtop);

		// set id and back end vector
		std::vector<const_iterator> Vbacksort(rout - rlow);
		for ( iterator it = rlow; it != rout; ++it )
		{
			it->id = it - rlow;
			Vbacksort[it - rlow] = it;
		}

		// sort back end vector by end position on y
		std::sort(Vbacksort.begin(),Vbacksort.end(),fragment_type::EnvelopeFragmentBackComparator< const_iterator >());

		libmaus2::lcs::FragmentEnvelope FE(1,2/* x+y */ * 3 /* multiple of score allowed for distance */);

		std::vector<const_iterator>::const_iterator itz = Vbacksort.begin();
		for ( iterator it = rlow; it != rout; ++it )
		{
			fragment_type const & frag = *it;
			int64_t const y = frag.y;

			std::cerr << std::string(80,'=') << std::endl;
			std::cerr << "[V] processing " << frag << std::endl;

			while ( itz != Vbacksort.end() && (*itz)->getEnd() <= y )
			{
				fragment_type const & insfrag = **(itz++);
				FE.insert(insfrag.x+insfrag.k,insfrag.y+insfrag.k,insfrag.k,insfrag.id);
				std::cerr << "inserted " << insfrag << std::endl;
			}

			int64_t const best = FE.find(frag.x,frag.y);
			std::cerr << "best " << best << " " << rlow[best] << std::endl;
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
