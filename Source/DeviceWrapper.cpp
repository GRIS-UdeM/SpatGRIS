//#include "DeviceWrapper.h"
//
//#include "AudioIODeviceCombiner.h"
//
// DeviceWrapper::DeviceWrapper(AudioIODeviceCombiner & cd, AudioIODevice * d, bool useIns, bool useOuts)
//    : owner(cd)
//    , device(d)
//    , useInputs(useIns)
//    , useOutputs(useOuts)
//{
//    d->setDeviceWrapperRestartCallback([this] { owner.restartAsync(); });
//}
//
// void DeviceWrapper::readInput(AudioBuffer<float> & destBuffer, int numSamples)
//{
//    if (numInputChans == 0)
//        return;
//
//    int start1, size1, start2, size2;
//    inputFifo.prepareToRead(numSamples, start1, size1, start2, size2);
//
//    for (int i = 0; i < numInputChans; ++i)
//    {
//        auto index = inputIndex + i;
//        auto dest = destBuffer.getWritePointer(index);
//        auto src = owner.fifoReadPointers[index];
//
//        if (size1 > 0)
//            FloatVectorOperations::copy(dest, src + start1, size1);
//        if (size2 > 0)
//            FloatVectorOperations::copy(dest + size1, src + start2, size2);
//    }
//
//    inputFifo.finishedRead(size1 + size2);
//}
//
// void DeviceWrapper::pushOutputData(AudioBuffer<float> & srcBuffer, int numSamples)
//{
//    if (numOutputChans == 0)
//        return;
//
//    int start1, size1, start2, size2;
//    outputFifo.prepareToWrite(numSamples, start1, size1, start2, size2);
//
//    for (int i = 0; i < numOutputChans; ++i)
//    {
//        auto index = outputIndex + i;
//        auto dest = owner.fifoWritePointers[index];
//        auto src = srcBuffer.getReadPointer(index);
//
//        if (size1 > 0)
//            FloatVectorOperations::copy(dest + start1, src, size1);
//        if (size2 > 0)
//            FloatVectorOperations::copy(dest + start2, src + size1, size2);
//    }
//
//    outputFifo.finishedWrite(size1 + size2);
//}
//
// void DeviceWrapper::audioDeviceIOCallback(const float ** inputChannelData,
//                                          int numInputChannels,
//                                          float ** outputChannelData,
//                                          int numOutputChannels,
//                                          int numSamples)
//{
//    if (numInputChannels > 0)
//    {
//        int start1, size1, start2, size2;
//        inputFifo.prepareToWrite(numSamples, start1, size1, start2, size2);
//
//        if (size1 + size2 < numSamples)
//        {
//            inputFifo.reset();
//            inputFifo.prepareToWrite(numSamples, start1, size1, start2, size2);
//        }
//
//        for (int i = 0; i < numInputChannels; ++i)
//        {
//            auto dest = owner.fifoWritePointers[inputIndex + i];
//            auto src = inputChannelData[i];
//
//            if (size1 > 0)
//                FloatVectorOperations::copy(dest + start1, src, size1);
//            if (size2 > 0)
//                FloatVectorOperations::copy(dest + start2, src + size1, size2);
//        }
//
//        auto totalSize = size1 + size2;
//        inputFifo.finishedWrite(totalSize);
//
//        if (numSamples > totalSize)
//        {
//            auto samplesRemaining = numSamples - totalSize;
//
//            for (int i = 0; i < numInputChans; ++i)
//                FloatVectorOperations::clear(owner.fifoWritePointers[inputIndex + i] + totalSize, samplesRemaining);
//
//            owner.underrun();
//        }
//    }
//
//    if (numOutputChannels > 0)
//    {
//        int start1, size1, start2, size2;
//        outputFifo.prepareToRead(numSamples, start1, size1, start2, size2);
//
//        if (size1 + size2 < numSamples)
//        {
//            Thread::sleep(1);
//            outputFifo.prepareToRead(numSamples, start1, size1, start2, size2);
//        }
//
//        for (int i = 0; i < numOutputChannels; ++i)
//        {
//            auto dest = outputChannelData[i];
//            auto src = owner.fifoReadPointers[outputIndex + i];
//
//            if (size1 > 0)
//                FloatVectorOperations::copy(dest, src + start1, size1);
//            if (size2 > 0)
//                FloatVectorOperations::copy(dest + size1, src + start2, size2);
//        }
//
//        auto totalSize = size1 + size2;
//        outputFifo.finishedRead(totalSize);
//
//        if (numSamples > totalSize)
//        {
//            auto samplesRemaining = numSamples - totalSize;
//
//            for (int i = 0; i < numOutputChannels; ++i)
//                FloatVectorOperations::clear(outputChannelData[i] + totalSize, samplesRemaining);
//
//            owner.underrun();
//        }
//    }
//
//    owner.notify();
//}
//
// void DeviceWrapper::audioDeviceAboutToStart(AudioIODevice * d)
//{
//    owner.handleAudioDeviceAboutToStart(d);
//}
//
// void DeviceWrapper::audioDeviceStopped()
//{
//    owner.handleAudioDeviceStopped();
//}
//
// void DeviceWrapper::audioDeviceError(const String & errorMessage)
//{
//    owner.handleAudioDeviceError(errorMessage);
//}
