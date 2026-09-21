#pragma once
#include "Theme.h"
#include "../PresetManager.h"

namespace blg::ui
{
//==============================================================================
/** Top bar: previous / current preset / next, Save and Load. */
class PresetBar : public juce::Component, private juce::ChangeListener
{
public:
    explicit PresetBar (blg::PresetManager& pm) : manager (pm)
    {
        prev.setButtonText ("<");
        next.setButtonText (">");
        save.setButtonText ("Save");
        load.setButtonText ("Load");

        for (auto* b : { &prev, &next, &save, &load })
            addAndMakeVisible (*b);

        prev.onClick = [this] { manager.loadRelative (-1); };
        next.onClick = [this] { manager.loadRelative (+1); };
        save.onClick = [this] { showSaveDialog(); };
        load.onClick = [this] { showLoadDialog(); };

        manager.addChangeListener (this);
    }

    ~PresetBar() override { manager.removeChangeListener (this); }

    void paint (juce::Graphics& g) override
    {
        g.setColour (col::screen);
        g.fillRoundedRectangle (displayArea, 8.0f);
        g.setColour (col::edgeLight);
        g.drawRoundedRectangle (displayArea.reduced (0.5f), 8.0f, 1.0f);

        const auto a = displayArea.reduced (14.0f, 0.0f);

        g.setColour (col::textDim);
        g.setFont (makeFont (11.0f, true));
        g.drawText (manager.getCurrentCategory(), a.withTrimmedTop (3.0f).withHeight (18.0f),
                    juce::Justification::centredLeft);

        g.setColour (col::white);
        g.setFont (makeFont (17.0f, true));
        g.drawText (manager.getCurrentName(), a.withTrimmedTop (20.0f).withHeight (24.0f),
                    juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        load.setBounds (r.removeFromRight (64));  r.removeFromRight (6);
        save.setBounds (r.removeFromRight (64));  r.removeFromRight (12);
        prev.setBounds (r.removeFromLeft (40));   r.removeFromLeft (6);
        next.setBounds (r.removeFromRight (40));  r.removeFromRight (6);
        displayArea = r.toFloat();
    }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }

    void showSaveDialog()
    {
        auto* w = new juce::AlertWindow ("Save preset", "Name and category",
                                         juce::MessageBoxIconType::NoIcon);
        w->addTextEditor ("name", manager.getCurrentName(), "Name");

        auto cats = manager.getCategories();
        cats.addIfNotAlreadyThere ("User");
        w->addComboBox ("cat", cats, "Category (type a new one to create it)");
        if (auto* cb = w->getComboBoxComponent ("cat"))
        {
            cb->setEditableText (true);
            cb->setText ("User", juce::dontSendNotification);
        }

        w->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        w->enterModalState (true,
            juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<PresetBar> (this), w] (int result)
            {
                if (result != 1 || safe == nullptr)
                    return;

                const auto name = w->getTextEditorContents ("name");
                const auto cat  = w->getComboBoxComponent ("cat") != nullptr
                                      ? w->getComboBoxComponent ("cat")->getText() : juce::String ("User");
                safe->manager.save (cat, name);
            }),
            true);
    }

    void showLoadDialog()
    {
        chooser = std::make_unique<juce::FileChooser> ("Load preset",
                                                       blg::PresetManager::getUserPresetDirectory(),
                                                       "*.blgpreset");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe = juce::Component::SafePointer<PresetBar> (this)] (const juce::FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (safe != nullptr && f.existsAsFile())
                    safe->manager.loadFromFile (f);
            });
    }

    blg::PresetManager& manager;
    juce::TextButton prev, next, save, load;
    juce::Rectangle<float> displayArea;
    std::unique_ptr<juce::FileChooser> chooser;
};

//==============================================================================
/** Two lists (categories | presets) shown on the screen in the middle of the bottom row. */
class PresetBrowser : public juce::Component, private juce::ChangeListener
{
public:
    explicit PresetBrowser (blg::PresetManager& pm)
        : manager (pm), categoryModel (*this), presetModel (*this)
    {
        setupList (categoryList, categoryModel);
        setupList (presetList,   presetModel);

        manager.addChangeListener (this);
        selectedCategory = manager.getCurrentCategory();
        rebuild();
    }

    ~PresetBrowser() override { manager.removeChangeListener (this); }

    void paint (juce::Graphics& g) override
    {
        g.setColour (col::textDim);
        g.setFont (makeFont (11.0f, true));
        g.drawText ("Category", categoryHeader, juce::Justification::centredLeft);
        g.drawText ("Preset",   presetHeader,   juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10, 8);
        auto header = r.removeFromTop (16);
        categoryHeader = header.removeFromLeft (118);
        presetHeader   = header.withTrimmedLeft (10);

        categoryList.setBounds (r.removeFromLeft (118));
        presetList.setBounds (r.withTrimmedLeft (10));
    }

private:
    struct CategoryModel : juce::ListBoxModel
    {
        explicit CategoryModel (PresetBrowser& o) : owner (o) {}
        int getNumRows() override { return owner.categoryNames.size(); }
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
        {
            if (juce::isPositiveAndBelow (row, owner.categoryNames.size()))
                paintRow (g, w, h, selected, owner.categoryNames[row], false);
        }
        void listBoxItemClicked (int row, const juce::MouseEvent&) override { owner.categoryClicked (row); }
        PresetBrowser& owner;
    };

    struct PresetModel : juce::ListBoxModel
    {
        explicit PresetModel (PresetBrowser& o) : owner (o) {}
        int getNumRows() override { return (int) owner.presetsInCategory.size(); }
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
        {
            if (juce::isPositiveAndBelow (row, (int) owner.presetsInCategory.size()))
            {
                const auto& p = owner.presetsInCategory[(size_t) row];
                paintRow (g, w, h, selected, p.name, ! p.factory);
            }
        }
        void listBoxItemClicked (int row, const juce::MouseEvent&) override { owner.presetClicked (row); }
        PresetBrowser& owner;
    };

    static void paintRow (juce::Graphics& g, int w, int h, bool selected, const juce::String& text, bool userPreset)
    {
        const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (1.0f);
        if (selected)
        {
            g.setColour (col::orange);
            g.fillRoundedRectangle (r, 5.0f);
        }

        g.setColour (selected ? col::screen : col::text);
        g.setFont (makeFont (13.0f, selected));
        g.drawText (text, r.reduced (8.0f, 0.0f), juce::Justification::centredLeft);

        if (userPreset)   // small dot marks user presets
        {
            g.setColour (selected ? col::screen : col::orange);
            g.fillEllipse (r.getRight() - 12.5f, r.getCentreY() - 2.5f, 5.0f, 5.0f);
        }
    }

    void setupList (juce::ListBox& list, juce::ListBoxModel& model)
    {
        list.setModel (&model);
        list.setRowHeight (22);
        list.setOutlineThickness (0);
        addAndMakeVisible (list);
    }

    void categoryClicked (int row)
    {
        if (! juce::isPositiveAndBelow (row, categoryNames.size()))
            return;

        selectedCategory = categoryNames[row];
        presetsInCategory = manager.getPresetsIn (selectedCategory);
        presetList.updateContent();
        selectCurrentPreset();
    }

    void presetClicked (int row)
    {
        if (juce::isPositiveAndBelow (row, (int) presetsInCategory.size()))
            manager.load (presetsInCategory[(size_t) row]);
    }

    void selectCurrentPreset()
    {
        int current = -1;
        if (selectedCategory == manager.getCurrentCategory())
            for (int i = 0; i < (int) presetsInCategory.size(); ++i)
                if (presetsInCategory[(size_t) i].name == manager.getCurrentName())
                    current = i;

        if (current >= 0)
            presetList.selectRow (current, false, true);
        else
            presetList.deselectAllRows();

        presetList.repaint();
    }

    void rebuild()
    {
        categoryNames = manager.getCategories();
        if (! categoryNames.contains (selectedCategory))
            selectedCategory = categoryNames.isEmpty() ? juce::String() : categoryNames[0];

        presetsInCategory = manager.getPresetsIn (selectedCategory);

        categoryList.updateContent();
        presetList.updateContent();

        const int catRow = categoryNames.indexOf (selectedCategory);
        if (catRow >= 0)
            categoryList.selectRow (catRow, false, true);

        selectCurrentPreset();
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        selectedCategory = manager.getCurrentCategory();
        rebuild();
    }

    blg::PresetManager& manager;
    CategoryModel categoryModel;
    PresetModel   presetModel;
    juce::ListBox categoryList, presetList;
    juce::StringArray categoryNames;
    std::vector<blg::PresetManager::Info> presetsInCategory;
    juce::String selectedCategory;
    juce::Rectangle<int> categoryHeader, presetHeader;
};
} // namespace blg::ui
