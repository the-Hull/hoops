/**
 ******************************************************************************
 * @file    ResultsScreen.cpp
 * @brief   Session results in four pages: court, categories, highlights,
 *          trend.
 ******************************************************************************
 */

#include "gui/screens/ResultsScreen.hpp"

#include <cstdio>
#include <cstring>

#include "score/Spots.hpp"
#include "score/Stats.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{

using Font = Theme::Font;

// --- Layout (docs/mockup-layout.md §11-§14; y values are centres) -----------

constexpr int32_t kHeaderY[ResultsScreen::kPages] = { 27, 37, 37, 39 };
constexpr const char* kTitles[ResultsScreen::kPages] = { "SESSION", "BY CATEGORY", "HIGHLIGHTS", "TREND" };
constexpr Hints::Style kUpDown[ResultsScreen::kPages] = { Hints::Dot, Hints::Dot, Hints::Off, Hints::Dot };

// Page 1
constexpr int32_t kTotalY     = 48;
constexpr int32_t kCourtX     = 30;
constexpr int32_t kCourtY     = 63;
constexpr float   kCourtScale = 0.904f;
constexpr int32_t kCourtLineW = 2;
constexpr int32_t kCourtSpotR = 6;
constexpr int32_t kCaptionY   = 197;
constexpr int32_t kCaptionW   = 180;
constexpr int32_t kLegendY    = 214;
constexpr int32_t kLegendDotR = 3;

// Page 2
constexpr int32_t kRowLeft    = 35;
constexpr int32_t kRowRight   = 205;
constexpr int32_t kRowLabelY[score::kCategoryCount] = { 61, 97, 133, 169 };
constexpr int32_t kRowBarY[score::kCategoryCount]   = { 76, 112, 148, 184 };
constexpr int32_t kBarH       = 6;
constexpr int32_t kRowGap     = 5;    // between the parts of a row
constexpr int32_t kFooterY    = 202;
constexpr int32_t kFooterW    = 170;

// Page 3
constexpr int32_t kCardW      = 85;
constexpr int32_t kCardH      = 65;
constexpr int32_t kCardBorder = 2;
constexpr int32_t kCardR      = 10;
constexpr int32_t kCardTextW  = kCardW - 2 * kCardBorder - 2;
constexpr int32_t kCardX[2]   = { 74, 166 };
constexpr int32_t kCardY[2]   = { 83, 155 };
constexpr int32_t kHrY        = 202;
constexpr int32_t kHrW        = 175;
constexpr int32_t kHeartR     = 3;

constexpr const char* kDot = " \xC2\xB7 ";   // " · " (rendered " - ")

// --- Text helpers ------------------------------------------------------------

/// A label with the first of @p texts that fits @p w in one of @p fonts
/// (texts outer, fonts inner, normal spacing before -1 px); the last
/// text in the last font, clipped, when none does.
lv_obj_t* fitLabel(lv_obj_t* parent, const char* const* texts, uint8_t nTexts, const Font* fonts, uint8_t nFonts,
                   int32_t x, int32_t w, int32_t yCentre, lv_text_align_t align, uint32_t color)
{
    const char* text  = texts[nTexts - 1];
    Font        font  = fonts[nFonts - 1];
    int32_t     space = -1;
    bool        found = false;
    for (uint8_t t = 0; t < nTexts && !found; ++t) {
        for (uint8_t f = 0; f < nFonts && !found; ++f) {
            for (int32_t ls = 0; ls >= -1 && !found; --ls) {
                if (Theme::textWidth(fonts[f], texts[t], ls) <= w) {
                    text = texts[t], font = fonts[f], space = ls, found = true;
                }
            }
        }
    }
    lv_obj_t* l = Theme::label(parent, font, text, x, Theme::topFor(font, yCentre), w, align, color);
    lv_obj_set_style_text_letter_space(l, space, LV_PART_MAIN);
    return l;
}

/// Baseline of text in @p f centred on @p yCentre.
int32_t baselineFor(Font f, int32_t yCentre)
{
    const lv_font_t* fn = Theme::font(f);
    return Theme::topFor(f, yCentre) + lv_font_get_line_height(fn) - fn->base_line;
}

/// Two labels on one baseline (@p fa's, centred on @p yCentre), centred
/// together across the screen.
void pair(lv_obj_t* parent, Font fa, const char* a, uint32_t ca, Font fb, const char* b, uint32_t cb,
          int32_t yCentre, int32_t gap)
{
    const int32_t wa = Theme::textWidth(fa, a);
    const int32_t wb = Theme::textWidth(fb, b);
    const int32_t x  = 120 - (wa + gap + wb) / 2;
    Theme::label(parent, fa, a, x, Theme::topFor(fa, yCentre), wa + 1, LV_TEXT_ALIGN_LEFT, ca);
    Theme::label(parent, fb, b, x + wa + gap, Theme::topForBaseline(fb, baselineFor(fa, yCentre)), wb + 1,
                 LV_TEXT_ALIGN_LEFT, cb);
}

/// "3PT · L-Wing" -> "L-Wing"; names without a category part unchanged.
const char* spotShort(const char* name)
{
    const char* p = strstr(name, kDot);
    return p ? p + strlen(kDot) : name;
}

void fmtMa(char* out, size_t n, const score::SpotStat& s)
{
    snprintf(out, n, "%u/%u", static_cast<unsigned>(s.makes), static_cast<unsigned>(s.attempts));
}

/// "56 %", or "--" with no attempts.
void fmtPct(char* out, size_t n, const score::SpotStat& s)
{
    if (s.attempts == 0) {
        snprintf(out, n, "--");
    } else {
        snprintf(out, n, "%u %%", static_cast<unsigned>(score::pct(s.makes, s.attempts)));
    }
}

uint32_t colorOf(const score::SpotStat& s)
{
    return Theme::pctColor(s.makes, s.attempts);
}

/// A highlight card: title, value and the first of @p subs that fits.
void card(lv_obj_t* parent, uint8_t col, uint8_t row, const char* title, const char* value, uint32_t valueColor,
          const char* const* subs, uint8_t nSubs)
{
    const int32_t cx = kCardX[col];
    const int32_t cy = kCardY[row];
    lv_obj_t* b = Theme::box(parent, cx - kCardW / 2, cy - kCardH / 2, kCardW, kCardH, Theme::kBg, kCardR);
    lv_obj_set_style_border_width(b, kCardBorder, LV_PART_MAIN);
    lv_obj_set_style_border_color(b, Theme::rgb(Theme::kLine), LV_PART_MAIN);
    lv_obj_set_style_border_opa(b, LV_OPA_COVER, LV_PART_MAIN);

    const int32_t x = cx - kCardTextW / 2;
    Theme::label(parent, Font::Regular14, title, x, Theme::topFor(Font::Regular14, cy - 20), kCardTextW,
                 LV_TEXT_ALIGN_CENTER, Theme::kDim);
    Theme::label(parent, Font::SemiBold25, value, x, Theme::topFor(Font::SemiBold25, cy + 1), kCardTextW,
                 LV_TEXT_ALIGN_CENTER, valueColor);
    constexpr Font kSubFont[] = { Font::Regular14 };
    fitLabel(parent, subs, nSubs, kSubFont, 1, x, kCardTextW, cy + 20, LV_TEXT_ALIGN_CENTER, Theme::kDim);
}

/// BEST SPOT / WORK ON: the spot's percentage and name, "--" when no spot
/// had enough attempts to rank.
void spotCard(lv_obj_t* parent, uint8_t col, uint8_t row, const char* title, uint8_t spot,
              const score::SessionStats& st, uint32_t color)
{
    char value[12];
    char minShots[24];
    if (spot >= score::kSpotCount) {
        snprintf(minShots, sizeof(minShots), "min %u shots", static_cast<unsigned>(score::kMinAttemptsForRank));
        const char* subs[] = { minShots };
        card(parent, col, row, title, "--", Theme::kDim, subs, 1);
        return;
    }
    fmtPct(value, sizeof(value), st.spot[spot]);
    const char* name   = score::spotInfo(spot).name;
    const char* subs[] = { name, spotShort(name) };
    card(parent, col, row, title, value, color, subs, 2);
}

} // namespace

ResultsScreen::ResultsScreen(Model& model)
    : Screen(model)
    , mKeys(&KeyInput::noHold, nullptr)
    , mPage(model.resultsPage < kPages ? model.resultsPage : 0)
{
}

void ResultsScreen::build()
{
    mPageObj = Theme::container(mRoot, 0, 0, 240, 240);
    if (mModel.hasResults()) {
        mSpot = spotWithAttempts(score::kSpotCount - 1, 1);
    }
    rebuild();
}

void ResultsScreen::onShow()
{
    if (!mModel.hasResults()) {
        mWaiting = true;
        mModel.requestResults(mModel.resultsFrom == Model::ResultsFrom::History ? mModel.resultsSessionId : 0);
        rebuild();
    }
}

// --- Pages -------------------------------------------------------------------

void ResultsScreen::rebuild()
{
    // The Court's lines point into the Court object: delete them first.
    lv_obj_clean(mPageObj);
    mCourt.reset();
    mHints.reset();

    if (!mModel.hasResults()) {
        message(mWaiting ? "Loading\xE2\x80\xA6" : "No data");
        mHints = std::make_unique<Hints>(mPageObj);
        mHints->set(Hints::Off, Hints::Off, Hints::Off, Hints::Dot);
        return;
    }

    pageHeader(kTitles[mPage]);
    switch (mPage) {
        case kCourt:      buildCourt(); break;
        case kCategories: buildCategories(); break;
        case kHighlights: buildHighlights(); break;
        case kTrend:      buildTrend(); break;
        default:          break;
    }
    mHints = std::make_unique<Hints>(mPageObj);
    mHints->set(kUpDown[mPage], kUpDown[mPage], Hints::Dot, Hints::Dot);
}

void ResultsScreen::pageHeader(const char* title)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%s%s%u / %u", title, kDot, static_cast<unsigned>(mPage + 1),
             static_cast<unsigned>(kPages));
    Theme::header(mPageObj, buf, kHeaderY[mPage], Theme::kDim);
}

void ResultsScreen::message(const char* text)
{
    Theme::label(mPageObj, Font::Regular16, text, 20, Theme::topFor(Font::Regular16, 120), 200,
                 LV_TEXT_ALIGN_CENTER, Theme::kDim);
}

void ResultsScreen::buildCourt()
{
    const score::SessionStats& st = mModel.results().stats;

    // Total "84/140 · " in white, "60 %" in its colour.
    char ma[24];
    char pct[12];
    fmtMa(ma, sizeof(ma), st.total);
    strncat(ma, kDot, sizeof(ma) - strlen(ma) - 1);
    fmtPct(pct, sizeof(pct), st.total);
    pair(mPageObj, Font::SemiBold25, ma, Theme::kText, Font::SemiBold25, pct, colorOf(st.total), kTotalY, 0);

    mCourt = std::make_unique<Court>(mPageObj, kCourtX, kCourtY, kCourtScale, kCourtLineW, kCourtSpotR);
    for (uint8_t i = 0; i < score::kSpotCount; ++i) {
        mCourt->setSpotColor(i, colorOf(st.spot[i]));
    }
    mCourt->setSelected(mSpot);

    // Caption "3PT · Top · 5/10 · 50 %"; long spot names drop the category.
    constexpr Font kCaptionFonts[] = { Font::Medium18, Font::Regular16, Font::Regular14 };
    if (mSpot == Court::kNone) {
        const char* none[] = { "No shots" };
        fitLabel(mPageObj, none, 1, kCaptionFonts, 1, 120 - kCaptionW / 2, kCaptionW, kCaptionY,
                 LV_TEXT_ALIGN_CENTER, Theme::kDim);
    } else {
        const score::SpotStat& s = st.spot[mSpot];
        char nums[48];
        fmtMa(ma, sizeof(ma), s);
        fmtPct(pct, sizeof(pct), s);
        snprintf(nums, sizeof(nums), "%s%s%s%s", kDot, ma, kDot, pct);
        const char* name = score::spotInfo(mSpot).name;
        char full[64];
        char brief[64];
        snprintf(full, sizeof(full), "%s%s", name, nums);
        snprintf(brief, sizeof(brief), "%s%s", spotShort(name), nums);
        const char* texts[] = { full, brief };
        fitLabel(mPageObj, texts, 2, kCaptionFonts, 3, 120 - kCaptionW / 2, kCaptionW, kCaptionY,
                 LV_TEXT_ALIGN_CENTER, Theme::kText);
    }

    // Legend: ● >=50  ● 35-49  ● <35 (the faces have neither ≥ nor the en dash).
    struct Entry { uint32_t color; const char* text; };
    constexpr Entry kLegend[] = { { Theme::kGood, ">=50" }, { Theme::kAmber, "35-49" }, { Theme::kBad, "<35" } };
    constexpr int32_t kDotGap = 3;    // dot .. text
    constexpr int32_t kItemGap = 10;  // text .. next dot
    int32_t total = 0;
    for (const Entry& e : kLegend) {
        total += 2 * kLegendDotR + kDotGap + Theme::textWidth(Font::Regular14, e.text) + kItemGap;
    }
    int32_t x = 120 - (total - kItemGap) / 2;
    for (const Entry& e : kLegend) {
        Theme::dot(mPageObj, x + kLegendDotR, kLegendY, kLegendDotR, e.color);
        x += 2 * kLegendDotR + kDotGap;
        const int32_t w = Theme::textWidth(Font::Regular14, e.text);
        Theme::label(mPageObj, Font::Regular14, e.text, x, Theme::topFor(Font::Regular14, kLegendY), w + 1,
                     LV_TEXT_ALIGN_LEFT, Theme::kDim);
        x += w + kItemGap;
    }
}

void ResultsScreen::buildCategories()
{
    const score::SessionStats& st = mModel.results().stats;

    for (uint8_t c = 0; c < score::kCategoryCount; ++c) {
        const score::SpotStat& s = st.cat[c];
        const int32_t y = kRowLabelY[c];

        // Right: "18/50" dim + "36 %" coloured, right-aligned to kRowRight,
        // on the name's baseline.
        char ma[16];
        char pct[12];
        fmtMa(ma, sizeof(ma), s);
        fmtPct(pct, sizeof(pct), s);
        const int32_t baseline = baselineFor(Font::Medium18, y);
        const int32_t wp = Theme::textWidth(Font::SemiBold20, pct);
        const int32_t wm = Theme::textWidth(Font::Regular14, ma);
        const int32_t xp = kRowRight - wp;
        const int32_t xm = xp - kRowGap - wm;
        Theme::label(mPageObj, Font::SemiBold20, pct, xp, Theme::topForBaseline(Font::SemiBold20, baseline), wp + 1,
                     LV_TEXT_ALIGN_LEFT, colorOf(s));
        Theme::label(mPageObj, Font::Regular14, ma, xm, Theme::topForBaseline(Font::Regular14, baseline), wm + 1,
                     LV_TEXT_ALIGN_LEFT, Theme::kDim);

        // Left: the name, short when the long one would reach the numbers.
        const auto cat = static_cast<score::Category>(c);
        const char* names[] = { score::categoryName(cat), score::categoryShort(cat) };
        constexpr Font kNameFont[] = { Font::Medium18 };
        fitLabel(mPageObj, names, 2, kNameFont, 1, kRowLeft, xm - 2 * kRowGap - kRowLeft, y, LV_TEXT_ALIGN_LEFT,
                 c == mCat ? Theme::kAccent : Theme::kText);

        // Bar: track and the percentage's share of it.
        const int32_t barW = kRowRight - kRowLeft;
        const int32_t barTop = kRowBarY[c] - kBarH / 2;
        Theme::box(mPageObj, kRowLeft, barTop, barW, kBarH, Theme::kLine, kBarH / 2);
        const int32_t fill = barW * score::pct(s.makes, s.attempts) / 100;
        if (fill > 0) {
            Theme::box(mPageObj, kRowLeft, barTop, fill < kBarH ? kBarH : fill, kBarH, colorOf(s), kBarH / 2);
        }
    }

    // Footer: the selected category's spots with attempts, as many as fit.
    char line[96] = "";
    bool cut = false;
    for (uint8_t i = 0; i < score::kSpotCount; ++i) {
        const score::SpotInfo& info = score::spotInfo(i);
        const score::SpotStat& s    = st.spot[i];
        if (static_cast<uint8_t>(info.category) != mCat || s.attempts == 0) {
            continue;
        }
        char entry[32];
        char ma[16];
        fmtMa(ma, sizeof(ma), s);
        snprintf(entry, sizeof(entry), "%s%s %s", line[0] ? kDot : "", spotShort(info.name), ma);
        char next[sizeof(line)];
        memcpy(next, line, sizeof(next));
        strncat(next, entry, sizeof(next) - strlen(next) - 1);
        if (strlen(next) > 63 || Theme::textWidth(Font::Regular14, next) > kFooterW) {
            cut = true;
            break;
        }
        memcpy(line, next, sizeof(line));
    }
    if (cut && Theme::textWidth(Font::Regular14, line) + Theme::textWidth(Font::Regular14, " \xE2\x80\xA6")
                   <= kFooterW) {
        strncat(line, " \xE2\x80\xA6", sizeof(line) - strlen(line) - 1);
    }
    const bool none = line[0] == '\0';
    lv_obj_t* l = Theme::label(mPageObj, Font::Regular14, none ? "No shots" : line, 120 - kFooterW / 2,
                               Theme::topFor(Font::Regular14, kFooterY), kFooterW, LV_TEXT_ALIGN_CENTER,
                               none ? Theme::kDim : Theme::kText);
    lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
}

void ResultsScreen::buildHighlights()
{
    const score::SessionStats& st = mModel.results().stats;
    char value[16];

    // STREAK: longest run of makes, and the category it ended in.
    snprintf(value, sizeof(value), "%u", static_cast<unsigned>(st.longestStreak));
    char inRow[32];
    char at[24];
    if (st.streakSpot < score::kSpotCount) {
        const char* cat = score::categoryShort(score::spotInfo(st.streakSpot).category);
        snprintf(inRow, sizeof(inRow), "in a row%s%s", kDot, cat);
        snprintf(at, sizeof(at), "at %s", cat);
    } else {
        snprintf(inRow, sizeof(inRow), "--");
        snprintf(at, sizeof(at), "--");
    }
    const char* streakSubs[] = { inRow, at };
    card(mPageObj, 0, 0, "STREAK", value, Theme::kAccent, streakSubs, 2);

    spotCard(mPageObj, 1, 0, "BEST SPOT", st.bestSpot, st, Theme::kGood);
    spotCard(mPageObj, 0, 1, "WORK ON", st.worstSpot, st, Theme::kBad);

    // TIME and pace.
    Theme::fmtDuration(value, sizeof(value), st.elapsed_s);
    char pace[24];
    char paceShort[16];
    const unsigned whole = st.shotsPerMin10 / 10u;
    const unsigned tenth = st.shotsPerMin10 % 10u;
    snprintf(pace, sizeof(pace), "%u.%u shots/min", whole, tenth);
    snprintf(paceShort, sizeof(paceShort), "%u.%u/min", whole, tenth);
    const char* paceSubs[] = { pace, paceShort };
    card(mPageObj, 1, 1, "TIME", value, Theme::kText, paceSubs, 2);

    // Heart rate: a red dot stands in for the heart (in no face).
    char hr[40];
    char hrShort[24];
    if (st.hrAvg == 0) {
        snprintf(hr, sizeof(hr), "no data");
        snprintf(hrShort, sizeof(hrShort), "no data");
    } else {
        snprintf(hr, sizeof(hr), "avg %u%smax %u bpm", static_cast<unsigned>(st.hrAvg), kDot,
                 static_cast<unsigned>(st.hrMax));
        snprintf(hrShort, sizeof(hrShort), "avg %u%smax %u", static_cast<unsigned>(st.hrAvg), kDot,
                 static_cast<unsigned>(st.hrMax));
    }
    constexpr int32_t kHeartGap = 5;
    const char* hrTexts[] = { hr, hrShort };
    constexpr Font kHrFont[] = { Font::Regular14 };
    const int32_t textW = kHrW - 2 * kHeartR - kHeartGap;
    lv_obj_t* l = fitLabel(mPageObj, hrTexts, 2, kHrFont, 1, 0, textW, kHrY, LV_TEXT_ALIGN_LEFT, Theme::kDim);
    const int32_t w = Theme::textWidth(Font::Regular14, lv_label_get_text(l),
                                       lv_obj_get_style_text_letter_space(l, LV_PART_MAIN));
    const int32_t x = 120 - (2 * kHeartR + kHeartGap + w) / 2;
    Theme::dot(mPageObj, x + kHeartR, kHrY, kHeartR, Theme::kBad);
    lv_obj_set_x(l, x + 2 * kHeartR + kHeartGap);
    lv_obj_set_width(l, w + 1);
}

void ResultsScreen::buildTrend()
{
    Theme::label(mPageObj, Font::Regular16, "No history yet", 20, Theme::topFor(Font::Regular16, 120), 200,
                 LV_TEXT_ALIGN_CENTER, Theme::kDim);
}

// --- Input -------------------------------------------------------------------

uint8_t ResultsScreen::spotWithAttempts(uint8_t from, int8_t dir) const
{
    const score::SessionStats& st = mModel.results().stats;
    for (uint8_t n = 1; n <= score::kSpotCount; ++n) {
        const uint8_t i = static_cast<uint8_t>((from + score::kSpotCount + dir * n) % score::kSpotCount);
        if (st.spot[i].attempts > 0) {
            return i;
        }
    }
    return Court::kNone;
}

void ResultsScreen::setPage(uint8_t page)
{
    mPage              = static_cast<uint8_t>(page % kPages);
    mModel.resultsPage = mPage;
    rebuild();
}

void ResultsScreen::step(int8_t dir)
{
    switch (mPage) {
        case kCourt:
            if (mSpot == Court::kNone) {
                return;
            }
            mSpot = spotWithAttempts(mSpot, dir);
            break;
        case kCategories:
            mCat = static_cast<uint8_t>((mCat + score::kCategoryCount + dir) % score::kCategoryCount);
            break;
        case kTrend:
            mTrendCat = static_cast<uint8_t>((mTrendCat + score::kCategoryCount + 1 + dir) % (score::kCategoryCount + 1));
            break;
        default:
            return;
    }
    rebuild();
}

void ResultsScreen::onKey(uint8_t code)
{
    const KeyInput::Action a = mKeys.feed(code);
    if (a.kind != KeyInput::Kind::Click) {
        return;
    }
    if (a.btn == KeyInput::Btn::Back) {
        ScreenManager::instance().goTo(mModel.resultsFrom == Model::ResultsFrom::History ? ScreenId::History
                                                                                          : ScreenId::Home);
        return;
    }
    if (!mModel.hasResults()) {
        return;
    }
    switch (a.btn) {
        case KeyInput::Btn::Select: setPage(static_cast<uint8_t>(mPage + 1)); break;
        case KeyInput::Btn::Up:     step(-1); break;
        case KeyInput::Btn::Down:   step(1); break;
        default:                    break;
    }
}

// --- Model events ------------------------------------------------------------

void ResultsScreen::onResults()
{
    mWaiting = false;
    mSpot    = mModel.hasResults() ? spotWithAttempts(score::kSpotCount - 1, 1) : Court::kNone;
    rebuild();
}

void ResultsScreen::onHistory()
{
    if (mPage == kTrend && mModel.hasResults()) {
        rebuild();
    }
}

void ResultsScreen::onSuspend()
{
    mKeys.cancel();
}
