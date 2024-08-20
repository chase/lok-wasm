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

#pragma once

#define USE_FONT_CONFIGLESS 1

#include <sal/config.h>

#include <o3tl/lru_map.hxx>
#include <o3tl/sorted_vector.hxx>
#include <tools/fontenum.hxx>
#include <vcl/dllapi.h>
#include <vcl/timer.hxx>
#include <com/sun/star/lang/Locale.hpp>
#include <unx/fc_fontoptions.hxx>

#include <font/PhysicalFontFace.hxx>

#include <set>
#include <memory>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <o3tl/hash_combine.hxx>

namespace
{
struct FontOptionsKey
{
    OUString m_sFamilyName;
    int m_nFontSize;
    FontItalic m_eItalic;
    FontWeight m_eWeight;
    FontWidth m_eWidth;
    FontPitch m_ePitch;

    bool operator==(const FontOptionsKey& rOther) const
    {
        return m_sFamilyName == rOther.m_sFamilyName && m_nFontSize == rOther.m_nFontSize
               && m_eItalic == rOther.m_eItalic && m_eWeight == rOther.m_eWeight
               && m_eWidth == rOther.m_eWidth && m_ePitch == rOther.m_ePitch;
    }
};
}

namespace std
{
template <> struct hash<FontOptionsKey>
{
    std::size_t operator()(const FontOptionsKey& k) const noexcept
    {
        std::size_t seed = k.m_sFamilyName.hashCode();
        o3tl::hash_combine(seed, k.m_nFontSize);
        o3tl::hash_combine(seed, k.m_eItalic);
        o3tl::hash_combine(seed, k.m_eWeight);
        o3tl::hash_combine(seed, k.m_eWidth);
        o3tl::hash_combine(seed, k.m_ePitch);
        return seed;
    }
};
}

class FontOptionsCache
{
private:
    o3tl::lru_map<FontOptionsKey, std::unique_ptr<FontConfigFontOptions>> lru_options_cache;

public:
    FontOptionsCache()
        : lru_options_cache(10) // arbitrary cache size of 10
    {
    }

    std::unique_ptr<FontConfigFontOptions> lookup(const FontOptionsKey& rKey)
    {
        auto it = lru_options_cache.find(rKey);
        if (it != lru_options_cache.end())
            return std::make_unique<FontConfigFontOptions>(*it->second);
        return nullptr;
    }

    void cache(const FontOptionsKey& rKey, const OString& fontfile)
    {
        lru_options_cache.insert(
            std::make_pair(rKey, std::make_unique<FontConfigFontOptions>(fontfile)));
    }
};

/*
 *  some words on metrics: every length returned by PrintFontManager and
 *  friends are PostScript afm style, that is they are 1/1000 font height
 */

class FontAttributes;
class FontConfigFontOptions;
namespace vcl::font
{
class FontSelectPattern;
}
namespace vcl
{
struct NameRecord;
}
class GenericUnixSalData;

namespace psp
{
class PPDParser;

typedef int fontID;

struct VCL_DLLPRIVATE PrintFont
{
    FontAttributes m_aFontAttributes;

    int m_nDirectory; // atom containing system dependent path
    OString m_aFontFile; // relative to directory
    int m_nCollectionEntry; // 0 for regular fonts, 0 to ... for fonts stemming from collections
    int m_nVariationEntry; // 0 for regular fonts, 0 to ... for fonts stemming from font variations

    explicit PrintFont();
};

// a class to manage printable fonts
class VCL_PLUGIN_PUBLIC PrintFontManager
{
    // PrintFontManager Singleton Instance

private:
    static std::unique_ptr<PrintFontManager> printFontManagerInstance;

    PrintFontManager();

    void initialize();

public:
    static PrintFontManager& get();

    PrintFontManager(PrintFontManager const&) = delete;
    void operator=(PrintFontManager const&) = delete;
    friend struct PrintFont;

    ~PrintFontManager();

private:
    fontID m_nNextFontID;
    std::unordered_map<fontID, PrintFont> m_aFonts;
    std::unordered_map<OString, o3tl::sorted_vector<fontID>>
        m_aFontFileToFontID; // for speeding up findFontFileID

    std::unordered_map<OString, int> m_aDirToAtom;
    std::unordered_map<int, OString> m_aAtomToDir;
    int m_nNextDirAtom;

    FontOptionsCache m_FontOptionsCache;

    std::vector<PrintFont> analyzeFontFile(int nDirID, const OString& rFileName,
                                           const char* pFormat = nullptr) const;
    bool analyzeSfntFile(PrintFont& rFont) const;
    // finds the font id for the nFaceIndex face in this font file
    // There may be multiple font ids for font collections
    fontID findFontFileID(int nDirID, const OString& rFile, int nFaceIndex,
                          int nVariationIndex) const;

    // There may be multiple font ids for font collections
    std::vector<fontID> findFontFileIDs(int nDirID, const OString& rFile) const;

    static FontFamily matchFamilyName(std::u16string_view rFamily);

    OString getDirectory(int nAtom) const;
    int getDirectoryAtom(const OString& rDirectory);

    /* try to initialize fonts from libfontconfig

    called from <code>initialize()</code>
    */
    static void initFontconfig();
    void countFontconfigFonts();
    void CountFontConfigFonts_Configless();
    /* deinitialize fontconfig
     */
    static void deinitFontconfig();

    /* register an application specific font directory for libfontconfig

    since fontconfig is asked for font substitutes before OOo will check for font availability
    and fontconfig will happily substitute fonts it doesn't know (e.g. "Arial Narrow" -> "DejaVu Sans Book"!)
    it becomes necessary to tell the library about all the hidden font treasures
    */
    static void addFontconfigDir(const OString& rDirectory);

    /* register an application specific font file for libfontconfig */
    static void addFontconfigFile(const OString& rFile);

    PrintFont* getFontFromAttributes(const std::string fontfamily, const FontItalic fontItalic,
                                     const FontWeight fontWeight);
    PrintFont* MatchFromCompatibilityChart(const FontAttributes& attributes);
    PrintFont* MatchSansOrSerifFamily(const FontAttributes& attributes);
    std::vector<fontID> MatchWidthStyleWeight(const FontAttributes& attributes);
    std::vector<fontID> MatchCharSet(const FontAttributes& attributes, const std::vector<fontID> fontset);
    PrintFont* GetBestRankedFont(const std::vector<fontID> fontset);
    PrintFont* FontSetMatch_Configless(const FontAttributes& attributes);

    std::set<OString> m_aPreviousLangSupportRequests;
    std::vector<OUString> m_aCurrentRequests;
    Timer m_aFontInstallerTimer;

    DECL_DLLPRIVATE_LINK(autoInstallFontLangSupport, Timer*, void);

public:
    // There may be multiple font ids for font collections
    std::vector<fontID> addFontFile(std::u16string_view rFileUrl);

    const PrintFont* getFont(fontID nID) const
    {
        auto it = m_aFonts.find(nID);
        return it == m_aFonts.end() ? nullptr : &it->second;
    }

    OString getFontFile(const PrintFont& rFont) const;

    // returns the ids of all managed fonts.
    void getFontList(std::vector<fontID>& rFontIDs);

    // routines to get font info in small pieces

    // get a specific fonts system dependent filename
    OString getFontFileSysPath(fontID nFontID) const { return getFontFile(*getFont(nFontID)); }

    // get the ttc face number
    int getFontFaceNumber(fontID nFontID) const;

    // get the ttc face variation
    int getFontFaceVariation(fontID nFontID) const;

    // font administration functions

    /*  system dependent font matching

    <p>
    <code>matchFont</code> matches a pattern of font characteristics
    and returns the closest match if possible. If a match was found
    it will update rDFA to the found matching font.
    </p>
    <p>
    implementation note: currently the function is only implemented
    for fontconfig.
    </p>

    @param rDFA
    out of the FontAttributes structure the following
    fields will be used for the match:
    <ul>
    <li>family name</li>
    <li>italic</li>
    <li>width</li>
    <li>weight</li>
    <li>pitch</li>
    </ul>

    @param rLocale
    if <code>rLocal</code> contains non empty strings the corresponding
    locale will be used for font matching also; e.g. "Sans" can result
    in different fonts in e.g. english and japanese
     */
    bool matchFont(FontAttributes& rDFA, const css::lang::Locale& rLocale);
    bool MatchFont_Configless(FontAttributes& rDFA);

    static std::unique_ptr<FontConfigFontOptions>
        getFontOptions(const FontAttributes& rFontAttributes, int nSize);
    std::unique_ptr<FontConfigFontOptions>
        GetFontOptions_Configless(const FontAttributes& rFontAttributes, int nSize);

    void Substitute(vcl::font::FontSelectPattern& rPattern, OUString& rMissingCodes);
    void Substitute_Configless(vcl::font::FontSelectPattern& rPattern, OUString& rMissingCodes);
};

} // namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
