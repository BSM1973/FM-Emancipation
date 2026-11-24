#include "LibertyUtf8.h"
#include <iostream>
int main()
{
    const auto s = LIBERTY_UTF8(u8"éèêàâùûôîïçÉÀŒœ–→★×’");
    constexpr juce::juce_wchar expected[] = {
        0x00e9,0x00e8,0x00ea,0x00e0,0x00e2,0x00f9,0x00fb,0x00f4,
        0x00ee,0x00ef,0x00e7,0x00c9,0x00c0,0x0152,0x0153,0x2013,
        0x2192,0x2605,0x00d7,0x2019
    };
    if (s.length() != static_cast<int>(sizeof(expected)/sizeof(expected[0]))) return 1;
    for (int i=0;i<s.length();++i)
        if(s[i]!=expected[i]) { std::cerr<<"Unicode mismatch at "<<i<<"\n"; return 2; }
    const auto text=LIBERTY_UTF8(u8"Réglages – Réverbération – Vélocité");
    const auto restored=juce::String::fromUTF8(text.toRawUTF8());
    if(restored!=text)return 3;
    std::cout<<"Liberty Unicode runtime test: PASS\n";
    return 0;
}
