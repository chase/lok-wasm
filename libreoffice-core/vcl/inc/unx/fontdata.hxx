/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* 
The purpose of this file is to store all FontConfig data that will be injected into PrintFont.
See MACRO-2573
*/

#pragma once

#include <vector>
#include <unordered_map>

#include <fontattributes.hxx>
#include <unx/fontmanager.hxx>

namespace psp
{

class FontData
{
public:
    static void populatePrintFontManagerData(
        std::unordered_map<fontID, PrintFont>& PrintFontMap,
        std::unordered_map<OString, o3tl::sorted_vector<fontID>>& FontFileToFontIDMap);

private:
    // Print Fonts
    static PrintFont GetPrintFontCaladeaBold();
    static PrintFont GetPrintFontCaladeaBoldItalic();
    static PrintFont GetPrintFontCaladeaItalic();
    static PrintFont GetPrintFontCaladeaRegular();
    static PrintFont GetPrintFontCarlitoBold();
    static PrintFont GetPrintFontCarlitoBoldItalic();
    static PrintFont GetPrintFontCarlitoItalic();
    static PrintFont GetPrintFontCarlitoRegular();
    static PrintFont GetPrintFontLiberationMonoBold();
    static PrintFont GetPrintFontLiberationMonoBoldItalic();
    static PrintFont GetPrintFontLiberationMonoItalic();
    static PrintFont GetPrintFontLiberationMonoRegular();
    static PrintFont GetPrintFontLiberationSansBold();
    static PrintFont GetPrintFontLiberationSansBoldItalic();
    static PrintFont GetPrintFontLiberationSansItalic();
    static PrintFont GetPrintFontLiberationSansRegular();
    static PrintFont GetPrintFontLiberationSansNarrowBold();
    static PrintFont GetPrintFontLiberationSansNarrowBoldItalic();
    static PrintFont GetPrintFontLiberationSansNarrowItalic();
    static PrintFont GetPrintFontLiberationSansNarrowRegular();
    static PrintFont GetPrintFontLiberationSerifBold();
    static PrintFont GetPrintFontLiberationSerifBoldItalic();
    static PrintFont GetPrintFontLiberationSerifItalic();
    static PrintFont GetPrintFontLiberationSerifRegular();
    static PrintFont GetPrintFontNotoSansBold();
    static PrintFont GetPrintFontNotoSansBoldItalic();
    static PrintFont GetPrintFontNotoSansItalic();
    static PrintFont GetPrintFontNotoSansRegular();
    static PrintFont GetPrintFontNotoSerifBold();
    static PrintFont GetPrintFontNotoSerifRegular();
    static PrintFont GetPrintFontNotoSerifBoldItalic();
    static PrintFont GetPrintFontNotoSerifItalic();
    static PrintFont GetPrintFontOpens();

    // Font Attributes
    static void SetCaladeaBoldFont(FontAttributes& fontAttributes);
    static void SetCaladeaBoldItalicFont(FontAttributes& fontAttributes);
    static void SetCaladeaItalicFont(FontAttributes& fontAttributes);
    static void SetCaladeaRegularFont(FontAttributes& fontAttributes);
    static void SetCarlitoBoldFont(FontAttributes& fontAttributes);
    static void SetCarlitoBoldItalicFont(FontAttributes& fontAttributes);
    static void SetCarlitoItalicFont(FontAttributes& fontAttributes);
    static void SetCarlitoRegularFont(FontAttributes& fontAttributes);
    static void SetLiberationMonoBoldFont(FontAttributes& fontAttributes);
    static void SetLiberationMonoBoldItalicFont(FontAttributes& fontAttributes);
    static void SetLiberationMonoItalicFont(FontAttributes& fontAttributes);
    static void SetLiberationMonoRegularFont(FontAttributes& fontAttributes);
    static void SetLiberationSansBoldFont(FontAttributes& fontAttributes);
    static void SetLiberationSansBoldItalicFont(FontAttributes& fontAttributes);
    static void SetLiberationSansItalicFont(FontAttributes& fontAttributes);
    static void SetLiberationSansRegularFont(FontAttributes& fontAttributes);
    static void SetLiberationSansNarrowBoldFont(FontAttributes& fontAttributes);
    static void SetLiberationSansNarrowBoldItalicFont(FontAttributes& fontAttributes);
    static void SetLiberationSansNarrowItalicFont(FontAttributes& fontAttributes);
    static void SetLiberationSansNarrowRegularFont(FontAttributes& fontAttributes);
    static void SetLiberationSerifBoldFont(FontAttributes& fontAttributes);
    static void SetLiberationSerifBoldItalicFont(FontAttributes& fontAttributes);
    static void SetLiberationSerifItalicFont(FontAttributes& fontAttributes);
    static void SetLiberationSerifRegularFont(FontAttributes& fontAttributes);
    static void SetNotoSansBoldFont(FontAttributes& fontAttributes);
    static void SetNotoSansBoldItalicFont(FontAttributes& fontAttributes);
    static void SetNotoSansItalicFont(FontAttributes& fontAttributes);
    static void SetNotoSansRegularFont(FontAttributes& fontAttributes);
    static void SetNotoSerifBoldFont(FontAttributes& fontAttributes);
    static void SetNotoSerifRegularFont(FontAttributes& fontAttributes);
    static void SetNotoSerifBoldItalicFont(FontAttributes& fontAttributes);
    static void SetNotoSerifItalicFont(FontAttributes& fontAttributes);
    static void SetOpenSymbolRegularFont(FontAttributes& fontAttributes);
};

}