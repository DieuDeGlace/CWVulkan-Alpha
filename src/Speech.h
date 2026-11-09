#pragma once
// What NPCs say (docs/specs/alpha-talk-vendors.md §2): the dictionary's speech entries parsed
// into a tree of words, alternatives and coloured spans, rendered into pages with tags and
// variables, and the choice of entry for an NPC (the innkeeper, pet food, random talk).
#include "Dictionary.h"
#include "worldgen/Regions.h"
#include "creatures/Inventory.h"
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct SpeechSegment {
    std::string text; // one word (UTF-8)
    glm::vec4 colour{1};
};
using SpeechPage = std::vector<SpeechSegment>;

// §2.1: a parsed speech entry.
class SpeechText {
  public:
    explicit SpeechText(const std::string &markup);
    // §2.2: the pages, with these tags present and these variables.
    std::vector<SpeechPage> render(const std::set<std::string> &tags,
                                   const std::map<std::string, std::string> &variables) const;

  private:
    struct Node {
        std::set<std::string> conditions, styles;
        std::string text;
        Node *parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;
    };
    Node root;
    bool render(const Node &n, const std::set<std::string> &tags, const std::map<std::string, std::string> &variables,
                std::vector<SpeechPage> &pages) const;
};

// §2.3: tags #p_g and variables @p_c from a dictionary `name` entry.
void speechNameVariables(const Dictionary &d, const std::string &key, const std::string &prefix,
                         std::set<std::string> &tags, std::map<std::string, std::string> &variables);

// §2.4: what an NPC says. `draw` is rand() after srand(the id's low 32 bits). `place` is the
// location of its talk target (§2.4 steps 0-2), for talk kind 4; without one such NPCs fall
// back to random talk.
std::vector<SpeechPage> npcSpeech(const Dictionary &d, uint64_t id, uint8_t characterClass, int32_t talkKind,
                                  const worldgen::Location *place = nullptr);
// §1.3: the player's "There is nothing special."
std::vector<SpeechPage> nothingSpecial();
