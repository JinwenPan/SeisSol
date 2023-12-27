/**
 * @file
 * This file is part of SeisSol.
 *
 * @author Carsten Uphoff (c.uphoff AT tum.de, http://www5.in.tum.de/wiki/index.php/Carsten_Uphoff,_M.Sc.)
 *
 * @section LICENSE
 * Copyright (c) 2016, SeisSol Group
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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 **/
 
#ifndef INITIALIZER_LTS_H_
#define INITIALIZER_LTS_H_

#include "tree/Layer.hpp"
#include <Initializer/typedefs.hpp>
#include <Initializer/tree/LTSTree.hpp>
#include <generated_code/tensor.h>
#include <Kernels/common.hpp>

#ifndef ACL_DEVICE
#   define MEMKIND_GLOBAL   AllocationMode::HostOnlyHBM
#if CONVERGENCE_ORDER <= 7
#   define MEMKIND_TIMEDOFS AllocationMode::HostOnlyHBM
#   define MEMKIND_TIMEDOFS_CONSTANT AllocationMode::HostOnlyHBM
#else
#   define MEMKIND_TIMEDOFS AllocationMode::HostOnly
#   define MEMKIND_TIMEDOFS_CONSTANT AllocationMode::HostOnly
#endif
#define MEMKIND_TIMEBUCKET MEMKIND_TIMEDOFS
#if CONVERGENCE_ORDER <= 4
#   define MEMKIND_CONSTANT AllocationMode::HostOnlyHBM
#else
#   define MEMKIND_CONSTANT AllocationMode::HostOnly
#endif
#if CONVERGENCE_DOFS <= 3
#   define MEMKIND_DOFS     AllocationMode::HostOnlyHBM
#else
#   define MEMKIND_DOFS     AllocationMode::HostOnly
#endif
# define MEMKIND_UNIFIED  AllocationMode::HostOnly
#else // ACL_DEVICE
#	define MEMKIND_GLOBAL   AllocationMode::HostOnly
#	define MEMKIND_CONSTANT AllocationMode::HostOnly
# define MEMKIND_TIMEDOFS_CONSTANT AllocationMode::HostOnly
#	define MEMKIND_DOFS     AllocationMode::HostDeviceSplit // HostDeviceUnified
#	define MEMKIND_TIMEDOFS AllocationMode::HostDeviceSplit // HostDeviceUnified
#	define MEMKIND_TIMEBUCKET AllocationMode::DeviceOnly
# define MEMKIND_UNIFIED  AllocationMode::HostDeviceSplit // HostDeviceUnified
#endif // ACL_DEVICE

namespace seissol {
  namespace initializers {
    struct LTS;
  }
  namespace tensor {
    class Qane;
  }
}

struct seissol::initializers::LTS {
  Variable<real[tensor::Q::size()]>       dofs;
  // size is zero if Qane is not defined
  Variable<real[ALLOW_POSSILBE_ZERO_LENGTH_ARRAY(kernels::size<tensor::Qane>())]> dofsAne;
  Variable<real*>                         buffers;
  Variable<real*>                         derivatives;
  Variable<CellLocalInformation>          cellInformation;
  Variable<real*[4]>                      faceNeighbors;
  Variable<LocalIntegrationData>          localIntegration;
  Variable<NeighboringIntegrationData>    neighboringIntegration;
  Variable<CellMaterialData>              material;
  Variable<PlasticityData>                plasticity;
  Variable<CellDRMapping[4]>              drMapping;
  Variable<CellBoundaryMapping[4]>        boundaryMapping;
  Variable<real[tensor::QStress::size() + tensor::QEtaModal::size()]> pstrain;
  Variable<real*[4]>                      faceDisplacements;
  Bucket                                  buffersDerivatives;
  Bucket                                  faceDisplacementsBuffer;

#ifdef ACL_DEVICE
  Variable<real*>                         buffersDevice;
  Variable<real*>                         derivativesDevice;
  Variable<real*[4]>                      faceDisplacementsDevice;
  Variable<LocalIntegrationData>          localIntegrationOnDevice;
  Variable<NeighboringIntegrationData>    neighIntegrationOnDevice;
  ScratchpadMemory                        integratedDofsScratch;
  ScratchpadMemory                        derivativesScratch;
  ScratchpadMemory                        nodalAvgDisplacements;
#endif
  
  /// \todo Memkind
  void addTo(LTSTree& tree, bool usePlasticity) {
    LayerMask plasticityMask;
    if (usePlasticity) {
      plasticityMask = LayerMask(Ghost);
    } else {
      plasticityMask = LayerMask(Ghost) | LayerMask(Copy) | LayerMask(Interior);
    }

    tree.addVar(                    dofs, LayerMask(Ghost),     PAGESIZE_HEAP,      MEMKIND_DOFS );
    if (kernels::size<tensor::Qane>() > 0) {
      tree.addVar(                 dofsAne, LayerMask(Ghost),     PAGESIZE_HEAP,      MEMKIND_DOFS );
    }
    tree.addVar(                 buffers,      LayerMask(),                 1,      MEMKIND_TIMEDOFS_CONSTANT );
    tree.addVar(             derivatives,      LayerMask(),                 1,      MEMKIND_TIMEDOFS_CONSTANT );
    tree.addVar(         cellInformation,      LayerMask(),                 1,      MEMKIND_CONSTANT );
    tree.addVar(           faceNeighbors, LayerMask(Ghost),                 1,      MEMKIND_TIMEDOFS_CONSTANT );
    tree.addVar(        localIntegration, LayerMask(Ghost),                 1,      MEMKIND_CONSTANT );
    tree.addVar(  neighboringIntegration, LayerMask(Ghost),                 1,      MEMKIND_CONSTANT );
    tree.addVar(                material, LayerMask(Ghost),                 1,      AllocationMode::HostOnly );
    tree.addVar(              plasticity,   plasticityMask,                 1,      MEMKIND_UNIFIED );
    tree.addVar(               drMapping, LayerMask(Ghost),                 1,      MEMKIND_CONSTANT );
    tree.addVar(         boundaryMapping, LayerMask(Ghost),                 1,      MEMKIND_CONSTANT );
    tree.addVar(                 pstrain,   plasticityMask,     PAGESIZE_HEAP,      MEMKIND_UNIFIED );
    tree.addVar(       faceDisplacements, LayerMask(Ghost),     PAGESIZE_HEAP,      AllocationMode::HostOnly );

    tree.addBucket(buffersDerivatives,                          PAGESIZE_HEAP,      MEMKIND_TIMEBUCKET );
    tree.addBucket(faceDisplacementsBuffer,                     PAGESIZE_HEAP,      MEMKIND_TIMEDOFS );

#ifdef ACL_DEVICE
    tree.addVar(   buffersDevice, LayerMask(Ghost),     1,      AllocationMode::HostOnly );
    tree.addVar(   derivativesDevice, LayerMask(Ghost),     1,      AllocationMode::HostOnly );
    tree.addVar(   faceDisplacementsDevice, LayerMask(Ghost),     1,      AllocationMode::HostOnly );
    tree.addVar(   localIntegrationOnDevice,   LayerMask(Ghost),  1,      AllocationMode::DeviceOnly);
    tree.addVar(   neighIntegrationOnDevice,   LayerMask(Ghost),  1,      AllocationMode::DeviceOnly);
    tree.addScratchpadMemory(  integratedDofsScratch,             1,      AllocationMode::HostDeviceSplit);
    tree.addScratchpadMemory(derivativesScratch,                  1,      AllocationMode::DeviceOnly);
    tree.addScratchpadMemory(nodalAvgDisplacements,               1,      AllocationMode::DeviceOnly);
#endif
  }
};
#endif
