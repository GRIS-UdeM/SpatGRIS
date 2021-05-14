#pragma once

#include "MinSizedComponent.hpp"

class GrisLookAndFeel;

//==============================================================================
class SpatTextEditor
    : public MinSizedComponent
    , public juce::TextEditor::Listener
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
        virtual void textEditorChanged(juce::String const & value, SpatTextEditor * editor) = 0;

    private:
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)
    };

private:
    //==============================================================================
    Listener & mListener;
    juce::Label mLabel{};
    juce::TextEditor mEditor{};

public:
    //==============================================================================
    SpatTextEditor(juce::String const & label,
                   juce::String const & tooltip,
                   Listener & listener,
                   GrisLookAndFeel & lookAndFeel);
    ~SpatTextEditor() override = default;
    //==============================================================================
    SpatTextEditor(SpatTextEditor const &) = delete;
    SpatTextEditor(SpatTextEditor &&) = delete;
    SpatTextEditor & operator=(SpatTextEditor const &) = delete;
    SpatTextEditor & operator=(SpatTextEditor &&) = delete;
    //==============================================================================
    void setText(juce::String const & text);
    //==============================================================================
    void textEditorFocusLost(juce::TextEditor & editor) override;
    void resized() override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpatTextEditor)
};