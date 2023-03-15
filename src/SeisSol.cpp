/**
 * @file
 * This file is part of SeisSol.
 *
 * @author Sebastian Rettenberger (sebastian.rettenberger AT tum.de, http://www5.in.tum.de/wiki/index.php/Sebastian_Rettenberger)
 *
 * @section LICENSE
 * Copyright (c) 2014-2016, SeisSol Group
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 * Main C++ SeisSol file
 */

#include <climits>
#include <unistd.h>
#include <sys/resource.h>
#include <fty/fty.hpp>

#ifdef ACL_DEVICE
#include "device.h"
#endif

#ifdef _OPENMP
#include <omp.h>
#endif // _OPENMP

#include "utils/args.h"

#include "SeisSol.h"
#include "Modules/Modules.h"
#include "Parallel/MPI.h"
#include "Parallel/Pin.h"
#include "ResultWriter/ThreadsPinningWriter.h"

// Autogenerated file
#include "version.h"

#ifdef USE_ASAGI
#include "Reader/AsagiModule.h"
#endif

bool seissol::SeisSol::init(int argc, char* argv[]) {
#ifdef USE_ASAGI
  // Construct an instance of AsagiModule, to initialize it.
  // It needs to be done here, as it registers PRE_MPI hooks
  asagi::AsagiModule::getInstance();
#endif
  // Call pre MPI hooks
  seissol::Modules::callHook<seissol::PRE_MPI>();

#if defined(ACL_DEVICE) && defined(USE_MPI)
  MPI::mpi.bindAcceleratorDevice();
#endif

  MPI::mpi.init(argc, argv);
  const int rank = MPI::mpi.rank();

  // Print welcome message
  logInfo(rank) << "Welcome to SeisSol";
  logInfo(rank) << "Copyright (c) 2012-2021, SeisSol Group";
  logInfo(rank) << "Built on:" << __DATE__ << __TIME__ ;
  logInfo(rank) << "Version:" << VERSION_STRING;

  if (MPI::mpi.rank() == 0) {
    const auto& hostNames = MPI::mpi.getHostNames();
    logInfo() << "Running on:" << hostNames.front();
  }

#ifdef USE_MPI
  logInfo(rank) << "Using MPI with #ranks:" << MPI::mpi.size();
#endif
#ifdef _OPENMP
  logInfo(rank) << "Using OMP with #threads/rank:" << omp_get_max_threads();
  logInfo(rank) << "OpenMP worker affinity (this process):" << parallel::Pinning::maskToString(
      pinning.getWorkerUnionMask());
  logInfo(rank) << "OpenMP worker affinity (this node)   :" << parallel::Pinning::maskToString(
      pinning.getNodeMask());
#endif
#ifdef USE_COMM_THREAD
  auto freeCpus = pinning.getFreeCPUsMask();
  logInfo(rank) << "Communication thread affinity        :" << parallel::Pinning::maskToString(freeCpus);
  if (parallel::Pinning::freeCPUsMaskEmpty(freeCpus)) {
    logError() << "There are no free CPUs left. Make sure to leave one for the communication thread.";
  }
#endif // _OPENMP

#ifdef ACL_DEVICE
  device::DeviceInstance &device = device::DeviceInstance::getInstance();
  device.api->initialize();
  device.api->allocateStackMem();
#endif

  // Check if the ulimit for the stacksize is reasonable.
  // A low limit can lead to segmentation faults.
  rlimit rlim;
  if (getrlimit(RLIMIT_STACK, &rlim) == 0) {
    const auto rlimInKb = rlim.rlim_cur / 1024;
    // Softlimit (rlim_cur) is enforced by the kernel.
    // This limit is pretty arbitrarily set at 2GB.
    const auto reasonableStackLimit = 2097152; // [kb]
    if (rlim.rlim_cur == RLIM_INFINITY) {
      logInfo(rank) << "The stack size ulimit is unlimited.";
    } else {
      logInfo(rank) << "The stack size ulimit is " << rlimInKb
		    << "[kb].";
    }
    if (rlimInKb < reasonableStackLimit) {
      logWarning(rank) << "Stack size of"
		       << rlimInKb
		       << "[kb] is lower than recommended minimum of"
		       << reasonableStackLimit
		       << "[kb]."
		       << "You can increase the stack size by running the command: ulimit -Ss unlimited.";
    }
  } else {
    logError() << "Stack size cannot be determined because getrlimit syscall failed!";
  }
  
  // Call post MPI initialization hooks
  seissol::Modules::callHook<seissol::POST_MPI_INIT>();

  // Parse command line arguments
  utils::Args args;
  args.addAdditionalOption("file", "The parameter file", false);
  switch (args.parse(argc, argv)) {
  case utils::Args::Help:
  case utils::Args::Error:
	  MPI::mpi.finalize();
	  exit(1);
	  break;
  case utils::Args::Success:
	  break;
  }

  // Initialize the ASYNC I/O library
  if (!m_asyncIO.init())
	  return false;

  m_parameterFile = args.getAdditionalArgument("file", "PARAMETER.par");
  m_memoryManager->initialize();
  // read parameter file input
  readInputParams();

  m_seissolparameters.readPar(*m_inputParams);

  m_memoryManager->setInputParams(m_inputParams);

  if (auto outputDirectory = (*m_inputParams)["output"]["outputfile"]) {
    writer::ThreadsPinningWriter pinningWriter(outputDirectory.as<std::string>());
    pinningWriter.write(pinning);
  }
  else {
    logError() << "no output path given";
  }

  return true;
}

void seissol::SeisSol::finalize()
{
	// Cleanup ASYNC I/O library
	m_asyncIO.finalize();

	const int rank = MPI::mpi.rank();

#ifdef ACL_DEVICE
	device::DeviceInstance &device = device::DeviceInstance::getInstance();
	device.api->finalize();
#endif

	MPI::mpi.finalize();

	logInfo(rank) << "SeisSol done. Goodbye.";
}

void seissol::SeisSol::readInputParams() {
  // Read parameter file input from file
  fty::Loader<fty::AsLowercase> loader{};
  try {
    m_inputParams = std::make_shared<YAML::Node>(loader.load(m_parameterFile));
  }
  catch (const std::exception& error) {
    std::cerr << error.what() << std::endl;
    finalize();
  }
}

seissol::SeisSol seissol::SeisSol::main;
