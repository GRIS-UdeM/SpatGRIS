/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "MinSizedComponent.hpp"
#include "StrongTypes.hpp"

class GrisLookAndFeel;

//==============================================================================
class AttenuationSettingsComponent final
    : public MinSizedComponent
    , public juce::ComboBox::Listener
{
public:
    //==============================================================================
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        //==============================================================================
        Listener(Listener const &) = delete;
        Listener(Listener &&) = delete;
        Listener & operator=(Listener const &) = delete;
        Listener & operator=(Listener &&) = delete;
        //==============================================================================
        virtual void cubeAttenuationDbChanged(dbfs_t value) = 0;
        virtual void cubeAttenuationHzChanged(hz_t value) = 0;

    private:
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)
    };

private:
    //==============================================================================
    struct Infos {
        juce::Array<dbfs_t> dbValues{};
        juce::StringArray dbStrings{};
        juce::Array<hz_t> hzValues{};
        juce::StringArray hzStrings{};
    };
    //==============================================================================
    Listener & mListener;
    GrisLookAndFeel & mLookAndFeel;
    juce::Label mLabel{};
    juce::ComboBox mDbCombobox{};
    juce::ComboBox mHzComboBox{};
    Infos mComboboxInfos{};

public:
    //==============================================================================
    AttenuationSettingsComponent(Listener & listener, GrisLookAndFeel & lookAndFeel);
    ~AttenuationSettingsComponent() override = default;
    //==============================================================================
    AttenuationSettingsComponent(AttenuationSettingsComponent const &) = delete;
    AttenuationSettingsComponent(AttenuationSettingsComponent &&) = delete;
    AttenuationSettingsComponent & operator=(AttenuationSettingsComponent const &) = delete;
    AttenuationSettingsComponent & operator=(AttenuationSettingsComponent &&) = delete;
    //==============================================================================
    void setAttenuationDb(dbfs_t value);
    void setAttenuationHz(hz_t value);
    //==============================================================================
    void resized() override;
    void comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged) override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    static Infos getInfos();
    //==============================================================================
    JUCE_LEAK_DETECTOR(AttenuationSettingsComponent)
};