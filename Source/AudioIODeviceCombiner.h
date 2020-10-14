//#pragma once
//
//#include "macros.h"
//
// DISABLE_WARNINGS
//#include <JuceHeader.h>
// ENABLE_WARNINGS
//
//#include "DeviceWrapper.h"
//
// class AudioIODeviceCombiner : public AudioIODevice,
//    private Thread,
//    private Timer
//{
//    friend DeviceWrapper;
//
//    WeakReference<AudioIODeviceType> owner;
//    CriticalSection callbackLock;
//    AudioIODeviceCallback* callback = nullptr;
//    AudioIODeviceCallback* previousCallback = nullptr;
//    double currentSampleRate = 0;
//    int currentBufferSize = 0;
//    bool active = false;
//    String lastError;
//    AudioBuffer<float> fifos;
//    const float** fifoReadPointers = nullptr;
//    float** fifoWritePointers = nullptr;
//    WaitableEvent threadInitialised;
//    CriticalSection closeLock;
//
//    BigInteger inputChannelsRequested, outputChannelsRequested;
//    double sampleRateRequested = 44100;
//    int bufferSizeRequested = 512;
//
//    OwnedArray<DeviceWrapper> devices;
//
// public:
//    AudioIODeviceCombiner(const String& deviceName, AudioIODeviceType* deviceType)
//        : AudioIODevice(deviceName, "CoreAudio"),
//        Thread(deviceName),
//        owner(deviceType)
//    {
//    }
//
//    ~AudioIODeviceCombiner() override
//    {
//        close();
//        devices.clear();
//    }
//
//    void addDevice(AudioIODevice* device, bool useInputs, bool useOutputs)
//    {
//        jassert(device != nullptr);
//        jassert(!isOpen());
//        jassert(!device->isOpen());
//        devices.add(new DeviceWrapper(*this, device, useInputs, useOutputs));
//
//        if (currentSampleRate == 0)
//            currentSampleRate = device->getCurrentSampleRate();
//
//        if (currentBufferSize == 0)
//            currentBufferSize = device->getCurrentBufferSizeSamples();
//    }
//
//    Array<AudioIODevice*> getDevices() const
//    {
//        Array<AudioIODevice*> devs;
//
//        for (auto* d : devices)
//            devs.add(d->device.get());
//
//        return devs;
//    }
//
//    StringArray getOutputChannelNames() override
//    {
//        StringArray names;
//
//        for (auto* d : devices)
//            names.addArray(d->getOutputChannelNames());
//
//        names.appendNumbersToDuplicates(false, true);
//        return names;
//    }
//
//    StringArray getInputChannelNames() override
//    {
//        StringArray names;
//
//        for (auto* d : devices)
//            names.addArray(d->getInputChannelNames());
//
//        names.appendNumbersToDuplicates(false, true);
//        return names;
//    }
//
//    Array<double> getAvailableSampleRates() override
//    {
//        Array<double> commonRates;
//        bool first = true;
//
//        for (auto* d : devices)
//        {
//            auto rates = d->device->getAvailableSampleRates();
//
//            if (first)
//            {
//                first = false;
//                commonRates = rates;
//            }
//            else
//            {
//                commonRates.removeValuesNotIn(rates);
//            }
//        }
//
//        return commonRates;
//    }
//
//    Array<int> getAvailableBufferSizes() override
//    {
//        Array<int> commonSizes;
//        bool first = true;
//
//        for (auto* d : devices)
//        {
//            auto sizes = d->device->getAvailableBufferSizes();
//
//            if (first)
//            {
//                first = false;
//                commonSizes = sizes;
//            }
//            else
//            {
//                commonSizes.removeValuesNotIn(sizes);
//            }
//        }
//
//        return commonSizes;
//    }
//
//    bool isOpen() override { return active; }
//    bool isPlaying() override { return callback != nullptr; }
//    double getCurrentSampleRate() override { return currentSampleRate; }
//    int getCurrentBufferSizeSamples() override { return currentBufferSize; }
//
//    int getCurrentBitDepth() override
//    {
//        int depth = 32;
//
//        for (auto* d : devices)
//            depth = jmin(depth, d->device->getCurrentBitDepth());
//
//        return depth;
//    }
//
//    int getDefaultBufferSize() override
//    {
//        int size = 0;
//
//        for (auto* d : devices)
//            size = jmax(size, d->device->getDefaultBufferSize());
//
//        return size;
//    }
//
//    String open(const BigInteger& inputChannels,
//        const BigInteger& outputChannels,
//        double sampleRate, int bufferSize) override
//    {
//        inputChannelsRequested = inputChannels;
//        outputChannelsRequested = outputChannels;
//        sampleRateRequested = sampleRate;
//        bufferSizeRequested = bufferSize;
//
//        close();
//        active = true;
//
//        if (bufferSize <= 0)
//            bufferSize = getDefaultBufferSize();
//
//        if (sampleRate <= 0)
//        {
//            auto rates = getAvailableSampleRates();
//
//            for (int i = 0; i < rates.size() && sampleRate < 44100.0; ++i)
//                sampleRate = rates.getUnchecked(i);
//        }
//
//        currentSampleRate = sampleRate;
//        currentBufferSize = bufferSize;
//
//        const int fifoSize = bufferSize * 3 + 1;
//        int totalInputChanIndex = 0, totalOutputChanIndex = 0;
//        int chanIndex = 0;
//
//        for (auto* d : devices)
//        {
//            BigInteger ins(inputChannels >> totalInputChanIndex);
//            BigInteger outs(outputChannels >> totalOutputChanIndex);
//
//            int numIns = d->getInputChannelNames().size();
//            int numOuts = d->getOutputChannelNames().size();
//
//            totalInputChanIndex += numIns;
//            totalOutputChanIndex += numOuts;
//
//            String err = d->open(ins, outs, sampleRate, bufferSize,
//                chanIndex, fifoSize);
//
//            if (err.isNotEmpty())
//            {
//                close();
//                lastError = err;
//                return err;
//            }
//
//            chanIndex += d->numInputChans + d->numOutputChans;
//        }
//
//        fifos.setSize(chanIndex, fifoSize);
//        fifoReadPointers = fifos.getArrayOfReadPointers();
//        fifoWritePointers = fifos.getArrayOfWritePointers();
//        fifos.clear();
//        startThread(9);
//        threadInitialised.wait();
//
//        return {};
//    }
//
//    void close() override
//    {
//        stop();
//        stopThread(10000);
//        fifos.clear();
//        active = false;
//
//        for (auto* d : devices)
//            d->close();
//    }
//
//    void restart(AudioIODeviceCallback* cb)
//    {
//        const ScopedLock sl(closeLock);
//
//        close();
//
//        auto newSampleRate = sampleRateRequested;
//        auto newBufferSize = bufferSizeRequested;
//
//        for (auto* d : devices)
//        {
//            auto deviceSampleRate = d->getCurrentSampleRate();
//
//            if (deviceSampleRate != sampleRateRequested)
//            {
//                if (!getAvailableSampleRates().contains(deviceSampleRate))
//                    return;
//
//                for (auto* d2 : devices)
//                    if (d2 != d)
//                        d2->setCurrentSampleRate(deviceSampleRate);
//
//                newSampleRate = deviceSampleRate;
//                break;
//            }
//        }
//
//        for (auto* d : devices)
//        {
//            auto deviceBufferSize = d->getCurrentBufferSizeSamples();
//
//            if (deviceBufferSize != bufferSizeRequested)
//            {
//                if (!getAvailableBufferSizes().contains(deviceBufferSize))
//                    return;
//
//                newBufferSize = deviceBufferSize;
//                break;
//            }
//        }
//
//        open(inputChannelsRequested, outputChannelsRequested,
//            newSampleRate, newBufferSize);
//
//        start(cb);
//    }
//
//    void restartAsync()
//    {
//        {
//            const ScopedLock sl(closeLock);
//
//            if (active)
//            {
//                if (callback != nullptr)
//                    previousCallback = callback;
//
//                close();
//            }
//        }
//
//        startTimer(100);
//    }
//
//    BigInteger getActiveOutputChannels() const override
//    {
//        BigInteger chans;
//        int start = 0;
//
//        for (auto* d : devices)
//        {
//            auto numChans = d->getOutputChannelNames().size();
//
//            if (numChans > 0)
//            {
//                chans |= (d->device->getActiveOutputChannels() << start);
//                start += numChans;
//            }
//        }
//
//        return chans;
//    }
//
//    BigInteger getActiveInputChannels() const override
//    {
//        BigInteger chans;
//        int start = 0;
//
//        for (auto* d : devices)
//        {
//            auto numChans = d->getInputChannelNames().size();
//
//            if (numChans > 0)
//            {
//                chans |= (d->device->getActiveInputChannels() << start);
//                start += numChans;
//            }
//        }
//
//        return chans;
//    }
//
//    int getOutputLatencyInSamples() override
//    {
//        int lat = 0;
//
//        for (auto* d : devices)
//            lat = jmax(lat, d->device->getOutputLatencyInSamples());
//
//        return lat + currentBufferSize * 2;
//    }
//
//    int getInputLatencyInSamples() override
//    {
//        int lat = 0;
//
//        for (auto* d : devices)
//            lat = jmax(lat, d->device->getInputLatencyInSamples());
//
//        return lat + currentBufferSize * 2;
//    }
//
//    void start(AudioIODeviceCallback* newCallback) override
//    {
//        if (callback != newCallback)
//        {
//            stop();
//            fifos.clear();
//
//            for (auto* d : devices)
//                d->start();
//
//            if (newCallback != nullptr)
//                newCallback->audioDeviceAboutToStart(this);
//
//            const ScopedLock sl(callbackLock);
//            callback = newCallback;
//            previousCallback = callback;
//        }
//    }
//
//    void stop() override { shutdown({}); }
//
//    String getLastError() override
//    {
//        return lastError;
//    }
//
// private:
//    void run() override
//    {
//        auto numSamples = currentBufferSize;
//
//        AudioBuffer<float> buffer(fifos.getNumChannels(), numSamples);
//        buffer.clear();
//
//        Array<const float*> inputChans;
//        Array<float*> outputChans;
//
//        for (auto* d : devices)
//        {
//            for (int j = 0; j < d->numInputChans; ++j)   inputChans.add(buffer.getReadPointer(d->inputIndex + j));
//            for (int j = 0; j < d->numOutputChans; ++j)  outputChans.add(buffer.getWritePointer(d->outputIndex + j));
//        }
//
//        auto numInputChans = inputChans.size();
//        auto numOutputChans = outputChans.size();
//
//        inputChans.add(nullptr);
//        outputChans.add(nullptr);
//
//        auto blockSizeMs = jmax(1, (int)(1000 * numSamples / currentSampleRate));
//
//        jassert(numInputChans + numOutputChans == buffer.getNumChannels());
//
//        threadInitialised.signal();
//
//        while (!threadShouldExit())
//        {
//            readInput(buffer, numSamples, blockSizeMs);
//
//            bool didCallback = true;
//
//            {
//                const ScopedLock sl(callbackLock);
//
//                if (callback != nullptr)
//                    callback->audioDeviceIOCallback((const float**)inputChans.getRawDataPointer(), numInputChans,
//                        outputChans.getRawDataPointer(), numOutputChans, numSamples);
//                else
//                    didCallback = false;
//            }
//
//            if (didCallback)
//            {
//                pushOutputData(buffer, numSamples, blockSizeMs);
//            }
//            else
//            {
//                for (int i = 0; i < numOutputChans; ++i)
//                    FloatVectorOperations::clear(outputChans[i], numSamples);
//
//                reset();
//            }
//        }
//    }
//
//    void timerCallback() override
//    {
//        stopTimer();
//
//        restart(previousCallback);
//    }
//
//    void shutdown(const String& error)
//    {
//        AudioIODeviceCallback* lastCallback = nullptr;
//
//        {
//            const ScopedLock sl(callbackLock);
//            std::swap(callback, lastCallback);
//        }
//
//        for (auto* d : devices)
//            d->device->stopInternal();
//
//        if (lastCallback != nullptr)
//        {
//            if (error.isNotEmpty())
//                lastCallback->audioDeviceError(error);
//            else
//                lastCallback->audioDeviceStopped();
//        }
//    }
//
//    void reset()
//    {
//        for (auto* d : devices)
//            d->reset();
//    }
//
//    void underrun()
//    {
//    }
//
//    void readInput(AudioBuffer<float>& buffer, const int numSamples, const int blockSizeMs)
//    {
//        for (auto* d : devices)
//            d->done = (d->numInputChans == 0);
//
//        for (int tries = 5;;)
//        {
//            bool anyRemaining = false;
//
//            for (auto* d : devices)
//            {
//                if (!d->done)
//                {
//                    if (d->isInputReady(numSamples))
//                    {
//                        d->readInput(buffer, numSamples);
//                        d->done = true;
//                    }
//                    else
//                        anyRemaining = true;
//                }
//            }
//
//            if (!anyRemaining)
//                return;
//
//            if (--tries == 0)
//                break;
//
//            wait(blockSizeMs);
//        }
//
//        for (auto* d : devices)
//            if (!d->done)
//                for (int i = 0; i < d->numInputChans; ++i)
//                    buffer.clear(d->inputIndex + i, 0, numSamples);
//    }
//
//    void pushOutputData(AudioBuffer<float>& buffer, const int numSamples, const int blockSizeMs)
//    {
//        for (auto* d : devices)
//            d->done = (d->numOutputChans == 0);
//
//        for (int tries = 5;;)
//        {
//            bool anyRemaining = false;
//
//            for (auto* d : devices)
//            {
//                if (!d->done)
//                {
//                    if (d->isOutputReady(numSamples))
//                    {
//                        d->pushOutputData(buffer, numSamples);
//                        d->done = true;
//                    }
//                    else
//                        anyRemaining = true;
//                }
//            }
//
//            if ((!anyRemaining) || --tries == 0)
//                return;
//
//            wait(blockSizeMs);
//        }
//    }
//
//    void handleAudioDeviceAboutToStart(AudioIODevice* device)
//    {
//        const ScopedLock sl(callbackLock);
//
//        auto newSampleRate = device->getCurrentSampleRate();
//        auto commonRates = getAvailableSampleRates();
//
//        if (!commonRates.contains(newSampleRate))
//        {
//            commonRates.sort();
//
//            if (newSampleRate < commonRates.getFirst() || newSampleRate > commonRates.getLast())
//            {
//                newSampleRate = jlimit(commonRates.getFirst(), commonRates.getLast(), newSampleRate);
//            }
//            else
//            {
//                for (auto it = commonRates.begin(); it < commonRates.end() - 1; ++it)
//                {
//                    if (it[0] < newSampleRate && it[1] > newSampleRate)
//                    {
//                        newSampleRate = newSampleRate - it[0] < it[1] - newSampleRate ? it[0] : it[1];
//                        break;
//                    }
//                }
//            }
//        }
//
//        currentSampleRate = newSampleRate;
//        bool anySampleRateChanges = false;
//
//        for (auto* d : devices)
//        {
//            if (d->getCurrentSampleRate() != currentSampleRate)
//            {
//                d->setCurrentSampleRate(currentSampleRate);
//                anySampleRateChanges = true;
//            }
//        }
//
//        if (anySampleRateChanges && owner != nullptr)
//            owner->audioDeviceListChanged();
//
//        if (callback != nullptr)
//            callback->audioDeviceAboutToStart(device);
//    }
//};