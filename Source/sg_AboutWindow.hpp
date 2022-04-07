/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

//==============================================================================

#pragma once

#include "sg_Macros.hpp"

#include <JuceHeader.h>

namespace gris
{
class MainContentComponent;
class GrisLookAndFeel;

//==============================================================================
class AboutWindow final : public juce::DocumentWindow
{
    MainContentComponent & mMainContentComponent;

public:
    //==============================================================================
    AboutWindow(juce::String const & name, GrisLookAndFeel & lookAndFeel, MainContentComponent & mainContentComponent);
    //==============================================================================
    AboutWindow() = delete;
    ~AboutWindow() override = default;
    SG_DELETE_COPY_AND_MOVE(AboutWindow)
    //==============================================================================
    void closeButtonPressed() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(AboutWindow)
}; // class AboutWindow

//==============================================================================
class AboutComponent final
    : public juce::Component
    , public juce::TextButton::Listener
{
    AboutWindow & mParentWindow;

    juce::ImageComponent mLogoImage;
    juce::Label mTitleLabel;
    juce::Label mVersionLabel;
    juce::Label mInfosLabel;
    juce::HyperlinkButton mWebsiteHyperlink;
    juce::TextButton mCloseButton;

public:
    //==============================================================================
    AboutComponent(AboutWindow & parentWindow, GrisLookAndFeel & lookAndFeel);
    //==============================================================================
    AboutComponent() = delete;
    ~AboutComponent() override = default;
    SG_DELETE_COPY_AND_MOVE(AboutComponent)
    //==============================================================================
    void buttonClicked([[maybe_unused]] juce::Button * button) override { mParentWindow.closeButtonPressed(); }

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(AboutComponent)
}; // class AboutComponent

} // namespace gris