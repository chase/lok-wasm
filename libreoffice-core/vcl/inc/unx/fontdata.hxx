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
        std::unordered_map<fontID, PrintFontManager::PrintFont>& PrintFontMap,
        std::unordered_map<OString, o3tl::sorted_vector<fontID>>& FontFileToFontIDMap);

private:
    // Print Fonts
    static PrintFontManager::PrintFont GetPrintFontCaladeaBold();
    static PrintFontManager::PrintFont GetPrintFontCaladeaBoldItalic();
    static PrintFontManager::PrintFont GetPrintFontCaladeaItalic();
    static PrintFontManager::PrintFont GetPrintFontCaladeaRegular();
    static PrintFontManager::PrintFont GetPrintFontCarlitoBold();
    static PrintFontManager::PrintFont GetPrintFontCarlitoBoldItalic();
    static PrintFontManager::PrintFont GetPrintFontCarlitoItalic();
    static PrintFontManager::PrintFont GetPrintFontCarlitoRegular();
    static PrintFontManager::PrintFont GetPrintFontLiberationMonoBold();
    static PrintFontManager::PrintFont GetPrintFontLiberationMonoBoldItalic();
    static PrintFontManager::PrintFont GetPrintFontLiberationMonoItalic();
    static PrintFontManager::PrintFont GetPrintFontLiberationMonoRegular();
    static PrintFontManager::PrintFont GetPrintFontLiberationSansBold();
    static PrintFontManager::PrintFont GetPrintFontLiberationSansBoldItalic();
    static PrintFontManager::PrintFont GetPrintFontLiberationSansItalic();
    static PrintFontManager::PrintFont GetPrintFontLiberationSansRegular();
    static PrintFontManager::PrintFont GetPrintFontLiberationSansNarrowBold();
    static PrintFontManager::PrintFont GetPrintFontLiberationSansNarrowBoldItalic();
    static PrintFontManager::PrintFont GetPrintFontLiberationSansNarrowItalic();
    static PrintFontManager::PrintFont GetPrintFontLiberationSansNarrowRegular();
    static PrintFontManager::PrintFont GetPrintFontLiberationSerifBold();
    static PrintFontManager::PrintFont GetPrintFontLiberationSerifBoldItalic();
    static PrintFontManager::PrintFont GetPrintFontLiberationSerifItalic();
    static PrintFontManager::PrintFont GetPrintFontLiberationSerifRegular();
    static PrintFontManager::PrintFont GetPrintFontNotoSansBold();
    static PrintFontManager::PrintFont GetPrintFontNotoSansBoldItalic();
    static PrintFontManager::PrintFont GetPrintFontNotoSansItalic();
    static PrintFontManager::PrintFont GetPrintFontNotoSansRegular();
    static PrintFontManager::PrintFont GetPrintFontNotoSerifBold();
    static PrintFontManager::PrintFont GetPrintFontNotoSerifRegular();
    static PrintFontManager::PrintFont GetPrintFontNotoSerifBoldItalic();
    static PrintFontManager::PrintFont GetPrintFontNotoSerifItalic();
    static PrintFontManager::PrintFont GetPrintFontOpens();

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