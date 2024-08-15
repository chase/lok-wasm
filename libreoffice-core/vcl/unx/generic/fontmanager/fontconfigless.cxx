/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <array>
#include <map>
#include <numeric>
#include <vector>

#include <unx/fontdata.hxx>
#include "fontconfig.cxx"

using namespace psp;
using namespace osl;

/* Metric Compatibility Font Fallback Chart */
const std::map<std::string, std::string> fontToFallback = {
    // Fallback to Liberation Sans
    { "Helvetica", "LiberationSans" },
    { "Nimbus Sans", "LiberationSans" },
    { "TeX Gyre Heros", "LiberationSans" },
    { "Arial", "LiberationSans" },
    { "Liberation Sans", "LiberationSans" },
    { "Arimo", "LiberationSans" },
    { "Albany", "LiberationSans" },
    { "Albany AMT", "LiberationSans" },

    // Fallback to Liberation Sans Narrow
    { "Helvetica Narrow", "LiberationSansNarrow" },
    { "Nimbus Sans Narrow", "LiberationSansNarrow" },
    { "TeX Gyre Heros Cn", "LiberationSansNarrow" },
    { "Arial Narrow", "LiberationSansNarrow" },
    { "Liberation Sans Narrow", "LiberationSansNarrow" },

    // Fallback to Liberation Serif
    { "Times", "LiberationSerif" },
    { "Nimbus Roman", "LiberationSerif" },
    { "TeX Gyre Termes", "LiberationSerif" },
    { "Times New Roman", "LiberationSerif" },
    { "Liberation Serif", "LiberationSerif" },
    { "Tinos", "LiberationSerif" },
    { "Thorndale", "LiberationSerif" },
    { "Thorndale AMT", "LiberationSerif" },

    // Fallback to Liberation Mono
    { "Courier", "LiberationMono" },
    { "Nimbus Mono PS", "LiberationMono" },
    { "TeX Gyre Cursor", "LiberationMono" },
    { "Courier New", "LiberationMono" },
    { "Liberation Mono", "LiberationMono" },
    { "Cousine", "LiberationMono" },
    { "Cumberland", "LiberationMono" },
    { "Cumberland AMT", "LiberationMono" },

    // Fallback to Caladea
    { "ITC Avant Garde Gothic", "Caladea" },
    { "URW Gothic", "Caladea" },
    { "TeX Gyre Adventor", "Caladea" },
    { "Cambria", "Caladea" },
    { "Caladea", "Caladea" },

    // Fallback to Carlito
    { "ITC Bookman", "Carlito" },
    { "URW Bookman", "Carlito" },
    { "TeX Gyre Bonum", "Carlito" },
    { "Bookman Old Style", "Carlito" },
    { "Calibri", "Carlito" },
    { "Carlito", "Carlito" },

    // Not explicitly listed to have metric compatibility
    // Fallback to OpenSymbol
    { "Symbol", "OpenSymbol" },
    { "SymbolNeu", "OpenSymbol" }
};

/**
 * @brief Retrieve the PrintFont using the font name, italics, and weight
 * 
 * @param fontfamily 
 * @param fontItalic 
 * @param fontWeight 
 * @return PrintFontManager::PrintFont* 
 */
PrintFontManager::PrintFont* PrintFontManager::getFontFromAttributes(const std::string fontfamily,
                                                                     const FontItalic fontItalic,
                                                                     const FontWeight fontWeight)
{
    const bool isItalic
        = fontItalic == FontItalic::ITALIC_NORMAL || fontItalic == FontItalic::ITALIC_OBLIQUE;
    const bool isBold = fontWeight >= FontWeight::WEIGHT_MEDIUM;

    std::string fallbackFont = fontfamily;
    if (fallbackFont == "OpenSymbol")
    {
        fallbackFont = "opens___";
    }
    else
    {
        if (isBold && isItalic)
        {
            fallbackFont += "-BoldItalic";
        }
        else if (isBold)
        {
            fallbackFont += "-Bold";
        }
        else if (isItalic)
        {
            fallbackFont += "-Italic";
        }
        else
        {
            fallbackFont += "-Regular";
        }
    }
    std::erase(fallbackFont, ' ');
    fallbackFont += ".ttf";

    // type std::unordered_map<OString, o3tl::sorted_vector<fontID>>::iterator
    auto it_fontid = m_aFontFileToFontID.find(OString(fallbackFont));
    if (it_fontid == m_aFontFileToFontID.end() || (it_fontid->second).empty())
        return nullptr;

    // type std::unordered_map<fontID, PrintFont>::iterator
    auto it_printfont = m_aFonts.find((it_fontid->second).front());
    if (it_printfont == m_aFonts.end())
        return nullptr;

    return &it_printfont->second;
}

/**
 * @brief Matching Fonts based off the Font Config metric based compatibility
 * 
 * @param attributes 
 * @return PrintFontManager::PrintFont* 
 */
PrintFontManager::PrintFont*
PrintFontManager::MatchFromCompatibilityChart(const FontAttributes& attributes)
{
    std::string familyname
        = OUStringToOString(attributes.GetFamilyName(), RTL_TEXTENCODING_UTF8).getStr();

    // type std::map<std::string, std::string>::iterator
    auto it_fallback = fontToFallback.find(familyname);
    if (it_fallback == fontToFallback.end())
        return nullptr;

    return getFontFromAttributes(it_fallback->second, attributes.GetItalic(),
                                 attributes.GetWeight());
}

/**
 * @brief Matches Fonts based off the Family Type
 * 
 * @param attributes 
 * @return PrintFontManager::PrintFont* 
 */
PrintFontManager::PrintFont*
PrintFontManager::MatchSansOrSerifFamily(const FontAttributes& attributes)
{
    std::string font_family;
    switch (attributes.GetFamilyType())
    {
        case FontFamily::FAMILY_ROMAN:
            font_family = "NotoSerif";
            break;

        case FontFamily::FAMILY_MODERN:
            font_family = "NotoSans";
            break;

        default:
            return nullptr;
    }
    return getFontFromAttributes(font_family, attributes.GetItalic(), attributes.GetWeight());
}

/**
 * @brief Gets the numerical value of the Font Weight for sorting purposes
 * 
 * @param fontWeight 
 * @return int 
 */
int getFontWeightValue(const FontWeight fontWeight)
{
    switch (fontWeight)
    {
        case WEIGHT_DONTKNOW:
            return 0;
        case WEIGHT_THIN:
            return 100;
        case WEIGHT_ULTRALIGHT:
            return 200;
        case WEIGHT_LIGHT:
            return 300;
        case WEIGHT_SEMILIGHT:
            return 350;
        case WEIGHT_NORMAL:
            return 400;
        case WEIGHT_MEDIUM:
            return 500;
        case WEIGHT_SEMIBOLD:
            return 600;
        case WEIGHT_BOLD:
            return 700;
        case WEIGHT_ULTRABOLD:
            return 800;
        case WEIGHT_BLACK:
            return 900;
        default:
            return 400;
    }
}

/**
 * @brief Match the Font to the list of supported fonts in PrintFontManager
 *        Step 4 of the following font matching algorithm
 * 
 * @param attributes 
 * @return A vector of eligible fonts
 */
std::vector<fontID> PrintFontManager::MatchWidthStyleWeight(const FontAttributes& attributes)
{
    std::vector<fontID> matching_set(m_aFonts.size());
    std::iota(matching_set.begin(), matching_set.end(), 0);

    /***        Matching the Font Width         ***/
    {
        const FontWidth queryWidth = attributes.GetWidthType();
        const FontWidth matching_stretch = [&]() -> FontWidth
        {
            // Check for fonts with the exact queryWidth
            if (std::any_of(
                    matching_set.begin(), matching_set.end(),
                    [&](size_t index) {
                        return m_aFonts[index].m_aFontAttributes.GetWidthType()
                               == queryWidth;
                    }))
            {
                return queryWidth;
            }

            if (queryWidth <= FontWidth::WIDTH_NORMAL)
            {
                // Finds the closest condensed font before looking for the closest expanded font
                const int min_element = *std::min_element(
                    matching_set.begin(), matching_set.end(),
                    [&](size_t a, size_t b)
                    {
                        const FontWidth a_width
                            = m_aFonts[a].m_aFontAttributes.GetWidthType();
                        const FontWidth b_width
                            = m_aFonts[b].m_aFontAttributes.GetWidthType();

                        const int a_diff = a_width > queryWidth ? a_width : queryWidth - a_width;
                        const int b_diff = b_width > queryWidth ? b_width : queryWidth - b_width;
                        return a_diff < b_diff;
                    });
                return m_aFonts[min_element].m_aFontAttributes.GetWidthType();
            }
            else
            {
                // Finds the closest expanded font before looking for the closest condensed font
                const int min_element = *std::min_element(
                    matching_set.begin(), matching_set.end(),
                    [&](size_t a, size_t b)
                    {
                        const FontWidth a_width
                            = m_aFonts[a].m_aFontAttributes.GetWidthType();
                        const FontWidth b_width
                            = m_aFonts[b].m_aFontAttributes.GetWidthType();

                        // WIDTH_ULTRA_EXPANDED is the largest FontWidth ENUM
                        const int a_diff = a_width >= queryWidth
                                               ? a_width - queryWidth
                                               : FontWidth::WIDTH_ULTRA_EXPANDED - a_width;
                        const int b_diff = b_width >= queryWidth
                                               ? b_width - queryWidth
                                               : FontWidth::WIDTH_ULTRA_EXPANDED - b_width;
                        return a_diff < b_diff;
                    });
                return m_aFonts[min_element].m_aFontAttributes.GetWidthType();
            }
        }();
        std::vector<fontID> matching_set_temp;
        for (int num : matching_set)
        {
            if (m_aFonts[num].m_aFontAttributes.GetWidthType() == matching_stretch)
            {
                matching_set_temp.push_back(num);
            }
        }
        matching_set = matching_set_temp;
    }

    /***        Matching the Font Style         ***/
    {
        const std::array<FontItalic, 3> style_preference = [&]() -> std::array<FontItalic, 3>
        {
            switch (attributes.GetItalic())
            {
                case FontItalic::ITALIC_NORMAL:
                    return { FontItalic::ITALIC_NORMAL, FontItalic::ITALIC_OBLIQUE,
                             FontItalic::ITALIC_NONE };

                case FontItalic::ITALIC_OBLIQUE:
                    return { FontItalic::ITALIC_OBLIQUE, FontItalic::ITALIC_NORMAL,
                             FontItalic::ITALIC_NONE };

                // case FontItalic::ITALIC_NONE:
                default:
                    return { FontItalic::ITALIC_NONE, FontItalic::ITALIC_OBLIQUE,
                             FontItalic::ITALIC_NORMAL };
            }
        }();

        // Match based on the Style Preference priority
        const FontItalic matching_style = *std::find_if(
            style_preference.begin(), style_preference.end(),
            [&](FontItalic query_style)
            {
                return std::any_of(
                    matching_set.begin(), matching_set.end(),
                    [&](size_t index) {
                        return m_aFonts[index].m_aFontAttributes.GetItalic()
                               == query_style;
                    });
            });
        std::vector<fontID> matching_set_temp;
        for (int num : matching_set)
        {
            if (m_aFonts[num].m_aFontAttributes.GetItalic() == matching_style)
            {
                matching_set_temp.push_back(num);
            }
        }
        matching_set = matching_set_temp;
    }

    /***        Matching the Font Weight        ***/
    {
        const FontWeight queryWeight = attributes.GetWeight();
        const int queryWeightValue = getFontWeightValue(queryWeight);
        const FontWeight matching_weight = [&]() -> FontWeight
        {
            if (std::any_of(matching_set.begin(), matching_set.end(),
                            [&](size_t index) {
                                return m_aFonts[index].m_aFontAttributes.GetWeight()
                                       == queryWeight;
                            }))
            {
                return queryWeight;
            }

            // Check for 400 - 500 Font Weight Values
            if (400 <= queryWeightValue && queryWeightValue <= 450)
            {
                if (std::any_of(
                        matching_set.begin(), matching_set.end(),
                        [&](size_t index) {
                            return getFontWeightValue(
                                       m_aFonts[index].m_aFontAttributes.GetWeight())
                                   == 500;
                        }))
                {
                    return FontWeight::WEIGHT_MEDIUM;
                }
            }
            else if (450 < queryWeightValue && queryWeightValue <= 500)
            {
                if (std::any_of(
                        matching_set.begin(), matching_set.end(),
                        [&](size_t index) {
                            return getFontWeightValue(
                                       m_aFonts[index].m_aFontAttributes.GetWeight())
                                   == 400;
                        }))
                {
                    return FontWeight::WEIGHT_NORMAL;
                }
            }

            // Match based on the closest Font Weight Value
            if (queryWeightValue < 400)
            {
                // Check for < 400 Font Weight Values
                const int min_element = *std::min_element(
                    matching_set.begin(), matching_set.end(),
                    [&](size_t a, size_t b)
                    {
                        const int a_weight = getFontWeightValue(
                            m_aFonts[a].m_aFontAttributes.GetWeight());
                        const int b_weight = getFontWeightValue(
                            m_aFonts[b].m_aFontAttributes.GetWeight());

                        const int a_diff
                            = a_weight > queryWeightValue ? a_weight : queryWeightValue - a_weight;
                        const int b_diff
                            = b_weight > queryWeightValue ? b_weight : queryWeightValue - b_weight;
                        return a_diff < b_diff;
                    });
                return m_aFonts[min_element].m_aFontAttributes.GetWeight();
            }
            else
            {
                // Check for > 500 Font Weight Values
                const int min_element = *std::min_element(
                    matching_set.begin(), matching_set.end(),
                    [&](size_t a, size_t b)
                    {
                        const int a_weight = getFontWeightValue(
                            m_aFonts[a].m_aFontAttributes.GetWeight());
                        const int b_weight = getFontWeightValue(
                            m_aFonts[b].m_aFontAttributes.GetWeight());

                        // WEIGHT_BLACK is the largest FontWeight ENUM
                        const int a_diff
                            = a_weight >= queryWeightValue
                                  ? a_weight - queryWeightValue
                                  : getFontWeightValue(FontWeight::WEIGHT_BLACK) - a_weight;
                        const int b_diff
                            = b_weight >= queryWeightValue
                                  ? b_weight - queryWeightValue
                                  : getFontWeightValue(FontWeight::WEIGHT_BLACK) - b_weight;
                        return a_diff < b_diff;
                    });
                return m_aFonts[min_element].m_aFontAttributes.GetWeight();
            }
        }();
        std::vector<fontID> matching_set_temp;
        for (int num : matching_set)
        {
            if (m_aFonts[num].m_aFontAttributes.GetWeight() == matching_weight)
            {
                matching_set_temp.push_back(num);
            }
        }
        matching_set = matching_set_temp;
    }

    /***    Return all fonts that matched the width / style / weight criteria   ***/
    return matching_set;
}

/**
 * @brief 
 * 
 * @param attributes 
 * @param fontset 
 * @return std::vector<fontID> 
 */
std::vector<fontID> PrintFontManager::MatchCharSet(const FontAttributes& attributes, 
                                                   const std::vector<fontID> fontset)
{
    if (attributes.GetCodepointBitmap().isEmpty())
    {
        return fontset;
    }

    std::unordered_map<size_t, int> missingCharacters;
    for (int num : fontset)
    {
        int difference
            = (attributes.GetCodepointBitmap() - m_aFonts[num].m_aFontAttributes.GetCodepointBitmap())
                  .cardinality();
        missingCharacters.emplace(num, difference);
    }
    const int min_element
        = *std::min_element(fontset.begin(), fontset.end(), [&](size_t a, size_t b)
                        { return missingCharacters[fontset[a]] < missingCharacters[fontset[b]]; });
    std::vector<fontID> matching_set_temp;
    for (int num : fontset)
    {
        if (missingCharacters[num] == missingCharacters[min_element])
        {
            matching_set_temp.push_back(num);
        }
    }
    return matching_set_temp;
}

// The ranking of the fonts by preference
const std::map<std::string, int> fontRankings = { 
    { "Noto Serif", 0 },
    { "Liberation Serif", 1 },
    { "Noto Sans", 2 },
    { "Liberation Sans", 3 },
    { "Liberation Mono", 4 },
    { "Liberation Sans Narrow", 5 },
    { "Caladea", 6 },
    { "Carlito", 7 },
    { "OpenSymbol", 8 }
};

/**
 * @brief Getting the best ranked font from the fontset
 * 
 * @param fontset 
 * @return PrintFont* 
 */
PrintFont* PrintFontManager::GetBestRankedFont(const std::vector<fontID> fontset)
{
    std::map<int, int> ranking_map;
    for (int fontid: fontset) { 
        ranking_map[fontid] = fontRankings.find(
            std::string(m_aFonts[fontid].m_aFontAttributes.GetFamilyName().toUtf8())
            )->second;
    }
    const int min_element = *std::min_element(
        fontset.begin(), fontset.end(),
        [&](fontID a, fontID b)
        {
            return ranking_map[a] < ranking_map[b];
        });
    return &m_aFonts[min_element];
}

/**
 * @brief Assigns default FontAttribute styles if not specified
 * 
 * @param attributes 
 */
void FADefaultSubstitute(FontAttributes& attributes)
{
    if (attributes.GetFamilyType() == FontFamily::FAMILY_DONTKNOW)
        attributes.SetFamilyType(FontFamily::FAMILY_ROMAN);

    if (attributes.GetWidthType() == FontWidth::WIDTH_DONTKNOW)
        attributes.SetWidthType(FontWidth::WIDTH_NORMAL);

    if (attributes.GetWeight() == FontWeight::WEIGHT_DONTKNOW)
        attributes.SetWeight(FontWeight::WEIGHT_NORMAL);
}

/**
 * @brief Matches the Font to the list of supported fonts in PrintFontManager
 * https://www.w3.org/TR/2018/REC-css-fonts-3-20180920/#font-matching-algorithm
 * 
 * @param attributes 
 * @return PrintFontManager::PrintFont* 
 */
PrintFontManager::PrintFont*
PrintFontManager::FontSetMatch_Configless(const FontAttributes& attributes)
{
    if (PrintFontManager::PrintFont* result = MatchFromCompatibilityChart(attributes))
        return result;

    // Try to match to a Sans or Serif family
    if (PrintFontManager::PrintFont* result = MatchSansOrSerifFamily(attributes))
        return result;

    // Font Face Matching Algorithm
    std::vector<fontID> matched_set;
    if (matched_set = MatchWidthStyleWeight(attributes); matched_set.empty()) {
        // Add back in OpenSymbol
        const fontID OpenSymbolID = 32;
        if (std::find(matched_set.begin(), matched_set.end(), OpenSymbolID) == matched_set.end()) {
            matched_set.push_back(OpenSymbolID);
        }

        // Match by Font Character Set, This might not apply 
        matched_set = MatchCharSet(attributes, matched_set);

        // Select best RankedFont
        return GetBestRankedFont(matched_set);
    }
    /*
        Last Fallback - Noto Serif
        Noto Serif supports a wide range of languages and scripts
        and conveys a formal and literary feel to documents and texts
    */
    return getFontFromAttributes("Noto Serif", attributes.GetItalic(), attributes.GetWeight());
}

/**
 * @brief Initialized harcoded PrintFont Data
 * 
 */
void PrintFontManager::CountFontConfigFonts_Configless()
{
    FontData::populatePrintFontManagerData(m_aFonts, m_aFontFileToFontID);
    getDirectoryAtom(OString("/instdir/share/fonts/truetype"));
    getDirectoryAtom(OString("/instdir/program/resource/common/fonts"));
    m_nNextFontID = 33;
}

/**
 * @brief Matches the Font to the list of supported fonts
 * 
 * @param rDFA 
 * @return true 
 * @return false 
 */
bool PrintFontManager::MatchFont_Configless(FontAttributes& rDFA)
{
    FADefaultSubstitute(rDFA);
    PrintFont* matchedPrintFont = FontSetMatch_Configless(rDFA);
    if (matchedPrintFont)
    {
        rDFA = matchedPrintFont->m_aFontAttributes;
        return true;
    }
    return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
