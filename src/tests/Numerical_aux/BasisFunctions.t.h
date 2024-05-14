#include "doctest.h"
#include "Numerical_aux/BasisFunction.h"
#include "Kernels/precision.hpp"
#include "generated_code/init.h"

namespace seissol::unit_test {
TEST_CASE("Sampled Basis Functions") {
  constexpr double epsilon = 10 * std::numeric_limits<real>::epsilon();
  std::vector<real> precomputedValues = {
      1.0, 0.19999999999999998, 0.20000000000000007, 0.20000000000000018, -0.019999999999999997,
      0.16000000000000003, -0.29, 0.16000000000000014, 0.1600000000000002, -0.6500000000000001,
      -0.027999999999999987, -0.02799999999999999, -0.028, -0.028000000000000025, -0.028, 0.2240000000000001,
      -0.406, -0.13599999999999998, -0.13600000000000004, 0.242, -0.007399999999999997, -0.05599999999999999,
      -0.007399999999999997, -0.056000000000000015, 0.11409999999999997, -0.05599999999999997,
      -0.05599999999999998, -0.056, -0.05600000000000005, 0.006999999999999989, -0.05599999999999993,
      0.10149999999999984, -0.05600000000000014, -0.05600000000000016, 0.34900000000000025,
      0.0009199999999999995, -0.01923999999999999, -0.03471999999999999, 0.008479999999999994,
      0.014779999999999995, -0.008259999999999955, -0.019240000000000004, -0.14560000000000006,
      -0.019240000000000004, -0.14560000000000012, 0.2966600000000001, -0.009520000000000042,
      -0.009520000000000044, -0.009520000000000048, -0.009520000000000056, 0.018200000000000008,
      -0.14560000000000012, 0.26390000000000013, 0.13790000000000005, 0.1379000000000001, -0.44044
  };

  basisFunction::SampledBasisFunctions<real> sampledBasisFunctions(6, 0.3, 0.3, 0.3);
  for (size_t i = 0; i < precomputedValues.size(); ++i) {
    REQUIRE(sampledBasisFunctions.m_data.at(i) == AbsApprox(precomputedValues.at(i)).epsilon(epsilon));

  }
}

TEST_CASE("Sampled Derivatives Functions") {
  constexpr double epsilon = 100 * std::numeric_limits<real>::epsilon();

  std::array<std::vector<real>, 3> precomputedValues = {{
      {
        0.0, 1.9999999999999996, 0.0, 0.0, 1.1999999999999997, 1.6000000000000003, 0.0, 1.6000000000000012, 
        0.0, -0.0, 0.11999999999999994, 1.6799999999999993, -0.28, 0.0, 1.68, 2.2400000000000007, 0.0, 
        -1.3599999999999997, 0.0, 0.0, -0.1999999999999999, 0.23999999999999994, 0.4439999999999998, 
        -0.5600000000000002, 0.0, 0.23999999999999988, 3.3599999999999985, -0.56, -0.0, -0.41999999999999926,
        -0.5599999999999993, 0.0, -0.5600000000000014, -0.0, 0.0, -0.11399999999999993, -0.5199999999999998, 
        0.14879999999999993, -0.5087999999999997, 0.14779999999999996, 0.0, -0.5200000000000001, 
        0.6240000000000002, 1.1544000000000003, -1.4560000000000013, 0.0, 0.04080000000000018, 0.5712000000000026,
        -0.09520000000000048, -0.0, -1.0920000000000005, -1.4560000000000013, 0.0, 1.3790000000000004, 0.0, 0.0
      }, {
        0.0, 0.9999999999999998, 2.9999999999999996, 0.0, 0.9999999999999999, 1.8000000000000003, 0.400000000000001,
        0.8000000000000007, 2.400000000000002, -0.0, 0.29999999999999993, 1.2599999999999998, 0.7, -2.1,
        1.4000000000000004, 2.5200000000000005, 0.5600000000000016, -0.68, -2.04, 0.0, -0.07599999999999998,
        0.34800000000000003, 0.16199999999999987, -0.7419999999999999, 0.08399999999999871, 0.5999999999999999,
        2.5199999999999996, 1.4, -4.2, -0.3499999999999994, -0.6299999999999991, -0.14000000000000018,
        -0.2800000000000007, -0.8400000000000022, 0.0, -0.09699999999999998, -0.27899999999999997, -0.1599999999999999,
        -0.4456, -0.4364999999999998, 1.2425000000000002, -0.1976000000000001, 0.9048000000000006, 0.42119999999999996,
        -1.9292000000000007, 0.2183999999999968, 0.10200000000000048, 0.42840000000000206, 0.23800000000000116,
        -0.7140000000000036, -0.9100000000000006, -1.6380000000000012, -0.3640000000000012, 0.6895000000000002,
        2.0685000000000007, 0.0
      }, {
        0.0, 0.9999999999999999, 0.9999999999999998, 4.0, 0.9999999999999999, 1.0, 1.0000000000000004,
        2.0000000000000004, 2.000000000000001, -0.9999999999999964, 0.2999999999999999, 1.3799999999999997, 0.2999999999999999,
        -0.7799999999999999, 1.2400000000000002, 2.6800000000000006, -0.9199999999999988, -0.11999999999999966,
        -0.11999999999999922, -4.680000000000003, -0.07600000000000001, 0.5719999999999998, 0.3019999999999999,
        -0.238, -0.6160000000000003, 0.3199999999999999, 2.4799999999999995, 0.3199999999999998, -1.84,
        -0.5299999999999994, 1.0900000000000005, -2.959999999999999, -1.9600000000000006, -1.960000000000001,
        4.759999999999996, -0.09699999999999998, -0.205, 0.24319999999999992, -0.46959999999999985, -0.22929999999999987,
        0.5914999999999998, -0.2864000000000001, 0.8152000000000008, 0.6964000000000002, -1.2908000000000004,
        -0.23240000000000216, -0.3907999999999997, -0.023599999999998067, -0.3908, -0.7580000000000022, -0.7180000000000005,
        -2.446000000000001, 1.8739999999999992, 0.9414999999999983, 0.9414999999999984, 1.876000000000009
      }
  }};

  basisFunction::SampledBasisFunctionDerivatives<real> sampledBasisFunctionDerivatives(6, 0.3, 0.3, 0.3);
  auto dataView = init::basisFunctionDerivativesAtPoint::view::create(sampledBasisFunctionDerivatives.m_data.data());
  for (size_t i = 0; i < precomputedValues[0].size(); ++i) {
    for (size_t direction = 0; direction < 3; ++direction) {
      REQUIRE(dataView(i, direction) == AbsApprox(precomputedValues[direction].at(i)).epsilon(epsilon));
    }
  }
}
}
