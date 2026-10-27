#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <cmath>
#include <vector>

namespace ms
{
// ------------------------------------------------------------------ data tables
struct Mode { const char* name; std::vector<int> iv; };

inline const std::vector<Mode>& modes()
{
    static const std::vector<Mode> m = {
        {"Majeur",{0,4,7}},{"Mineur",{0,3,7}},{"7 (dominante)",{0,4,7,10}},{"Maj7",{0,4,7,11}},{"Min7",{0,3,7,10}},
        {"Min7b5",{0,3,6,10}},{"Diminue",{0,3,6}},{"Dim7",{0,3,6,9}},{"Augmente",{0,4,8}},{"Sus2",{0,2,7}},{"Sus4",{0,5,7}},
        {"6",{0,4,7,9}},{"Min6",{0,3,7,9}},{"9",{0,4,7,10,14}},{"Maj9",{0,4,7,11,14}},{"Min9",{0,3,7,10,14}},{"Add9",{0,4,7,14}},
        {"Ionien (majeur)",{0,2,4,5,7,9,11}},{"Dorien",{0,2,3,5,7,9,10}},{"Phrygien",{0,1,3,5,7,8,10}},{"Lydien",{0,2,4,6,7,9,11}},
        {"Mixolydien",{0,2,4,5,7,9,10}},{"Eolien (mineur naturel)",{0,2,3,5,7,8,10}},{"Locrien",{0,1,3,5,6,8,10}},
        {"Mineur harmonique",{0,2,3,5,7,8,11}},{"Mineur melodique",{0,2,3,5,7,9,11}},{"Pentatonique majeure",{0,2,4,7,9}},
        {"Pentatonique mineure",{0,3,5,7,10}},{"Blues",{0,3,5,6,7,10}},{"Gamme par tons",{0,2,4,6,8,10}} };
    return m;
}
inline const char* noteNames(int i)
{
    static const char* n[12] = {"Do","Do#","Re","Re#","Mi","Fa","Fa#","Sol","Sol#","La","La#","Si"};
    return n[((i % 12) + 12) % 12];
}
struct Formant { float f, q, lv; };
inline const std::array<std::array<Formant,3>,6>& vowels()
{
    static const std::array<std::array<Formant,3>,6> v = {{
        {{{650,8,1.f},{1000,10,.85f},{2500,14,.25f}}},   // x : formants measured on the voice sample
        {{{850,10,1.f},{1200,12,.7f},{2800,14,.4f}}},    // a
        {{{400,10,1.f},{1700,12,.6f},{2600,14,.4f}}},    // e
        {{{270,10,1.f},{2100,12,.5f},{3000,14,.4f}}},    // i
        {{{450,10,1.f},{800,12,.7f},{2800,14,.3f}}},     // o
        {{{325,10,1.f},{700,12,.6f},{2700,14,.25f}}} }}; // u
    return v;
}

// ------------------------------------------------------------------ parameters snapshot
struct Params
{
    int wave = 0, uni = 3, vowel = 0;
    float det = 16, sub = .15f, cut = 3400, res = 2, fenv = 3800, fdec = .16f, a = .004f, d = .35f, s = .45f, r = .45f;
    float glide = 0, form = 0, drive = 0, chor = .85f, dly = .28f, vol = .7f;
};

// ------------------------------------------------------------------ small DSP blocks
struct SVF // TPT state-variable filter, outputs lowpass and bandpass
{
    float s1 = 0, s2 = 0;
    void reset() { s1 = s2 = 0; }
    inline void process (float x, float g, float k, float& lp, float& bp)
    {
        const float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.f * v1 - s1; s2 = 2.f * v2 - s2; lp = v2; bp = v1;
    }
};
inline float blep (float t, float dt)
{
    if (t < dt)       { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}

// ------------------------------------------------------------------ voice
struct Voice
{
    bool active = false, isArp = false, held = false;
    int note = -1, arpRemain = 0, stage = 0; // 0 attack 1 decay/sustain 2 release
    juce::uint64 age = 0;
    float vel = 1, level = 0, fe = 1, base = 1000, peak = 1000;
    float freqCur = 440, freqTarget = 440, ph[3] {}, subPh = 0;
    SVF fL, fR;

    void start (float f, float fromF, float velocity, const Params& P, float sr, int nUni, juce::Random& rnd)
    {
        active = true; stage = 0; level = 0; fe = 1; vel = velocity;
        freqTarget = f; freqCur = (P.glide > 0.f && fromF > 0.f) ? fromF : f;
        base = P.cut * std::pow (f / 440.f, 0.4f);
        peak = juce::jmin (base + P.fenv * vel, 18000.f);
        for (int i = 0; i < nUni; ++i) ph[i] = rnd.nextFloat();
        subPh = 0; fL.reset(); fR.reset(); (void) sr;
    }
    void release() { if (active) stage = 2; }
};

// Per-block constants shared by all voices
struct BlockCtx
{
    float sr = 44100, glideCoef = 1, feCoef = .9f, dCoef = .9f, rCoef = .9f, attInc = 1, k = 1, norm = 1;
    int n = 3;
    float ratio[3] {1,1,1}, gl[3] {1,1,1}, gr[3] {1,1,1};
};

inline void prepareBlock (BlockCtx& b, const Params& P, float sr)
{
    b.sr = sr; b.n = juce::jlimit (1, 3, P.uni); b.norm = 1.f / std::sqrt ((float) b.n);
    b.glideCoef = P.glide > 0.001f ? 1.f - std::exp (-1.f / (sr * P.glide / 3.f)) : 1.f;
    b.feCoef = std::exp (-1.f / (sr * juce::jmax (P.fdec, .01f) / 3.f));
    b.dCoef = 1.f - std::exp (-1.f / (sr * juce::jmax (P.d, .01f) / 3.f));
    b.rCoef = 1.f - std::exp (-1.f / (sr * juce::jmax (P.r, .02f) / 3.f));
    b.attInc = 1.f / (sr * juce::jmax (P.a, .001f));
    b.k = 1.f / std::pow (10.f, P.res / 20.f); // Web Audio style: resonance in dB
    for (int i = 0; i < b.n; ++i)
    {
        const float sp = b.n == 1 ? 0.f : ((float) i / (float) (b.n - 1)) * 2.f - 1.f;
        b.ratio[i] = std::pow (2.f, sp * P.det / 1200.f);
        const float pan = sp * .95f, ang = (pan + 1.f) * juce::MathConstants<float>::pi / 4.f;
        b.gl[i] = std::cos (ang); b.gr[i] = std::sin (ang);
    }
}

// renders one sample; returns false when the voice has finished
inline bool renderVoice (Voice& v, const Params& P, const BlockCtx& b, float& mL, float& mR, float& sub)
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
    v.freqCur += (v.freqTarget - v.freqCur) * b.glideCoef;
    float L = 0, R = 0;
    for (int i = 0; i < b.n; ++i)
    {
        const float dt = juce::jmin (v.freqCur * b.ratio[i] / b.sr, .45f);
        v.ph[i] += dt; if (v.ph[i] >= 1.f) v.ph[i] -= 1.f;
        const float p = v.ph[i]; float o;
        switch (P.wave)
        {
            case 0: o = 2.f * p - 1.f - blep (p, dt); break;
            case 1: { o = p < .5f ? 1.f : -1.f; o += blep (p, dt); float p2 = p + .5f; if (p2 >= 1.f) p2 -= 1.f; o -= blep (p2, dt); break; }
            case 2: o = 2.f * std::abs (2.f * p - 1.f) - 1.f; break;
            default: o = std::sin (twoPi * p); break;
        }
        o *= b.norm; L += o * b.gl[i]; R += o * b.gr[i];
    }
    // filter
    v.fe *= b.feCoef;
    const float fc = juce::jlimit (20.f, juce::jmin (18000.f, b.sr * .45f), v.base + (v.peak - v.base) * v.fe);
    const float g = std::tan (juce::MathConstants<float>::pi * fc / b.sr);
    float lpL, bpL, lpR, bpR;
    v.fL.process (L, g, b.k, lpL, bpL); v.fR.process (R, g, b.k, lpR, bpR);
    // envelope
    switch (v.stage)
    {
        case 0: v.level += b.attInc; if (v.level >= 1.f) { v.level = 1.f; v.stage = 1; } break;
        case 1: v.level += (P.s - v.level) * b.dCoef; break;
        default: v.level -= v.level * b.rCoef; if (v.level < 1e-4f) { v.active = false; return false; } break;
    }
    const float amp = v.level * v.vel * .32f;
    mL += lpL * amp; mR += lpR * amp;
    if (P.sub > 0.f)
    {
        v.subPh += v.freqCur * .5f / b.sr; if (v.subPh >= 1.f) v.subPh -= 1.f;
        sub += std::sin (twoPi * v.subPh) * P.sub * amp;
    }
    return true;
}

// ------------------------------------------------------------------ effects
struct Effects
{
    float sr = 44100;
    std::vector<float> chL, chR, dlL, dlR;
    int chPos = 0, dlPos = 0;
    float lfo1 = 0, lfo2 = 0, dlTime = 20000, fbLP = 0, fbLP2 = 0;
    SVF fm[2][3];

    void prepare (float sampleRate)
    {
        sr = sampleRate;
        chL.assign ((size_t) (sr * .1f) + 4, 0.f); chR = chL;
        dlL.assign ((size_t) (sr * 2.2f) + 4, 0.f); dlR = dlL;
        chPos = dlPos = 0; fbLP = fbLP2 = 0; dlTime = sr * .5f;
        for (auto& c : fm) for (auto& f : c) f.reset();
    }
    static float readLin (const std::vector<float>& buf, int pos, float delay)
    {
        const int n = (int) buf.size();
        float rp = (float) pos - delay; while (rp < 0) rp += (float) n;
        const int i0 = (int) rp % n, i1 = (i0 + 1) % n; const float fr = rp - std::floor (rp);
        return buf[(size_t) i0] * (1.f - fr) + buf[(size_t) i1] * fr;
    }
    // in: filtered voices (main) and sub, per channel
    void process (const Params& P, float bpm, float mL, float mR, float sub, float& outL, float& outR)
    {
        // formants (parallel band-passes on the main path)
        float L = mL, R = mR;
        if (P.form > 0.001f)
        {
            const auto& vw = vowels()[(size_t) juce::jlimit (0, 5, P.vowel)];
            float aL = mL * (1.f - P.form), aR = mR * (1.f - P.form);
            const float pi = juce::MathConstants<float>::pi;
            for (int i = 0; i < 3; ++i)
            {
                const float g = std::tan (pi * vw[(size_t) i].f / sr), k = 1.f / vw[(size_t) i].q, gain = P.form * vw[(size_t) i].lv * 3.f;
                float lp, bp;
                fm[0][i].process (mL, g, k, lp, bp); aL += bp * k * gain;
                fm[1][i].process (mR, g, k, lp, bp); aR += bp * k * gain;
            }
            L = aL; R = aR;
        }
        L += sub; R += sub;
        // saturation
        if (P.drive > 0.001f)
        {
            const float k = 1.f + P.drive * 25.f, comp = .4f / (1.f + P.drive) / std::tanh (k);
            L = std::tanh (k * L) * comp; R = std::tanh (k * R) * comp;
        }
        const float dryL = L, dryR = R, mid = .5f * (L + R);
        // chorus
        const float twoPi = juce::MathConstants<float>::twoPi;
        lfo1 += twoPi * .31f / sr; lfo2 += twoPi * .43f / sr;
        if (lfo1 > twoPi) lfo1 -= twoPi;
        if (lfo2 > twoPi) lfo2 -= twoPi;
        chL[(size_t) chPos] = mid; chR[(size_t) chPos] = mid;
        const float c1 = readLin (chL, chPos, (.014f + .004f * std::sin (lfo1)) * sr);
        const float c2 = readLin (chR, chPos, (.019f + .004f * std::sin (lfo2)) * sr);
        chPos = (chPos + 1) % (int) chL.size();
        float oL = dryL + c1 * P.chor * .7f * 1.4f, oR = dryR + c2 * P.chor * .7f * 1.4f;
        // ping-pong delay (dotted eighth)
        const float target = juce::jlimit (10.f, sr * 2.f, 60.f / bpm * .75f * sr);
        dlTime += (target - dlTime) * .001f;
        const float dL = readLin (dlL, dlPos, dlTime), dR = readLin (dlR, dlPos, dlTime);
        fbLP += .33f * (dL * .45f - fbLP);
        dlL[(size_t) dlPos] = mid * P.dly + dR * .45f;
        dlR[(size_t) dlPos] = fbLP;
        dlPos = (dlPos + 1) % (int) dlL.size();
        oL += dL * .988f + dR * .156f; oR += dL * .156f + dR * .988f;
        const float out = P.vol * .8f;
        oL *= out; oR *= out;
        auto soft = [] (float x) { const float a = std::abs (x); if (a < .9f) return x; const float y = .9f + .1f * std::tanh ((a - .9f) / .1f); return x < 0 ? -y : y; };
        outL = soft (oL); outR = soft (oR);
    }
};
} // namespace ms
