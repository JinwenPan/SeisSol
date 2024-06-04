#ifndef SEISSOL_DR_OUTPUT_MANAGER_HPP
#define SEISSOL_DR_OUTPUT_MANAGER_HPP

#include "DynamicRupture/Output/Builders/ElementWiseBuilder.hpp"
#include "DynamicRupture/Output/Builders/PickPointBuilder.hpp"
#include "DynamicRupture/Output/ReceiverBasedOutput.hpp"
#include "Initializer/Parameters/SeisSolParameters.h"
#include <memory>

namespace seissol {
class SeisSol;

namespace dr::output {

class OutputManager {
  public:
  ~OutputManager();
  OutputManager() = delete;
  OutputManager(std::unique_ptr<ReceiverOutput> concreteImpl, seissol::SeisSol& seissolInstance);
  void setInputParam(seissol::geometry::MeshReader& userMesher);
  void setLtsData(seissol::initializer::LTSTree* userWpTree,
                  seissol::initializer::LTS* userWpDescr,
                  seissol::initializer::Lut* userWpLut,
                  seissol::initializer::LTSTree* userDrTree,
                  seissol::initializer::DynamicRupture* userDrDescr);
  void setBackupTimeStamp(const std::string& stamp) { this->backupTimeStamp = stamp; }

  void init();
  void initFaceToLtsMap();
  void writePickpointOutput(double time, double dt);
  void flushPickpointDataToFile();
  void updateElementwiseOutput();

  private:
  seissol::SeisSol& seissolInstance;

  protected:
  bool isAtPickpoint(double time, double dt);
  void initElementwiseOutput();
  void initPickpointOutput();

  std::unique_ptr<ElementWiseBuilder> ewOutputBuilder{nullptr};
  std::unique_ptr<PickPointBuilder> ppOutputBuilder{nullptr};

  std::shared_ptr<ReceiverOutputData> ewOutputData{nullptr};
  std::shared_ptr<ReceiverOutputData> ppOutputData{nullptr};

  seissol::initializer::LTS* wpDescr{nullptr};
  seissol::initializer::LTSTree* wpTree{nullptr};
  seissol::initializer::Lut* wpLut{nullptr};
  seissol::initializer::LTSTree* drTree{nullptr};
  seissol::initializer::DynamicRupture* drDescr{nullptr};

  FaceToLtsMapType faceToLtsMap{};
  std::vector<std::size_t> globalFaceToLtsMap;
  seissol::geometry::MeshReader* meshReader{nullptr};

  size_t iterationStep{0};
  static constexpr double timeMargin{1.005};
  std::string backupTimeStamp{};

  std::unique_ptr<ReceiverOutput> impl{nullptr};
};
} // namespace dr::output
} // namespace seissol

#endif // SEISSOL_DR_OUTPUT_MANAGER_HPP
