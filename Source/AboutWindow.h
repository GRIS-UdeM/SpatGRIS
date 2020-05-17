/*
 This file is part of SpatGRIS2.
 
 Developers: Samuel Béland, Nicolas Masson
 
 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

//==============================================================================

#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent;
class GrisLookAndFeel;

//==============================================================================
class AboutWindow final : public juce::DocumentWindow
{
public:
    AboutWindow(juce::String const& name, GrisLookAndFeel& lookAndFeel, MainContentComponent& mainContentComponent);
    ~AboutWindow() final = default;
    //==============================================================================
    void closeButtonPressed() final;
private:
    //==============================================================================
    MainContentComponent& mMainContentComponent;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutWindow);
};

//==============================================================================
class AboutComponent final 
    : public juce::Component
    , public juce::TextButton::Listener
{
public:
    AboutComponent(AboutWindow& parentWindow, GrisLookAndFeel& lookAndFeel);
    ~AboutComponent() final = default;
    //==============================================================================
    void buttonClicked([[maybe_unused]] juce::Button* button) final { mParentWindow.closeButtonPressed(); }
private:
    //==============================================================================
    AboutWindow& mParentWindow;
    
    juce::ImageComponent mLogoImage;
    juce::Label mTitleLabel;
    juce::Label mVersionLabel;
    juce::Label mInfosLabel;
    juce::HyperlinkButton mWebsiteHyperlink;
    juce::TextButton mCloseButton;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutComponent);
};

#endif // ABOUTWINDOW_H
