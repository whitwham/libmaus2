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
#if ! defined(LIBMAUS2_LCS_LOCALALIGNMENTPRINT_HPP)
#define LIBMAUS2_LCS_LOCALALIGNMENTPRINT_HPP

#include <libmaus2/lcs/LocalBaseConstants.hpp>
#include <libmaus2/lcs/LocalEditDistanceResult.hpp>
#include <libmaus2/lcs/AlignmentTraceContainer.hpp>
#include <iostream>
#include <sstream>

namespace libmaus2
{
	namespace lcs
	{
		struct LocalAlignmentPrint : public BaseConstants
		{
			virtual ~LocalAlignmentPrint() {}

			static std::string stepToString(step_type const s)
			{
				switch ( s )
				{
					case STEP_MATCH: return "+";
					case STEP_MISMATCH: return "-";
					case STEP_DEL: return "D";
					case STEP_INS: return "I";
					case STEP_RESET: return "R";
					default: return "?";
				}
			}

			template<typename alignment_iterator>
			static std::ostream & printTrace(
				std::ostream & out,
				alignment_iterator const rta,
				alignment_iterator const rte,
				uint64_t const offset = 0
			)
			{
				out << std::string(offset,' ');
				for ( alignment_iterator tc = rta; tc != rte; ++tc )
					out << stepToString(*tc);
				return out;
			}

			template<typename string_iterator, typename alignment_iterator>
			static std::ostream & printAlignment(
				std::ostream & out,
				string_iterator ita,
				string_iterator itb,
				alignment_iterator const rta,
				alignment_iterator const rte
			)
			{
				printTrace(out,rta,rte);
				out << std::endl;

				for ( alignment_iterator ta = rta; ta != rte; ++ta )
				{
					switch ( *ta )
					{
						case STEP_MATCH:
						case STEP_MISMATCH:
						case STEP_DEL:
							out << (*ita++);
							break;
						case STEP_INS:
							out << " ";
							// ita++;
							break;
						default:
							break;
					}
				}
				out << std::endl;

				for ( alignment_iterator ta = rta; ta != rte; ++ta )
				{
					switch ( *ta )
					{
						case STEP_MATCH:
						case STEP_MISMATCH:
						case STEP_INS:
							out << (*itb++);
							break;
						case STEP_DEL:
							out << " ";
							// ita++;
							break;
						default:
							break;
					}
				}
				out << std::endl;

				return out;
			}

			template<typename value_type>
			static value_type identityMapFunction(value_type const & a)
			{
				return a;
			}

			template<
				typename a_iterator,
				typename b_iterator,
				typename alignment_iterator
			>
			static std::ostream & printAlignmentLines(
				std::ostream & out,
				a_iterator ita,
				a_iterator aend,
				b_iterator itb,
				b_iterator bend,
				uint64_t const rlinewidth,
				alignment_iterator const rta,
				alignment_iterator const rte,
				typename std::iterator_traits<a_iterator>::value_type (*mapfunction_a)(typename std::iterator_traits<a_iterator>::value_type const &) =
					identityMapFunction< typename std::iterator_traits<a_iterator>::value_type >,
				typename std::iterator_traits<b_iterator>::value_type (*mapfunction_b)(typename std::iterator_traits<b_iterator>::value_type const &) =
					identityMapFunction< typename std::iterator_traits<b_iterator>::value_type >
			)
			{
				uint64_t const l_a = aend - ita;
				uint64_t const l_b = bend - itb;

				std::ostringstream astr;

				for ( alignment_iterator ta = rta; ta != rte; ++ta )
				{
					switch ( *ta )
					{
						case STEP_MATCH:
						case STEP_MISMATCH:
						case STEP_DEL:
							if ( ita == aend )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "LocalAlignmentPrint::printAlignmentLines: accessing a beyond end" << std::endl;
								lme.getStream() << AlignmentTraceContainer::getAlignmentStatistics(rta,rte) << std::endl;
								lme.getStream() << "l_a=" << l_a << " l_b=" << l_b << std::endl;
								lme.finish();
								throw lme;
							}
							astr << mapfunction_a(*ita++);
							break;
						case STEP_INS:
							astr << " ";
							// ita++;
							break;
						default:
							break;
					}
				}
				while ( ita != aend )
					astr << mapfunction_a(*ita++);

				std::ostringstream bstr;
				// out << std::string(SPR.aclip,' ') << std::endl;

				for ( alignment_iterator ta = rta; ta != rte; ++ta )
				{
					switch ( *ta )
					{
						case STEP_MATCH:
						case STEP_MISMATCH:
						case STEP_INS:
							if ( itb == bend )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "LocalAlignmentPrint::printAlignmentLines: accessing b beyond end" << std::endl;
								lme.getStream() << AlignmentTraceContainer::getAlignmentStatistics(rta,rte) << std::endl;
								lme.getStream() << "l_a=" << l_a << " l_b=" << l_b << std::endl;
								lme.finish();
								throw lme;
							}
							bstr << mapfunction_b(*itb++);
							break;
						case STEP_DEL:
							bstr << " ";
							// ita++;
							break;
						default:
							break;
					}
				}
				while ( itb != bend )
					bstr <<  mapfunction_b(*itb++);

				std::ostringstream cstr;
				printTrace(cstr,rta,rte);

				std::string const aa = astr.str();
				std::string const ba = bstr.str();
				std::string const ca = cstr.str();
				uint64_t const linewidth = rlinewidth-2;
				uint64_t const numlines = (std::max(aa.size(),ba.size()) + linewidth-1) / linewidth;

				for ( uint64_t i = 0; i < numlines; ++i )
				{
					uint64_t pl = i*linewidth;

					out << "A ";
					if ( pl < aa.size() )
					{
						uint64_t const alen = std::min(linewidth,aa.size()-pl);
						out << aa.substr(pl,alen);
					}
					out << std::endl;

					out << "B ";
					if ( pl < ba.size() )
					{
						uint64_t const blen = std::min(linewidth,ba.size()-pl);
						out << ba.substr(pl,blen);
					}
					out << std::endl;

					out << "  ";
					if ( pl < ca.size() )
					{
						uint64_t const clen = std::min(linewidth,ca.size()-pl);
						out << ca.substr(pl,clen);
					}
					out << std::endl;
				}

				return out;
			}

			template<
				typename a_iterator,
				typename b_iterator,
				typename alignment_iterator
			>
			static std::ostream & printAlignmentLines(
				std::ostream & out,
				a_iterator ita,
				a_iterator aend,
				b_iterator itb,
				b_iterator bend,
				uint64_t const rlinewidth,
				alignment_iterator const rta,
				alignment_iterator const rte,
				libmaus2::lcs::LocalEditDistanceResult const & LED,
				typename std::iterator_traits<a_iterator>::value_type (*mapfunction_a)(typename std::iterator_traits<a_iterator>::value_type const &) =
					identityMapFunction< typename std::iterator_traits<a_iterator>::value_type >,
				typename std::iterator_traits<b_iterator>::value_type (*mapfunction_b)(typename std::iterator_traits<b_iterator>::value_type const &) =
					identityMapFunction< typename std::iterator_traits<b_iterator>::value_type >
			)
			{
				return printAlignmentLines(out,ita+LED.a_clip_left,aend-LED.a_clip_right,itb+LED.b_clip_left,bend-LED.b_clip_right,rlinewidth,rta,rte,mapfunction_a,mapfunction_b);
			}

			template<
				typename a_type,
				typename b_type,
				typename alignment_iterator
			>
			static std::ostream & printAlignmentLines(
				std::ostream & out,
				a_type const & a,
				b_type const & b,
				uint64_t const rlinewidth,
				alignment_iterator const rta,
				alignment_iterator const rte,
				libmaus2::lcs::LocalEditDistanceResult const & LED
			)
			{
				return printAlignmentLines(out,a.begin()+LED.a_clip_left,a.end()-LED.a_clip_right,b.begin()+LED.b_clip_left,b.end()-LED.b_clip_right,rlinewidth,rta,rte);
			}
		};
	}
}
#endif
