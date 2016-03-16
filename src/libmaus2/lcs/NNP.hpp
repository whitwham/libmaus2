/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_LCS_NNP_HPP)
#define LIBMAUS2_LCS_NNP_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lcs/BaseConstants.hpp>
#include <libmaus2/lcs/AlignmentTraceContainer.hpp>
#include <libmaus2/rank/popcnt.hpp>
#include <cstdlib>

namespace libmaus2
{
	namespace lcs
	{
		struct NNPTraceElement
		{
			libmaus2::lcs::BaseConstants::step_type step;
			int32_t slide;
			int32_t parent;

			NNPTraceElement(
				libmaus2::lcs::BaseConstants::step_type rstep = libmaus2::lcs::BaseConstants::STEP_RESET,
				int32_t rslide = 0,
				int32_t rparent = -1
			) : step(rstep), slide(rslide), parent(rparent)
			{
			}
		};

		struct NNPTraceContainer
		{
			libmaus2::autoarray::AutoArray<NNPTraceElement> Atrace;
			int64_t traceid;
			uint64_t otrace;

			NNPTraceContainer() : traceid(-1), otrace(0)
			{}

			void reset()
			{
				traceid = -1;
				otrace = 0;
			}

			template<bool forward = true>
			static void computeTrace(libmaus2::autoarray::AutoArray<NNPTraceElement> const & Atrace, int64_t const traceid, libmaus2::lcs::AlignmentTraceContainer & ATC)
			{
				uint64_t reserve = 0;
				for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
				{
					switch ( Atrace[curtraceid].step )
					{
						case libmaus2::lcs::BaseConstants::STEP_INS:
						case libmaus2::lcs::BaseConstants::STEP_DEL:
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
							reserve += 1;
							break;
						default:
							break;
					}
					reserve += Atrace[curtraceid].slide;
				}

				if ( ATC.capacity() < reserve )
					ATC.resize(reserve);
				ATC.reset();

				// std::cerr << "reserve " << reserve << std::endl;

				if ( forward )
				{
					for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
					{
						for ( int64_t i = 0; i < Atrace[curtraceid].slide; ++i )
							*(--ATC.ta) = libmaus2::lcs::BaseConstants::STEP_MATCH;

						switch ( Atrace[curtraceid].step )
						{
							case libmaus2::lcs::BaseConstants::STEP_INS:
							case libmaus2::lcs::BaseConstants::STEP_DEL:
							case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
								*(--ATC.ta) = Atrace[curtraceid].step;
								break;
							default:
								break;
						}
					}

					assert ( ATC.ta >= ATC.trace.begin() );

					#if 0
					std::cerr << "here" << std::endl;
					std::cerr << ATC.getStringLengthUsed().first << std::endl;
					std::cerr << ATC.getStringLengthUsed().second << std::endl;
					#endif
				}
				else
				{
					ATC.ta -= reserve;

					BaseConstants::step_type * tc = ATC.ta;

					for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
					{
						for ( int64_t i = 0; i < Atrace[curtraceid].slide; ++i )
							*(tc++) = libmaus2::lcs::BaseConstants::STEP_MATCH;

						switch ( Atrace[curtraceid].step )
						{
							case libmaus2::lcs::BaseConstants::STEP_INS:
							case libmaus2::lcs::BaseConstants::STEP_DEL:
							case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
								*(tc++) = Atrace[curtraceid].step;
								break;
							default:
								break;
						}
					}

					ATC.te = tc;

					#if 0
					std::cerr << "here" << std::endl;
					std::cerr << ATC.getStringLengthUsed().first << std::endl;
					std::cerr << ATC.getStringLengthUsed().second << std::endl;
					#endif

				}
			}

			template<bool forward = true>
			void computeTrace(libmaus2::lcs::AlignmentTraceContainer & ATC) const
			{
				computeTrace(Atrace,traceid,ATC);
			}

		};

		struct NNP : public libmaus2::lcs::BaseConstants
		{
			typedef int32_t score_type;
			typedef int32_t offset_type;

			static unsigned int popcnt(uint64_t const w)
			{
				return libmaus2::rank::PopCnt8<sizeof(long)>::popcnt8(w);
			}

			struct DiagElement
			{
				score_type score;
				offset_type offset;
				int32_t id;
				uint64_t evec;

				bool operator==(DiagElement const & o) const
				{
					return score == o.score && offset == o.offset && id == o.id && evec == o.evec;
				}
			};


			libmaus2::autoarray::AutoArray<DiagElement> Acontrol;
			DiagElement * control;
			libmaus2::autoarray::AutoArray<DiagElement> Ancontrol;
			DiagElement * ncontrol;

			struct CompInfo
			{
				int64_t d;
				bool prevactive;
				bool thisactive;
				bool nextactive;

				CompInfo(
					int64_t rd = 0,
					bool rprevactive = false,
					bool rthisactive = false,
					bool rnextactive = false
				) : d(rd), prevactive(rprevactive), thisactive(rthisactive), nextactive(rnextactive)
				{

				}
			};

			libmaus2::autoarray::AutoArray<CompInfo> Acompdiag;
			libmaus2::autoarray::AutoArray<int64_t> Aactivediag;

			NNP() : Acontrol(1), control(Acontrol.begin()), Ancontrol(1), ncontrol(Ancontrol.begin())
			{
			}

			template<typename iterator>
			static uint64_t slide(iterator a, iterator ae, iterator b, iterator be)
			{
				ptrdiff_t const alen = ae-a;
				ptrdiff_t const blen = be-b;

				if ( alen <= blen )
				{
					iterator as = a;
					while ( a != ae && *a == *b )
						++a, ++b;
					return a - as;
				}
				else
				{
					iterator bs = b;
					while ( b != be && *a == *b )
						++a, ++b;
					return b-bs;
				}
			}

			template<typename iterator>
			static uint64_t slider(iterator a, iterator ae, iterator b, iterator be)
			{
				ptrdiff_t const alen = ae-a;
				ptrdiff_t const blen = be-b;

				if ( alen <= blen )
				{
					iterator ac = ae;

					while ( ac-- != a )
						if ( *(--be) != *ac )
							break;

					return (ae-ac)-1;
				}
				else
				{
					iterator bc = be;
					while ( bc-- != b )
						if ( *(--ae) != *bc )
							break;

					return (be-bc)-1;
				}
			}

			static void
				allocate(
					int64_t const mindiag, int64_t const maxdiag, libmaus2::autoarray::AutoArray<DiagElement> & Acontrol,
					DiagElement * & control
				)
			{
				// maximum absolute diagonal index
				int64_t const maxabsdiag = std::max(std::abs(mindiag),std::abs(maxdiag));
				// required diagonals
				int64_t const reqdiag = 2*maxabsdiag + 1;

				#if 0
				std::map<int,DiagElement> M;
				for ( DiagElement * p = Acontrol.begin(); p != Acontrol.end(); ++p )
				{
					ptrdiff_t off = p - control;
					M [ off ] = *p;
				}
				#endif

				// while array is too small
				while ( reqdiag > static_cast<int64_t>(Acontrol.size()) )
				{
					// previous max absolute diagonal index
					ptrdiff_t const oldabsdiag = control - Acontrol.begin();
					// one
					ptrdiff_t const one = 1;
					// new max absolute diagonal index
					ptrdiff_t const newabsdiag = std::max(2*oldabsdiag,one);

					libmaus2::autoarray::AutoArray<DiagElement> Anewcontrol(2*newabsdiag+1);
					std::copy(Acontrol.begin(),Acontrol.end(),Anewcontrol.begin() + (newabsdiag - oldabsdiag));

					DiagElement * newcontrol = Anewcontrol.begin() + newabsdiag;
					for ( int64_t i = -oldabsdiag; i <= oldabsdiag; ++i )
						assert ( newcontrol[i] == control[i] );

					Acontrol = Anewcontrol;
					control = newcontrol;
				}

				#if 0
				for ( std::map<int,DiagElement>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
					assert (
						control[ita->first] == ita->second
					);
				#endif
			}

			template<typename iterator, bool forward = true>
			void align(
				iterator ab,
				iterator ae,
				iterator bb,
				iterator be,
				NNPTraceContainer & tracecontainer
			)
			{
				ptrdiff_t const alen = ae-ab; // *S*
				ptrdiff_t const blen = be-bb; // *S*
				ptrdiff_t const mlen = std::min(alen,blen);
				uint64_t otrace = tracecontainer.otrace;

				allocate(0,0,Acontrol,control);

				control[0].offset = forward ? slide(ab,ae,bb,be) : slider(ab,ae,bb,be); // *S*
				control[0].score = control[0].offset;
				control[0].evec = 0;
				control[0].id = otrace;

				libmaus2::autoarray::AutoArray<NNPTraceElement> & Atrace = tracecontainer.Atrace;

				Atrace.push(otrace,NNPTraceElement(libmaus2::lcs::BaseConstants::STEP_RESET,control[0].offset,-1));

				bool foundalignment = (alen==blen) && (control[0].offset==alen);
				bool active = true;

				tracecontainer.traceid = foundalignment ? control[0].id : -1;
				unsigned int const maxwerr = 24;

				int64_t maxscore = -1;
				int64_t maxscoretraceid = -1;

				uint64_t oactivediag = 0;
				Aactivediag.ensureSize(1);
				Aactivediag[oactivediag++] = 0;

				while ( !foundalignment && active )
				{
					active = false;

					// allocate control for next round
					assert ( oactivediag );
					allocate(
						Aactivediag[0]-1,
						Aactivediag[oactivediag-1]+1,Ancontrol,ncontrol);

					#if 0
					std::set<int64_t> Sactive(
						Aactivediag.begin(),
						Aactivediag.begin()+oactivediag
					);

					for ( uint64_t i = 1; i < oactivediag; ++i )
						assert ( Aactivediag[i-1] < Aactivediag[i] );
					#endif

					uint64_t compdiago = 0;
					Acompdiag.ensureSize(oactivediag*3);

					for ( uint64_t i = 0; i < oactivediag; ++i )
					{
						int64_t const d = Aactivediag[i];
						int64_t const prevd = d-1;
						int64_t const nextd = d+1;

						bool const prevactive = (i   > 0          ) && (Aactivediag.at(i-1) == prevd);
						bool const nextactive = (i+1 < oactivediag) && (Aactivediag.at(i+1) == nextd);

						// std::cerr << "d=" << d << " prevd=" << prevd << " nextd=" << nextd << std::endl;

						#if 0
						assert ( Sactive.find(d) != Sactive.end() );
						if ( prevactive )
							assert ( Sactive.find(prevd) != Sactive.end() );
						else
							assert ( Sactive.find(prevd) == Sactive.end() );
						if ( nextactive )
							assert ( Sactive.find(nextd) != Sactive.end() );
						else
							assert ( Sactive.find(nextd) == Sactive.end() );
						#endif

						if (
							! prevactive
							&&
							(!compdiago || Acompdiag[compdiago-1].d != Aactivediag[i]-1)
						)
						{
							bool const prevprevactive = i && (Aactivediag[i-1] == Aactivediag[i]-2);
							Acompdiag[compdiago++] = CompInfo(Aactivediag[i]-1,prevprevactive,false /* this active */,true);
						}

						Acompdiag[compdiago++] = CompInfo(Aactivediag[i],prevactive,true /* this active */,nextactive);

						if ( ! nextactive )
						{
							bool const nextnextactive = i+1 < oactivediag && (Aactivediag[i+1] == Aactivediag[i]+2);
							Acompdiag[compdiago++] = CompInfo(Aactivediag[i]+1,true,false /* this active */,nextnextactive);
						}
					}

					int64_t maxroundoffset = -1;

					// iterate over diagonals for next round
					for ( uint64_t di = 0; di < compdiago; ++di )
					{
						int64_t const d = Acompdiag[di].d;

						bool const prevactive = Acompdiag[di].prevactive;
						bool const nextactive = Acompdiag[di].nextactive;
						bool const thisactive = Acompdiag[di].thisactive;

						#if 0
						if ( prevactive )
							assert ( Sactive.find(d-1) != Sactive.end() );
						else
							assert ( Sactive.find(d-1) == Sactive.end() );
						if ( nextactive )
							assert ( Sactive.find(d+1) != Sactive.end() );
						else
							assert ( Sactive.find(d+1) == Sactive.end() );
						if ( thisactive )
							assert ( Sactive.find(d) != Sactive.end() );
						else
							assert ( Sactive.find(d) == Sactive.end() );
						#endif

						// offsets on a and b due to diagonal
						int64_t const apre = (d >= 0) ? d : 0;
						int64_t const bpre = (d >= 0) ? 0 : -d;
						assert ( apre >= 0 );
						assert ( bpre >= 0 );
						// maximum offset on this diagonal
						int64_t const maxoffset = std::min(alen-apre,blen-bpre);

						/*
						 * find offset we will try to slide from
						 */

						/*
						 * if we can try from top / next higher diagonal
						 */
						int64_t offtop = -1;
						if ( nextactive && (control[d+1].offset >= 0) )
						{
							if ( d >= 0 )
							{
								offtop = control[d+1].offset + 1;
							}
							else
							{
								offtop = control[d+1].offset;
							}
							if ( offtop > maxoffset )
								offtop = -1;
						}
						/*
						 * if we can try from left / next lower diagonal
						 */
						int64_t offleft = -1;
						if ( prevactive && (control[d-1].offset >= 0) )
						{
							if ( d > 0 )
							{
								offleft = control[d-1].offset;
							}
							else
							{
								offleft = control[d-1].offset+1;
							}
							if ( offleft > maxoffset )
								offleft = -1;
						}
						/*
						 * if we can start from this diagonal
						 */
						int64_t offdiag = -1;
						if ( thisactive && (control[d].offset >= 0) )
						{
							offdiag = control[d].offset + 1; // mismatch
							if ( offdiag > maxoffset )
								offdiag = -1;
						}

						#if 0
						if ( thisactive )
						{
							std::cerr << "diagonal " << d << " offset " << control[d].offset << " werr=" << popcnt(control[d].evec) << std::endl;
						}
						else
						{
							std::cerr << "new diagonal " << d << std::endl;
						}

						std::cerr << "offleft=" << offleft << " offdiag=" << offdiag << " offtop=" << offtop << std::endl;
						#endif

						// mark as invalid
						ncontrol[d].offset = -1;

						if ( offtop >= offleft )
						{
							if ( offdiag >= offtop )
							{
								if ( offdiag >= 0 && (offdiag > control[d].offset) )
								{
									int64_t const aoff = apre + offdiag;
									int64_t const boff = bpre + offdiag;
									int64_t const slidelen = forward ? slide(ab+aoff,ae,bb+boff,be) : slider(ab,ae-aoff,bb,be-boff); // *S*
									uint64_t const newevec = (slidelen < 64) ? (((control[d].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr < maxwerr )
									{
										ncontrol[d].offset = offdiag + slidelen;
										ncontrol[d].score = control[d].score - 1 + slidelen;
										ncontrol[d].id = otrace;
										ncontrol[d].evec = newevec;

										if ( ncontrol[d].score > maxscore )
										{
											maxscore = ncontrol[d].score;
											maxscoretraceid = ncontrol[d].id;
										}

										Atrace.push(otrace,NNPTraceElement(libmaus2::lcs::BaseConstants::STEP_MISMATCH,slidelen,control[d].id));
									}
								}
							}
							else // offtop maximal
							{
								assert ( offtop >= 0 );

								if ( (!thisactive) || (offtop > control[d].offset) )
								{
									int64_t const aoff = apre + offtop;
									int64_t const boff = bpre + offtop;

									int64_t const slidelen = forward ? slide(ab+aoff,ae,bb+boff,be) : slider(ab,ae-aoff,bb,be-boff); // *S*
									uint64_t const newevec = (slidelen < 64) ? (((control[d+1].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr < maxwerr )
									{
										ncontrol[d].offset = offtop + slidelen;
										ncontrol[d].id = otrace;
										ncontrol[d].score = control[d+1].score - 1 + slidelen;
										ncontrol[d].evec = newevec;

										if ( ncontrol[d].score > maxscore )
										{
											maxscore = ncontrol[d].score;
											maxscoretraceid = ncontrol[d].id;
										}

										Atrace.push(otrace,NNPTraceElement(libmaus2::lcs::BaseConstants::STEP_INS,slidelen,control[d+1].id));
									}
								}
							}
						}
						else // offtop < offleft
						{
							if ( offdiag >= offleft )
							{
								if ( offdiag >= 0 && (offdiag > control[d].offset) )
								{
									int64_t const aoff = apre + offdiag;
									int64_t const boff = bpre + offdiag;
									int64_t const slidelen = forward ? slide(ab+aoff,ae,bb+boff,be) : slider(ab,ae-aoff,bb,be-boff); // *S*
									uint64_t const newevec = (slidelen < 64) ? (((control[d].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr < maxwerr )
									{
										ncontrol[d].offset = offdiag + slidelen;
										ncontrol[d].id = otrace;
										ncontrol[d].score = control[d].score - 1 + slidelen;
										ncontrol[d].evec = newevec;

										if ( ncontrol[d].score > maxscore )
										{
											maxscore = ncontrol[d].score;
											maxscoretraceid = ncontrol[d].id;
										}

										Atrace.push(otrace,NNPTraceElement(libmaus2::lcs::BaseConstants::STEP_MISMATCH,slidelen,control[d].id));
									}
								}
							}
							else // offleft maximal
							{
								if ( (!thisactive) || (offleft > control[d].offset) )
								{
									assert ( offleft >= 0 );
									int64_t const aoff = apre + offleft;
									int64_t const boff = bpre + offleft;
									int64_t const slidelen = forward ? slide(ab+aoff,ae,bb+boff,be) : slider(ab,ae-aoff,bb,be-boff); // *S*
									uint64_t const newevec = (slidelen < 64) ? (((control[d-1].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr < maxwerr )
									{
										ncontrol[d].offset = offleft + slidelen;
										ncontrol[d].id = otrace;
										ncontrol[d].score = control[d-1].score - 1 + slidelen;
										ncontrol[d].evec = newevec;

										if ( ncontrol[d].score > maxscore )
										{
											maxscore = ncontrol[d].score;
											maxscoretraceid = ncontrol[d].id;
										}

										Atrace.push(otrace,NNPTraceElement(libmaus2::lcs::BaseConstants::STEP_DEL,slidelen,control[d-1].id));
									}
								}
							}
						}

						// have we found a full alignment of the two strings?
						if ( d == (alen-blen) && ncontrol[d].offset == mlen )
						{
							#if 0
							std::cerr << "yes " << d << ",offset="
								<< ncontrol[d].offset << ",score=" << ncontrol[d].score
								<< ",alen=" << alen << ",blen=" << blen << ",mlen=" << mlen
								<< std::endl;
							#endif

							foundalignment = true;
							tracecontainer.traceid = otrace-1;
						}

						if ( ncontrol[d].offset > maxroundoffset )
							maxroundoffset = ncontrol[d].offset;

					}

					Acontrol.swap(Ancontrol);
					std::swap(control,ncontrol);

					// filter active diagonals
					Aactivediag.ensureSize(compdiago);
					oactivediag = 0;
					for ( uint64_t di = 0; di < compdiago; ++di )
					{
						int64_t const d = Acompdiag[di].d;
						if ( control[d].offset >= 0 && control[d].offset >= maxroundoffset-75 )
						{
							Aactivediag[oactivediag++] = d;
							active = true;
						}
					}
				}

				if ( ! foundalignment )
				{
					// std::cerr << "no full alignment found, cutting back to best score" << std::endl;
					tracecontainer.traceid = maxscoretraceid;
				}

				#if 0
				std::cerr << "maxscore=" << maxscore << std::endl;
				std::cerr << "maxscoretraceid=" << maxscoretraceid << std::endl;
				std::cerr << "traceid=" << traceid << std::endl;
				#endif

				#if 0
				for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
				{
					std::cerr << "traceid=" << curtraceid << " op " << Atrace[curtraceid].step << " slide " << Atrace[curtraceid].slide << std::endl;
				}
				#endif

				tracecontainer.otrace = otrace;
			}

		};
	}
}
#endif