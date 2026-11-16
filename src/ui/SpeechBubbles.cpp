// Speech bubbles (docs/specs/alpha-talk-vendors.md §1): four shared bubbles over creatures'
// heads, typing their text at 40 ms a character with voice blips as it types, paged by R
// or three seconds after a page is typed.
#include "ui/Screens.h"
#include "ui/Widgets.h"
#include <cmath>
#include <optional>

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

void Interface::advanceBubbles(float dtMs) {
    // §1.5 step 2: every bubble's time first, then step 4's strict 3000 hold and close.
    for (Bubble &b : bubbles)
        if (b.creature != 0)
            b.elapsed += dtMs;
    for (Bubble &b : bubbles) {
        if (b.creature == 0)
            continue;
        if (b.elapsed > pageLength(b.pages, b.page) + 3000)
            b.page++, b.elapsed = 0, b.typedBefore = 0;
        // Engine lifetime policy: a bubble whose creature has gone is freed (the original keeps
        // a stale pointer, B-1).
        if (b.page >= b.pages.size() || !(creatureAt && creatureAt(b.creature)))
            b = Bubble{};
    }
}

void Interface::followBubbles(float dtMs) {
    // §1.6 step 1: each bubble glides after its creature by the follow share of this step.
    const double share = (double)speechFollowShare((int32_t)dtMs);
    for (Bubble &b : bubbles) {
        if (b.creature == 0)
            continue;
        if (const auto at = creatureAt ? creatureAt(b.creature) : std::nullopt)
            b.anchor += (at->first - b.anchor) * share;
    }
}

float speechFollowShare(int32_t dt) {
    // From 0 in double, s := s + (1 - s) x 0.02 applied dt times, then rounded to binary32.
    double s = 0;
    for (int32_t i = 0; i < dt; i++)
        s = s + (1.0 - s) * 0.02;
    return (float)s;
}

std::optional<float> voiceBlip(int32_t typedBefore, int32_t consumed, const std::function<int()> &draw) {
    // §1.7: at most one blip per draw, when the typed count crosses a multiple of three; each
    // blip takes one draw of the shared stream for its pitch.
    if (typedBefore / 3 == consumed / 3)
        return std::nullopt;
    const float r = draw ? (float)draw() : 0.f;
    return 1.f + (0.5f * r) / 32767.f;
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
            if (segment.text.empty())
                continue; // §1.7 step 1: skipped, the page goes on
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
        // The voice blip (human-voice02.wav, sound 0x32 at the player's position, volume 1).
        if (const auto pitch = voiceBlip(b.typedBefore, consumed, speechDraw))
            voiceBlips.push_back(*pitch);
        b.typedBefore = consumed;
    }
}

} // namespace ui
