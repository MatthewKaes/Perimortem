// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/language/persistence/profile.hpp"
#include "tetrodotoxin/linker/manifest.hpp"
#include "tetrodotoxin/package/archive/artifact.hpp"
#include "tetrodotoxin/package/archive/export.hpp"
#include "tetrodotoxin/package/archive/member.hpp"
#include "tetrodotoxin/package/language/dependency.hpp"

namespace Tetrodotoxin::Package::Archive {

// The source free Package terminal is a value over stable views. Archive owns
// no backing storage and applies no Format 2 policy. Reader retains decoded
// record ranges in its caller Arena while the input owner retains their byte
// views. Other producers keep every supplied view valid for the complete use
// of the Archive.
class Archive {
 public:
  // Defines the complete Format 2 section vocabulary shared by Reader and
  // Writer. The closed set fits in one byte and is widened into the existing
  // unsigned 16 bit tag when encoded.
  enum class Sections : U8 {
    Identity = 1,
    Version,
    Dependencies,
    Members,
    ArtifactIds,
    Exports,
    ArtifactMetadata,
  };

  // Counts the fixed magic, format, flags, and body size prefix. Reader
  // requires the first section to begin here and Writer adds the same prefix
  // to its measured body.
  static constexpr Count header_size = 12;

  // Creates one complete Package from views retained by its producer.
  // Construction copies only the views and preserves their supplied order.
  constexpr Archive(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Perimortem::Core::View::Vector<Language::Dependency> dependencies,
      Perimortem::Core::View::Vector<Member> members,
      Perimortem::Core::View::Vector<Artifact> artifacts,
      Perimortem::Core::View::Vector<Export> exports,
      Tetrodotoxin::Language::Persistence::Profile profile =
          Tetrodotoxin::Language::Persistence::Profile::Complete)
      : identity(identity),
        version(version),
        dependencies(dependencies),
        members(members),
        artifacts(artifacts),
        exports(exports),
        profile(profile) {};

  constexpr auto get_identity() const -> Perimortem::Core::View::Bytes {
    return identity;
  }

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

  constexpr auto get_dependencies() const
      -> Perimortem::Core::View::Vector<Language::Dependency> {
    return dependencies;
  }

  constexpr auto get_members() const -> Perimortem::Core::View::Vector<Member> {
    return members;
  }

  constexpr auto get_artifacts() const
      -> Perimortem::Core::View::Vector<Artifact> {
    return artifacts;
  }

  constexpr auto get_exports() const -> Perimortem::Core::View::Vector<Export> {
    return exports;
  }

  constexpr auto get_profile() const
      -> Tetrodotoxin::Language::Persistence::Profile {
    return profile;
  }

  // Matches one physical Manifest against the complete native agreement stored
  // by this semantic Archive. The requested artifact remains the caller's
  // selection while this operation proves that the two products belong
  // together.
  auto matches(const Tetrodotoxin::Linker::Manifest& manifest) const -> Bool;

 private:
  Perimortem::Core::View::Bytes identity;
  Perimortem::System::Version version;
  Perimortem::Core::View::Vector<Language::Dependency> dependencies;
  Perimortem::Core::View::Vector<Member> members;
  Perimortem::Core::View::Vector<Artifact> artifacts;
  Perimortem::Core::View::Vector<Export> exports;
  Tetrodotoxin::Language::Persistence::Profile profile;
};

}  // namespace Tetrodotoxin::Package::Archive
