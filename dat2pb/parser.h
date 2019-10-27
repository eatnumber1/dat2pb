#ifndef DAT2PB_PARSER_H_
#define DAT2PB_PARSER_H_

#include <istream>

#include "dat2pb/romdat.pb.h"
#include "rhutil/status.h"

namespace dat2pb {

rhutil::StatusOr<RomDat> ParseRomDat(std::istream *input);

}  // namespace dat2pb

#endif  // DAT2PB_PARSER_H_
