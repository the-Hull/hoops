#include "score/Stats.hpp"

namespace score {

uint8_t pct(uint16_t makes, uint16_t attempts)
{
    if (attempts == 0) return 0;
    return static_cast<uint8_t>((makes * 100u + attempts / 2u) / attempts);
}

SessionStats SessionStats::compute(const Session& s)
{
    SessionStats st{};
    st.streakSpot = kNoSpot;
    st.bestSpot   = kNoSpot;
    st.worstSpot  = kNoSpot;

    const uint16_t n = s.count < kMaxShots ? s.count : kMaxShots;
    uint16_t run = 0;
    for (uint16_t i = 0; i < n; ++i) {
        const Shot& sh = s.shots[i];
        const uint8_t sp = sh.spot < kSpotCount ? sh.spot : kSpotCount - 1;   // clamp like spotInfo()
        const auto c = static_cast<uint8_t>(spotInfo(sp).category);
        const uint16_t made = sh.made ? 1 : 0;
        st.spot[sp].attempts++; st.spot[sp].makes += made;
        st.cat[c].attempts++;   st.cat[c].makes   += made;
        st.total.attempts++;    st.total.makes    += made;
        if (!made) { run = 0; continue; }
        if (++run > st.longestStreak) { st.longestStreak = run; st.streakSpot = sp; }
    }

    int best = -1, worst = 101;
    for (uint8_t i = 0; i < kSpotCount; ++i) {
        if (st.spot[i].attempts < kMinAttemptsForRank) continue;
        const int p = pct(st.spot[i].makes, st.spot[i].attempts);
        if (p > best)  { best = p;  st.bestSpot = i; }
        if (p < worst) { worst = p; st.worstSpot = i; }
    }

    st.elapsed_s = s.elapsed_s;
    if (s.elapsed_s) {
        const uint32_t spm = static_cast<uint32_t>(s.count) * 600u / s.elapsed_s;
        st.shotsPerMin10 = static_cast<uint16_t>(spm < 0xFFFFu ? spm : 0xFFFFu);
    }
    st.hrAvg = s.hrN ? static_cast<uint8_t>(s.hrSum / s.hrN) : 0;
    st.hrMax = s.hrMax;
    return st;
}

} // namespace score
