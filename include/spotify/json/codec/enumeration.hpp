/*
 * Copyright (c) 2015 Spotify AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#pragma once

#include <algorithm>
#include <stdexcept>
#include <vector>

#include <spotify/json/decoding_context.hpp>
#include <spotify/json/default_codec.hpp>
#include <spotify/json/detail/decoding_helpers.hpp>
#include <spotify/json/detail/writer.hpp>

namespace spotify {
namespace json {
namespace codec {

/**
 * Codec that maps values from a set of JSON values to values of another C++
 * type. This is useful for enums.
 */
template<typename OuterObject, typename InnerCodec>
class enumeration_t final {
  using InnerType = typename InnerCodec::object_type;
  using Mapping = std::vector<std::pair<OuterObject, InnerType>>;

 public:
  using object_type = OuterObject;

  enumeration_t(InnerCodec inner_codec, Mapping &&mapping)
      : _inner_codec(std::move(inner_codec)),
        _mapping(std::move(mapping)) {}

  void encode(const object_type &value, writer &w) const {
    const auto it = std::find_if(
        _mapping.begin(),
        _mapping.end(),
        [&](const std::pair<OuterObject, InnerType> &pair) {
          return pair.first == value;
        });
    if (it == _mapping.end()) {
      // This should never happen because should_encode should cause this
      // method to never be called with an unknown value.
      throw std::invalid_argument("Encoding unknown enumeration value");
    }
    _inner_codec.encode(it->second, w);
  }

  bool should_encode(const object_type &value) const {
    return std::find_if(
        _mapping.begin(),
        _mapping.end(),
        [&](const std::pair<OuterObject, InnerType> &pair) {
          return pair.first == value;
        }) != _mapping.end();
  }

  object_type decode(decoding_context &context) const {
    const auto result = _inner_codec.decode(context);
    const auto it = std::find_if(
        _mapping.begin(),
        _mapping.end(),
        [&](const std::pair<OuterObject, InnerType> &pair) {
          return pair.second == result;
        });
    detail::fail_if(
        context,
        it == _mapping.end(),
        "Encountered unknown enumeration value");
    return it->first;
  }

 private:
  InnerCodec _inner_codec;
  Mapping _mapping;
};

template<
    typename OuterObject,
    typename InnerCodec>
enumeration_t<OuterObject, InnerCodec> enumeration(
    InnerCodec &&inner_codec,
    std::initializer_list<std::pair<OuterObject, typename InnerCodec::object_type>> objs) {
  std::vector<std::pair<OuterObject, typename InnerCodec::object_type>> mapping;
  for (const auto &obj : objs) {
    mapping.push_back(obj);
  }
  return enumeration_t<OuterObject, InnerCodec>(
      std::forward<InnerCodec>(inner_codec),
      std::move(mapping));
}

template<
    typename OuterObject,
    typename InnerObject>
enumeration_t<OuterObject, decltype(default_codec<InnerObject>())> enumeration(
    std::initializer_list<std::pair<OuterObject, InnerObject>> objs) {
  std::vector<std::pair<OuterObject, InnerObject>> mapping;
  for (const auto &obj : objs) {
    mapping.push_back(obj);
  }
  return enumeration_t<OuterObject, decltype(default_codec<InnerObject>())>(
      default_codec<InnerObject>(),
      std::move(mapping));
}

}  // namespace codec
}  // namespace json
}  // namespace spotify