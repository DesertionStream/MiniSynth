#include "PluginEditor.h"
using namespace juce;

MiniSynthEditor::MiniSynthEditor (MiniSynthProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    const char* names[3] = { "Arpege & accords", "Son", "Effets" };
    for (int i = 0; i < 3; ++i)
    {
        auto* b = tabButtons.add (new TextButton (names[i]));
        b->setClickingTogglesState (false); b->onClick = [this, i] { showPage (i); }; addAndMakeVisible (b);
    }
    // page 0 : arpeggiator and chords
    addToggle (0, "arpOn", "Arpege"); addToggle (0, "sync", "Sync hote (tempo + lecture)");
    addSlider (0, "bpm", "Tempo (50-300) si sync off"); addCombo (0, "pattern", "Motif"); addSlider (0, "octs", "Octaves", true);
    addSlider (0, "gate", "Duree note"); addSlider (0, "bars", "Mesures par accord", true);
    for (int i = 1; i <= 4; ++i) { addCombo (0, (String ("root") + String (i)).toRawUTF8(), "Accord " + String (i) + " : note");
                                   addCombo (0, (String ("mode") + String (i)).toRawUTF8(), "Accord " + String (i) + " : mode"); }
    // page 1 : sound
    addCombo (1, "wave", "Forme d'onde"); addSlider (1, "uni", "Voix (unisson)", true); addSlider (1, "det", "Desaccord");
    addSlider (1, "sub", "Sous-oscillateur"); addSlider (1, "cut", "Coupure filtre"); addSlider (1, "res", "Resonance");
    addSlider (1, "fenv", "Enveloppe filtre"); addSlider (1, "fdec", "Declin filtre"); addSlider (1, "a", "Attaque");
    addSlider (1, "d", "Declin"); addSlider (1, "s", "Soutien"); addSlider (1, "r", "Relache"); addSlider (1, "glide", "Glissando");
    // page 2 : effects
    addCombo (2, "vowel", "Voyelle"); addSlider (2, "form", "Formants (voix)"); addSlider (2, "drive", "Saturation");
    addSlider (2, "chor", "Chorus stereo"); addSlider (2, "dly", "Echo ping-pong"); addSlider (2, "vol", "Volume");

    setSize (640, 570); showPage (0);
}

void MiniSynthEditor::addSlider (int pg, const char* id, const String& text, bool)
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
    for (int i = 0; i < 3; ++i) for (auto& r : rows[i]) { r->label->setVisible (i == p); r->ctl->setVisible (i == p); }
    for (int i = 0; i < tabButtons.size(); ++i) tabButtons[i]->setToggleState (i == p, dontSendNotification);
    resized(); repaint();
}
void MiniSynthEditor::paint (Graphics& g) { g.fillAll (Colour (0xff161b24)); }
void MiniSynthEditor::resized()
{
    auto a = getLocalBounds().reduced (12);
    presetBox.setBounds (a.removeFromTop (28)); a.removeFromTop (8);
    auto t = a.removeFromTop (28); for (auto* b : tabButtons) b->setBounds (t.removeFromLeft (150).reduced (2)); a.removeFromTop (8);
    for (auto& r : rows[page])
    {
        auto line = a.removeFromTop (28);
        r->label->setBounds (line.removeFromLeft (210)); r->ctl->setBounds (line.reduced (2));
        if (dynamic_cast<ToggleButton*> (r->ctl.get())) r->ctl->setBounds (line.getX() - 210, line.getY(), 400, 28);
    }
}
