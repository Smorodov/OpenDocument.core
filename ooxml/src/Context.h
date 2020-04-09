#ifndef ODR_OOXML_CONTEXT_H
#define ODR_OOXML_CONTEXT_H

#include <common/TableCursor.h>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <tinyxml2.h>
#include <unordered_map>
#include <vector>

namespace odr {
struct Config;
struct FileMeta;

namespace access {
class Storage;
}
} // namespace odr

namespace odr {
namespace ooxml {

struct Context {
  const Config *config;
  const FileMeta *meta;

  const access::Storage *storage;

  std::ostream *output;

  std::unordered_map<std::string, std::list<std::string>> styleDependencies;
  std::unordered_map<std::string, std::string> relations;
  std::unique_ptr<tinyxml2::XMLDocument> sharedStringsDocument; // xlsx
  std::vector<const tinyxml2::XMLElement *> sharedStrings;      // xlsx

  std::uint32_t currentEntry;
  std::uint32_t currentTableRowStart;
  std::uint32_t currentTableRowEnd;
  std::uint32_t currentTableColStart;
  std::uint32_t currentTableColEnd;
  common::TableCursor tableCursor;
  std::unordered_map<std::uint32_t, std::string> defaultCellStyles;

  // editing
  std::uint32_t currentTextTranslationIndex;
  std::unordered_map<std::uint32_t, const tinyxml2::XMLText *> textTranslation;
};

} // namespace ooxml
} // namespace odr

#endif // ODR_OOXML_CONTEXT_H