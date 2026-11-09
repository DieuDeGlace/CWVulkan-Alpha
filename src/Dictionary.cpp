#include "Dictionary.h"
#include <cctype>
#include <cstring>

namespace {

// A forward-only reader over the XML text, enough for the dictionaries: elements, attributes,
// text, comments, the XML declaration and the five predefined entities. No DTDs or CDATA.
class Reader {
  public:
    explicit Reader(const std::string &text) : s(text) {
        if (s.compare(0, 3, "\xEF\xBB\xBF") == 0)
            pos = 3; // UTF-8 byte order mark
    }

    bool failed() const { return !error.empty(); }
    std::string error;

    // Skips the declaration, comments and white space before the next element or text.
    void skipMisc() {
        for (;;) {
            while (pos < s.size() && std::isspace((unsigned char)s[pos]))
                pos++;
            if (startsWith("<?"))
                skipPast("?>");
            else if (startsWith("<!--"))
                skipPast("-->");
            else
                return;
        }
    }

    // Reads one element and everything inside it. `text` gets its direct text (trimmed) and
    // `children` its child elements as (name, text) pairs.
    bool element(std::string &name, std::map<std::string, std::string> &attributes, std::string &text,
                 std::vector<std::pair<std::string, std::string>> *children, int depth = 0) {
        skipMisc();
        if (!startsWith("<"))
            return fail("expected an element");
        pos++;
        name = word();
        if (name.empty())
            return fail("element without a name");
        for (;;) {
            skipSpace();
            if (startsWith("/>")) {
                pos += 2;
                return true;
            }
            if (startsWith(">")) {
                pos++;
                break;
            }
            std::string attribute = word();
            skipSpace();
            if (attribute.empty() || !startsWith("="))
                return fail("bad attribute in <" + name + ">");
            pos++;
            skipSpace();
            if (pos >= s.size() || (s[pos] != '"' && s[pos] != '\''))
                return fail("unquoted attribute in <" + name + ">");
            const char quote = s[pos++];
            const size_t end = s.find(quote, pos);
            if (end == std::string::npos)
                return fail("unterminated attribute in <" + name + ">");
            attributes[attribute] = decode(s.substr(pos, end - pos));
            pos = end + 1;
        }
        // Content: text, comments and child elements, until the closing tag.
        std::string raw;
        for (;;) {
            if (pos >= s.size())
                return fail("<" + name + "> is not closed");
            if (startsWith("<!--")) {
                skipPast("-->");
            } else if (startsWith("</")) {
                pos += 2;
                const std::string closing = word();
                skipSpace();
                if (closing != name || !startsWith(">"))
                    return fail("</" + closing + "> does not close <" + name + ">");
                pos++;
                text = trim(decode(raw));
                return true;
            } else if (startsWith("<")) {
                if (depth > 16)
                    return fail("elements nested too deeply");
                std::string childName, childText;
                std::map<std::string, std::string> childAttributes;
                if (!element(childName, childAttributes, childText, nullptr, depth + 1))
                    return false;
                if (children)
                    children->emplace_back(childName, childText);
            } else {
                raw += s[pos++];
            }
        }
    }

    bool atEnd() {
        skipMisc();
        return pos >= s.size();
    }
    bool atClosing() {
        skipMisc();
        return startsWith("</");
    }
    bool closing(const std::string &name) {
        skipMisc();
        if (!startsWith("</"))
            return fail("expected </" + name + ">");
        pos += 2;
        const bool ok = word() == name;
        skipSpace();
        if (!ok || !startsWith(">"))
            return fail("expected </" + name + ">");
        pos++;
        return true;
    }
    // Reads an opening tag only (for the root, whose children are read one by one).
    bool open(std::string &name) {
        skipMisc();
        if (!startsWith("<"))
            return fail("expected the root element");
        pos++;
        name = word();
        const size_t end = s.find('>', pos);
        if (name.empty() || end == std::string::npos)
            return fail("bad root element");
        pos = end + 1;
        return true;
    }

  private:
    const std::string &s;
    size_t pos = 0;

    bool fail(const std::string &why) {
        // The line number makes a broken owner file easy to find.
        size_t line = 1;
        for (size_t i = 0; i < pos && i < s.size(); i++)
            line += s[i] == '\n';
        error = "line " + std::to_string(line) + ": " + why;
        return false;
    }
    bool startsWith(const char *t) const { return s.compare(pos, std::strlen(t), t) == 0; }
    void skipPast(const char *t) {
        const size_t end = s.find(t, pos);
        pos = end == std::string::npos ? s.size() : end + std::strlen(t);
    }
    void skipSpace() {
        while (pos < s.size() && std::isspace((unsigned char)s[pos]))
            pos++;
    }
    std::string word() {
        const size_t start = pos;
        while (pos < s.size() && (std::isalnum((unsigned char)s[pos]) || s[pos] == '_' || s[pos] == '-' ||
                                  s[pos] == ':' || s[pos] == '.'))
            pos++;
        return s.substr(start, pos - start);
    }
    static std::string trim(const std::string &t) {
        const auto first = t.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            return {};
        return t.substr(first, t.find_last_not_of(" \t\r\n") - first + 1);
    }
    static std::string decode(const std::string &t) {
        static const std::pair<const char *, char> ENTITIES[] = {
            {"&amp;", '&'}, {"&lt;", '<'}, {"&gt;", '>'}, {"&quot;", '"'}, {"&apos;", '\''}};
        std::string out;
        for (size_t i = 0; i < t.size();) {
            bool replaced = false;
            if (t[i] == '&')
                for (const auto &[entity, c] : ENTITIES)
                    if (t.compare(i, std::strlen(entity), entity) == 0) {
                        out += c;
                        i += std::strlen(entity);
                        replaced = true;
                        break;
                    }
            if (!replaced)
                out += t[i++];
        }
        return out;
    }
};

} // namespace

const std::string *DictionaryEntry::field(const std::string &name) const {
    for (const auto &[fieldName, value] : fields)
        if (fieldName == name)
            return &value;
    return nullptr;
}

bool Dictionary::parse(const std::string &xml, std::string *error) {
    list.clear();
    index.clear();
    Reader reader(xml);
    std::string root;
    if (reader.open(root)) {
        while (!reader.atClosing() && !reader.atEnd() && !reader.failed()) {
            DictionaryEntry entry;
            if (!reader.element(entry.kind, entry.attributes, entry.text, &entry.fields))
                break;
            if (auto key = entry.attributes.find("key"); key != entry.attributes.end()) {
                entry.key = key->second;
                entry.attributes.erase(key);
            }
            index.emplace(std::make_pair(entry.kind, entry.key), list.size());
            list.push_back(std::move(entry));
        }
        if (!reader.failed())
            reader.closing(root);
    }
    if (reader.failed() && error)
        *error = reader.error;
    return !reader.failed();
}

const DictionaryEntry *Dictionary::find(const std::string &kind, const std::string &key) const {
    const auto it = index.find({kind, key});
    return it == index.end() ? nullptr : &list[it->second];
}
