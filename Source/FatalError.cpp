#include "FatalError.hpp"

//==============================================================================
void fatalError(juce::String const & message, juce::Component * component)
{
    jassertfalse;
    juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                                      "Fatal Error",
                                      message + "\nTry reinstalling SpatGRIS.",
                                      "Ok",
                                      component);
    std::terminate();
}
