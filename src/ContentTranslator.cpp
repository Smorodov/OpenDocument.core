#include "ContentTranslator.h"
#include <utility>
#include <string>
#include <list>
#include <unordered_map>
#include "tinyxml2.h"
#include "glog/logging.h"
#include "odr/TranslationConfig.h"
#include "Context.h"

namespace odr {

class ElementTranslator {
public:
    virtual ~ElementTranslator() = default;
    virtual void translateStart(const tinyxml2::XMLElement &in, std::ostream &out, Context &context) const = 0;
    virtual void translateEnd(const tinyxml2::XMLElement &in, std::ostream &out, Context &context) const = 0;
};

class AttributeTranslator {
public:
    virtual ~AttributeTranslator() = default;
    virtual void translate(const tinyxml2::XMLAttribute &in, std::ostream &out, Context &context) const = 0;
};

class DefaultAttributeTranslator : public AttributeTranslator {
public:
    const std::string name;

    explicit DefaultAttributeTranslator(const std::string &name) : name(name) {}
    ~DefaultAttributeTranslator() override = default;
    void translate(const tinyxml2::XMLAttribute &in, std::ostream &out, Context &context) const override {
        out << " " << name << "=\"" << in.Value() << "\"";
    }
};

class StyleAttributeTranslator : public AttributeTranslator {
public:
    std::list<std::string> newClasses;

    ~StyleAttributeTranslator() override = default;
    void translate(const tinyxml2::XMLAttribute &in, std::ostream &out, Context &context) const override {
        const std::string styleName = in.Value();
        auto styleIt = context.styleDependencies.find(styleName);
        if (styleIt == context.styleDependencies.end()) {
            LOG(WARNING) << "unknown style: " << styleName;
            return;
        }

        out << "class=\"";
        for (auto i = styleIt->second.rbegin(); i != styleIt->second.rend(); ++i) {
            out << *i << " ";
        }
        out << styleName;
        for (auto &&c : newClasses) {
            out << " " << c;
        }
        out << "\"";
    }
};

class DefaultElementTranslator : public ElementTranslator {
public:
    const std::string name;
    std::list<std::pair<std::string, std::string>> newAttributes;
    std::unordered_map<std::string, std::unique_ptr<AttributeTranslator>> attributeTranslator;

    explicit DefaultElementTranslator(const std::string &name) : name(name) {
        attributeTranslator["text:style-name"] = std::make_unique<StyleAttributeTranslator>();
        attributeTranslator["table:style-name"] = std::make_unique<StyleAttributeTranslator>();
    }
    ~DefaultElementTranslator() override = default;
    void translateStart(const tinyxml2::XMLElement &in, std::ostream &out, Context &context) const override {
        out << "<" << name;

        for (auto &&a : newAttributes) {
            out << " " << a.first << "=\"" << a.second << "\"";
        }

        for (auto attr = in.FirstAttribute(); attr != nullptr; attr = attr->Next()) {
            const std::string attributeName = attr->Name();
            auto attributeTranslatorIt = attributeTranslator.find(attributeName);
            if (attributeTranslatorIt == attributeTranslator.end()) {
                LOG(WARNING) << "unhandled attribute: " << in.Name() << " " << attributeName;
                continue;
            }
            if (attributeTranslatorIt->second == nullptr) {
                continue;
            }

            out << " ";
            attributeTranslatorIt->second->translate(*attr, out, context);
        }

        out << ">";
    }
    void translateEnd(const tinyxml2::XMLElement &in, std::ostream &out, Context &context) const override {
        out << "</" << name << ">";
    }
};

class ParagraphTranslator : public DefaultElementTranslator {
public:
    ParagraphTranslator() : DefaultElementTranslator("p") {}
    ~ParagraphTranslator() override = default;
};

class HeadlineTranslator : public DefaultElementTranslator {
public:
    HeadlineTranslator() : DefaultElementTranslator("h") {
        attributeTranslator["text:outline-level"] = nullptr;
    }
    ~HeadlineTranslator() override = default;
};

class SpaceTranslator : public ElementTranslator {
public:
    ~SpaceTranslator() override = default;
    void translateStart(const tinyxml2::XMLElement &in, std::ostream &out, Context &context) const override {
        int64_t count = in.Int64Attribute("text:c", -1);
        if (count <= 0) {
            return;
        }
        // TODO: use maxRepeat
        for (uint64_t i = 0; i < count; ++i) {
            // TODO: use "&nbsp;"?
            out << " ";
        }
    }
    void translateEnd(const tinyxml2::XMLElement &in, std::ostream &out, Context &context) const override {}
};

class TabTranslator : public ElementTranslator {
public:
    ~TabTranslator() override = default;
    void translateStart(const tinyxml2::XMLElement &in, std::ostream &out, Context &context) const override {
        // TODO: use "&emsp;"?
        out << "\t";
    }
    void translateEnd(const tinyxml2::XMLElement &in, std::ostream &out, Context &context) const override {}
};

class LinkTranslator : public ElementTranslator {
public:
    ~LinkTranslator() override = default;
    void translateStart(const tinyxml2::XMLElement &in, std::ostream &out, Context &context) const override {
        auto href = in.FindAttribute("xlink:href");
        out << "<a";
        if (href != nullptr) {
            out << " href\"" << href->Value() << "\"";
            // NOTE: there is a trim in java
            if ((std::strlen(href->Value()) > 0) && (href->Value()[0] == '#')) {
                out << " target\"_self\"";
            }
        } else {
            LOG(WARNING) << "empty link";
        }
        out << ">";
    }
    void translateEnd(const tinyxml2::XMLElement &in, std::ostream &out, Context &context) const override {
        out << "</a>";
    }
};

class BookmarkTranslator : public ElementTranslator {
public:
    ~BookmarkTranslator() override = default;
    void translateStart(const tinyxml2::XMLElement &in, std::ostream &out, Context &context) const override {
        auto id = in.FindAttribute("text:name");
        out << "<a";
        if (id != nullptr) {
            out << " id=\"" << id->Value() << "\"";
        } else {
            LOG(WARNING) << "empty bookmark";
        }
        out << ">";
    }
    void translateEnd(const tinyxml2::XMLElement &in, std::ostream &out, Context &context) const override {
        out << "</a>";
    }
};

class ListTranslator : public DefaultElementTranslator {
public:
    ListTranslator() : DefaultElementTranslator("ul") {
        attributeTranslator["xml:id"] = nullptr;
    }
    ~ListTranslator() override = default;
};

// TODO: simple table translator

class TableTranslator : public DefaultElementTranslator {
public:
    TableTranslator() : DefaultElementTranslator("table") {
        newAttributes.emplace_back("border", "0");
        newAttributes.emplace_back("cellspacing", "0");
        newAttributes.emplace_back("cellpadding", "0");

        attributeTranslator["table:name"] = nullptr;
        attributeTranslator["table:print"] = nullptr;
        attributeTranslator["table:protected"] = nullptr;
        attributeTranslator["table:protection-key"] = nullptr;
        attributeTranslator["table:protection-key-digest-algorithm"] = nullptr;
    }
    ~TableTranslator() override = default;
};

class TableColumnTranslator : public DefaultElementTranslator {
public:
    TableColumnTranslator() : DefaultElementTranslator("col") {
        attributeTranslator["table:default-cell-style-name"] = nullptr; // TODO: implement
        attributeTranslator["table:number-columns-repeated"] = nullptr; // TODO: implement
    }
    ~TableColumnTranslator() override = default;
};

class TableRowTranslator : public DefaultElementTranslator {
public:
    TableRowTranslator() : DefaultElementTranslator("tr") {
        attributeTranslator["table:number-rows-repeated"] = nullptr; // TODO: implement
    }
    ~TableRowTranslator() override = default;
};

class TableCellTranslator : public DefaultElementTranslator {
public:
    TableCellTranslator() : DefaultElementTranslator("td") {
        attributeTranslator["table:formula"] = nullptr;
        attributeTranslator["table:number-columns-spanned"] = std::make_unique<DefaultAttributeTranslator>("colspan");
        attributeTranslator["table:number-rows-spanned"] = std::make_unique<DefaultAttributeTranslator>("rowspan");
        attributeTranslator["table:number-columns-repeated"] = nullptr; // TODO: implement

        attributeTranslator["office:value"] = nullptr;
        attributeTranslator["office:value-type"] = nullptr;
        attributeTranslator["office:string-value"] = nullptr;

        attributeTranslator["calcext:value-type"] = nullptr;
    }
    ~TableCellTranslator() override = default;
};

class DefaultContentTranslatorImpl : public ContentTranslator {
public:
    std::unordered_map<std::string, std::unique_ptr<ElementTranslator>> elementTranslator;

    DefaultContentTranslatorImpl() {
        elementTranslator["text:p"] = std::make_unique<DefaultElementTranslator>("p");
        elementTranslator["text:h"] = std::make_unique<HeadlineTranslator>();
        elementTranslator["text:span"] = std::make_unique<DefaultElementTranslator>("span");
        elementTranslator["text:a"] = std::make_unique<LinkTranslator>();
        elementTranslator["text:s"] = std::make_unique<SpaceTranslator>();
        elementTranslator["text:tab"] = std::make_unique<TabTranslator>();
        elementTranslator["text:line-break"] = std::make_unique<DefaultElementTranslator>("br");
        elementTranslator["text:soft-page-break"] = nullptr;
        elementTranslator["text:list"] = std::make_unique<ListTranslator>();
        elementTranslator["text:list-item"] = std::make_unique<DefaultElementTranslator>("li");
        elementTranslator["text:bookmark"] = std::make_unique<BookmarkTranslator>();
        elementTranslator["text:bookmark-start"] = std::make_unique<BookmarkTranslator>();
        elementTranslator["text:bookmark-end"] = nullptr;
        elementTranslator["text:sequence-decls"] = nullptr;
        elementTranslator["text:sequence-decl"] = nullptr;
        elementTranslator["text:table-of-content"] = nullptr;
        elementTranslator["text:table-of-content-source"] = nullptr;
        elementTranslator["text:table-of-content-entry-template"] = nullptr;
        elementTranslator["text:index-title"] = nullptr;
        elementTranslator["text:index-body"] = nullptr;
        elementTranslator["text:index-title-template"] = nullptr;
        elementTranslator["text:index-entry-link-start"] = nullptr;
        elementTranslator["text:index-entry-link-end"] = nullptr;
        elementTranslator["text:index-entry-text"] = nullptr;
        elementTranslator["text:index-entry-tab-stop"] = nullptr;
        elementTranslator["text:index-entry-page-number"] = nullptr;
        elementTranslator["text:index-entry-chapter"] = nullptr;

        elementTranslator["table:table"] = std::make_unique<TableTranslator>();
        elementTranslator["table:table-column"] = std::make_unique<TableColumnTranslator>();
        elementTranslator["table:table-row"] = std::make_unique<TableRowTranslator>();
        elementTranslator["table:table-cell"] = std::make_unique<TableCellTranslator>();
        elementTranslator["table:tracked-changes"] = nullptr;
        elementTranslator["table:calculation-settings"] = nullptr;
        elementTranslator["table:iteration"] = nullptr;
        elementTranslator["table:covered-table-cell"] = nullptr;
        elementTranslator["table:named-expressions"] = nullptr;
        elementTranslator["table:shapes"] = nullptr;

        elementTranslator["draw:page"] = nullptr;
        elementTranslator["draw:text-box"] = nullptr;
        elementTranslator["draw:object"] = nullptr;
        elementTranslator["draw:frame"] = nullptr;
        elementTranslator["draw:image"] = nullptr;
        elementTranslator["draw:page-thumbnail"] = nullptr;

        elementTranslator["presentation:notes"] = nullptr;
        elementTranslator["presentation:settings"] = nullptr;

        elementTranslator["office:body"] = nullptr;
        elementTranslator["office:text"] = nullptr;
        elementTranslator["office:spreadsheet"] = nullptr;
        elementTranslator["office:presentation"] = nullptr;
        elementTranslator["office:annotation"] = nullptr;
        elementTranslator["office:forms"] = nullptr;

        elementTranslator["svg:desc"] = nullptr;

        elementTranslator["dc:date"] = nullptr;
        elementTranslator["calcext:condition"] = nullptr;
        elementTranslator["calcext:conditional-format"] = nullptr;
        elementTranslator["calcext:conditional-formats"] = nullptr;
        elementTranslator["loext:p"] = nullptr;
        elementTranslator["loext:table-protection"] = nullptr;
    }
    ~DefaultContentTranslatorImpl() override = default;

    void translate(const tinyxml2::XMLElement &in, Context &context) const override {
        auto &out = *context.output;
        context.currentElement = &in;

        const std::string elementName = in.Name();
        auto elementTranslatorIt = elementTranslator.find(elementName);
        const ElementTranslator *translator = nullptr;
        if (elementTranslatorIt != elementTranslator.end()) {
            translator = elementTranslatorIt->second.get();
        } else {
            LOG(WARNING) << "unhandled element: " << elementName;
        }

        if (translator != nullptr) {
            translator->translateStart(in, out, context);
        }

        for (auto child = in.FirstChild(); child != nullptr; child = child->NextSibling()) {
            if (child->ToText() != nullptr) {
                out << child->ToText()->Value();
            } else if (child->ToElement() != nullptr) {
                translate(*child->ToElement(), context);
            }
        }

        if (translator != nullptr) {
            translator->translateEnd(in, out, context);
        }
    }
};

std::unique_ptr<ContentTranslator> ContentTranslator::create() {
    return std::make_unique<DefaultContentTranslatorImpl>();
}

}