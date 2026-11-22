// Light-write trace for the Alpha extractor: an ordered record of every original operation that
// can change stored terrain light, per docs/specs/alpha-native-light-write-observation-points.md.
#pragma once
#include <cstdint>
#include <string>

namespace light_trace {

// Event kinds in AVLT files. Values are part of the file format.
enum Kind : uint16_t {
    LightEnter = 1,     // rectangle light routine entry: caller, bounds, margin, explicit zone
    LightPreCommit = 2, // all sixteen rounds finished, commit about to begin
    LightExit = 3,      // commit finished
    AdmitYes = 4,       // mesh admission decided: admitted
    AdmitNo = 5,        // mesh admission decided: denied
    BuildLocked = 6,    // mesh builder entered D8
    RelightChosen = 7,  // mesh builder chose a relight: dirty byte, chunk origin
    BuildLightDone = 8, // mesh builder past its relight (or its skip)
    StepTest = 9,       // world-step relight test of world +0xB8 (bomb, building, variable radius)
    CellWriteEnter = 10,
    CellWriteExit = 11,
    ClearEnter = 12,
    ClearStored = 13,
    CaptureBegin = 14,  // extractor dump started (C0 held by the extractor)
    CaptureEnd = 15,
    ProbesChanged = 16, // a new probe list took effect (v[0] its index)
};

// Reads probe cells from AlphaExtract\lighttrace.txt ("X Y Z" whole cells per line) and installs
// the observation points. Only at DLL load, before the game's threads run. False with a reason.
bool install(uintptr_t base, const std::wstring &folder, const char *&why);
bool installed();
void mark(Kind kind);                   // capture markers from the extractor's own thread
bool save(const std::wstring &path);    // completed events so far, in sequence order
bool replaceProbes(const std::wstring &path); // a new probe list from then on (up to 16 lists)

} // namespace light_trace
