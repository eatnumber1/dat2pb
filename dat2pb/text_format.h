#ifndef DAT2PB_TEXT_FORMAT_H_
#define DAT2PB_TEXT_FORMAT_H_

#include <fstream>
#include <ios>

#include "dat2pb/romdat.pb.h"
#include "rhutil/status.h"

namespace dat2pb {

rhutil::StatusOr<RomDat> RomDatFromTextProto(std::istream *input);

}  // namespace dat2pb

#endif  // DAT2PB_CUESHEET_H_
