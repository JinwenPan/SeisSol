#! /usr/bin/env sh

# NOTE: just an adapted format.sh script

# sample command line usage: $0 clang-tidy $SEISSOL_SOURCE_DIR $SEISSOL_BUILD_DIR [$ARGS for clang-tidy]
# e.g. do, if SEISSOL_BUILD_DIR=$SEISSOL_SOURCE_DIR/build
# $ cd $SEISSOL_SOURCE_DIR; sh ./.ci/tidy.sh ./ ./build
# $ cd $SEISSOL_SOURCE_DIR/.ci; sh tidy.sh ../ ../build

format() {
    # don't use a directory with whitespace
    local allowlist_dir="
        src/DynamicRupture
        src/Geometry
        src/Initializer/BatchRecorders
        src/Initializer/InitProcedure
        src/Initializer/Parameters
        src/Initializer/tree
        src/IO
        src/Modules
        src/Monitoring
        src/Numerical
        src/Parallel
        src/Physics
        src/Reader
        src/SourceTerm
        src/tests
        "

    # NOTE: once the files of a directory are (almost) fully covered, consider moving it to allowlist_dir instead
    local allowlist_file="
        src/Equations/elastic/Model/datastructures.hpp
        src/Equations/elastic/Model/integrationData.hpp
        src/Equations/viscoelastic/Model/integrationData.hpp
        src/Equations/viscoelastic2/Model/datastructures.hpp
        src/Equations/viscoelastic2/Model/integrationData.hpp
        src/Equations/anisotropic/Model/datastructures.hpp
        src/Equations/anisotropic/Model/integrationData.hpp
        src/Equations/poroelastic/Model/datastructures.hpp
        src/Equations/poroelastic/Model/integrationData.hpp
        src/Equations/datastructures.hpp
        src/Equations/Setup.h
        src/Initializer/BasicTypedefs.hpp
        src/Initializer/Boundary.h
        src/Initializer/DynamicRupture.h
        src/Initializer/DeviceGraph.h
        src/Initializer/GlobalData.h
        src/Initializer/GlobalData.cpp
        src/Initializer/InitialFieldProjection.h
        src/Initializer/InitialFieldProjection.cpp
        src/Initializer/InputAux.hpp
        src/Initializer/LTS.h
        src/Initializer/MemoryAllocator.h
        src/Initializer/MemoryAllocator.cpp
        src/Initializer/ParameterDB.h
        src/Initializer/ParameterDB.cpp
        src/Initializer/PointMapper.h
        src/Initializer/PointMapper.cpp
        src/Initializer/preProcessorMacros.hpp
        src/Initializer/time_stepping/GlobalTimestep.hpp
        src/Initializer/time_stepping/GlobalTimestep.cpp
        src/Kernels/common.hpp
        src/Kernels/DynamicRupture.h
        src/Kernels/DynamicRupture.cpp
        src/Kernels/Local.h
        src/Kernels/Neighbor.h
        src/Kernels/Plasticity.h
        src/Kernels/Plasticity.cpp
        src/Kernels/PointSourceCluster.h
        src/Kernels/PointSourceClusterOnHost.h
        src/Kernels/PointSourceClusterOnHost.cpp
        src/Kernels/PointSourceClusterOnDevice.h
        src/Kernels/PointSourceClusterOnDevice.cpp
        src/Kernels/precision.hpp
        src/Kernels/Receiver.h
        src/Kernels/Receiver.cpp
        src/Kernels/Time.h
        src/Kernels/TimeCommon.h
        src/Kernels/TimeCommon.cpp
        src/Kernels/Touch.h
        src/Kernels/Touch.cpp
        src/Model/common_datastructures.hpp
        src/Model/plasticity.hpp
        src/ResultWriter/WaveFieldWriter.h
        src/ResultWriter/EnergyOutput.h
        src/ResultWriter/EnergyOutput.cpp
        src/ResultWriter/AnalysisWriter.h
        src/ResultWriter/AnalysisWriter.cpp
        src/ResultWriter/AsyncCellIDs.h
        src/ResultWriter/AsyncIO.h
        src/ResultWriter/AsyncIO.cpp
        src/ResultWriter/MiniSeisSolWriter.h
        src/ResultWriter/MiniSeisSolWriter.cpp
        src/ResultWriter/PostProcessor.h
        src/ResultWriter/PostProcessor.cpp
        src/ResultWriter/ThreadsPinningWriter.h
        src/ResultWriter/ThreadsPinningWriter.cpp
        src/SeisSol.h
        src/SeisSol.cpp
        src/main.cpp
        "


    local SEISSOL_SOURCE_DIR="${1}"
    local SEISSOL_BUILD_DIR="${2}"
    shift 2

    # check for self
    if [ ! -f "${SEISSOL_SOURCE_DIR}/.ci/tidy.sh" ]; then
        echo "Please ensure that SEISSOL_SOURCE_DIR is passed as the first argument"
        exit 177
    fi

    # (we'll treat all files as headers here, but that should not have any effect on the behavior of clang-tidy)
    # regex escaping is from https://unix.stackexchange.com/a/209744
    # (it may not be 100%ly exact, but it works for our case)

    FILE_REGEX="^\$"
    for dir in ${allowlist_dir}; do
        escaped="$(printf '%s' "$dir" | sed 's/[.[\(*^$+?{|]/\\&/g')"
        FILE_REGEX="${FILE_REGEX}|${escaped}/"
    done
    for file in ${allowlist_file}; do
        escaped="$(printf '%s' "$file" | sed 's/[.[\(*^$+?{|]/\\&/g')"
        FILE_REGEX="${FILE_REGEX}|${escaped}\$"
    done

    run-clang-tidy -header-filter=$FILE_REGEX -p $SEISSOL_BUILD_DIR $@ $FILE_REGEX
}

format $@
