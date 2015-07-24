/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2013-2014 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Jeff Thompson <jefft0@remap.ucla.edu>
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 * @author Zhenkai Zhu <http://irl.cs.ucla.edu/~zhenkai/>
 */

#include "name.hpp"

#include "util/time.hpp"
#include "util/string-helper.hpp"
#include "util/concepts.hpp"
#include "encoding/block.hpp"
#include "encoding/encoding-buffer.hpp"

#include <boost/functional/hash.hpp>

namespace ndn {

BOOST_CONCEPT_ASSERT((boost::EqualityComparable<Name>));
BOOST_CONCEPT_ASSERT((WireEncodable<Name>));
BOOST_CONCEPT_ASSERT((WireDecodable<Name>));
static_assert(std::is_base_of<tlv::Error, Name::Error>::value,
              "Name::Error must inherit from tlv::Error");

template<bool T>
size_t
Name::wireEncode(EncodingImpl<T>& encoder) const
{
  size_t totalLength = 0;

  for (const_reverse_iterator i = rbegin(); i != rend(); ++i)
    {
      totalLength += i->wireEncode(encoder);
    }

  totalLength += encoder.prependVarNumber(totalLength);
  totalLength += encoder.prependVarNumber(tlv::Name);
  return totalLength;
}

template size_t
Name::wireEncode<true>(EncodingImpl<true>& estimator) const;

template size_t
Name::wireEncode<false>(EncodingImpl<false>& encoder) const;

const Block&
Name::wireEncode() const
{
  if (m_nameBlock.hasWire())
    return m_nameBlock;

  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  m_nameBlock = buffer.block();
  m_nameBlock.parse();

  return m_nameBlock;
}

void
Name::wireDecode(const Block& wire)
{
  if (wire.type() != tlv::Name)
    throw tlv::Error("Unexpected TLV type when decoding Name");

  m_nameBlock = wire;
  m_nameBlock.parse();
}

void
Name::set(const char* uriOrig)
{
  clear();

  std::string uri = uriOrig;
  trim(uri);
  if (uri.size() == 0)
    return;

  size_t iColon = uri.find(':');
  if (iColon != std::string::npos) {
    // Make sure the colon came before a '/'.
    size_t iFirstSlash = uri.find('/');
    if (iFirstSlash == std::string::npos || iColon < iFirstSlash) {
      // Omit the leading protocol such as ndn:
      uri.erase(0, iColon + 1);
      trim(uri);
    }
  }

  // Trim the leading slash and possibly the authority.
  if (uri[0] == '/') {
    if (uri.size() >= 2 && uri[1] == '/') {
      // Strip the authority following "//".
      size_t iAfterAuthority = uri.find('/', 2);
      if (iAfterAuthority == std::string::npos)
        // Unusual case: there was only an authority.
        return;
      else {
        uri.erase(0, iAfterAuthority + 1);
        trim(uri);
      }
    }
    else {
      uri.erase(0, 1);
      trim(uri);
    }
  }

  size_t iComponentStart = 0;

  // Unescape the components.
  while (iComponentStart < uri.size()) {
    size_t iComponentEnd = uri.find("/", iComponentStart);
    if (iComponentEnd == std::string::npos)
      iComponentEnd = uri.size();

    Component component = Component::fromEscapedString(&uri[0], iComponentStart, iComponentEnd);
    // Ignore illegal components.  This also gets rid of a trailing '/'.
    if (!component.empty())
      append(Component(component));

    iComponentStart = iComponentEnd + 1;
  }
}

std::string
Name::toUri() const
{
  std::ostringstream os;
  os << *this;
  return os.str();
}

Name&
Name::append(const Name& name)
{
  if (&name == this)
    // Copying from this name, so need to make a copy first.
    return append(Name(name));

  for (size_t i = 0; i < name.size(); ++i)
    append(name.at(i));

  return *this;
}

Name&
Name::appendNumber(uint64_t number)
{
  m_nameBlock.push_back(Component::fromNumber(number));
  return *this;
}

Name&
Name::appendNumberWithMarker(uint8_t marker, uint64_t number)
{
  m_nameBlock.push_back(Component::fromNumberWithMarker(marker, number));
  return *this;
}

Name&
Name::appendVersion(uint64_t version)
{
  m_nameBlock.push_back(Component::fromVersion(version));
  return *this;
}

Name&
Name::appendVersion()
{
  appendVersion(time::toUnixTimestamp(time::system_clock::now()).count());
  return *this;
}

Name&
Name::appendSegment(uint64_t segmentNo)
{
  m_nameBlock.push_back(Component::fromSegment(segmentNo));
  return *this;
}

Name&
Name::appendSegmentOffset(uint64_t offset)
{
  m_nameBlock.push_back(Component::fromSegmentOffset(offset));
  return *this;
}

Name&
Name::appendTimestamp(const time::system_clock::TimePoint& timePoint)
{
  m_nameBlock.push_back(Component::fromTimestamp(timePoint));
  return *this;
}

Name&
Name::appendSequenceNumber(uint64_t seqNo)
{
  m_nameBlock.push_back(Component::fromSequenceNumber(seqNo));
  return *this;
}

Name&
Name::appendImplicitSha256Digest(const ConstBufferPtr& digest)
{
  m_nameBlock.push_back(Component::fromImplicitSha256Digest(digest));
  return *this;
}

Name&
Name::appendImplicitSha256Digest(const uint8_t* digest, size_t digestSize)
{
  m_nameBlock.push_back(Component::fromImplicitSha256Digest(digest, digestSize));
  return *this;
}

Name
Name::getSubName(size_t iStartComponent, size_t nComponents) const
{
  Name result;

  size_t iEnd = iStartComponent + nComponents;
  for (size_t i = iStartComponent; i < iEnd && i < size(); ++i)
    result.append(at(i));

  return result;
}

Name
Name::getSubName(size_t iStartComponent) const
{
  Name result;

  for (size_t i = iStartComponent; i < size(); ++i)
    result.append(at(i));

  return result;
}

Name
Name::getSuccessor() const
{
  if (empty()) {
    static uint8_t firstValue[] = { 0 };
    Name firstName;
    firstName.append(firstValue, 1);
    return firstName;
  }

  return getPrefix(-1).append(get(-1).getSuccessor());
}

bool
Name::equals(const Name& name) const
{
  if (size() != name.size())
    return false;

  for (size_t i = 0; i < size(); ++i) {
    if (at(i) != name.at(i))
      return false;
  }

  return true;
}

bool
Name::isPrefixOf(const Name& name) const
{
  // This name is longer than the name we are checking against.
  if (size() > name.size())
    return false;

  // Check if at least one of given components doesn't match.
  for (size_t i = 0; i < size(); ++i) {
    if (at(i) != name.at(i))
      return false;
  }

  return true;
}


int
Name::compare(const Name& other) const
{
  for (size_t i = 0; i < size() && i < other.size(); ++i) {
    int comparison = at(i).compare(other.at(i));
    if (comparison == 0)
      // The components at this index are equal, so check the next components.
      continue;

    // Otherwise, the result is based on the components at this index.
    return comparison;
  }

  // The components up to min(this.size(), other.size()) are equal, so the shorter name is less.
  if (size() < other.size())
    return -1;
  else if (size() > other.size())
    return 1;
  else
    return 0;
}

std::ostream&
operator<<(std::ostream& os, const Name& name)
{
  if (name.empty())
    {
      os << "/";
    }
  else
    {
      for (Name::const_iterator i = name.begin(); i != name.end(); i++) {
        os << "/";
        i->toUri(os);
      }
    }
  return os;
}

std::istream&
operator>>(std::istream& is, Name& name)
{
  std::string inputString;
  is >> inputString;
  name.set(inputString);

  return is;
}

} // namespace ndn

namespace std {
size_t
hash<ndn::Name>::operator()(const ndn::Name& name) const
{
  return boost::hash_range(name.wireEncode().wire(),
                           name.wireEncode().wire() + name.wireEncode().size());
}

} // namespace std