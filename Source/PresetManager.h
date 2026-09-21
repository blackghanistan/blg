#pragma once
#include <JuceHeader.h>
#include <vector>

namespace blg
{
/**
    Factory presets (compiled in) + user presets (*.blgpreset XML files).

    User presets live in
        macOS   ~/Library/Audio/Presets/blackghanistan/blg/<Category>/<Name>.blgpreset
        Windows %APPDATA%\blackghanistan\blg\<Category>\<Name>.blgpreset
        Linux   ~/.config/blackghanistan/blg/<Category>/<Name>.blgpreset

    All methods must be called on the message thread.
    The manager is a ChangeBroadcaster: the UI listens to it.
*/
class PresetManager : public juce::ChangeBroadcaster
{
public:
    struct Info
    {
        juce::String category, name;
        bool  factory      = false;
        int   factoryIndex = -1;
        juce::File file;
    };

    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    static juce::File getUserPresetDirectory();

    void refresh();

    const std::vector<Info>& getAll() const noexcept { return presets; }
    juce::StringArray        getCategories() const;
    std::vector<Info>        getPresetsIn (const juce::String& category) const;

    bool load (const Info& info);
    bool loadFromFile (const juce::File& file);
    bool save (const juce::String& category, const juce::String& name);
    void loadRelative (int delta);     // +1 next, -1 previous (wraps around)

    juce::String getCurrentName() const;
    juce::String getCurrentCategory() const;

private:
    void setCurrent (const juce::String& category, const juce::String& name);

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Info> presets;
};
} // namespace blg
