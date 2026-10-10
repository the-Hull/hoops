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

// Page 4
constexpr uint8_t kTrendTotal = score::kCategoryCount;   // after the categories
constexpr uint8_t kBars       = score::kTrendCount;
constexpr int32_t kTrendCatY  = 58;
constexpr int32_t kTrendPctY  = 86;   // mockup 90: clear of a 100 % bar's label
constexpr int32_t kChartLeft  = 38;
constexpr int32_t kChartRight = 202;
constexpr int32_t kChartBase  = 190;   // bars stand on this line
constexpr int32_t kBarX[kBars] = { 50, 85, 120, 155, 190 };   // newest on the right
constexpr int32_t kBarW       = 19;
constexpr int32_t kBarR       = 3;
constexpr int32_t kPlotH1000  = 768;   // px per 1000 % (77 px for 100 %)
constexpr int32_t kSlotW      = 35;
constexpr int32_t kDateGap    = 4;     // between neighbouring dates
constexpr int32_t kTodayExtra = 8;     // the newest date may spill right
constexpr int32_t kValueGap   = 8;     // bar top .. value label centre
constexpr int32_t kDateY      = 202;

constexpr const char* kDot = Theme::kMidDot;

// --- Text helpers ------------------------------------------------------------

using Theme::fitLabel;
using Theme::pair;
using Theme::spotShort;
using Theme::baselineFor;

void fmtMa(char* out, size_t n, const score::SpotStat& s)
{
    snprintf(out, n, "%u/%u", static_cast<unsigned>(s.makes), static_cast<unsigned>(s.attempts));
}

/// "56 %", or "--" with no attempts.
void fmtPct(char* out, size_t n, const score::SpotStat& s)
{
    Theme::fmtPct(out, n, s.makes, s.attempts);
}

/// One bar of the trend: a session's percentage in the shown category.
struct TrendPoint {
    uint8_t  pct;
    bool     has;   ///< attempts in the category
    uint32_t ymd;
};

/// Trend values of a saved session (category, or kTrendTotal). The total
/// is the service's totalPct; the category sum only tells whether there
/// were attempts.
TrendPoint trendPoint(const score::SessionSummary& it, uint8_t cat)
{
    if (cat < score::kCategoryCount) {
        return { score::pct(it.catMakes[cat], it.catAtt[cat]), it.catAtt[cat] > 0, it.ymd };
    }
    uint32_t att = 0;
    for (uint8_t c = 0; c < score::kCategoryCount; ++c) {
        att += it.catAtt[c];
    }
    return { it.totalPct, att > 0, it.ymd };
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
    , mKeys(&Screen::keyAction, this)
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
    mModel.requestHistory();   // the trend's older sessions
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

    // Caption "3PT · Top 5/10 · 50 %"; long spot names drop the category.
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
        snprintf(nums, sizeof(nums), " %s%s%s", ma, kDot, pct);
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
    const Entry legend[] = { { Theme::kGood, ">=50" }, { Theme::kAmber, "35-49" }, { Theme::kBad, "<35" } };
    constexpr int32_t kDotGap = 3;    // dot .. text
    constexpr int32_t kItemGap = 10;  // text .. next dot
    int32_t total = 0;
    for (const Entry& e : legend) {
        total += 2 * kLegendDotR + kDotGap + Theme::textWidth(Font::Regular14, e.text) + kItemGap;
    }
    int32_t x = 120 - (total - kItemGap) / 2;
    for (const Entry& e : legend) {
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

    // STREAK: longest run of makes, and the spot it was made at (its
    // category when the spot does not fit).
    snprintf(value, sizeof(value), "%u", static_cast<unsigned>(st.longestStreak));
    char inRow[32];
    char at[24];
    char inRowCat[32];
    char atCat[24];
    if (st.streakSpot < score::kSpotCount) {
        const score::SpotInfo& info = score::spotInfo(st.streakSpot);
        const char* shortName = spotShort(info.name);
        const char* spot = strlen(shortName) > 1 ? shortName : info.name;   // "Layup · L", not "L"
        const char* cat  = score::categoryShort(info.category);
        snprintf(inRow, sizeof(inRow), "in a row%s%s", kDot, spot);
        snprintf(at, sizeof(at), "at %s", spot);
        snprintf(inRowCat, sizeof(inRowCat), "in a row%s%s", kDot, cat);
        snprintf(atCat, sizeof(atCat), "at %s", cat);
    } else {
        snprintf(inRow, sizeof(inRow), "--");
        snprintf(at, sizeof(at), "--");
        snprintf(inRowCat, sizeof(inRowCat), "--");
        snprintf(atCat, sizeof(atCat), "--");
    }
    const char* streakSubs[] = { inRow, at, inRowCat, atCat };
    card(mPageObj, 0, 0, "STREAK", value, Theme::kAccent, streakSubs, 4);

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
    const CustomMessage::Results& r = mModel.results();

    // Category name, upper case as in the mockup.
    char name[24];
    snprintf(name, sizeof(name), "%s",
             mTrendCat < kTrendTotal ? score::categoryName(static_cast<score::Category>(mTrendCat)) : "Total");
    for (char* p = name; *p; ++p) {
        if (*p >= 'a' && *p <= 'z') {
            *p = static_cast<char>(*p - 'a' + 'A');
        }
    }
    Theme::label(mPageObj, Font::SemiBold20, name, 20, Theme::topFor(Font::SemiBold20, kTrendCatY), 200,
                 LV_TEXT_ALIGN_CENTER, Theme::kAccent);

    // The newest bar is the session viewed; then the older history rows
    // (the kBars - 1 after it in the newest-first list, or the newest kBars
    // when it is not listed) that have attempts in the category.
    const score::SpotStat& cs = mTrendCat < kTrendTotal ? r.stats.cat[mTrendCat] : r.stats.total;
    const TrendPoint current = { score::pct(cs.makes, cs.attempts), cs.attempts > 0, r.ymd };
    TrendPoint older[kBars - 1] = {};
    uint8_t    nOlder = 0;
    if (mModel.hasHistory()) {
        const uint8_t count = mModel.historyCount();
        uint8_t first = 0;
        uint8_t end   = kBars;
        for (uint8_t i = 0; i < count; ++i) {
            if (mModel.historyItem(i).id == r.sessionId) {
                first = static_cast<uint8_t>(i + 1);
                end   = static_cast<uint8_t>(first + kBars - 1);
                break;
            }
        }
        if (end > count) {
            end = count;
        }
        for (uint8_t i = first; i < end && nOlder < kBars - 1; ++i) {
            const score::SessionSummary& it = mModel.historyItem(i);
            const TrendPoint p = trendPoint(it, mTrendCat);
            if (p.has && it.id != r.sessionId) {
                older[nOlder++] = p;
            }
        }
    }

    // "60 %" and, against the nearest older session, "+6 vs last".
    char pct[12];
    if (current.has) {
        snprintf(pct, sizeof(pct), "%u %%", static_cast<unsigned>(current.pct));
    } else {
        snprintf(pct, sizeof(pct), "--");
    }
    if (current.has && nOlder > 0) {
        const int delta = static_cast<int>(current.pct) - static_cast<int>(older[0].pct);
        char vs[20];
        snprintf(vs, sizeof(vs), "%+d vs last", delta);
        const uint32_t c = delta > 0 ? Theme::kGood : (delta < 0 ? Theme::kBad : Theme::kDim);
        pair(mPageObj, Font::SemiBold35, pct, Theme::kText, Font::SemiBold20, vs, c, kTrendPctY, 8);
    } else {
        Theme::label(mPageObj, Font::SemiBold35, pct, 20, Theme::topFor(Font::SemiBold35, kTrendPctY), 200,
                     LV_TEXT_ALIGN_CENTER, current.has ? Theme::kText : Theme::kDim);
    }

    // Chart.
    Theme::box(mPageObj, kChartLeft, kChartBase, kChartRight - kChartLeft, 2, Theme::kLine);
    const uint32_t today = mModel.todayYmd();
    for (uint8_t b = 0; b <= nOlder; ++b) {
        const bool        newest = b == 0;
        const TrendPoint& p      = newest ? current : older[b - 1];
        const int32_t     x      = kBarX[kBars - 1 - b];
        const uint32_t    color  = newest ? Theme::kAccent : Theme::kDim;
        const uint32_t    text   = newest ? Theme::kText : Theme::kDim;

        int32_t h = 0;
        if (p.has) {
            h = p.pct * kPlotH1000 / 1000;
            h = h < 2 ? 2 : h;
            Theme::box(mPageObj, x - kBarW / 2, kChartBase - h, kBarW, h, color, kBarR);
        }
        char value[8];
        snprintf(value, sizeof(value), p.has ? "%u" : "--", static_cast<unsigned>(p.pct));
        Theme::label(mPageObj, Font::Regular14, value, x - kSlotW / 2,
                     Theme::topFor(Font::Regular14, kChartBase - h - kValueGap), kSlotW, LV_TEXT_ALIGN_CENTER, text);

        // Date under the newest bar of each day: "Today", "6.10.", or
        // shorter forms when the slot is narrow.
        const TrendPoint* newer = b == 0 ? nullptr : (b == 1 ? &current : &older[b - 2]);
        if (newer && newer->ymd == p.ymd) {
            continue;
        }

        char full[12];
        Theme::fmtDate(full, sizeof(full), p.ymd);
        char noDot[12];
        snprintf(noDot, sizeof(noDot), "%s", full);
        const size_t len = strlen(noDot);
        if (len > 1 && noDot[len - 1] == '.') {
            noDot[len - 1] = '\0';
        }
        char day[12];
        snprintf(day, sizeof(day), "%s", full);
        if (char* dot = strchr(day, '.')) {
            dot[1] = '\0';
        }
        const bool isToday = newest && today != 0 && p.ymd == today;
        const char* dates[] = { isToday ? "Today" : full, full, noDot, day };
        constexpr Font kDateFont[] = { Font::Regular14 };
        const int32_t w = kSlotW - kDateGap + (newest ? kTodayExtra : 0);
        fitLabel(mPageObj, dates, 4, kDateFont, 1, newest ? x - kSlotW / 2 : x - w / 2, w, kDateY,
                 LV_TEXT_ALIGN_CENTER, text);
    }
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
            mTrendCat = static_cast<uint8_t>((mTrendCat + kTrendTotal + 1 + dir) % (kTrendTotal + 1));
            break;
        default:
            return;
    }
    rebuild();
}

void ResultsScreen::onKey(uint8_t code)
{
    onAction(mKeys.feed(code));
}

void ResultsScreen::onAction(KeyInput::Action a)
{
    if (!KeyInput::isActivate(a)) {
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
    if (mModel.resultsFrom == Model::ResultsFrom::History && mModel.hasResults()
        && mModel.results().sessionId != mModel.resultsSessionId) {
        // A reply to an earlier selection: drop it and keep waiting for ours.
        mModel.clearResults();
        return;
    }
    mWaiting = false;
    mSpot    = mModel.hasResults() ? spotWithAttempts(score::kSpotCount - 1, 1) : Court::kNone;
    rebuild();
}

void ResultsScreen::onHistory()
{
    // Pages stream in a burst: redraw on the first (the newest rows) and on
    // the last (an older session's neighbours), not on every one.
    if (mPage == kTrend && mModel.hasResults()
        && (mModel.historyCount() <= score::kTrendCount || mModel.historyComplete())) {
        rebuild();
    }
}

void ResultsScreen::onSuspend()
{
    mKeys.cancel();
}
