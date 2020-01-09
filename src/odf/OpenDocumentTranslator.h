#ifndef ODR_OPENDOCUMENTTRANSLATOR_H
#define ODR_OPENDOCUMENTTRANSLATOR_H

#include <string>
#include <memory>

namespace odr {

class TranslationContext;

class OpenDocumentTranslator final {
public:
    OpenDocumentTranslator();
    ~OpenDocumentTranslator();

    bool translate(const std::string &outPath, TranslationContext &context) const;
    bool backTranslate(const std::string &diff, const std::string &outPath, TranslationContext &context) const;

private:
    class Impl;
    const std::unique_ptr<Impl> impl;
};

}

#endif //ODR_OPENDOCUMENTTRANSLATOR_H
