/*
 This file is part of SpatGRIS.

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

#include "SpeakerSetupTreeView.hpp"

namespace gris
{

SpeakerSetupTreeView::SpeakerSetupTreeView(const juce::String &componentName)
        :juce::TreeView(componentName) {
}

void SpeakerSetupTreeView::requestScrollReset(double scrollPosition) {
    requestedScrollPosition = scrollPosition;
    scrollResetRequested = true;
    tries = 0;
}

void SpeakerSetupTreeView::resetScrollRequest() {
    requestedScrollPosition = 0.0;
    scrollResetRequested = false;
    tries = 0;
}

void SpeakerSetupTreeView::paint (juce::Graphics & g){
    juce::TreeView::paint(g);
    if (!scrollResetRequested) {
        return;
    }
    // if someone requested a scroll position, we try our hardest to comply.

    // if our requested scroll position is 0.0, we are certainly already there since
    // its the default scroll position we have when our items are reseted. We can safely assume
    // everything is ok, clear the flag and return.
    if (requestedScrollPosition == 0.0) {
        resetScrollRequest();
        return;
    }
    const auto currentScrollPosition = getViewport()->getVerticalScrollBar().getCurrentRangeStart();

    const auto maxRangeLimit = getViewport()->getVerticalScrollBar().getMaximumRangeLimit();

    const auto currentRangeSize = getViewport()->getVerticalScrollBar().getCurrentRangeSize();

    // This is the maximum scroll position we can get according to juce source code. (and docs I think)
    const auto maxScrollValue = maxRangeLimit - currentRangeSize;

    auto possibleScrollValue = requestedScrollPosition;
    if (maxScrollValue < possibleScrollValue) {
        possibleScrollValue = maxScrollValue;
    }

    // try to set the range here.
    getViewport()->getVerticalScrollBar().setCurrentRangeStart(possibleScrollValue);

    const auto newScrollPos = getViewport()->getVerticalScrollBar().getCurrentRangeStart();

    // if the position moved, we did our best, consider the request honoured.
    if (newScrollPos != currentScrollPosition) {
        resetScrollRequest();
        return;
    }
    // I think there's a possible world where we try to set stuff, we are not at 0,
    // and the position doesn't change. if this happens a bung of times, just give up.
    // Note: I have not seen this happen.
    tries +=1;
    if (tries > 10) {
        resetScrollRequest();
    }
}
}
