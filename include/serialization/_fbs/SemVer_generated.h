// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_SEMVER_OPENSHOCK_SERIALIZATION_TYPES_H_
#define FLATBUFFERS_GENERATED_SEMVER_OPENSHOCK_SERIALIZATION_TYPES_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 24 &&
              FLATBUFFERS_VERSION_MINOR == 3 &&
              FLATBUFFERS_VERSION_REVISION == 25,
             "Non-compatible flatbuffers version included");

namespace OpenShock {
namespace Serialization {
namespace Types {

struct SemVer;
struct SemVerBuilder;

struct SemVer FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef SemVerBuilder Builder;
  struct Traits;
  static FLATBUFFERS_CONSTEXPR_CPP11 const char *GetFullyQualifiedName() {
    return "OpenShock.Serialization.Types.SemVer";
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_MAJOR = 4,
    VT_MINOR = 6,
    VT_PATCH = 8,
    VT_PRERELEASE = 10,
    VT_BUILD = 12
  };
  uint16_t major() const {
    return GetField<uint16_t>(VT_MAJOR, 0);
  }
  uint16_t minor() const {
    return GetField<uint16_t>(VT_MINOR, 0);
  }
  uint16_t patch() const {
    return GetField<uint16_t>(VT_PATCH, 0);
  }
  const ::flatbuffers::String *prerelease() const {
    return GetPointer<const ::flatbuffers::String *>(VT_PRERELEASE);
  }
  const ::flatbuffers::String *build() const {
    return GetPointer<const ::flatbuffers::String *>(VT_BUILD);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint16_t>(verifier, VT_MAJOR, 2) &&
           VerifyField<uint16_t>(verifier, VT_MINOR, 2) &&
           VerifyField<uint16_t>(verifier, VT_PATCH, 2) &&
           VerifyOffset(verifier, VT_PRERELEASE) &&
           verifier.VerifyString(prerelease()) &&
           VerifyOffset(verifier, VT_BUILD) &&
           verifier.VerifyString(build()) &&
           verifier.EndTable();
  }
};

struct SemVerBuilder {
  typedef SemVer Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_major(uint16_t major) {
    fbb_.AddElement<uint16_t>(SemVer::VT_MAJOR, major, 0);
  }
  void add_minor(uint16_t minor) {
    fbb_.AddElement<uint16_t>(SemVer::VT_MINOR, minor, 0);
  }
  void add_patch(uint16_t patch) {
    fbb_.AddElement<uint16_t>(SemVer::VT_PATCH, patch, 0);
  }
  void add_prerelease(::flatbuffers::Offset<::flatbuffers::String> prerelease) {
    fbb_.AddOffset(SemVer::VT_PRERELEASE, prerelease);
  }
  void add_build(::flatbuffers::Offset<::flatbuffers::String> build) {
    fbb_.AddOffset(SemVer::VT_BUILD, build);
  }
  explicit SemVerBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<SemVer> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<SemVer>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<SemVer> CreateSemVer(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    uint16_t major = 0,
    uint16_t minor = 0,
    uint16_t patch = 0,
    ::flatbuffers::Offset<::flatbuffers::String> prerelease = 0,
    ::flatbuffers::Offset<::flatbuffers::String> build = 0) {
  SemVerBuilder builder_(_fbb);
  builder_.add_build(build);
  builder_.add_prerelease(prerelease);
  builder_.add_patch(patch);
  builder_.add_minor(minor);
  builder_.add_major(major);
  return builder_.Finish();
}

struct SemVer::Traits {
  using type = SemVer;
  static auto constexpr Create = CreateSemVer;
};

inline ::flatbuffers::Offset<SemVer> CreateSemVerDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    uint16_t major = 0,
    uint16_t minor = 0,
    uint16_t patch = 0,
    const char *prerelease = nullptr,
    const char *build = nullptr) {
  auto prerelease__ = prerelease ? _fbb.CreateString(prerelease) : 0;
  auto build__ = build ? _fbb.CreateString(build) : 0;
  return OpenShock::Serialization::Types::CreateSemVer(
      _fbb,
      major,
      minor,
      patch,
      prerelease__,
      build__);
}

}  // namespace Types
}  // namespace Serialization
}  // namespace OpenShock

#endif  // FLATBUFFERS_GENERATED_SEMVER_OPENSHOCK_SERIALIZATION_TYPES_H_
