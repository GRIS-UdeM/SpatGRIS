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

#include "sg_LogBuffer.hpp"

namespace gris
{
//==============================================================================
void LogBuffer::addListener(Listener * l)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mListeners.add(l);
}

//==============================================================================
void LogBuffer::removeListener(Listener * l)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mListeners.remove(l);
}

//==============================================================================
void LogBuffer::add(juce::String const & event)
{
    // OSC thread
    {
        juce::ScopedLock const lock{ mMutex };
        mBuffer.add(event);
    }
    triggerAsyncUpdate();
}

//==============================================================================
void LogBuffer::start()
{
    juce::ScopedLock const lock{ mMutex };
    mBuffer.clearQuick();
    mActive.store(true);
}

//==============================================================================
void LogBuffer::stop()
{
    mActive.store(false);
}

//==============================================================================
bool LogBuffer::isActive() const noexcept
{
    return mActive.load();
}

//==============================================================================
juce::StringArray LogBuffer::stealData()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedLock const lock{ mMutex };
    auto result{ std::move(mBuffer) };
    mBuffer.clearQuick();
    return result;
}

//==============================================================================
void LogBuffer::handleAsyncUpdate()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    auto const events{ stealData() };
    if (events.isEmpty()) {
        return;
    }
    auto const consolidatedString{ events.joinIntoString("\n") };
    auto const callback = [&](Listener & l) { l.oscEventReceived(consolidatedString); };
    mListeners.call(callback);
}
} // namespace gris
