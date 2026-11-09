// Speech bubbles (docs/specs/alpha-talk-vendors.md §1): four shared bubbles over creatures'
// heads, typing their text at 40 ms a character with a voice blip every three, paged by R
// or three seconds after a page is typed.
#include "ui/Screens.h"
#include "ui/Widgets.h"
#include <cmath>

namespace ui {
namespace {

using namespace widgets;

constexpr float MS_PER_CHARACTER = 40;

// §1.2: a page's length in ms (every word counts one more character for its space).
float pageLength(const std::vector<SpeechPage> &pages, size_t page) {
    if (page >= pages.size())
        return 0;
    size_t units = 0;
    for (const SpeechSegment &s : pages[page])
        units += utf16(s.text).size() + 1;
    return MS_PER_CHARACTER * (float)units;
}

} // namespace

void Interface::talk(uint64_t creature, const std::function<std::vector<SpeechPage>()> &text) {
    // Step 1: the creature's own bubble finishes typing, or goes to its next page.
    for (Bubble &b : bubbles)
        if (b.creature == creature) {
            if (b.elapsed >= pageLength(b.pages, b.page))
                b.page++, b.elapsed = 0, b.typedBefore = 0;
            else
                b.elapsed = pageLength(b.pages, b.page);
            return;
        }
    // Step 2: the last free bubble; none free, nothing shows.
    Bubble *free = nullptr;
    for (Bubble &b : bubbles)
        if (b.creature == 0)
            free = &b;
    if (!free)
        return;
    std::vector<SpeechPage> pages = text();
    if (pages.empty() || pages[0].empty())
        return;
    const auto at = creatureAt ? creatureAt(creature) : std::nullopt;
    *free = Bubble{creature, at ? at->first : glm::dvec3(0), std::move(pages)};
}

void Interface::stepBubbles(float dtMs) {
    for (Bubble &b : bubbles) {
        if (b.creature == 0)
            continue;
        b.elapsed += dtMs;
        if (b.elapsed > pageLength(b.pages, b.page) + 3000)
            b.page++, b.elapsed = 0, b.typedBefore = 0;
        // (A bubble whose creature has gone is freed: the original keeps a stale pointer, B-1.)
        const auto at = creatureAt ? creatureAt(b.creature) : std::nullopt;
        if (b.page >= b.pages.size() || !at) {
            b = Bubble{};
            continue;
        }
        // §1.5 step 1: glide after the creature, time constant about 50 ms.
        const double s = 1.0 - std::pow(0.98, (double)dtMs);
        b.anchor += (at->first - b.anchor) * s;
    }
}

void Interface::drawBubbles(Painter &painter, glm::vec2 W) {
    for (Bubble &b : bubbles) {
        if (b.creature == 0 || !project || b.page >= b.pages.size())
            continue;
        const auto at = creatureAt ? creatureAt(b.creature) : std::nullopt;
        const float height = at ? at->second : 2;
        // §1.5 steps 2-5: the tail on the head point, the box above it.
        const auto head = project(b.anchor + glm::dvec3(0, 0, height / 2 + 1));
        if (!head)
            continue;
        const glm::vec2 box = *head - glm::vec2(125, 170);
        if (speechBubble) {
            anchor(*speechBubble, box);
            painter.draw(*speechBubble, 0, Transform(1));
        }
        // §1.6: the typed part of the page, 14 px words, 18 px lines, wrapped at 220 px.
        TextStyle style;
        style.pixelSize = 14;
        style.outline = 3;
        const int32_t typed = (int32_t)std::floor(b.elapsed / MS_PER_CHARACTER);
        int32_t consumed = 0;
        float x = 0, y = 0;
        bool glue = true;
        for (const SpeechSegment &segment : b.pages[b.page]) {
            if (typed < consumed)
                break;
            const std::u16string word = utf16(segment.text);
            const float w = painter.textWidth(word, style);
            static const std::u16string PUNCT = u".-,;!?:";
            const bool punct = word.size() == 1 && PUNCT.find(word[0]) != std::u16string::npos;
            if (!glue)
                x += punct ? 3.f : 14.f;
            glue = word == u"-";
            if (!punct && x + w > 220)
                x = 0, y += 18;
            const size_t visible = std::min(word.size(), (size_t)(typed - consumed));
            style.colour = segment.colour;
            painter.text(word.substr(0, visible), box + glm::vec2(10 + x, 25 + y), style);
            x += w;
            consumed += (int32_t)visible + 1;
        }
        // A voice blip every three typed characters (human-voice02.wav, at the player).
        if (b.typedBefore / 3 != consumed / 3)
            inventorySounds.push_back(0x32);
        b.typedBefore = consumed;
    }
}

} // namespace ui
