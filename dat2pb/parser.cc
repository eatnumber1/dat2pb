#include "dat2pb/parser.h"

#include <string>
#include <string_view>
#include <utility>
#include <cstdint>
#include <cstdlib>

#include "absl/strings/ascii.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "rhutil/status.h"
#include "libxml/parser.h"

namespace dat2pb {

using ::rhutil::StatusOr;
using ::rhutil::Status;
using ::rhutil::OkStatus;
using ::rhutil::InvalidArgumentError;
using ::rhutil::InvalidArgumentErrorBuilder;
using ::rhutil::UnknownError;
using ::rhutil::UnknownErrorBuilder;

namespace {

class XMLDocDeleter {
 public:
  void operator()(xmlDoc *doc) {
    xmlFreeDoc(doc);
  };
};

template <typename T>
class XMLFreeDeleter {
 public:
  void operator()(T *ptr) {
    xmlFree(ptr);
  };
};

class XMLString : public std::unique_ptr<char, XMLFreeDeleter<char>> {
 public:
   using std::unique_ptr<char, XMLFreeDeleter<char>>::unique_ptr;
   explicit XMLString(xmlChar *s) : std::unique_ptr<char, XMLFreeDeleter<char>>(reinterpret_cast<char*>(s)) {}
};

class XMLStringView : public std::string_view {
 public:
  using std::string_view::string_view;

  XMLStringView(xmlChar *s) : std::string_view(reinterpret_cast<char*>(s)) {}
  XMLStringView(const xmlChar *s) : std::string_view(reinterpret_cast<const char*>(s)) {}
};

const xmlChar *operator ""_xml(const char *s, std::size_t len) {
  return reinterpret_cast<const xmlChar *>(s);
}

class XMLErrorCollector {
 public:
  static XMLErrorCollector *Instance() {
    static XMLErrorCollector *collector = []() {
      return new XMLErrorCollector();
    }();
    return collector;
  }

  Status LastError() {
    return last_error_;
  }

 private:
  friend void InitXML();

  XMLErrorCollector() = default;

  void HandleError(xmlError *error) {
    // Ignore unresolved entity warnings.
    if (error->domain == XML_FROM_PARSER &&
        error->code == XML_WAR_UNDECLARED_ENTITY) {
      return;
    }

    auto b = UnknownErrorBuilder();
    b << "Error at " << error->line << ":" << error->int2
      << ": " << absl::StripAsciiWhitespace(error->message) << " "
      << absl::StrFormat("(err %d:%d)", error->domain, error->code);
    last_error_ = std::move(b);
  }

  static void HandleError(void *ctx, xmlError *error) {
    reinterpret_cast<XMLErrorCollector*>(ctx)->HandleError(error);
  }

  static thread_local Status last_error_;
};
thread_local Status XMLErrorCollector::last_error_;

void InitXML() {
  static bool initialized = []() {
    xmlInitParser();
    xmlSetStructuredErrorFunc(XMLErrorCollector::Instance(),
                              &XMLErrorCollector::HandleError);
    return true;
  }();
  (void)initialized;
}

Status LastXMLErrorStatus() {
  return XMLErrorCollector::Instance()->LastError();
}

Status ParseHeader(xmlDoc *doc, const xmlNode &node, RomDat::Header *header) {
  for (xmlNode *child = node.xmlChildrenNode; child != nullptr;
       child = child->next) {
    XMLStringView name(child->name);
    XMLString body(xmlNodeListGetString(doc, child->xmlChildrenNode, /*inLine=*/true));
    if (name == "name") {
      header->set_name(body.get());
    } else if (name == "date") {
      header->set_date(body.get());
    } else if (name == "version") {
      header->set_version(body.get());
    } else if (name == "description") {
      header->set_description(body.get());
    } else if (name == "author") {
      header->set_author(body.get());
    } else if (name == "homepage") {
      header->set_homepage(body.get());
    } else if (name == "url") {
      header->set_url(body.get());
    } else {
      return InvalidArgumentErrorBuilder()
          << "Unknown node type " << name;
    }
  }
  return OkStatus();
}

Status ParseRom(xmlDoc *doc, const xmlNode &node, RomDat::Game::Rom *rom) {
  XMLString name(xmlGetProp(&node, "name"_xml));
  XMLString size_str(xmlGetProp(&node, "size"_xml));
  XMLString crc(xmlGetProp(&node, "crc"_xml));
  XMLString md5(xmlGetProp(&node, "md5"_xml));
  XMLString sha1(xmlGetProp(&node, "sha1"_xml));
  if (!name || !size_str || !crc || !md5 || !sha1) {
    return InvalidArgumentError("Missing data in rom");
  }

  int64_t size;
  if (!absl::SimpleAtoi(size_str.get(), &size)) {
    return InvalidArgumentErrorBuilder() << "Invalid size: " << size;
  }

  rom->set_name(name.get());
  rom->set_size(size);
  rom->set_crc(absl::AsciiStrToLower(crc.get()));
  rom->set_md5(absl::AsciiStrToLower(md5.get()));
  rom->set_sha1(absl::AsciiStrToLower(sha1.get()));

  return OkStatus();
}

Status ParseGame(xmlDoc *doc, const xmlNode &node, RomDat::Game *game) {
  XMLString game_name(xmlGetProp(&node, "name"_xml));
  game->set_name(game_name.get());

  for (xmlNode *child = node.xmlChildrenNode; child != nullptr;
       child = child->next) {
    XMLStringView name(child->name);
    if (name == "rom") {
      RETURN_IF_ERROR(ParseRom(doc, *child, game->add_rom()))
          << "Failed to parse rom";
    } else if (name == "category") {
      XMLString category(xmlNodeListGetString(doc, child->xmlChildrenNode,
                                              /*inLine=*/true));
      game->set_category(category.get());
    } else if (name == "serial") {
      XMLString serial(xmlNodeListGetString(doc, child->xmlChildrenNode,
                                            /*inLine=*/true));
      game->set_serial(serial.get());
    } else if (name == "version") {
      XMLString version(xmlNodeListGetString(doc, child->xmlChildrenNode,
                                                 /*inLine=*/true));
      game->set_version(version.get());
    } else if (name == "description") {
      XMLString description(xmlNodeListGetString(doc, child->xmlChildrenNode,
                                                 /*inLine=*/true));
      game->set_description(description.get());
    } else {
      return InvalidArgumentErrorBuilder()
          << "Unknown node type " << name;
    }
  }
  return OkStatus();
}

}  // namespace

StatusOr<RomDat> ParseRomDat(std::istream *input) {
  InitXML();

  RomDat dat;

  xmlInputReadCallback xml_read = [](void *ctx, char *buffer, int len) -> int {
    std::istream *input = reinterpret_cast<std::istream*>(ctx);
    input->read(buffer, len);
    if (!input->eof() && input->fail()) return -1;
    return input->gcount();
  };
  xmlInputCloseCallback xml_close = [](void *ctx) -> int {
    // std::istream cannot be closed explicitly
    return 0;
  };
  std::unique_ptr<xmlDoc, XMLDocDeleter> doc{
    xmlReadIO(
      xml_read, xml_close, input, "noname.xml", /*encoding=*/nullptr,
      /*options=*/(XML_PARSE_NONET | XML_PARSE_COMPACT | XML_PARSE_NOBLANKS |
                   XML_PARSE_NOENT | XML_PARSE_NOCDATA))};
  RETURN_IF_ERROR(LastXMLErrorStatus());

  // TODO(eatnumber1): Suppress warnings about undefined entities.

  xmlNode *node = xmlDocGetRootElement(doc.get());
  RETURN_IF_ERROR(LastXMLErrorStatus());
  if (!node) return InvalidArgumentError("Empty ROM dat file");

  if (XMLStringView(node->name) != "datafile") {
    return InvalidArgumentErrorBuilder()
        << "ROM dat file needs to start with a 'datafile' node. Instead it was "
        << node->name;
  }

  for (xmlNode *child = node->xmlChildrenNode; child != nullptr;
       child = child->next) {
    XMLStringView name(child->name);
    if (name == "header") {
      RETURN_IF_ERROR(ParseHeader(doc.get(), *child, dat.mutable_header()))
          << "Failed to parse header";
    } else if (name == "game") {
      RETURN_IF_ERROR(ParseGame(doc.get(), *child, dat.add_game()))
          << "Failed to parse game";
    } else {
      return InvalidArgumentErrorBuilder()
          << "Unknown node type '" << name << "' in parent '" << node->name
          << "'";
    }
  }

  return std::move(dat);
}

}  // namespace dat2pb
