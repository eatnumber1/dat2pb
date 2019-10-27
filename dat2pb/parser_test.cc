#include "dat2pb/parser.h"

#include <string>
#include <utility>
#include <fstream>
#include <sstream>

#include "dat2pb/text_format.h"
#include "gtest/gtest.h"
#include "rhutil/file.h"
#include "rhutil/status.h"
#include "rhutil/testing/assertions.h"
#include "rhutil/testing/protobuf_assertions.h"

namespace dat2pb {

using ::rhutil::IsOk;
using ::rhutil::IsEqual;
using ::rhutil::StatusOr;
using ::rhutil::OpenInputFile;

namespace {

std::string TestdataToPath(std::string_view filename) {
  return "dat2pb/testdata/" + std::string(filename);
}

std::ifstream OpenTestdataOrDie(std::string_view filename) {
  return OpenInputFile(TestdataToPath(filename)).ValueOrDie();
}

RomDat DatFromProtoFileOrDie(std::string_view filename) {
  std::ifstream istrm = OpenTestdataOrDie(filename);
  auto dat_or = RomDatFromTextProto(&istrm);
  CHECK_OK(dat_or.status());
  return std::move(dat_or).ValueOrDie();
}

RomDat DatFromProtoStringOrDie(std::string_view textproto) {
  std::istringstream istrm{std::string(textproto)};
  auto dat_or = RomDatFromTextProto(&istrm);
  CHECK_OK(dat_or.status());
  return std::move(dat_or).ValueOrDie();
}

StatusOr<RomDat> ParseRomDatFromString(std::string_view s) {
  std::istringstream istrm{std::string(s)};
  return ParseRomDat(&istrm);
}

struct DatProtoFiles {
  std::string dat;
  std::string proto;
};

class DatEqualsProtoFilesTest :
    public testing::TestWithParam<DatProtoFiles> {};

struct DatProtoSample {
  DatProtoSample(std::string_view textproto, std::string dat)
      : dat(std::move(dat)) {
    expected = DatFromProtoStringOrDie(textproto);
  }

  RomDat expected;
  std::string dat;
};

class DatEqualsProtoTest :
  public testing::TestWithParam<DatProtoSample> {};

TEST_P(DatEqualsProtoFilesTest, MatchFiles) {
  auto files = GetParam();

  RomDat expected = DatFromProtoFileOrDie(files.proto);

  std::ifstream istrm = OpenTestdataOrDie(files.dat);

  auto found_or = ParseRomDat(&istrm);
  ASSERT_TRUE(IsOk(found_or));

  EXPECT_TRUE(IsEqual(expected, found_or.ValueOrDie()));
}

TEST_P(DatEqualsProtoTest, MatchSample) {
  const auto &p = GetParam();
  const RomDat &expected = p.expected;
  const std::string &dat = p.dat;

  auto found_or = ParseRomDatFromString(dat);
  ASSERT_TRUE(IsOk(found_or));

  EXPECT_TRUE(IsEqual(expected, found_or.ValueOrDie()));
}

INSTANTIATE_TEST_SUITE_P(
    Examples,
    DatEqualsProtoFilesTest,
    testing::Values(
        DatProtoFiles{"nointro_supergrafx.dat", "nointro_supergrafx.textproto"},
        DatProtoFiles{"redump_pce_cd.dat", "redump_pce_cd.textproto"},
        DatProtoFiles{"redump_ps2.dat", "redump_ps2.textproto"}
    ));

}  // namespace
}  // namespace dat2pb
