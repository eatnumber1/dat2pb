#include "dat2pb/text_format.h"

#include <utility>

#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/io/tokenizer.h"
#include "absl/strings/str_format.h"

namespace dat2pb {

using ::rhutil::Status;
using ::rhutil::StatusOr;
using ::rhutil::StatusCode;
using ::google::protobuf::io::IstreamInputStream;
using ::google::protobuf::TextFormat;

class StatusCollector : public ::google::protobuf::io::ErrorCollector {
 public:
  struct Options {
    bool include_warnings = true;
    StatusCode code = StatusCode::kInvalidArgument;
  };

  StatusCollector(Options options)
    : options_(std::move(options))
    {}

  void AddError(
      int line, google::protobuf::io::ColumnNumber column,
      const std::string &message) override {
    line++, column++;  // line and column numbers come in zero-indexed
    status_.Update({options_.code, absl::StrFormat("Error at line %d:%d: %s",
                                                   line, column, message)});
  }

  void AddWarning(
      int line, google::protobuf::io::ColumnNumber column,
      const std::string &message) override {
    line++, column++;  // line and column numbers come in zero-indexed

    if (!options_.include_warnings) return;

    status_.Update({options_.code, absl::StrFormat("Warning at line %d:%d: %s",
                                                   line, column, message)});
  }

  Status status() const {
    return status_;
  }

 private:
  Options options_;
  Status status_;
};

StatusOr<RomDat> RomDatFromTextProto(std::istream *input) {
  IstreamInputStream istrm(input);
  StatusCollector collector(/*options=*/{});
  TextFormat::Parser parser;
  RomDat dat;

  parser.RecordErrorsTo(&collector);
  if (!parser.Parse(&istrm, &dat)) {
    return collector.status();
  }

  return std::move(dat);
}

}  // namespace dat2pb
