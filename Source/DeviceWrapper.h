//#pragma once
//
//#include "macros.h"
//
// DISABLE_WARNINGS
//#include <JuceHeader.h>
// ENABLE_WARNINGS
//
// class AudioIODeviceCombiner;
//
// struct DeviceWrapper : private AudioIODeviceCallback
//{
//    AudioIODeviceCombiner& owner;
//    std::unique_ptr<AudioIODevice> device;
//    int inputIndex = 0, numInputChans = 0, outputIndex = 0, numOutputChans = 0;
//    bool useInputs = false, useOutputs = false;
//    AbstractFifo inputFifo{ 32 }, outputFifo{ 32 };
//    bool done = false;
//
//    DeviceWrapper(AudioIODeviceCombiner & cd, AudioIODevice * d, bool useIns, bool useOuts);
//
//    ~DeviceWrapper() override
//    {
//        close();
//    }
//
//    String open(const BigInteger& inputChannels, const BigInteger& outputChannels,
//        double sampleRate, int bufferSize, int channelIndex, int fifoSize)
//    {
//        inputFifo.setTotalSize(fifoSize);
//        outputFifo.setTotalSize(fifoSize);
//        inputFifo.reset();
//        outputFifo.reset();
//
//        auto err = device->open(useInputs ? inputChannels : BigInteger(),
//            useOutputs ? outputChannels : BigInteger(),
//            sampleRate, bufferSize);
//
//        numInputChans = useInputs ? device->getActiveInputChannels().countNumberOfSetBits() : 0;
//        numOutputChans = useOutputs ? device->getActiveOutputChannels().countNumberOfSetBits() : 0;
//
//        inputIndex = channelIndex;
//        outputIndex = channelIndex + numInputChans;
//
//        return err;
//    }
//
//    void close()
//    {
//        device->close();
//    }
//
//    void start()
//    {
//        reset();
//        device->start(this);
//    }
//
//    void reset()
//    {
//        inputFifo.reset();
//        outputFifo.reset();
//    }
//
//    StringArray getOutputChannelNames() const { return useOutputs ? device->getOutputChannelNames() : StringArray(); }
//    StringArray getInputChannelNames()  const { return useInputs ? device->getInputChannelNames() : StringArray(); }
//
//    bool isInputReady(int numSamples) const noexcept
//    {
//        return numInputChans == 0 || inputFifo.getNumReady() >= numSamples;
//    }
//
//    void readInput(AudioBuffer<float> & destBuffer, int numSamples);
//
//    bool isOutputReady(int numSamples) const noexcept
//    {
//        return numOutputChans == 0 || outputFifo.getFreeSpace() >= numSamples;
//    }
//
//    void pushOutputData(AudioBuffer<float> & srcBuffer, int numSamples);
//
//    void audioDeviceIOCallback(const float ** inputChannelData,
//                               int numInputChannels,
//                               float ** outputChannelData,
//                               int numOutputChannels,
//                               int numSamples) override;
//
//    double getCurrentSampleRate() { return device->getCurrentSampleRate(); }
//    bool   setCurrentSampleRate(double newSampleRate) { return device->setCurrentSampleRate(newSampleRate); }
//    int  getCurrentBufferSizeSamples() { return device->getCurrentBufferSizeSamples(); }
//
//    void audioDeviceAboutToStart(AudioIODevice * d) override;
//    void audioDeviceStopped() override;
//    void audioDeviceError(const String & errorMessage) override;
//
//    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceWrapper)
//};