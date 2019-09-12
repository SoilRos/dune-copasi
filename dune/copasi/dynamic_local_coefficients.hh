#ifndef DUNE_COPASI_DYNAMIC_LOCAL_COEFFICIENTS_HH
#define DUNE_COPASI_DYNAMIC_LOCAL_COEFFICIENTS_HH

#include <dune/localfunctions/common/localkey.hh>

#include <cassert>
#include <map>
#include <type_traits>
#include <utility>
#include <vector>

namespace Dune::Copasi {

template<class Coefficients>
class DynamicPowerLocalCoefficients
{
public:
  DynamicPowerLocalCoefficients(const Coefficients& coefficients,
                                std::size_t power_size)
    : _size(power_size * coefficients.size())
  {
    assert(power_size >= 0);

    // reserve enough space for local keys
    _local_keys.reserve(size());

    // map to store max index for each sub entity and codim.
    std::map<std::pair<unsigned int, unsigned int>, unsigned int> max_index;

    for (std::size_t j = 0; j < power_size; ++j) {
      for (std::size_t i = 0; i < coefficients.size(); ++i) {
        auto local_key = coefficients.localKey(i);
        unsigned int sub_entity = local_key.subEntity();
        unsigned int codim = local_key.codim();

        unsigned int index = local_key.index();
        auto pair = std::make_pair(sub_entity, codim);

        auto it = max_index.find(pair);
        if (it == max_index.end())
          max_index[pair] = index;
        else
          it->second = std::max(max_index[pair], index);

        // assume coeff. indices consecutive and start at 0!
        // calculate the new index.
        unsigned int new_index = (max_index[pair] + 1) * j + index;
        _local_keys.push_back(LocalKey(sub_entity, codim, new_index));
      }
    }

    // ensure that we have the correct number of local keys
    assert(_local_keys.size() == size());
  }

  DynamicPowerLocalCoefficients(const Coefficients& coefficients)
    : DynamicPowerLocalCoefficients(coefficients, 1)
  {}

  template<
    class = std::enable_if_t<std::is_default_constructible_v<Coefficients>>>
  DynamicPowerLocalCoefficients(std::size_t power_size)
    : DynamicPowerLocalCoefficients(Coefficients{}, power_size)
  {}

  template<
    class = std::enable_if_t<std::is_default_constructible_v<Coefficients>>>
  DynamicPowerLocalCoefficients()
    : DynamicPowerLocalCoefficients(1)
  {}

  std::size_t size() const { return _size; }

  const LocalKey& localKey(std::size_t i) const { return _local_keys[i]; }

private:
  const std::size_t _size;
  std::vector<LocalKey> _local_keys;
};

} // namespace Dune::Copasi

#endif // DUNE_COPASI_DYNAMIC_LOCAL_COEFFICIENTS_HH