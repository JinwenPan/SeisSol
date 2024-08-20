#pragma once

#include "utils/logger.h"
#include <IO/Datatype/Datatype.hpp>
#include <IO/Datatype/Inference.hpp>
#include <IO/Datatype/MPIType.hpp>
#include <IO/Instance/SeisSolMemoryHelper.hpp>
#include <IO/Writer/Instructions/Data.hpp>
#include <IO/Writer/Instructions/Hdf5.hpp>
#include <IO/Writer/Instructions/Instruction.hpp>
#include <IO/Writer/Writer.hpp>
#include <Initializer/MemoryManager.h>
#include <functional>
#include <memory>

namespace seissol::io::instance::mesh {
class VtkHdfWriter {
  public:
  VtkHdfWriter(const std::string& name,
               std::size_t localElementCount,
               std::size_t dimension,
               std::size_t targetDegree);

  template <typename F>
  void addPointProjector(F&& projector) {
    auto selfLocalElementCount = localElementCount;
    auto selfPointsPerElement = pointsPerElement;

    instructionsConst.emplace_back([=](const std::string& filename, double time) {
      return std::make_shared<writer::instructions::Hdf5DataWrite>(
          writer::instructions::Hdf5Location(filename, {groupName}),
          "Points",
          writer::GeneratedBuffer::createElementwise<double>(
              selfLocalElementCount, selfPointsPerElement, std::vector<std::size_t>{3}, projector),
          datatype::inferDatatype<double>());
    });
  }

  template <typename T, typename F>
  void addPointData(const std::string& name,
                    const std::vector<std::size_t>& dimensions,
                    F&& pointMapper) {
    auto selfLocalElementCount = localElementCount;
    auto selfPointsPerElement = pointsPerElement;

    instructions.emplace_back([=](const std::string& filename, double time) {
      return std::make_shared<writer::instructions::Hdf5DataWrite>(
          writer::instructions::Hdf5Location(filename, {groupName, pointDataName}),
          name,
          writer::GeneratedBuffer::createElementwise<T>(
              selfLocalElementCount, selfPointsPerElement, dimensions, pointMapper),
          datatype::inferDatatype<T>());
    });
  }

  template <typename T, typename F>
  void addCellData(const std::string& name,
                   const std::vector<std::size_t>& dimensions,
                   F&& cellMapper) {
    auto selfLocalElementCount = localElementCount;

    instructions.emplace_back([=](const std::string& filename, double time) {
      return std::make_shared<writer::instructions::Hdf5DataWrite>(
          writer::instructions::Hdf5Location(filename, {groupName, cellDataName}),
          name,
          writer::GeneratedBuffer::createElementwise<T>(
              selfLocalElementCount, 1, dimensions, cellMapper),
          datatype::inferDatatype<T>());
    });
  }

  template <typename T>
  void addFieldData(const std::string& name,
                    const std::vector<std::size_t>& dimensions,
                    const std::vector<T>& data) {
    instructions.emplace_back([=](const std::string& filename, double time) {
      return std::make_shared<writer::instructions::Hdf5DataWrite>(
          writer::instructions::Hdf5Location(filename, {groupName, fieldDataName}),
          name,
          writer::WriteInline::createArray(dimensions, data),
          datatype::inferDatatype<T>());
    });
  }

  void addHook(const std::function<void(std::size_t, double)>& hook);

  std::function<writer::Writer(const std::string&, std::size_t, double)> makeWriter();

  private:
  std::string name;
  std::size_t localElementCount;
  std::size_t globalElementCount;
  std::size_t elementOffset;
  std::size_t localPointCount;
  std::size_t globalPointCount;
  std::size_t pointOffset;
  std::size_t pointsPerElement;
  std::vector<std::function<void(std::size_t, double)>> hooks;
  std::vector<std::function<std::shared_ptr<writer::instructions::WriteInstruction>(
      const std::string&, double)>>
      instructionsConst;
  std::vector<std::function<std::shared_ptr<writer::instructions::WriteInstruction>(
      const std::string&, double)>>
      instructions;
  std::size_t type;
  const static inline std::string groupName = "VTKHDF";
  const static inline std::string fieldDataName = "FieldData";
  const static inline std::string cellDataName = "CellData";
  const static inline std::string pointDataName = "PointData";
};
} // namespace seissol::io::instance::mesh
