

/**
 * @brief ROARING BITMAP TEST 1
 */

roaring::Roaring bitmap;

CPPUNIT_ASSERT(!bitmap.contains(100));
bitmap.add(100);
CPPUNIT_ASSERT(bitmap.contains(100));

CPPUNIT_ASSERT(!bitmap.contains(1000));
bitmap.add(1000);
CPPUNIT_ASSERT(bitmap.contains(1000));

/**
 * @brief ROARING BITMAP TEST 2
 */

roaring::Roaring bitmap;

CPPUNIT_ASSERT(!bitmap.contains(100));
CPPUNIT_ASSERT(!bitmap.contains(102));

std::vector<uint32_t> enabledBits;
enabledBits.push_back(100);
enabledBits.push_back(102);

CPPUNIT_ASSERT(bitmap.contains(100));
CPPUNIT_ASSERT(bitmap.contains(102));

/**
 * @brief FONT DATA TEST
 */

std::vector<std::string> font_names =
    {
        "Caladea",
        "Caladea",
        "Caladea",
        "Caladea",
        "Carlito",
        "Carlito",
        "Carlito",
        "Carlito",
        "Liberation Mono",
        "Liberation Mono",
        "Liberation Mono",
        "Liberation Mono",
        "Liberation Sans",
        "Liberation Sans",
        "Liberation Sans",
        "Liberation Sans",
        "Liberation Sans Narrow",
        "Liberation Sans Narrow",
        "Liberation Sans Narrow",
        "Liberation Sans Narrow",
        "Liberation Serif",
        "Liberation Serif",
        "Liberation Serif",
        "Liberation Serif",
        "Noto Sans",
        "Noto Sans",
        "Noto Sans",
        "Noto Sans",
        "Noto Serif",
        "Noto Serif",
        "Noto Serif",
        "Noto Serif",
        "OpenSymbol"};

std::vector<std::string> font_styles =
    {
        "Bold",
        "Bold Italic",
        "Italic",
        "Regular",
        "Bold",
        "Bold Italic",
        "Italic",
        "Regular",
        "Bold",
        "Bold Italic",
        "Italic",
        "Regular",
        "Bold",
        "Bold Italic",
        "Italic",
        "Regular",
        "Bold",
        "Bold Italic",
        "Italic",
        "Regular",
        "Bold",
        "Bold Italic",
        "Italic",
        "Regular",
        "Bold",
        "Bold Italic",
        "Italic",
        "Regular",
        "Bold",
        "Regular",
        "Bold Italic",
        "Italic",
        "Regular"};

for (int i = 0; i < 33; i++)
{
    const PrintFont *font = getFont(i)
        CPPUNIT_ASSERT(font != nullptr);
    const bool has_correct_font_attr = OUString::createFromAscii(font_names[i].c_str()) == font->m_aFontAttributes.GetFamilyName() &&
                                       OUString::createFromAscii(font_styles[i].c_str()) == font->m_aFontAttributes.GetStyleName());
    CPPUNIT_ASSERT(has_correct_font_attr);
}
CPPUNIT_ASSERT(getFont(33) == nullptr);

/**
 * @brief getFontFromAttributes
 */

bool has_correct_font_attr = false;
PrintFont *font = nullptr;

font = getFontFromAttributes("Caladea", FontItalic::ITALIC_NONE, FontWeight::WEIGHT_SEMIBOLD);
CPPUNIT_ASSERT(font != nullptr);

has_correct_font_attr = OUString::createFromAscii("Caladea") == font->m_aFontAttributes.GetFamilyName() &&
                        FontWeight::WEIGHT_BOLD == font->m_aFontAttributes.GetWeight() &&
                        FontItalic::ITALIC_NONE == font->m_aFontAttributes.GetItalic();
CPPUNIT_ASSERT(has_correct_font_attr);

font = getFontFromAttributes("Noto Serif", FontItalic::ITALIC_OBLIQUE, FontWeight::WEIGHT_THIN);
CPPUNIT_ASSERT(font != nullptr);

has_correct_font_attr = OUString::createFromAscii("Noto Serif") == font->m_aFontAttributes.GetFamilyName() &&
                        FontWeight::WEIGHT_NORMAL == font->m_aFontAttributes.GetWeight() &&
                        FontItalic::ITALIC_NORMAL == font->m_aFontAttributes.GetItalic();
CPPUNIT_ASSERT(has_correct_font_attr);

font = getFontFromAttributes("Liberation Serif", FontItalic::ITALIC_NORMAL, FontWeight::WEIGHT_NORMAL);
CPPUNIT_ASSERT(font != nullptr);

has_correct_font_attr = OUString::createFromAscii("Liberation Serif") == font->m_aFontAttributes.GetFamilyName() &&
                        FontWeight::WEIGHT_NORMAL == font->m_aFontAttributes.GetWeight() &&
                        FontItalic::ITALIC_NORMAL == font->m_aFontAttributes.GetItalic();

CPPUNIT_ASSERT(has_correct_font_attr);

font = getFontFromAttributes("Liberation Serif", FontItalic::ITALIC_NONE, FontWeight::WEIGHT_BLACK);
CPPUNIT_ASSERT(font != nullptr);

has_correct_font_attr = OUString::createFromAscii("Liberation Serif") == font->m_aFontAttributes.GetFamilyName() &&
                        FontWeight::WEIGHT_BOLD == font->m_aFontAttributes.GetWeight() &&
                        FontItalic::ITALIC_NONE == font->m_aFontAttributes.GetItalic();

CPPUNIT_ASSERT(has_correct_font_attr);

font = getFontFromAttributes("Times New Roman", FontItalic::ITALIC_NONE, FontWeight::WEIGHT_BLACK);
CPPUNIT_ASSERT(font == nullptr);

/**
 * @brief MatchFromCompatibilityChart
 */

PrintFont *font = nullptr;
FontAttributes attributes;
bool has_correct_font_attr = false;

attributes.SetFamilyName(OUString::createFromAscii("Helvetica"));
attributes.SetItalic(FontItalic::ITALIC_NORMAL);
attributes.SetWeight(FontWeight::WEIGHT_NORMAL);

font = MatchFromCompatibilityChart(attributes);
CPPUNIT_ASSERT(font != nullptr);

has_correct_font_attr = OUString::createFromAscii("Liberation Sans") == font->m_aFontAttributes.GetFamilyName() &&
                        FontWeight::WEIGHT_NORMAL == font->m_aFontAttributes.GetWeight() &&
                        FontItalic::ITALIC_NORMAL == font->m_aFontAttributes.GetItalic();
CPPUNIT_ASSERT(has_correct_font_attr);

attributes.SetFamilyName(OUString::createFromAscii("Symbol"));
attributes.SetItalic(FontItalic::ITALIC_OBLIQUE);
attributes.SetWeight(FontWeight::WEIGHT_THIN);

font = MatchFromCompatibilityChart(attributes);
CPPUNIT_ASSERT(font != nullptr);

has_correct_font_attr = OUString::createFromAscii("OpenSymbol") == font->m_aFontAttributes.GetFamilyName() &&
                        FontWeight::WEIGHT_NORMAL == font->m_aFontAttributes.GetWeight() &&
                        FontItalic::ITALIC_NONE == font->m_aFontAttributes.GetItalic();
CPPUNIT_ASSERT(has_correct_font_attr);

attributes.SetFamilyName(OUString::createFromAscii("Liberation Mono"));
attributes.SetItalic(FontItalic::ITALIC_OBLIQUE);
attributes.SetWeight(FontWeight::WEIGHT_BLACK);

font = MatchFromCompatibilityChart(attributes);
CPPUNIT_ASSERT(font != nullptr);

has_correct_font_attr = OUString::createFromAscii("Liberation Mono") == font->m_aFontAttributes.GetFamilyName() &&
                        FontWeight::WEIGHT_BOLD == font->m_aFontAttributes.GetWeight() &&
                        FontItalic::ITALIC_NORMAL == font->m_aFontAttributes.GetItalic();
CPPUNIT_ASSERT(has_correct_font_attr);

attributes.SetFamilyName(OUString::createFromAscii("Bookman Old Style"));
attributes.SetItalic(FontItalic::ITALIC_NONE);
attributes.SetWeight(FontWeight::WEIGHT_ULTRABOLD);

font = MatchFromCompatibilityChart(attributes);
CPPUNIT_ASSERT(font != nullptr);

has_correct_font_attr = OUString::createFromAscii("Carlito") == font->m_aFontAttributes.GetFamilyName() &&
                        FontWeight::WEIGHT_BOLD == font->m_aFontAttributes.GetWeight() &&
                        FontItalic::ITALIC_NONE == font->m_aFontAttributes.GetItalic();
CPPUNIT_ASSERT(has_correct_font_attr);

font = MatchFromCompatibilityChart(attributes);
CPPUNIT_ASSERT(font == nullptr);

/**
 * @brief MatchSansOrSerifFamily
 */

PrintFont *font = nullptr;
FontAttributes attributes;
bool has_correct_font_attr = false;

attributes.SetFamilyName(OUString::createFromAscii("Roboto"));
attributes.SetFamilyType(FontFamily::FAMILY_MODERN);
attributes.SetItalic(FontItalic::ITALIC_NORMAL);
attributes.SetWeight(FontWeight::WEIGHT_NORMAL);

font = MatchSansOrSerifFamily(attributes);
CPPUNIT_ASSERT(font != nullptr);

has_correct_font_attr = OUString::createFromAscii("Noto Sans") == font->m_aFontAttributes.GetFamilyName() &&
                        FontWeight::WEIGHT_NORMAL == font->m_aFontAttributes.GetWeight() &&
                        FontItalic::ITALIC_NORMAL == font->m_aFontAttributes.GetItalic();
CPPUNIT_ASSERT(has_correct_font_attr);

attributes.SetFamilyName(OUString::createFromAscii("Times New Roman"));
attributes.SetFamilyType(FontFamily::FAMILY_ROMAN);
attributes.SetItalic(FontItalic::ITALIC_OBLIQUE);
attributes.SetWeight(FontWeight::WEIGHT_BLACK);

font = MatchSansOrSerifFamily(attributes);
CPPUNIT_ASSERT(font != nullptr);

has_correct_font_attr = OUString::createFromAscii("Noto Serif") == font->m_aFontAttributes.GetFamilyName() &&
                        FontWeight::WEIGHT_BOLD == font->m_aFontAttributes.GetWeight() &&
                        FontItalic::ITALIC_NORMAL == font->m_aFontAttributes.GetItalic();
CPPUNIT_ASSERT(has_correct_font_attr);

attributes.SetFamilyName(OUString::createFromAscii("New Font"));
attributes.SetFamilyType(FontFamily::FAMILY_DECORATIVE);
attributes.SetItalic(FontItalic::ITALIC_NORMAL);
attributes.SetWeight(FontWeight::WEIGHT_NORMAL);

font = MatchSansOrSerifFamily(attributes);
CPPUNIT_ASSERT(font == nullptr);

/**
 * @brief MatchWidthStyleWeight
 */

FontAttributes attributes;
std::vector<fontID> fonts = {};

attributes.SetFamilyName(OUString::createFromAscii("Roboto"));
attributes.SetItalic(FontItalic::ITALIC_NONE);
attributes.SetWidthType(FontWidth::WIDTH_NORMAL);
attributes.SetWeight(FontWeight::WEIGHT_NORMAL);
fonts = MatchWidthStyleWeight(attributes);
CPPUNIT_ASSERT(std::vector<fontID>({3, 7, 11, 15, 23, 27, 29, 32}) == fonts);

attributes.SetFamilyName(OUString::createFromAscii("Roboto"));
attributes.SetItalic(FontItalic::ITALIC_OBLIQUE);
attributes.SetWidthType(FontWidth::WIDTH_CONDENSED);
attributes.SetWeight(FontWeight::WEIGHT_NORMAL);
fonts = MatchWidthStyleWeight(attributes);
CPPUNIT_ASSERT(std::vector<fontID>({18}) == fonts);

attributes.SetFamilyName(OUString::createFromAscii("Roboto"));
attributes.SetItalic(FontItalic::ITALIC_NORMAL);
attributes.SetWidthType(FontWidth::WIDTH_EXPANDED);
attributes.SetWeight(FontWeight::WEIGHT_BOLD);
fonts = MatchWidthStyleWeight(attributes);
CPPUNIT_ASSERT(std::vector<fontID>({1, 5, 9, 13, 21, 25, 30}) == fonts);

/**
 * @brief MatchCharSet
 *
 */

std::vector<fontID> fontset(33);
std::iota(fontset.begin(), fontset.end(), 0);

FontAttributes attributes;
attributes.SetFamilyName(OUString::createFromAscii("New Font"));

std::vector<fontID> matched_fonts = MatchCharSet(attributes, fontset);
CPPUNIT_ASSERT(fontset == matched_fonts);

std::cout << std::endl;
{
    // alpha-numeric characters
    roaring::Roaring roaring;
    for (int i = 45; i < 123; i++)
    {
        roaring.add(i);
    }
    attributes.SetBitmap(roaring);
    matched_fonts = MatchCharSet(attributes, fontset);
    std::vector<fontID> expected_fontset(32);
    std::iota(expected_fontset.begin(), expected_fontset.end(), 0);
    CPPUNIT_ASSERT(expected_fontset == matched_fonts);
}
{
    // Geometric Shapes:
    // U+9651 (△): White Up-Pointing Triangle
    // U+9655 (▵): White Up-Pointing Small Triangle
    // U+9661 (▽): White Down-Pointing Triangle
    roaring::Roaring roaring;
    roaring.add(9651);
    roaring.add(9655);
    roaring.add(9661);

    attributes.SetBitmap(roaring);
    matched_fonts = MatchCharSet(attributes, fontset);
    CPPUNIT_ASSERT(std::vector<fontID>({32}) == matched_fonts);
}

/**
 * @brief GetBestRankedFont
 *
 */

std::vector<fontID> fontset = {0, 4, 8, 12, 16, 20, 24, 28, 32};
PrintFont *printfont = GetBestRankedFont(fontset);
CPPUNIT_ASSERT("Noto Serif" == printfont->m_aFontAttributes.GetFamilyName());

fontset = {0, 4};
printfont = GetBestRankedFont(fontset);
CPPUNIT_ASSERT("Caladae" == printfont->m_aFontAttributes.GetFamilyName());

/**
 * @brief FontSetMatch_Configless
 *
 */
PrintFont *font = nullptr;
FontAttributes attributes;
bool has_correct_font_attr = false;

attributes.SetFamilyName(OUString::createFromAscii("Helvetica"));
attributes.SetItalic(FontItalic::ITALIC_NORMAL);
attributes.SetWeight(FontWeight::WEIGHT_NORMAL);
font = FontSetMatch_Configless(attributes);
CPPUNIT_ASSERT(font != nullptr);

has_correct_font_attr = OUString::createFromAscii("Liberation Sans") == font->m_aFontAttributes.GetFamilyName() &&
                        FontWeight::WEIGHT_NORMAL == font->m_aFontAttributes.GetWeight() &&
                        FontItalic::ITALIC_NORMAL == font->m_aFontAttributes.GetItalic();
CPPUNIT_ASSERT(has_correct_font_attr);

attributes.SetFamilyName(OUString::createFromAscii("Liberation Mono"));
attributes.SetItalic(FontItalic::ITALIC_OBLIQUE);
attributes.SetWeight(FontWeight::WEIGHT_BLACK);
font = FontSetMatch_Configless(attributes);
CPPUNIT_ASSERT(font != nullptr);

has_correct_font_attr = OUString::createFromAscii("Liberation Mono") == font->m_aFontAttributes.GetFamilyName() &&
                        FontWeight::WEIGHT_BOLD == font->m_aFontAttributes.GetWeight() &&
                        FontItalic::ITALIC_NORMAL == font->m_aFontAttributes.GetItalic();
CPPUNIT_ASSERT(has_correct_font_attr);
