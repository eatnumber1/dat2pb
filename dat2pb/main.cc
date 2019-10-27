#include <fstream>
#include <string>
#include <iostream>
#include <sysexits.h>
#include <cstdlib>
#include <stdlib.h>
#include <ios>

#include "rhutil/file.h"
#include "rhutil/status.h"
#include "dat2pb/parser.h"
#include "dat2pb/romdat.pb.h"
#include "absl/types/span.h"
#include "absl/strings/string_view.h"
#include "absl/strings/str_cat.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/flags/usage_config.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

ABSL_FLAG(bool, textformat, false, "Use the text protobuf format");

namespace dat2pb {

using ::google::protobuf::TextFormat;
using ::google::protobuf::io::OstreamOutputStream;
using ::rhutil::Status;
using ::rhutil::OkStatus;
using ::rhutil::UnknownError;
using ::rhutil::InvalidArgumentError;
using ::rhutil::OpenInputFile;

Status DatToProto(absl::string_view datfile) {
  bool textformat = absl::GetFlag(FLAGS_textformat);

  auto mode = std::ios::in;
  if (!textformat) {
    mode |= std::ios::binary;
  }

  ASSIGN_OR_RETURN(std::ifstream istrm, OpenInputFile(datfile, mode));
  ASSIGN_OR_RETURN(RomDat dat, ParseRomDat(&istrm));

  if (textformat) {
    OstreamOutputStream cout_os(&std::cout);
    if (!TextFormat::Print(dat, &cout_os)) {
      return UnknownError("Failed to print dat");
    }
  } else {
    if (!dat.SerializeToOstream(&std::cout)) {
      return UnknownError("Failed to serialize binary proto");
    }
  }

  return OkStatus();
}

Status Main(absl::Span<absl::string_view> args) {
  if (args.size() != 1) {
    return InvalidArgumentError("No datfile specified or too many arguments");
  }

  return DatToProto(args[0]);
}

}  // namespace dat2pb

int main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage(
      absl::StrCat(
        "Convert ROM dats to/from protobufs.\n",
        "Usage: ", getprogname(), " [flags] datfile"));
  absl::FlagsUsageConfig config;
  config.contains_help_flags = [](absl::string_view path) {
    return path == "dat2pb/main.cc";
  };
  absl::SetFlagsUsageConfig(std::move(config));
  std::vector<char*> cargs = absl::ParseCommandLine(argc, argv);
  std::vector<absl::string_view> args(cargs.begin(), cargs.end());

  using ::rhutil::Status;
  if (Status err = dat2pb::Main(absl::MakeSpan(args).subspan(1)); !err.ok()) {
    if (err.code() == rhutil::StatusCode::kInvalidArgument) {
      std::cerr << absl::ProgramUsageMessage() << std::endl << std::endl;
    }
    std::cerr << err << std::endl;
    return static_cast<int>(err.code());
  }
  return EXIT_SUCCESS;
}
