#pragma once

#include "MinSizedComponent.hpp"

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
    int mWidth;
    int mHeight;
    juce::TextEditor mEditor{};

public:
    //==============================================================================
    SpatTextEditor(juce::String const & tooltip, int width, int height, Listener & listener);
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
    [[nodiscard]] int getMinWidth() const noexcept override { return mWidth; }
    [[nodiscard]] int getMinHeight() const noexcept override { return mHeight; }

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpatTextEditor)
};