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

#pragma once

#include "sg_OutputPatch.hpp"
#include "sg_SpatButton.hpp"

#include "lib/tl/optional.hpp"

namespace gris
{
class SmallGrisLookAndFeel;

//==============================================================================
class DirectOutSelectorComponent final
    : public MinSizedComponent
    , private juce::TextButton::Listener
{
public:
    //==============================================================================
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        SG_DEFAULT_COPY_AND_MOVE(Listener)
        //==============================================================================
        virtual void directOutSelectorComponentClicked(tl::optional<output_patch_t> directOut) = 0;
    };

    //==============================================================================
    struct Choices {
        juce::Array<output_patch_t> directOutputPatches;
        juce::Array<output_patch_t> nonDirectOutputPatches;
    };

private:
    //==============================================================================
    static inline juce::String const NO_DIRECT_OUT_STRING = "-";

    Listener & mListener;
    std::shared_ptr<Choices> mChoices;
    juce::TextButton mButton{};

public:
    //==============================================================================
    DirectOutSelectorComponent(tl::optional<output_patch_t> const & directOut,
                               std::shared_ptr<Choices> choices,
                               Listener & listener,
                               SmallGrisLookAndFeel & lookAndFeel);
    ~DirectOutSelectorComponent() override = default;
    SG_DELETE_COPY_AND_MOVE(DirectOutSelectorComponent)
    //==============================================================================
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;
    void setDirectOut(tl::optional<output_patch_t> const & directOut);
    void setChoices(std::shared_ptr<Choices> choices);
    //==============================================================================
    void resized() override;

private:
    //==============================================================================
    void buttonClicked(juce::Button * button) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(DirectOutSelectorComponent)
};

} // namespace gris