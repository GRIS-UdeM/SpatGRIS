#pragma once

#include "MinSizedComponent.hpp"

//==============================================================================
class RecordButton final
    : public MinSizedComponent
    , private juce::Timer
{
public:
    enum class State { ready, recording };
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
        virtual void recordButtonPressed() = 0;

    private:
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)
    };

private:
    //==============================================================================
    Listener & mListener;
    State mState{ State::ready };
    bool mBlinkState{};
    bool mIsButtonDown{};
    juce::Rectangle<int> mActiveBounds{};

public:
    //==============================================================================
    explicit RecordButton(Listener & listener);
    ~RecordButton() override = default;
    //==============================================================================
    RecordButton(RecordButton const &) = delete;
    RecordButton(RecordButton &&) = delete;
    RecordButton & operator=(RecordButton const &) = delete;
    RecordButton & operator=(RecordButton &&) = delete;
    //==============================================================================
    void setState(State state);
    //==============================================================================
    void paint(juce::Graphics & g) override;
    void resized() override;
    void mouseUp(juce::MouseEvent const & event) override;
    void mouseMove(const juce::MouseEvent & event) override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    void timerCallback() override;
    void updateActiveBounds();
    //==============================================================================
    JUCE_LEAK_DETECTOR(RecordButton)
};