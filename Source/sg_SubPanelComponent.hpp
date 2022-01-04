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

#include "sg_LayoutComponent.hpp"

namespace gris
{
class GrisLookAndFeel;

//==============================================================================
class SubPanelComponent : public MinSizedComponent
{
    GrisLookAndFeel & mLookAndFeel;
    LayoutComponent mLayout;

public:
    //==============================================================================
    SubPanelComponent(LayoutComponent::Orientation orientation, GrisLookAndFeel & lookAndFeel);
    ~SubPanelComponent() override = default;
    SG_DELETE_COPY_AND_MOVE(SubPanelComponent)
    //==============================================================================
    LayoutComponent::Section & addSection(MinSizedComponent * component) { return mLayout.addSection(component); }
    LayoutComponent::Section & addSection(MinSizedComponent & component) { return mLayout.addSection(component); }
    LayoutComponent::Section & addSection(std::nullptr_t) { return mLayout.addSection(nullptr); }
    void clearSections() { mLayout.clearSections(); }
    //==============================================================================
    void resized() override;
    void paint(juce::Graphics & g) override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;
};

} // namespace gris