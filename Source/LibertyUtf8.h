#pragma once
#include <juce_core/juce_core.h>

// C++17: u8 literals are UTF-8 char arrays, regardless of the local code page.
// Never pass a non-ASCII const char* directly to a JUCE String constructor.
// This is the only allowed gateway for non-ASCII UI literals.
inline juce::String LIBERTY_UTF8(const char* bytes)
{
    return juce::String::fromUTF8(bytes);
}
