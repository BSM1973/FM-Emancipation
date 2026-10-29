#include "PluginEditor.h"
FMEmancipationAudioProcessorEditor::FMEmancipationAudioProcessorEditor(FMEmancipationAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p)
{
    auto add = [&](juce::Slider& s, const juce::String& id, bool vertical) {
        s.setSliderStyle(vertical ? juce::Slider::LinearVertical : juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 20);
        s.setColour(juce::Slider::thumbColourId, juce::Colour(0xff78c9fa));
        s.setColour(juce::Slider::trackColourId, juce::Colour(0xff4f9dd0));
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffc6e9ff));
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1b2a38));
        addAndMakeVisible(s);
        attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, id, s));
    };
    add(algorithm, "algorithm", false);
    add(fmDepth, "fmDepth", false);
    add(amDepth, "amDepth", false);
    add(output, "output", false);
    for (int i = 0; i < 6; ++i) {
        add(ratios[i], "ratio" + juce::String(i), false);
        add(levels[i], "level" + juce::String(i), true);
        add(opAttacks[i], "opAttack" + juce::String(i), false);
        add(opDecays[i], "opDecay" + juce::String(i), false);
        add(opSustains[i], "opSustain" + juce::String(i), false);
        add(opReleases[i], "opRelease" + juce::String(i), false);
        for (auto* slider : { &opAttacks[i], &opDecays[i], &opSustains[i], &opReleases[i] })
            slider->onValueChange = [this] { repaint(); };
    }
    setSize(1496, 990);
    setResizable(true, true);
    setResizeLimits(900, 650, 2200, 1500);
}
void FMEmancipationAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    const float scaleX = area.getWidth() / 1496.0f;
    const float scaleY = area.getHeight() / 990.0f;
    g.addTransform(juce::AffineTransform::scale(scaleX, scaleY));
    const auto bg = juce::Colour(0xff20262c);
    const auto blue = juce::Colour(0xff174568);
    const auto line = juce::Colour(0xff465869);
    g.fillAll(bg);
    g.setColour(juce::Colours::whitesmoke);
    g.setFont(juce::FontOptions(38.0f, juce::Font::bold));
    g.drawText("FM", 29, 20, 69, 50, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff78c9fa));
    g.drawText("Emancipation", 92, 20, 360, 50, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff9bafc0));
    g.setFont(juce::FontOptions(14.0f));
    g.drawText("S I X   O P E R A T O R S  ·  I N F I N I T E   E X P R E S S I O N", 30, 78, 560, 24, juce::Justification::left);
    g.setColour(blue); g.fillRoundedRectangle(30, 108, 1432, 143, 4);
    g.setColour(juce::Colour(0xff77bfe6)); g.drawRoundedRectangle(30, 108, 1432, 143, 4, 1);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("P R O G R A M   M E M O R Y", 54, 127, 400, 25, juce::Justification::left);
    g.setFont(juce::FontOptions(29.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffc4e5ff));
    g.drawText("Custom Sound", 54, 158, 530, 45, juce::Justification::left);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("ALGORITHM", 641, 130, 180, 25, juce::Justification::left);
    g.drawText("OPERATOR OUTPUT LEVELS", 1010, 126, 410, 25, juce::Justification::left);
    for (int i = 0; i < 5; ++i) {
        const int x = 30 + i * 192;
        g.setColour(juce::Colour(0xff303b47));
        g.fillRect(x, 270, 182, 44);
        g.setColour(juce::Colour(0xffb5c6d6));
        g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        const char* tabs[] = { "01   OPERATORS", "02   ALGORITHM", "03   ENVELOPES", "04   EFFECTS", "05   PRESETS" };
        g.drawText(tabs[i], x + 16, 279, 162, 25, juce::Justification::centred);
    }
    for (int i = 0; i < 6; ++i) {
        const int x = 30 + i * 238;
        g.setColour(juce::Colour(0xff29323b)); g.fillRoundedRectangle((float)x, 331, 238, 425, 3);
        g.setColour(line); g.drawRoundedRectangle((float)x, 331, 238, 425, 3, 1);
        g.setColour(juce::Colour(0xffcee5f7));
        g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
        g.drawText("OP " + juce::String(i + 1), x + 18, 352, 120, 30, juce::Justification::left);
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(i < 2 ? "CARRIER" : "FM MOD", x + 150, 358, 80, 23, juce::Justification::centred);
        g.drawText("Frequency ratio", x + 18, 395, 195, 20, juce::Justification::left);
        g.drawText("Output level", x + 18, 530, 195, 20, juce::Justification::left);
        // Live ADSR preview, using the same parameter values as the knobs.
        const auto envValue = [&](const juce::String& key) {
            return processor.parameters.getRawParameterValue(key + juce::String(i))->load();
        };
        const float attack = envValue("opAttack");
        const float decay = envValue("opDecay");
        const float sustain = envValue("opSustain");
        const float release = envValue("opRelease");
        const float total = attack + decay + 0.35f + release;
        const float x0 = (float)x + 20.0f, y0 = 665.0f;
        const float width = 198.0f, height = 53.0f;
        const float xa = x0 + width * attack / total;
        const float xd = xa + width * decay / total;
        const float xs = xd + width * 0.35f / total;
        const float top = y0 - height, sustainY = y0 - height * sustain;
        juce::Path envelopePath;
        envelopePath.startNewSubPath(x0, y0);
        envelopePath.lineTo(xa, top);
        envelopePath.lineTo(xd, sustainY);
        envelopePath.lineTo(xs, sustainY);
        envelopePath.lineTo(x0 + width, y0);
        g.setColour(juce::Colour(0xff213b4b));
        g.fillRoundedRectangle(x0 - 3, top - 4, width + 6, height + 9, 4.0f);
        g.setColour(juce::Colour(0xff70d6f5));
        g.strokePath(envelopePath, juce::PathStrokeType(2.0f));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        const char* adsrLabels[] = { "A", "D", "S", "R" };
        for (int k = 0; k < 4; ++k)
            g.drawText(adsrLabels[k], x + 18 + 54*k, 672, 48, 16, juce::Justification::centred);
    }
    g.setColour(juce::Colour(0xff29323b)); g.fillRoundedRectangle(30, 772, 1432, 149, 4);
    g.setColour(line); g.drawRoundedRectangle(30, 772, 1432, 149, 4, 1);
    g.setColour(juce::Colour(0xff8ba6bc)); g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("MODULATION ENGINE", 48, 785, 400, 20, juce::Justification::left);
    g.drawText("MASTER OUTPUT", 1030, 785, 400, 20, juce::Justification::left);
    g.drawText("FM EMANCIPATION  /  SIX-OPERATOR FM-AM SYNTHESIZER", 30, 934, 800, 20, juce::Justification::left);
}
void FMEmancipationAudioProcessorEditor::resized()
{
    auto fit = [&](juce::Slider& s, int x, int y, int w, int h) {
        s.setBounds(juce::Rectangle<int>(x, y, w, h)
            .toFloat().transformedBy(juce::AffineTransform::scale(getWidth()/1496.0f, getHeight()/990.0f)).getSmallestIntegerContainer());
    };
    fit(algorithm, 795, 151, 94, 79);
    fit(fmDepth, 60, 819, 80, 100);
    fit(amDepth, 164, 819, 80, 100);
    fit(output, 1190, 819, 100, 100);
    for (int i = 0; i < 6; ++i) {
        fit(ratios[i], 43 + 238*i, 420, 209, 88);
        fit(levels[i], 85 + 238*i, 548, 66, 112);
        fit(opAttacks[i], 34 + 238*i, 689, 52, 62);
        fit(opDecays[i], 88 + 238*i, 689, 52, 62);
        fit(opSustains[i], 142 + 238*i, 689, 52, 62);
        fit(opReleases[i], 196 + 238*i, 689, 52, 62);
    }
}
