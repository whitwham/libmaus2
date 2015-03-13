/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#include <libmaus/bambam/parallel/BlockSortControl.hpp>
#include <libmaus/bambam/parallel/BlockMergeControl.hpp>

#include <config.h>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferPosComparator.hpp>
#include <libmaus/parallel/NumCpus.hpp>

#include <libmaus/digest/Digests.hpp>

#if defined(LIBMAUS_HAVE_SHA2_ASSEMBLY)
#include <libmaus/digest/DigestFactory_SHA2_ASM.hpp>
#include <libmaus/digest/DigestFactory_CRC32C_SSE42.hpp>
#include <libmaus/digest/DigestFactoryContainer.hpp>
#endif

int main(int argc, char * argv[])
{
	try
	{
		#if defined(LIBMAUS_HAVE_SHA2_ASSEMBLY)
		libmaus::digest::DigestFactoryContainer::addFactories(libmaus::digest::DigestFactory_SHA2_ASM());
		#endif
		#if defined(LIBMAUS_HAVE_SMMINTRIN_H)
		libmaus::digest::DigestFactoryContainer::addFactories(libmaus::digest::DigestFactory_CRC32C_SSE42());
		#endif
		
		libmaus::timing::RealTimeClock progrtc; progrtc.start();
		libmaus::util::ArgInfo const arginfo(argc,argv);
		typedef libmaus::bambam::parallel::FragmentAlignmentBufferPosComparator order_type;
		// typedef libmaus::bambam::parallel::FragmentAlignmentBufferNameComparator order_type;
		
		libmaus::timing::RealTimeClock rtc;
		
		rtc.start();
		uint64_t const numlogcpus = arginfo.getValue<int>("threads",libmaus::parallel::NumCpus::getNumLogicalProcessors());
		libmaus::aio::PosixFdInputStream in(STDIN_FILENO,256*1024);
		std::string const tmpfilebase = arginfo.getUnparsedValue("tmpfile",arginfo.getDefaultTmpFileName());
		int const templevel = arginfo.getValue<int>("level",1);
		std::string const hash = arginfo.getValue<std::string>("hash","crc32prod");
		std::string const filehash = arginfo.getValue<std::string>("filehash","md5");

		std::string const sinputformat = arginfo.getUnparsedValue("inputformat","bam");
		libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_enum inform = libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_bam;
		
		if ( sinputformat == "bam" )
		{
			inform = libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_bam;
		}
		else if ( sinputformat == "sam" )
		{
			inform = libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_sam;			
		}
		else
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "Unknown input format " << sinputformat << std::endl;
			lme.finish();
			throw lme;				
		}
					
		libmaus::parallel::SimpleThreadPool STP(numlogcpus);
		libmaus::bambam::parallel::BlockSortControl<order_type>::unique_ptr_type VC(
			new libmaus::bambam::parallel::BlockSortControl<order_type>(
				inform,STP,in,templevel,tmpfilebase,hash
			)
		);
		VC->enqueReadPackage();
		VC->waitDecodingFinished();
		// VC->printChecksums(std::cerr);
		VC->printChecksumsForBamHeader(std::cerr);
		VC->printSizes(std::cerr);
		VC->printPackageFreeListSizes(std::cerr);
		#if defined(AUTOARRAY_TRACE)
		libmaus::autoarray::autoArrayPrintTraces(std::cerr);
		#endif
		VC->freeBuffers();
		VC->flushReadEndsLists(std::cerr /* metrics stream */,"testparallelbamblocksort");

		std::vector<libmaus::bambam::parallel::GenericInputControlStreamInfo> const BI = VC->getBlockInfo();
		libmaus::bitio::BitVector::unique_ptr_type Pdupvec(VC->releaseDupBitVector());
		libmaus::bambam::BamHeader::unique_ptr_type Pheader(VC->getHeader());
		::libmaus::bambam::BamHeader::unique_ptr_type uphead(libmaus::bambam::BamHeaderUpdate::updateHeader(arginfo,*Pheader,"testparallelbamblocksort",PACKAGE_VERSION));
		uphead->changeSortOrder("coordinate");
		std::ostringstream hostr;
		uphead->serialise(hostr);
		std::string const hostrstr = hostr.str();
		libmaus::autoarray::AutoArray<char> sheader(hostrstr.size(),false);
		std::copy(hostrstr.begin(),hostrstr.end(),sheader.begin());		
		VC.reset();
					
		std::cerr << "[V] blocks generated in time " << rtc.formatTime(rtc.getElapsedSeconds()) << std::endl;
		
		rtc.start();
		uint64_t const inputblocksize = 1024*1024;
		uint64_t const inputblocksperfile = 8;
		uint64_t const mergebuffersize = 256*1024*1024;
		uint64_t const mergebuffers = 4;
		uint64_t const complistsize = 32;
		int const level = arginfo.getValue<int>("level",Z_DEFAULT_COMPRESSION);

		libmaus::digest::DigestInterface::unique_ptr_type Pdigest(libmaus::digest::DigestFactoryContainer::construct(filehash));

		libmaus::bambam::parallel::BlockMergeControl BMC(
			STP,std::cout,sheader,BI,*Pdupvec,level,inputblocksize,inputblocksperfile /* blocks per channel */,mergebuffersize /* merge buffer size */,mergebuffers /* number of merge buffers */, complistsize /* number of bgzf preload blocks */,hash,tmpfilebase,Pdigest.get());
		BMC.addPending();			
		BMC.waitWritingFinished();		
	
		std::cerr << "[D]\t" << filehash << "\t" << BMC.getFileDigest() << std::endl;

		std::cerr << "[V] blocks merged in time " << rtc.formatTime(rtc.getElapsedSeconds()) << std::endl;

		STP.terminate();
		STP.join();

		std::cerr << "[V] run time " << progrtc.formatTime(progrtc.getElapsedSeconds()) << " (" << progrtc.getElapsedSeconds() << " s)" << "\t" << libmaus::util::MemUsage() << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
