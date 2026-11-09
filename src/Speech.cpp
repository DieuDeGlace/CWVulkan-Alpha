#include "Speech.h"
#include "ItemNames.h"
#include "Loot.h"
#include "creatures/Names.h"
#include "ui/LandNames.h"
#include "worldgen/Random.h"
#include <cstring>

SpeechText::SpeechText(const std::string &markup) {
    Node *current = &root;
    int depth = 1; // levels open, the root counted
    std::string token;
    const auto add = [&](Node *parent, std::string text) {
        auto n = std::make_unique<Node>();
        n->text = std::move(text);
        n->parent = parent;
        parent->children.push_back(std::move(n));
        return parent->children.back().get();
    };
    const auto enter = [&](std::string text = {}) {
        current = add(current, std::move(text));
        depth++;
    };
    const auto leave = [&](int levels) {
        for (int i = 0; i < levels; i++)
            current = current->parent, depth--;
    };
    for (size_t i = 0; i <= markup.size(); i++) {
        const char ch = i < markup.size() ? markup[i] : '\0';
        static const char *const PUNCT = ".:-,;!?/()";
        const bool punct = ch != '\0' && std::strchr(PUNCT, ch);
        const bool delimiter = ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\0' || ch == '{' ||
                               ch == '}' || ch == '[' || ch == ']' || ch == '|' || punct;
        if (!delimiter) {
            token += ch;
            continue;
        }
        // Steps 1, 2: groups and spans open before the pending word lands.
        if (ch == '{') {
            enter(); // the group
            enter(); // its first alternative
        } else if (ch == '[') {
            enter();
        }
        // Step 3: the pending token.
        if (!token.empty() || punct) {
            if (!token.empty() && token[0] == '#')
                current->conditions.insert(token);
            else if (!token.empty() && token[0] == '$')
                current->styles.insert(token);
            else {
                add(current, token);
                if (punct)
                    add(current, std::string(1, ch));
            }
        }
        token.clear();
        // Steps 4-6.
        if (ch == '|') {
            if (depth < 2)
                return;
            leave(1);
            enter();
        } else if (ch == '}') {
            if (depth < 3)
                return;
            leave(2);
        } else if (ch == ']') {
            if (depth < 2)
                return;
            leave(1);
        }
    }
}

bool SpeechText::render(const Node &n, const std::set<std::string> &tags,
                        const std::map<std::string, std::string> &variables, std::vector<SpeechPage> &pages) const {
    bool matched = false;
    if (!n.conditions.empty()) {
        for (const std::string &c : n.conditions)
            matched = matched || tags.count(c);
        if (!matched)
            return false; // skipped: continue
    }
    if (!n.text.empty()) {
        if (n.text == "%") {
            pages.emplace_back();
        } else {
            glm::vec4 colour(1);
            if (n.parent) {
                static const std::pair<const char *, glm::vec4> STYLES[] = {
                    {"$creature", {1, 0.3f, 0.3f, 1}}, {"$name", {0.6f, 0.3f, 1, 1}},   {"$item", {0.3f, 0.3f, 1, 1}},
                    {"$object", {1, 0.5f, 0.3f, 1}},   {"$zone", {0.3f, 1, 0.3f, 1}},   {"$stress", {0.6f, 0.3f, 1, 1}},
                    {"$number", {0.3f, 1, 0.3f, 1}}};
                for (const auto &[style, c] : STYLES)
                    if (n.parent->styles.count(style))
                        colour = c;
            }
            std::string words = n.text;
            if (words[0] == '@') {
                const auto it = variables.find(words);
                words = it == variables.end() ? std::string() : it->second;
            }
            size_t from = 0;
            while (from < words.size()) {
                const size_t end = words.find_first_of(" \t\r\n", from);
                const std::string word = words.substr(from, end == std::string::npos ? std::string::npos : end - from);
                if (!word.empty())
                    pages.back().push_back({word, colour});
                if (end == std::string::npos)
                    break;
                from = end + 1;
            }
        }
    }
    for (const auto &child : n.children)
        if (render(*child, tags, variables, pages))
            break;
    return matched;
}

std::vector<SpeechPage> SpeechText::render(const std::set<std::string> &tags,
                                           const std::map<std::string, std::string> &variables) const {
    std::vector<SpeechPage> pages(1);
    render(root, tags, variables, pages);
    return pages;
}

void speechNameVariables(const Dictionary &d, const std::string &key, const std::string &prefix,
                         std::set<std::string> &tags, std::map<std::string, std::string> &variables) {
    const DictionaryEntry *e = d.find("name", key);
    if (!e)
        return;
    if (const auto it = e->attributes.find("tags"); it != e->attributes.end()) {
        size_t from = 0;
        const std::string &list = it->second;
        while (from < list.size()) {
            const size_t end = list.find_first_of(" ,", from);
            const std::string g = list.substr(from, end == std::string::npos ? std::string::npos : end - from);
            if (!g.empty())
                tags.insert("#" + prefix + "_" + g);
            if (end == std::string::npos)
                break;
            from = end + 1;
        }
    }
    for (const auto &[child, text] : e->fields)
        variables["@" + prefix + "_" + child] = text;
}

namespace {

std::vector<SpeechPage> entry(const Dictionary &d, const std::string &key, const std::set<std::string> &tags,
                              const std::map<std::string, std::string> &variables) {
    const DictionaryEntry *e = d.find("speech", key);
    if (!e)
        return {};
    return SpeechText(e->text).render(tags, variables);
}

} // namespace

std::vector<SpeechPage> npcSpeech(const Dictionary &d, uint64_t id, uint8_t characterClass, int32_t talkKind,
                                  const worldgen::Location *place) {
    if (characterClass == 132)
        return entry(d, "innkeeper", {}, {});
    if (talkKind == 4 && place && place->type != 0) { // §2.4 step 3: no draw
        std::set<std::string> tags;
        std::map<std::string, std::string> variables;
        ui::locationVariables(*place, d, "scenery", tags, variables);
        return entry(d, "explored", tags, variables);
    }
    worldgen::Random random((uint32_t)id);
    std::set<std::string> tags;
    std::map<std::string, std::string> variables;
    const auto creature = [&](int32_t type, const std::string &prefix) {
        if (const char *key = creatures::speciesKey(type))
            speechNameVariables(d, key, prefix, tags, variables);
    };
    if (talkKind == 3) {
        const int32_t pet = TAMEABLE[(uint32_t)random.draw() % 45];
        creature(pet, "creature");
        speechNameVariables(d, itemKey(20, (uint8_t)pet), "food", tags, variables);
        return entry(d, "petfood", tags, variables);
    }
    static const std::pair<uint8_t, uint8_t> ITEMS[5] = {{11, 12}, {24, 0}, {23, 0}, {23, 1}, {1, 7}};
    const auto &[type, sub] = ITEMS[random.draw() % 5];
    speechNameVariables(d, itemKey(type, sub), "itemvendoritem", tags, variables);
    creature(TAMEABLE[(uint32_t)random.draw() % 45], "pet");
    static const int32_t MONSTERS[11] = {108, 109, 114, 115, 119, 111, 113, 112, 116, 110, 117};
    creature(MONSTERS[random.draw() % 11], "monster");
    return entry(d, "random:" + std::to_string(random.draw() % 30 + 1), tags, variables);
}

std::vector<SpeechPage> nothingSpecial() {
    SpeechPage page;
    for (const char *word : {"There", "is", "nothing", "special."})
        page.push_back({word, glm::vec4(1)});
    return {page};
}
