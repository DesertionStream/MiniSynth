#include "PluginEditor.h"
using namespace juce;

// ------------------------------------------------------------------ piano roll
bool PianoRoll::cellAt (Point<int> pt, int& step, int& row) const
{
    const float cw = (float) (getWidth() - labelW) / 16.f, ch = (float) getHeight() / (float) MiniSynthProcessor::rollRows;
    if (pt.x < labelW) return false;
    step = (int) ((float) (pt.x - labelW) / cw); const int ry = (int) ((float) pt.y / ch);
    row = MiniSynthProcessor::rollRows - 1 - ry;
    return step >= 0 && step < 16 && row >= 0 && row < MiniSynthProcessor::rollRows;
}
void PianoRoll::paint (Graphics& g)
{
    const int rows = MiniSynthProcessor::rollRows;
    const float cw = (float) (getWidth() - labelW) / 16.f, ch = (float) getHeight() / (float) rows;
    const int rootPc = (int) proc.apvts.getRawParameterValue ((String ("root") + String (chord + 1)).toRawUTF8())->load();
    const int mode = (int) proc.apvts.getRawParameterValue ((String ("mode") + String (chord + 1)).toRawUTF8())->load();
    bool inMode[12] {}; for (int i : ms::modes()[(size_t) mode].iv) inMode[((i % 12) + 12) % 12] = true;
    const int cur = proc.currentStep.load();
    g.fillAll (Colour (0xff10141b));
    for (int r = 0; r < rows; ++r)
    {
        const int off = r - 12, pc = (((rootPc + off) % 12) + 12) % 12, rel = ((off % 12) + 12) % 12;
        const float y = (float) (rows - 1 - r) * ch;
        const bool black = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
        g.setColour (rel == 0 ? Colour (0xff2c4f4a) : inMode[rel] ? Colour (0xff222c3a) : black ? Colour (0xff141922) : Colour (0xff181e29));
        g.fillRect ((float) labelW, y, (float) getWidth() - labelW, ch);
        g.setColour (Colour (0xff2c3543)); g.drawHorizontalLine ((int) y, (float) labelW, (float) getWidth());
        g.setColour (inMode[rel] ? Colours::white.withAlpha (.85f) : Colours::grey.withAlpha (.55f)); g.setFont (ch * .85f);
        const int midi = 60 + rootPc + off;
        g.drawText (String (ms::noteNames (pc)) + String (midi / 12 - 1), 2, (int) y, labelW - 6, (int) ch + 1, Justification::centredRight);
    }
    for (int s = 0; s <= 16; ++s)
    {
        g.setColour (s % 4 == 0 ? Colour (0xff4a5568) : Colour (0xff2c3543));
        g.drawVerticalLine ((int) ((float) labelW + (float) s * cw), 0.f, (float) getHeight());
    }
    if (cur >= 0) { g.setColour (Colour (0xffff8a4c).withAlpha (.18f)); g.fillRect ((float) labelW + (float) cur * cw, 0.f, cw, (float) getHeight()); }
    for (int s = 0; s < 16; ++s)
    {
        const auto m = proc.roll[(size_t) s].load();
        for (int r = 0; r < rows; ++r)
            if ((m >> r) & 1ull)
            {
                g.setColour (Colour (0xff3fd4bd)); g.fillRoundedRectangle ((float) labelW + (float) s * cw + 1.f, (float) (rows - 1 - r) * ch + 1.f, cw - 2.f, ch - 2.f, 2.f);
            }
    }
}
void PianoRoll::apply (const MouseEvent& e)
{
    int s, r; if (! cellAt (e.getPosition(), s, r) || (s == lastStep && r == lastRow)) return;
    lastStep = s; lastRow = r;
    auto m = proc.roll[(size_t) s].load(); const auto bit = 1ull << r;
    m = paintOn ? (m | bit) : (m & ~bit);
    proc.setRollStep (s, m);
    if (auto* pp = proc.apvts.getParameter ("pattern")) // editing the roll switches the arpeggio to it
        if ((int) proc.apvts.getRawParameterValue ("pattern")->load() != 5) pp->setValueNotifyingHost (pp->convertTo0to1 (5.f));
    repaint();
}
void PianoRoll::mouseDown (const MouseEvent& e)
{
    int s, r; lastStep = lastRow = -1;
    if (cellAt (e.getPosition(), s, r)) paintOn = ((proc.roll[(size_t) s].load() >> r) & 1ull) == 0; else return;
    apply (e);
}
void PianoRoll::mouseDrag (const MouseEvent& e) { apply (e); }

// ------------------------------------------------------------------ editor
MiniSynthEditor::MiniSynthEditor (MiniSynthProcessor& p) : AudioProcessorEditor (&p), proc (p), roll (p)
{
    for (int i = 0; i < proc.getNumPresets(); ++i) presetBox.addItem (proc.getPresetName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentPreset() + 1, dontSendNotification);
    presetBox.onChange = [this] { proc.applyPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox); addAndMakeVisible (saveBtn); addAndMakeVisible (loadBtn); addAndMakeVisible (defBtn);
    saveBtn.onClick = [this] {
        chooser = std::make_unique<FileChooser> ("Sauver le preset", MiniSynthProcessor::settingsDir(), "*.msynth");
        chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                              [this] (const FileChooser& fc) { auto f = fc.getResult(); if (f != File()) proc.saveStateTo (f.withFileExtension ("msynth")); }); };
    loadBtn.onClick = [this] {
        chooser = std::make_unique<FileChooser> ("Charger un preset", MiniSynthProcessor::settingsDir(), "*.msynth");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                              [this] (const FileChooser& fc) { auto f = fc.getResult(); if (f != File()) proc.loadStateFrom (f); }); };
    defBtn.onClick = [this] { proc.saveAsDefault(); };

    const char* names[4] = { "Arpege & accords", "Piano roll", "Son", "Effets" };
    for (int i = 0; i < 4; ++i)
    {
        auto* b = tabButtons.add (new TextButton (names[i]));
        b->setClickingTogglesState (false); b->onClick = [this, i] { showPage (i); }; addAndMakeVisible (b);
    }
    // page 0 : arpeggiator and chords
    addToggle (0, "arpOn", "Arpege"); addToggle (0, "sync", "Sync hote (tempo + lecture)");
    addCombo (0, "arpSrc", "Source des notes"); addSlider (0, "bpm", "Tempo (50-300) si sync off"); addCombo (0, "pattern", "Motif");
    addSlider (0, "octs", "Octaves"); addSlider (0, "gate", "Duree note"); addSlider (0, "bars", "Mesures par accord");
    for (int i = 1; i <= 4; ++i) { addCombo (0, (String ("root") + String (i)).toRawUTF8(), "Accord " + String (i) + " : note");
                                   addCombo (0, (String ("mode") + String (i)).toRawUTF8(), "Accord " + String (i) + " : mode"); }
    // page 1 : piano roll
    for (int i = 1; i <= 4; ++i) rollChord.addItem ("Voir sur l'accord " + String (i), i);
    rollChord.setSelectedId (1, dontSendNotification); rollChord.onChange = [this] { roll.setPreviewChord (rollChord.getSelectedId() - 1); };
    rollClear.onClick = [this] { proc.clearRoll(); roll.repaint(); };
    rollFill.onClick = [this] { proc.fillRollFromAuto (rollChord.getSelectedId() - 1); roll.repaint(); };
    rollHint.setText ("Cliquez ou glissez pour poser / retirer des notes (une mesure de 16 doubles croches, rejouee sur chaque accord). "
                      "Les notes se transposent avec l'accord ; les lignes vertes sont les notes du mode. Modifier la grille active le motif Piano roll.", dontSendNotification);
    rollHint.setFont (Font (FontOptions (12.f))); rollHint.setColour (Label::textColourId, Colours::grey); rollHint.setMinimumHorizontalScale (1.f);
    addChildComponent (roll); addChildComponent (rollChord); addChildComponent (rollClear); addChildComponent (rollFill); addChildComponent (rollHint);
    // page 2 : sound
    addCombo (2, "wave", "Forme d'onde"); addSlider (2, "uni", "Voix (unisson)"); addSlider (2, "det", "Desaccord");
    addSlider (2, "sub", "Sous-oscillateur"); addSlider (2, "cut", "Coupure filtre"); addSlider (2, "res", "Resonance");
    addSlider (2, "fenv", "Enveloppe filtre"); addSlider (2, "fdec", "Declin filtre"); addSlider (2, "a", "Attaque");
    addSlider (2, "d", "Declin"); addSlider (2, "s", "Soutien"); addSlider (2, "r", "Relache"); addSlider (2, "glide", "Glissando");
    // page 3 : effects
    addCombo (3, "vowel", "Voyelle"); addSlider (3, "form", "Formants (voix)"); addSlider (3, "drive", "Saturation");
    addSlider (3, "chor", "Chorus stereo"); addSlider (3, "dly", "Echo ping-pong"); addSlider (3, "vol", "Volume");

    setSize (720, 640); showPage (0);
}

void MiniSynthEditor::addSlider (int pg, const char* id, const String& text)
{
    auto r = std::make_unique<Row>(); auto* s = new Slider (Slider::LinearHorizontal, Slider::TextBoxRight);
    s->setTextBoxStyle (Slider::TextBoxRight, false, 90, 22);
    r->ctl.reset (s); r->label = std::make_unique<Label> ("", text);
    r->sa = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *s);
    addChildComponent (*r->label); addChildComponent (*r->ctl); rows[pg].push_back (std::move (r));
}
void MiniSynthEditor::addCombo (int pg, const char* id, const String& text)
{
    auto r = std::make_unique<Row>(); auto* c = new ComboBox();
    if (auto* prm = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (id))) c->addItemList (prm->choices, 1);
    r->ctl.reset (c); r->label = std::make_unique<Label> ("", text);
    r->ca = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, *c);
    addChildComponent (*r->label); addChildComponent (*r->ctl); rows[pg].push_back (std::move (r));
}
void MiniSynthEditor::addToggle (int pg, const char* id, const String& text)
{
    auto r = std::make_unique<Row>(); auto* b = new ToggleButton (text);
    r->ctl.reset (b); r->label = std::make_unique<Label>();
    r->ba = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, *b);
    addChildComponent (*r->label); addChildComponent (*r->ctl); rows[pg].push_back (std::move (r));
}
void MiniSynthEditor::showPage (int p)
{
    page = p;
    for (int i = 0; i < 4; ++i) for (auto& r : rows[i]) { r->label->setVisible (i == p); r->ctl->setVisible (i == p); }
    for (auto* c : std::initializer_list<Component*> { &roll, &rollChord, &rollClear, &rollFill, &rollHint }) c->setVisible (p == 1);
    for (int i = 0; i < tabButtons.size(); ++i) tabButtons[i]->setToggleState (i == p, dontSendNotification);
    resized(); repaint();
}
void MiniSynthEditor::paint (Graphics& g) { g.fillAll (Colour (0xff161b24)); }
void MiniSynthEditor::resized()
{
    auto a = getLocalBounds().reduced (12);
    auto top = a.removeFromTop (28);
    defBtn.setBounds (top.removeFromRight (150).reduced (2)); loadBtn.setBounds (top.removeFromRight (90).reduced (2));
    saveBtn.setBounds (top.removeFromRight (90).reduced (2)); presetBox.setBounds (top);
    a.removeFromTop (8);
    auto t = a.removeFromTop (28); for (auto* b : tabButtons) b->setBounds (t.removeFromLeft (150).reduced (2)); a.removeFromTop (8);
    if (page == 1)
    {
        auto bar = a.removeFromTop (28);
        rollChord.setBounds (bar.removeFromLeft (200).reduced (2)); rollClear.setBounds (bar.removeFromLeft (90).reduced (2)); rollFill.setBounds (bar.removeFromLeft (220).reduced (2));
        a.removeFromTop (6); rollHint.setBounds (a.removeFromBottom (44)); a.removeFromBottom (4); roll.setBounds (a);
        return;
    }
    for (auto& r : rows[page])
    {
        auto line = a.removeFromTop (28);
        r->label->setBounds (line.removeFromLeft (210)); r->ctl->setBounds (line.reduced (2));
        if (dynamic_cast<ToggleButton*> (r->ctl.get())) r->ctl->setBounds (line.getX() - 210, line.getY(), 400, 28);
    }
}
