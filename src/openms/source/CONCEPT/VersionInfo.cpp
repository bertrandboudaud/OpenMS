// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2017.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Chris Bielow $
// $Authors: Clemens Groepl, Chris Bielow $
// --------------------------------------------------------------------------
#include <OpenMS/CONCEPT/VersionInfo.h>
#include <OpenMS/DATASTRUCTURES/String.h>
#include <OpenMS/CONCEPT/Exception.h>

#include <fstream>

// these headers are generated by the build system
// and therefore intentionally break the naming convention (tagging them as automatically build)
#include <OpenMS/openms_package_version.h>

using namespace std;

namespace OpenMS
{
  const VersionInfo::VersionDetails VersionInfo::VersionDetails::EMPTY;

  bool VersionInfo::VersionDetails::operator<(const VersionInfo::VersionDetails & rhs) const
  {
    return (this->version_major  < rhs.version_major)
           || (this->version_major == rhs.version_major && this->version_minor  < rhs.version_minor)
           || (this->version_major == rhs.version_major && this->version_minor == rhs.version_minor && this->version_patch < rhs.version_patch);
  }

  bool VersionInfo::VersionDetails::operator==(const VersionInfo::VersionDetails & rhs) const
  {
    return this->version_major == rhs.version_major &&
      this->version_minor == rhs.version_minor &&
      this->version_patch == rhs.version_patch;
  }

  bool VersionInfo::VersionDetails::operator!=(const VersionInfo::VersionDetails & rhs) const
  {
    return !(this->operator==(rhs));
  }

  bool VersionInfo::VersionDetails::operator>(const VersionInfo::VersionDetails & rhs) const
  {
    return !(*this < rhs || *this == rhs);
  }

  VersionInfo::VersionDetails VersionInfo::VersionDetails::create(const String & version) //static
  {
    VersionInfo::VersionDetails result;

    size_t first_dot = version.find('.');
    // we demand at least one "."
    if (first_dot == string::npos)
      return VersionInfo::VersionDetails::EMPTY;

    try
    {
      result.version_major = String(version.substr(0, first_dot)).toInt();
    }
    catch (Exception::ConversionError & /*e*/)
    {
      return VersionInfo::VersionDetails::EMPTY;
    }

    // returns npos if no second "." is found - which does not hurt
    size_t second_dot = version.find('.', first_dot + 1);
    try
    {
      result.version_minor = String(version.substr(first_dot + 1, second_dot - (first_dot + 1))).toInt();
    }
    catch (Exception::ConversionError & /*e*/)
    {
      return VersionInfo::VersionDetails::EMPTY;
    }

    // if there is no second dot: return
    if (second_dot == string::npos)
      return result;

    // returns npos if no third "." is found - which does not hurt
    size_t third_dot = version.find('.', second_dot + 1);
    try
    {
      result.version_patch = String(version.substr(second_dot + 1, third_dot - (second_dot + 1))).toInt();
    }
    catch (Exception::ConversionError & /*e*/)
    {
      return VersionInfo::VersionDetails::EMPTY;
    }

    return result;
  }

  String VersionInfo::getTime()
  {
    static bool is_initialized = false;
    static String result;
    if (!is_initialized)
    {
      result = String(__DATE__) + ", " + __TIME__;
      is_initialized = true;
    }
    return result;
  }

  String VersionInfo::getVersion()
  {
    static bool is_initialized = false;
    static String result;
    if (!is_initialized)
    {
      result = OPENMS_PACKAGE_VERSION;
      result.trim();
      is_initialized = true;
    }
    return result;
  }

  VersionInfo::VersionDetails VersionInfo::getVersionStruct()
  {
    static bool is_initialized = false;
    static VersionDetails result;
    if (!is_initialized)
    {
      result = VersionDetails::create(getVersion());
      is_initialized = true;
    }
    return result;
  }

  String VersionInfo::getRevision()
  {
    return String(OPENMS_GIT_SHA1);
  }

  String VersionInfo::getBranch()
  {
    return String(OPENMS_GIT_BRANCH);
  }

}
