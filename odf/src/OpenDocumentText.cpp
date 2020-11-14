#include <OpenDocumentText.h>

namespace odr::odf {

namespace {
class Element;

std::shared_ptr<generic::GenericElement>
firstChildImpl(std::shared_ptr<const Element> parent, pugi::xml_node node);
std::shared_ptr<generic::GenericElement>
previousSiblingImpl(std::shared_ptr<const Element> parent, pugi::xml_node node);
std::shared_ptr<generic::GenericElement>
nextSiblingImpl(std::shared_ptr<const Element> parent, pugi::xml_node node);

class Element : public virtual generic::GenericElement,
                public std::enable_shared_from_this<Element> {
public:
  Element(std::shared_ptr<const Element> parent, pugi::xml_node node)
      : m_parent{std::move(parent)}, m_node{node} {}

  std::shared_ptr<const GenericElement> parent() const override {
    return m_parent;
  }

  std::shared_ptr<const GenericElement> firstChild() const override {
    return firstChildImpl(shared_from_this(), m_node);
  }

  std::shared_ptr<const GenericElement> previousSibling() const override {
    return previousSiblingImpl(m_parent, m_node);
  }

  std::shared_ptr<const GenericElement> nextSibling() const override {
    return nextSiblingImpl(m_parent, m_node);
  }

protected:
  const std::shared_ptr<const Element> m_parent;
  const pugi::xml_node m_node;
};

class Simple final : public Element {
public:
  Simple(std::shared_ptr<const Element> parent, pugi::xml_node node,
         const Type type)
      : Element(std::move(parent), node), m_type{type} {}

  Type type() const final { return m_type; }

private:
  const Type m_type;
};

class Text : public Element, public generic::GenericText {
public:
  Text(std::shared_ptr<const Element> parent, pugi::xml_node node)
      : Element(std::move(parent), node) {}

  std::string text() const override {
    if (m_node.type() == pugi::node_pcdata) {
      return m_node.text().as_string();
    }

    const std::string element = m_node.name();
    if (element == "text:s") {
      const auto count = m_node.attribute("text:c").as_uint(1);
      return std::string(count, '.');
    } else if (element == "text:tab") {
      return "\t";
    }

    // TODO this should never happen. log or throw?
    return "";
  }
};

class Paragraph : public Element, public generic::GenericParagraph {
public:
  Paragraph(std::shared_ptr<const Element> parent, pugi::xml_node node)
      : Element(std::move(parent), node) {}

  Properties properties() const override {
    return {}; // TODO
  }
};

std::shared_ptr<generic::GenericElement>
convert(std::shared_ptr<const Element> parent, pugi::xml_node node) {
  if (node.type() == pugi::node_pcdata) {
    return std::make_shared<Text>(std::move(parent), node);
  }

  if (node.type() == pugi::node_element) {
    const std::string element = node.name();

    if (element == "text:p" || element == "text:h")
      return std::make_shared<Paragraph>(std::move(parent), node);
    else if (element == "text:s" || element == "text:tab")
      return std::make_shared<Text>(std::move(parent), node);
    else if (element == "text:line-break")
      return std::make_shared<Simple>(
          std::move(parent), node, generic::GenericElement::Type::LINE_BREAK);
    // else if (element == "text:a")
    //  LinkTranslator(in, out, context);
    // else if (element == "text:bookmark" || element == "text:bookmark-start")
    //  BookmarkTranslator(in, out, context);
    // else if (element == "draw:frame" || element == "draw:custom-shape")
    //  FrameTranslator(in, out, context);
    // else if (element == "draw:image")
    //  ImageTranslator(in, out, context);
    // else if (element == "table:table")
    //  TableTranslator(in, out, context);

    return std::make_shared<Simple>(std::move(parent), node,
                                    generic::GenericElement::Type::UNKNOWN);
  }

  return nullptr;
}

std::shared_ptr<generic::GenericElement>
firstChildImpl(std::shared_ptr<const Element> parent, pugi::xml_node node) {
  if (auto &&c = node.first_child(); c)
    return convert(std::move(parent), c);
  return nullptr;
}

std::shared_ptr<generic::GenericElement>
previousSiblingImpl(std::shared_ptr<const Element> parent,
                    pugi::xml_node node) {
  if (auto &&c = node.previous_sibling(); c)
    return convert(std::move(parent), c);
  return nullptr;
}

std::shared_ptr<generic::GenericElement>
nextSiblingImpl(std::shared_ptr<const Element> parent, pugi::xml_node node) {
  if (auto &&c = node.next_sibling(); c)
    return convert(std::move(parent), c);
  return nullptr;
}
} // namespace

OpenDocumentText::OpenDocumentText(pugi::xml_document content,
                                   pugi::xml_document styles)
    : m_content{std::move(content)}, m_styles{std::move(styles)} {}

pugi::xml_document &OpenDocumentText::content() { return m_content; }

pugi::xml_document &OpenDocumentText::styles() { return m_styles; }

std::shared_ptr<const generic::GenericElement>
OpenDocumentText::firstContentElement() const {
  const pugi::xml_node body =
      m_content.child("office:document-content").child("office:body");
  return convert(nullptr, body.first_child());
}

} // namespace odr::odf