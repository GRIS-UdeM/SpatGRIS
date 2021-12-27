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

#include <JuceHeader.h>

namespace gris
{
//==============================================================================
/** A simple thread-safe buffer based on juce abstractions */
template<typename T>
class ThreadSafeBuffer
{
    juce::Array<T> mData;
    juce::CriticalSection mMutex;

public:
    //==============================================================================
    template<typename U>
    void add(U && value) noexcept;
    void add(T const & value) noexcept;
    [[nodiscard]] juce::Array<T> steal() noexcept;
    [[nodiscard]] bool empty() const noexcept;
};

//==============================================================================
template<typename T>
template<typename U>
void ThreadSafeBuffer<T>::add(U && value) noexcept
{
    juce::ScopedLock const lock{ mMutex };
    mData.add(std::forward<U>(value));
}

//==============================================================================
template<typename T>
void ThreadSafeBuffer<T>::add(T const & value) noexcept
{
    juce::ScopedLock const lock{ mMutex };
    mData.add(value);
}

//==============================================================================
template<typename T>
juce::Array<T> ThreadSafeBuffer<T>::steal() noexcept
{
    juce::ScopedLock const lock{ mMutex };
    return std::move(mData);
}

//==============================================================================
template<typename T>
bool ThreadSafeBuffer<T>::empty() const noexcept
{
    juce::ScopedLock const lock{ mMutex };
    return mData.isEmpty();
}
} // namespace gris
