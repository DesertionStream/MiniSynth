#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace
{
struct Preset { const char* name; int wave, uni; float det, sub, cut, res, fenv, fdec, a, d, s, r, chor, dly, vol, glide, form; int vowel; float drive; };
const Preset presets[] = {
 {"Extrait",   0,3,16,.15f,3400,2,3800,.16f,.004f,.35f,.45f,.45f,.85f,.28f,.70f,0,0,1,0},
 {"Pluck",     0,2,8, 0,   1500,3,4500,.12f,.002f,.22f,0,   .25f,.35f,.30f,.70f,0,0,1,0},
 {"Cloche",    2,2,6, 0,   5200,1,1500,.30f,.002f,.80f,0,   .90f,.50f,.35f,.70f,0,0,1,0},
 {"Basse",     0,1,0, .8f, 420, 4,1800,.14f,.004f,.30f,.60f,.12f,0,   0,    .75f,0,0,1,0},
 {"Cyberpunk", 0,3,30,.30f,1600,9,5200,.10f,.003f,.25f,.50f,.30f,.90f,.45f,.50f,.07f,0,1,.55f},
 {"Voix",      0,3,12,.50f,7000,1,0,   .10f,.35f, .50f,.85f,1.2f,.80f,.30f,.38f,.04f,1,0,0},
 {"Nappe",     0,3,22,.15f,1400,1,900, .80f,.45f, .60f,.80f,1.4f,.85f,.25f,.60f,0,0,1,0},
};
const int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));
}

AudioProcessorValueTreeState::ParameterLayout MiniSynthProcessor::createLayout()
{
    using PID = ParameterID;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto F = [&] (const char* id, const char* name, float lo, float hi, float def, float skew = 1.f, float step = 0.f)
    { p.push_back (std::make_unique<AudioParameterFloat> (PID { id, 1 }, name, NormalisableRange<float> (lo, hi, step, skew), def)); };

    StringArray noteList; for (int i = 0; i < 12; ++i) noteList.add (ms::noteNames (i));
    StringArray modeList; for (auto& m : ms::modes()) modeList.add (m.name);

    // --- arpeggiator
    p.push_back (std::make_unique<AudioParameterBool> (PID { "arpOn", 1 }, "Arpege", true));
    p.push_back (std::make_unique<AudioParameterBool> (PID { "sync", 1 }, "Sync hote (tempo + lecture)", true));
    F ("bpm", "Tempo (si sync off)", 50.f, 300.f, 158.f, 1.f, 1.f);
    p.push_back (std::make_unique<AudioParameterChoice> (PID { "pattern", 1 }, "Motif", StringArray { "Montee-descente", "Montee", "Descente", "Sauts", "Aleatoire" }, 0));
    p.push_back (std::make_unique<AudioParameterInt> (PID { "octs", 1 }, "Octaves", 1, 3, 2));
    F ("gate", "Duree note", .2f, 1.6f, 1.1f);
    p.push_back (std::make_unique<AudioParameterInt> (PID { "bars", 1 }, "Mesures par accord", 1, 4, 2));
    const int defRoot[4] = { 9, 9, 7, 5 }, defMode[4] = { 1, 1, 0, 0 };
    for (int i = 0; i < 4; ++i)
    {
        p.push_back (std::make_unique<AudioParameterChoice> (PID { String ("root") + String (i + 1), 1 }, "Accord " + String (i + 1) + " - note", noteList, defRoot[i]));
        p.push_back (std::make_unique<AudioParameterChoice> (PID { String ("mode") + String (i + 1), 1 }, "Accord " + String (i + 1) + " - mode", modeList, defMode[i]));
    }
    // --- sound
    p.push_back (std::make_unique<AudioParameterChoice> (PID { "wave", 1 }, "Forme d'onde", StringArray { "Dent de scie", "Carre", "Triangle", "Sinus" }, 0));
    p.push_back (std::make_unique<AudioParameterInt> (PID { "uni", 1 }, "Voix (unisson)", 1, 3, 3));
    F ("det", "Desaccord (ct)", 0, 40, 16);
    F ("sub", "Sous-oscillateur", 0, 1, .15f);
    F ("cut", "Coupure filtre", 100, 9000, 3400, .4f);
    F ("res", "Resonance", .5f, 14, 2);
    F ("fenv", "Enveloppe filtre", 0, 8000, 3800, .5f);
    F ("fdec", "Declin filtre", .02f, 1.5f, .16f, .5f);
    F ("a", "Attaque", .001f, 1.5f, .004f, .35f);
    F ("d", "Declin", .02f, 2, .35f, .5f);
    F ("s", "Soutien", 0, 1, .45f);
    F ("r", "Relache", .02f, 3, .45f, .5f);
    F ("glide", "Glissando", 0, .5f, 0, .5f);
    p.push_back (std::make_unique<AudioParameterChoice> (PID { "vowel", 1 }, "Voyelle", StringArray { "Extrait voix", "A", "E", "I", "O", "OU" }, 0));
    F ("form", "Formants (voix)", 0, 1, 0);
    F ("drive", "Saturation", 0, 1, 0);
    F ("chor", "Chorus stereo", 0, 1, .85f);
    F ("dly", "Echo ping-pong", 0, .8f, .28f);
    F ("vol", "Volume", 0, 1, .7f);
    return { p.begin(), p.end() };
}

MiniSynthProcessor::MiniSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

void MiniSynthProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate; fx.prepare ((float) sr);
    for (auto& v : voices) v.active = false;
    lastFreq = 0; arpWasActive = false;
}

void MiniSynthProcessor::readParams()
{
    P.wave = (int) get ("wave"); P.uni = (int) get ("uni"); P.vowel = (int) get ("vowel");
    P.det = get ("det"); P.sub = get ("sub"); P.cut = get ("cut"); P.res = get ("res"); P.fenv = get ("fenv"); P.fdec = get ("fdec");
    P.a = get ("a"); P.d = get ("d"); P.s = get ("s"); P.r = get ("r"); P.glide = get ("glide"); P.form = get ("form");
    P.drive = get ("drive"); P.chor = get ("chor"); P.dly = get ("dly"); P.vol = get ("vol");
    arpOn = get ("arpOn") > .5f; arpSync = get ("sync") > .5f; arpPattern = (int) get ("pattern"); arpOcts = (int) get ("octs");
    arpGate = get ("gate"); arpBars = (int) get ("bars");
    for (int i = 0; i < 4; ++i)
    {
        roots[i] = (int) get ((String ("root") + String (i + 1)).toRawUTF8());
        modeIdx[i] = (int) get ((String ("mode") + String (i + 1)).toRawUTF8());
    }
}

void MiniSynthProcessor::triggerNote (int midi, float vel, bool arp, int arpSamples)
{
    ms::Voice* v = nullptr;
    for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr) { // steal the oldest voice
        v = &voices[0]; for (auto& x : voices) if (x.age < v->age) v = &x;
    }
    const float f = 440.f * std::pow (2.f, ((float) midi - 69.f) / 12.f);
    v->start (f, lastFreq, vel, P, (float) sr, bc.n, rnd);
    v->note = midi; v->isArp = arp; v->held = ! arp; v->arpRemain = arpSamples; v->age = ++ageCounter;
    lastFreq = f;
}

void MiniSynthProcessor::releaseArpVoices() { for (auto& v : voices) if (v.active && v.isArp) v.release(); }

void MiniSynthProcessor::arpStep (juce::int64 stepIdx)
{
    const int perChord = 16 * arpBars;
    const int ci = (int) ((stepIdx / perChord) % 4);
    const auto& iv = ms::modes()[(size_t) modeIdx[ci]].iv;
    const int root = 60 + roots[ci];
    std::vector<int> notes;
    for (int o = 0; o < arpOcts; ++o) for (int i : iv) if (i + 12 * o <= 12 * arpOcts + 4) notes.push_back (root + i + 12 * o);
    if (notes.empty()) return;
    const int len = (int) notes.size(); int idx = 0;
    const int step = (int) (stepIdx % 100000);
    switch (arpPattern)
    {
        case 0: { const int L = jmax (1, 2 * len - 2), k = step % L; idx = k < len ? k : L - k; break; }
        case 1: idx = step % len; break;
        case 2: idx = len - 1 - (step % len); break;
        case 3: { static const int seq[4] = { 0, 2, 1, 3 }; idx = (seq[step % 4] + 2 * ((step % (len * 2)) / 4)) % len; break; }
        default: idx = rnd.nextInt (len); break;
    }
    const float accent = (stepIdx % 4 == 0) ? 1.f : .8f;
    triggerNote (notes[(size_t) idx], accent, true, (int) (samplesPerStep * arpGate));
}

void MiniSynthProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    readParams();
    ms::prepareBlock (bc, P, (float) sr);

    // tempo + transport
    bool hostPlaying = false; double ppq = 0, hostBpm = 0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            hostPlaying = pos->getIsPlaying();
            if (auto b = pos->getBpm()) hostBpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
        }
    bpmNow = (arpSync && hostBpm > 1.0) ? hostBpm : (double) get ("bpm");
    samplesPerStep = sr * 60.0 / bpmNow / 4.0;

    const bool arpActive = arpOn && (! arpSync || hostPlaying);
    if (arpActive && arpSync)
    {
        const double stepPos = ppq * 4.0, next = std::ceil (stepPos - 1e-9);
        arpStepIdx = (juce::int64) next; arpCountdown = (next - stepPos) * samplesPerStep;
    }
    else if (arpActive && ! arpWasActive) { arpStepIdx = 0; arpCountdown = 0; }
    if (! arpActive && arpWasActive) releaseArpVoices();
    arpWasActive = arpActive;

    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getWritePointer (1);
    auto it = midi.begin();

    for (int s = 0; s < n; ++s)
    {
        while (it != midi.end() && (*it).samplePosition <= s)
        {
            const auto m = (*it).getMessage(); ++it;
            if (m.isNoteOn()) triggerNote (m.getNoteNumber(), m.getFloatVelocity(), false, 0);
            else if (m.isNoteOff()) { for (auto& v : voices) if (v.active && v.held && v.note == m.getNoteNumber()) { v.held = false; v.release(); } }
            else if (m.isAllNotesOff() || m.isAllSoundOff()) for (auto& v : voices) v.release();
        }
        if (arpActive)
        {
            if (arpCountdown <= 0.0) { arpStep (arpStepIdx++); arpCountdown += samplesPerStep; }
            arpCountdown -= 1.0;
        }
        float mL = 0, mR = 0, sub = 0;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            if (v.isArp && v.stage != 2 && --v.arpRemain <= 0) v.release();
            renderVoice (v, P, bc, mL, mR, sub);
        }
        fx.process (P, (float) bpmNow, mL, mR, sub, outL[s], outR[s]);
    }
    midi.clear();
}

int MiniSynthProcessor::getNumPrograms() { return numPresets; }
const String MiniSynthProcessor::getProgramName (int i) { return (i >= 0 && i < numPresets) ? presets[i].name : String(); }
void MiniSynthProcessor::setCurrentProgram (int i)
{
    if (i < 0 || i >= numPresets) return;
    currentProgram = i; const auto& q = presets[i];
    auto set = [this] (const char* id, float v) { if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    set ("wave", (float) q.wave); set ("uni", (float) q.uni); set ("det", q.det); set ("sub", q.sub); set ("cut", q.cut); set ("res", q.res);
    set ("fenv", q.fenv); set ("fdec", q.fdec); set ("a", q.a); set ("d", q.d); set ("s", q.s); set ("r", q.r); set ("chor", q.chor);
    set ("dly", q.dly); set ("vol", q.vol); set ("glide", q.glide); set ("form", q.form); set ("vowel", (float) q.vowel); set ("drive", q.drive);
}

void MiniSynthProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void MiniSynthProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessorEditor* MiniSynthProcessor::createEditor() { return new MiniSynthEditor (*this); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MiniSynthProcessor(); }
