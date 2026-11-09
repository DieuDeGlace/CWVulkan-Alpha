#pragma once
// Alpha's dictionaries, dict_en.xml and dict_de.xml in data4.db: a flat list of entries such as
//
//   <name key="Crow" tags="f"> <singular>Crow</singular> <plural>Crows</plural> </name>
//   <landscape key="Hills"> <normal>@ Hills</normal> <to>in @ Hills</to> ... </landscape>
//   <speech key="specialization:warrior:berserker"> [Berserker $stress] % ... </speech>
//
// This file only reads them. What the markup inside the texts means ($stress, @, %, [...]) is
// part of the dialogue rules, which come later with their own spec.
#include <map>
#include <optional>
#include <string>
#include <vector>

struct DictionaryEntry {
    std::string kind; // the element name: name, speech, landscape, ...
    std::string key;
    std::map<std::string, std::string> attributes; // every attribute but `key` (e.g. tags)
    std::string text;                              // the entry's own text, trimmed
    std::vector<std::pair<std::string, std::string>> fields; // child elements in order: singular, plural, ...

    // The first child element named `name`, if any.
    const std::string *field(const std::string &name) const;
};

class Dictionary {
  public:
    // Parses the XML text of a dictionary. False (with a reason) if it is malformed.
    bool parse(const std::string &xml, std::string *error = nullptr);
    const std::vector<DictionaryEntry> &entries() const { return list; }
    // The entry of `kind` with `key` (keys are unique within a kind).
    const DictionaryEntry *find(const std::string &kind, const std::string &key) const;

  private:
    std::vector<DictionaryEntry> list;
    std::map<std::pair<std::string, std::string>, size_t> index;
};
