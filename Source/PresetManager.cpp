#include "PresetManager.h"
#include "Parameters.h"
#include <algorithm>

namespace
{
struct FactoryPreset
{
    const char* category;
    const char* name;
    float sampleRate;
    int   bits;          // index into { 8, 12, 16, 24 }
    float turbo, tilt, dry, size, stereo, width, depth;
};

const FactoryPreset factoryPresets[] =
{
    // category  name              sr      bits turbo  tilt  dry   size  stereo width depth
    { "Init",    "Default",        22050.f, 2,  0.20f,  0.f, 0.80f, 0.50f, 1.00f, 1.00f, 0.30f },
    { "Init",    "Clean",          48000.f, 3,  0.00f,  0.f, 1.00f, 0.50f, 1.00f, 1.00f, 0.30f },
    { "Vintage", "Old Sampler",    12000.f, 1,  0.35f, -3.f, 0.85f, 0.35f, 0.90f, 0.80f, 0.20f },
    { "Vintage", "Dusty Cassette", 16000.f, 2,  0.30f, -5.f, 0.80f, 0.40f, 0.80f, 0.60f, 0.30f },
    { "Lo-Fi",   "8-bit Console",   8000.f, 0,  0.50f,  2.f, 1.00f, 0.50f, 1.00f, 1.00f, 0.30f },
    { "Lo-Fi",   "Bad Telephone",   4000.f, 1,  0.60f,  6.f, 0.90f, 0.15f, 0.50f, 0.30f, 0.10f },
    { "Crunch",  "Turbo Push",     24000.f, 2,  0.85f,  3.f, 1.00f, 0.50f, 1.00f, 1.00f, 0.30f },
    { "Crunch",  "Heavy Grit",     11025.f, 1,  1.00f, -2.f, 0.90f, 0.30f, 1.00f, 0.70f, 0.20f },
    { "Space",   "Dark Room",      22050.f, 2,  0.10f, -4.f, 0.60f, 0.70f, 1.20f, 1.00f, 0.50f },
    { "Space",   "Distant Hall",   32000.f, 2,  0.00f, -2.f, 0.45f, 0.92f, 1.40f, 1.00f, 0.80f },
};

constexpr int numFactory = (int) (sizeof (factoryPresets) / sizeof (factoryPresets[0]));

void setPlain (juce::AudioProcessorValueTreeState& apvts, const char* id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

void applyFactory (juce::AudioProcessorValueTreeState& apvts, const FactoryPreset& f)
{
    setPlain (apvts, blg::ids::sampleRate, f.sampleRate);
    setPlain (apvts, blg::ids::bits,       (float) f.bits);
    setPlain (apvts, blg::ids::turbo,      f.turbo);
    setPlain (apvts, blg::ids::tilt,       f.tilt);
    setPlain (apvts, blg::ids::revDry,     f.dry);
    setPlain (apvts, blg::ids::revSize,    f.size);
    setPlain (apvts, blg::ids::stereo,     f.stereo);
    setPlain (apvts, blg::ids::revWidth,   f.width);
    setPlain (apvts, blg::ids::revDepth,   f.depth);
}
} // namespace

namespace blg
{
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    refresh();

    if (! apvts.state.hasProperty ("presetName"))
        setCurrent (factoryPresets[0].category, factoryPresets[0].name);
}

juce::File PresetManager::getUserPresetDirectory()
{
   #if JUCE_MAC
    const auto base = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                          .getChildFile ("Library/Audio/Presets");
   #else
    const auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #endif

    return base.getChildFile ("blackghanistan").getChildFile ("blg");
}

void PresetManager::refresh()
{
    presets.clear();

    for (int i = 0; i < numFactory; ++i)
    {
        Info info;
        info.category     = factoryPresets[i].category;
        info.name         = factoryPresets[i].name;
        info.factory      = true;
        info.factoryIndex = i;
        presets.push_back (std::move (info));
    }

    const auto dir = getUserPresetDirectory();
    if (dir.isDirectory())
    {
        for (const auto& f : dir.findChildFiles (juce::File::findFiles, true, "*.blgpreset"))
        {
            Info info;
            info.category = f.getParentDirectory() == dir ? juce::String ("User")
                                                          : f.getParentDirectory().getFileName();
            info.name     = f.getFileNameWithoutExtension();
            info.file     = f;
            presets.push_back (std::move (info));
        }
    }

    std::stable_sort (presets.begin(), presets.end(), [] (const Info& a, const Info& b)
    {
        const int c = a.category.compareNatural (b.category);
        return c != 0 ? c < 0 : a.name.compareNatural (b.name) < 0;
    });
}

juce::StringArray PresetManager::getCategories() const
{
    juce::StringArray cats;
    for (const auto& p : presets)
        cats.addIfNotAlreadyThere (p.category);
    return cats;
}

std::vector<PresetManager::Info> PresetManager::getPresetsIn (const juce::String& category) const
{
    std::vector<Info> result;
    for (const auto& p : presets)
        if (p.category == category)
            result.push_back (p);
    return result;
}

bool PresetManager::load (const Info& info)
{
    if (info.factory)
    {
        if (info.factoryIndex < 0 || info.factoryIndex >= numFactory)
            return false;

        applyFactory (apvts, factoryPresets[info.factoryIndex]);
        setCurrent (info.category, info.name);
        return true;
    }

    return loadFromFile (info.file);
}

bool PresetManager::loadFromFile (const juce::File& file)
{
    auto xml = juce::parseXML (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    const auto uiWidth = apvts.state.getProperty ("uiWidth");   // keep the current window size
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    if (! uiWidth.isVoid())
        apvts.state.setProperty ("uiWidth", uiWidth, nullptr);

    setCurrent (file.getParentDirectory().getFileName(), file.getFileNameWithoutExtension());
    return true;
}

bool PresetManager::save (const juce::String& categoryIn, const juce::String& nameIn)
{
    const auto category = juce::File::createLegalFileName (categoryIn.trim().isEmpty() ? juce::String ("User")
                                                                                       : categoryIn.trim());
    const auto name = juce::File::createLegalFileName (nameIn.trim());
    if (name.isEmpty())
        return false;

    setCurrent (category, name);   // written into the state so it is stored inside the file too

    const auto dir = getUserPresetDirectory().getChildFile (category);
    if (! dir.createDirectory().wasOk())
        return false;

    auto xml = apvts.copyState().createXml();
    if (xml == nullptr || ! dir.getChildFile (name + ".blgpreset").replaceWithText (xml->toString()))
        return false;

    refresh();
    sendChangeMessage();
    return true;
}

void PresetManager::loadRelative (int delta)
{
    if (presets.empty())
        return;

    const auto cat = getCurrentCategory();
    const auto nm  = getCurrentName();
    const int  n   = (int) presets.size();

    int idx = -1;
    for (int i = 0; i < n; ++i)
        if (presets[(size_t) i].category == cat && presets[(size_t) i].name == nm)
        {
            idx = i;
            break;
        }

    idx = idx < 0 ? (delta > 0 ? 0 : n - 1) : (idx + delta + n) % n;
    load (presets[(size_t) idx]);
}

juce::String PresetManager::getCurrentName() const
{
    return apvts.state.getProperty ("presetName", "Default").toString();
}

juce::String PresetManager::getCurrentCategory() const
{
    return apvts.state.getProperty ("presetCategory", "Init").toString();
}

void PresetManager::setCurrent (const juce::String& category, const juce::String& name)
{
    apvts.state.setProperty ("presetCategory", category, nullptr);
    apvts.state.setProperty ("presetName", name, nullptr);
    sendChangeMessage();
}
} // namespace blg
