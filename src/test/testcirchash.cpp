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

#include <libmaus2/bambam/CircularHashCollatingBamDecoder.hpp>

#include <libmaus2/bambam/BamWriter.hpp>
#include <libmaus2/bambam/ProgramHeaderLineSet.hpp>

#include <config.h>

void bamcollate(libmaus2::util::ArgInfo const & arginfo)
{
	uint32_t const excludeflags = libmaus2::bambam::BamFlagBase::stringToFlags(arginfo.getValue<std::string>("exclude","SECONDARY,QCFAIL"));
	libmaus2::bambam::BamCircularHashCollatingBamDecoder CHCBD(
		std::cin,
		"tmpfile",
		excludeflags,
		false /* put rank */
	);
	libmaus2::bambam::CircularHashCollatingBamDecoder::OutputBufferEntry const * ob = 0;

	libmaus2::bambam::BamHeader const & bamheader = CHCBD.getHeader();
	std::string const headertext(bamheader.text);

	// add PG line to header
	std::string const upheadtext = ::libmaus2::bambam::ProgramHeaderLineSet::addProgramLine(
		headertext,
		arginfo.progname, // ID
		arginfo.progname, // PN
		arginfo.commandline, // CL
		::libmaus2::bambam::ProgramHeaderLineSet(headertext).getLastIdInChain(), // PP
		std::string(PACKAGE_VERSION) // VN
	);
	// construct new header
	::libmaus2::bambam::BamHeader uphead(upheadtext);
	// change sort order
	uphead.changeSortOrder("unknown");

	libmaus2::bambam::BamWriter bamwr(std::cout,uphead,0);

	uint64_t cnt = 0;

	unsigned int const verbshift = 20;
	libmaus2::timing::RealTimeClock rtc; rtc.start();

	while ( (ob = CHCBD.process()) )
	{
		uint64_t const precnt = cnt;

		if ( ob->fpair )
		{
			libmaus2::bambam::EncoderBase::putLE< libmaus2::lz::BgzfDeflate<std::ostream>,uint32_t>(bamwr.getStream(),ob->blocksizea);
			bamwr.getStream().write(reinterpret_cast<char const *>(ob->Da),ob->blocksizea);
			libmaus2::bambam::EncoderBase::putLE< libmaus2::lz::BgzfDeflate<std::ostream>,uint32_t>(bamwr.getStream(),ob->blocksizeb);
			bamwr.getStream().write(reinterpret_cast<char const *>(ob->Db),ob->blocksizeb);

			cnt += 2;
		}
		else if ( ob->fsingle || ob->forphan1 || ob->forphan2 )
		{
			libmaus2::bambam::EncoderBase::putLE< libmaus2::lz::BgzfDeflate<std::ostream>,uint32_t>(bamwr.getStream(),ob->blocksizea);
			bamwr.getStream().write(reinterpret_cast<char const *>(ob->Da),ob->blocksizea);

			cnt += 1;
		}

		if ( precnt >> verbshift != cnt >> verbshift )
		{
			std::cerr << (cnt >> 20) << "\t" << static_cast<double>(cnt)/rtc.getElapsedSeconds() << std::endl;
		}
	}

	std::cerr << cnt << std::endl;
}

#include <libmaus2/bambam/BamToFastqOutputFileSet.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>

void bamtofastqNonCollating(libmaus2::util::ArgInfo const & arginfo)
{
	libmaus2::timing::RealTimeClock rtc; rtc.start();
	uint32_t const excludeflags = libmaus2::bambam::BamFlagBase::stringToFlags(arginfo.getValue<std::string>("exclude","SECONDARY,QCFAIL"));
	libmaus2::bambam::BamDecoder bamdec(std::cin);
	libmaus2::bambam::BamAlignment const & algn = bamdec.getAlignment();
	::libmaus2::autoarray::AutoArray<uint8_t> T;
	uint64_t cnt = 0;
	uint64_t bcnt = 0;
	unsigned int const verbshift = 20;

	while ( bamdec.readAlignment() )
	{
		uint64_t const precnt = cnt++;

		if ( ! (algn.getFlags() & excludeflags) )
		{
			uint64_t la = libmaus2::bambam::BamAlignmentDecoderBase::putFastQ(algn.D.begin(),T);
			std::cout.write(reinterpret_cast<char const *>(T.begin()),la);
			bcnt += la;
		}

		if ( precnt >> verbshift != cnt >> verbshift )
		{
			std::cerr
				<< (cnt >> 20)
				<< "\t"
				<< (static_cast<double>(bcnt)/(1024.0*1024.0))/rtc.getElapsedSeconds() << "MB/s"
				<< "\t" << static_cast<double>(cnt)/rtc.getElapsedSeconds() << std::endl;
		}
	}

	std::cout.flush();
}

void bamtofastqCollating(
	libmaus2::util::ArgInfo const & arginfo,
	libmaus2::bambam::CircularHashCollatingBamDecoder & CHCBD
)
{
	libmaus2::bambam::BamToFastqOutputFileSet OFS(arginfo);

	libmaus2::bambam::CircularHashCollatingBamDecoder::OutputBufferEntry const * ob = 0;

	// number of alignments written to files
	uint64_t cnt = 0;
	// number of bytes written to files
	uint64_t bcnt = 0;
	unsigned int const verbshift = 20;
	libmaus2::timing::RealTimeClock rtc; rtc.start();
	::libmaus2::autoarray::AutoArray<uint8_t> T;

	while ( (ob = CHCBD.process()) )
	{
		uint64_t const precnt = cnt;

		if ( ob->fpair )
		{
			uint64_t la = libmaus2::bambam::BamAlignmentDecoderBase::putFastQ(ob->Da,T);
			OFS.Fout.write(reinterpret_cast<char const *>(T.begin()),la);
			uint64_t lb = libmaus2::bambam::BamAlignmentDecoderBase::putFastQ(ob->Db,T);
			OFS.F2out.write(reinterpret_cast<char const *>(T.begin()),lb);

			cnt += 2;
			bcnt += (la+lb);
		}
		else if ( ob->fsingle )
		{
			uint64_t la = libmaus2::bambam::BamAlignmentDecoderBase::putFastQ(ob->Da,T);
			OFS.Sout.write(reinterpret_cast<char const *>(T.begin()),la);

			cnt += 1;
			bcnt += (la);
		}
		else if ( ob->forphan1 )
		{
			uint64_t la = libmaus2::bambam::BamAlignmentDecoderBase::putFastQ(ob->Da,T);
			OFS.Oout.write(reinterpret_cast<char const *>(T.begin()),la);

			cnt += 1;
			bcnt += (la);
		}
		else if ( ob->forphan2 )
		{
			uint64_t la = libmaus2::bambam::BamAlignmentDecoderBase::putFastQ(ob->Da,T);
			OFS.O2out.write(reinterpret_cast<char const *>(T.begin()),la);

			cnt += 1;
			bcnt += (la);
		}

		if ( precnt >> verbshift != cnt >> verbshift )
		{
			std::cerr
				<< "[V] "
				<< (cnt >> 20)
				<< "\t"
				<< (static_cast<double>(bcnt)/(1024.0*1024.0))/rtc.getElapsedSeconds() << "MB/s"
				<< "\t" << static_cast<double>(cnt)/rtc.getElapsedSeconds() << std::endl;
		}
	}

	std::cerr << "[V] " << cnt << std::endl;
}

void bamtofastqCollating(libmaus2::util::ArgInfo const & arginfo)
{
	uint32_t const excludeflags = libmaus2::bambam::BamFlagBase::stringToFlags(arginfo.getValue<std::string>("exclude","SECONDARY,QCFAIL"));
	libmaus2::util::TempFileRemovalContainer::setup();
	std::string const tmpfilename = arginfo.getValue<std::string>("T",arginfo.getDefaultTmpFileName());
	libmaus2::util::TempFileRemovalContainer::addTempFile(tmpfilename);
	std::string const inputformat = arginfo.getValue<std::string>("inputformat","bam");

	if ( inputformat == "bam" )
	{
		libmaus2::bambam::BamCircularHashCollatingBamDecoder CHCBD(
			std::cin,
			tmpfilename,
			excludeflags,
			false /* put rank */
			);
		bamtofastqCollating(arginfo,CHCBD);
	}
	#if defined(LIBMAUS2_HAVE_DL_FUNCS)
	else if ( inputformat == "sam" )
	{
		libmaus2::bambam::ScramCircularHashCollatingBamDecoder CHCBD(
			"-","r","",
			tmpfilename,
			excludeflags,
			false /* put rank */
		);
		bamtofastqCollating(arginfo,CHCBD);
	}
	else if ( inputformat == "cram" )
	{
		std::string const reference = arginfo.getValue<std::string>("reference","");
		libmaus2::bambam::ScramCircularHashCollatingBamDecoder CHCBD(
			"-","rc",reference,
			tmpfilename,
			excludeflags,
			false/* put rank */
		);
		bamtofastqCollating(arginfo,CHCBD);
	}
	#endif
}

void bamtofastq(libmaus2::util::ArgInfo const & arginfo)
{
	if ( arginfo.getValue<uint64_t>("collate",1) )
		bamtofastqCollating(arginfo);
	else
		bamtofastqNonCollating(arginfo);
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo arginfo(argc,argv);
		bamtofastq(arginfo);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
