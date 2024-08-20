/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* 
The purpose of this file is to store all FontConfig data that will be injected into PrintFont.
See MACRO-2573
*/

#include <map>
#include <vector>
#include <rtl/ustring.hxx>
#include <unx/fontdata.hxx>

#include "roaring.hh"
#include "roaring.c"

namespace psp
{

/*
* Creating Print Fonts Data
*/
PrintFont FontData::GetPrintFontCaladeaBold()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("Caladea-Bold.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetCaladeaBoldFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontCaladeaBoldItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("Caladea-BoldItalic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetCaladeaBoldItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontCaladeaItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("Caladea-Italic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetCaladeaItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontCaladeaRegular()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("Caladea-Regular.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetCaladeaRegularFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontCarlitoBold()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("Carlito-Bold.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetCarlitoBoldFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontCarlitoBoldItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("Carlito-BoldItalic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetCarlitoBoldItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontCarlitoItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("Carlito-Italic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetCarlitoItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontCarlitoRegular()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("Carlito-Regular.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetCarlitoRegularFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationMonoBold()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationMono-Bold.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationMonoBoldFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationMonoBoldItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationMono-BoldItalic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationMonoBoldItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationMonoItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationMono-Italic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationMonoItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationMonoRegular()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationMono-Regular.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationMonoRegularFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationSansBold()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationSans-Bold.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationSansBoldFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationSansBoldItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationSans-BoldItalic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationSansBoldItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationSansItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationSans-Italic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationSansItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationSansRegular()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationSans-Regular.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationSansRegularFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationSansNarrowBold()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationSansNarrow-Bold.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationSansNarrowBoldFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationSansNarrowBoldItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationSansNarrow-BoldItalic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationSansNarrowBoldItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationSansNarrowItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationSansNarrow-Italic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationSansNarrowItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationSansNarrowRegular()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationSansNarrow-Regular.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationSansNarrowRegularFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationSerifBold()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationSerif-Bold.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationSerifBoldFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationSerifBoldItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationSerif-BoldItalic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationSerifBoldItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationSerifItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationSerif-Italic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationSerifItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontLiberationSerifRegular()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("LiberationSerif-Regular.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetLiberationSerifRegularFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontNotoSansBold()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("NotoSans-Bold.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetNotoSansBoldFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontNotoSansBoldItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("NotoSans-BoldItalic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetNotoSansBoldItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontNotoSansItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("NotoSans-Italic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetNotoSansItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontNotoSansRegular()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("NotoSans-Regular.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetNotoSansRegularFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontNotoSerifBold()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("NotoSerif-Bold.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetNotoSerifBoldFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontNotoSerifRegular()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("NotoSerif-Regular.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetNotoSerifRegularFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontNotoSerifBoldItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("NotoSerif-BoldItalic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetNotoSerifBoldItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontNotoSerifItalic()
{
    PrintFont printFont;
    printFont.m_nDirectory = 1;
    printFont.m_aFontFile = OString("NotoSerif-Italic.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetNotoSerifItalicFont(printFont.m_aFontAttributes);
    return printFont;
}

PrintFont FontData::GetPrintFontOpens()
{
    PrintFont printFont;
    printFont.m_nDirectory = 2;
    printFont.m_aFontFile = OString("opens___.ttf");
    printFont.m_nCollectionEntry = 0;
    printFont.m_nVariationEntry = 0;
    SetOpenSymbolRegularFont(printFont.m_aFontAttributes);
    return printFont;
}


/*
* Font Character Set Data
*/
const FontCharSet charset_CaladeaBold = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "00040000", "00000000", "00000000", "fc000000"}},
    { 0x0002, {"00000000", "00800000", "00000000", "00000000", "00000000", "00000000", "3f0002c0", "00000000"}},
    { 0x001e, {"00000000", "00000000", "00000000", "00000000", "0000003f", "30000000", "00000000", "030c0000"}},
    { 0x0020, {"7fb80000", "560d0047", "00000010", "80000000", "00000000", "00001098", "00000000", "00000000"}},
    { 0x0021, {"00480020", "00004044", "78000000", "00000000", "003f0000", "00000100", "00000000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000033", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f6, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000008", "00000000"}},
    { 0x00fb, {"00000006", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_CaladeaBoldItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "00040000", "00000000", "00000000", "fc000000"}},
    { 0x0002, {"00000000", "00800000", "00000000", "00000000", "00000000", "00000000", "3f0002c0", "00000000"}},
    { 0x001e, {"00000000", "00000000", "00000000", "00000000", "0000003f", "30000000", "00000000", "030c0000"}},
    { 0x0020, {"7fb80000", "560d0047", "00000010", "80000000", "00000000", "00001098", "00000000", "00000000"}},
    { 0x0021, {"00480020", "00004044", "78000000", "00000000", "003f0000", "00000100", "00000000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000033", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f6, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000008", "00000000"}},
    { 0x00fb, {"00000006", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_CaladeaItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "00040000", "00000000", "00000000", "fc000000"}},
    { 0x0002, {"00000000", "00800000", "00000000", "00000000", "00000000", "00000000", "3f0002c0", "00000000"}},
    { 0x001e, {"00000000", "00000000", "00000000", "00000000", "0000003f", "30000000", "00000000", "030c0000"}},
    { 0x0020, {"7fb80000", "560d0047", "00000010", "80000000", "00000000", "00001098", "00000000", "00000000"}},
    { 0x0021, {"00480020", "00004044", "78000000", "00000000", "003f0000", "00000100", "00000000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000033", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f6, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000008", "00000000"}},
    { 0x00fb, {"00000006", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_CaladeaRegular = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "00040000", "00000000", "00000000", "fc000000"}},
    { 0x0002, {"00000000", "00800000", "00000000", "00000000", "00000000", "00000000", "3f0002c0", "00000000"}},
    { 0x001e, {"00000000", "00000000", "00000000", "00000000", "0000003f", "30000000", "00000000", "030c0000"}},
    { 0x0020, {"7fb80000", "560d0047", "00000010", "80000000", "00000000", "00001098", "00000000", "00000000"}},
    { 0x0021, {"00480020", "00004044", "78000000", "00000000", "003f0000", "00000100", "00000000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000033", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f6, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000008", "00000000"}},
    { 0x00fb, {"00000006", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_CarlitoBold = 
{
    { 0x0000, {"00000001", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffdffe", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "fbffffff", "fffff9ff", "ffafffff"}},
    { 0x0003, {"ff3be020", "fe4c8e27", "feffffc8", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffbfffff", "ffffff7f", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"000fffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x000e, {"00000000", "80000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x001d, {"fffffeff", "ffffffff", "fdffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "bfdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"fffcffff", "76198047", "c0000010", "fff30000", "001fffff", "033fffff", "20000000", "00000000"}},
    { 0x0021, {"00c80020", "00044045", "fff86000", "00000000", "03ff0018", "00000100", "00000000", "00000000"}},
    { 0x0022, {"c4268044", "00000a00", "00000100", "00000033", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0024, {"00000000", "00000000", "00000000", "000fffff", "00000000", "00000000", "00000000", "801ffc00"}},
    { 0x0025, {"01111005", "00000000", "00000000", "00000000", "00000000", "00000c02", "00009c00", "00000040"}},
    { 0x0026, {"00000001", "10000000", "00000000", "00000200", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0027, {"00000000", "00000000", "00000000", "ffc00000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00f01fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"07800000", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f8, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00fb, {"0000001f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}}
};

const FontCharSet charset_CarlitoBoldItalic = 
{
    { 0x0000, {"00000001", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffdffe", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "fbffffff", "fffff9ff", "ffafffff"}},
    { 0x0003, {"ff3be020", "fe4c8e27", "feffffc8", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffbfffff", "ffffff7f", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"000fffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x000e, {"00000000", "80000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x001d, {"fffffeff", "ffffffff", "fdffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "bfdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"fffcffff", "76198047", "c0000010", "fff30000", "001fffff", "033fffff", "20000000", "00000000"}},
    { 0x0021, {"00c80020", "00044045", "fff86000", "00000000", "03ff0018", "00000100", "00000000", "00000000"}},
    { 0x0022, {"c4268044", "00000a00", "00000100", "00000033", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0024, {"00000000", "00000000", "00000000", "000fffff", "00000000", "00000000", "00000000", "801ffc00"}},
    { 0x0025, {"01111005", "00000000", "00000000", "00000000", "00000000", "00000c02", "00009c00", "00000040"}},
    { 0x0026, {"00000001", "10000000", "00000000", "00000200", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0027, {"00000000", "00000000", "00000000", "ffc00000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00f01fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"07800000", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f8, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00fb, {"0000001f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}}
};

const FontCharSet charset_CarlitoItalic = 
{
    { 0x0000, {"00000001", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffdffe", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "fbffffff", "fffff9ff", "ffafffff"}},
    { 0x0003, {"ff3be020", "fe4c8e27", "feffffc8", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffbfffff", "ffffff7f", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"000fffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x000e, {"00000000", "80000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x001d, {"fffffeff", "ffffffff", "fdffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "bfdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"fffcffff", "76198047", "c0000010", "fff30000", "001fffff", "033fffff", "20000000", "00000000"}},
    { 0x0021, {"00c80020", "00044045", "fff86000", "00000000", "03ff0018", "00000100", "00000000", "00000000"}},
    { 0x0022, {"c4268044", "00000a00", "00000100", "00000033", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0024, {"00000000", "00000000", "00000000", "000fffff", "00000000", "00000000", "00000000", "801ffc00"}},
    { 0x0025, {"01111005", "00000000", "00000000", "00000000", "00000000", "00000c02", "00009c00", "00000040"}},
    { 0x0026, {"00000001", "10000000", "00000000", "00000200", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0027, {"00000000", "00000000", "00000000", "ffc00000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00f01fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"07800000", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f8, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00fb, {"0000001f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}}
};

const FontCharSet charset_CarlitoRegular = 
{
    { 0x0000, {"00000001", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffdffe", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "fbffffff", "fffff9ff", "ffafffff"}},
    { 0x0003, {"ff3be020", "fe4c8e27", "feffffc8", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffbfffff", "ffffff7f", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"000fffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x000e, {"00000000", "80000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x001d, {"fffffeff", "ffffffff", "fdffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "bfdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"fffcffff", "76198047", "c0000010", "fff30000", "001fffff", "033fffff", "20000000", "00000000"}},
    { 0x0021, {"00c80020", "00044045", "fff86000", "00000000", "03ff0018", "00000100", "00000000", "00000000"}},
    { 0x0022, {"c4268044", "00000a00", "00000100", "00000033", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0024, {"00000000", "00000000", "00000000", "000fffff", "00000000", "00000000", "00000000", "801ffc00"}},
    { 0x0025, {"01111005", "00000000", "00000000", "00000000", "00000000", "00000c02", "00009c00", "00000040"}},
    { 0x0026, {"00000001", "10000000", "00000000", "00000200", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0027, {"00000000", "00000000", "00000000", "ffc00000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00f01fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"07800000", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f8, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00fb, {"0000001f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}}
};

const FontCharSet charset_LiberationMonoBold = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ff7fffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"3c0fffff", "00000000", "00000000", "00000000", "fffe0000", "ffffffff", "ffff00ff", "001f07ff"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"ffbdf080", "561d7c47", "40000010", "83f0fc00", "001f03ff", "803fffff", "00000000", "00010000"}},
    { 0x0021, {"00c80020", "00004044", "78186000", "00000000", "003f0010", "00000100", "00100000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000037", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03ff9c10", "00000040"}},
    { 0x0026, {"00000000", "1c000000", "00000005", "00009e69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00fe3fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ff800000", "00000003", "00000000", "00000000", "00001f00", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"e0000006", "5f7fffff", "0000ffdb", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_LiberationMonoBoldItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ff7fffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"3c0fffff", "00000000", "00000000", "00000000", "fffe0000", "ffffffff", "ffff00ff", "001f07ff"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"ffbdf080", "561d7c47", "40000010", "83f0fc00", "001f03ff", "803fffff", "00000000", "00010000"}},
    { 0x0021, {"00c80020", "00004044", "78186000", "00000000", "003f0010", "00000100", "00100000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000037", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03ff9c10", "00000040"}},
    { 0x0026, {"00000000", "1c000000", "00000005", "00009e69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00fe3fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ff800000", "00000003", "00000000", "00000000", "00001f00", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"e0000006", "5f7fffff", "0000ffdb", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_LiberationMonoItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ff7fffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"3c0fffff", "00000000", "00000000", "00000000", "fffe0000", "ffffffff", "ffff00ff", "001f07ff"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"ffbdf080", "561d7c47", "40000010", "83f0fc00", "001f03ff", "803fffff", "00000000", "00010000"}},
    { 0x0021, {"00c80020", "00004044", "78186000", "00000000", "003f0010", "00000100", "00100000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000037", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03ff9c10", "00000040"}},
    { 0x0026, {"00000000", "1c000000", "00000005", "00009e69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00fe3fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ff800000", "00000003", "00000000", "00000000", "00001f00", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"e0000006", "5f7fffff", "0000ffdb", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_LiberationMonoRegular = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ff7fffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"3c0fffff", "00000000", "00000000", "00000000", "fffe0000", "ffffffff", "ffff00ff", "001f07ff"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"ffbdf080", "561d7c47", "40000010", "83f0fc00", "001f03ff", "803fffff", "00000000", "00010000"}},
    { 0x0021, {"00c80020", "00004044", "78186000", "00000000", "003f0010", "00000100", "00100000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000037", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03ff9c10", "00000040"}},
    { 0x0026, {"00000000", "1c000000", "00000005", "00009e69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00fe3fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ff800000", "00000003", "00000000", "00000000", "00001f00", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"e0000006", "5f7fffff", "0000ffdb", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_LiberationSansBold = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"3c0fffff", "00000000", "00000000", "00000000", "fffe0000", "ffffffff", "ffff00ff", "001f07ff"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"fffdffff", "561dfc47", "40000010", "81f0fc00", "001f03ff", "803fffff", "00000000", "00010000"}},
    { 0x0021, {"00c80020", "00004044", "78186000", "00000000", "003f0010", "00000100", "00100000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000037", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03ff9c10", "00000040"}},
    { 0x0026, {"00000000", "9c000000", "000000ff", "00009e69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00fe3fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ff800000", "00000003", "00000000", "00000000", "00001f00", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"e0000006", "5f7fffff", "0000ffdb", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "10000000"}}
};

const FontCharSet charset_LiberationSansBoldItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"3c0fffff", "00000000", "00000000", "00000000", "fffe0000", "ffffffff", "ffff00ff", "001f07ff"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"fffdffff", "561dfc47", "40000010", "81f0fc00", "001f03ff", "803fffff", "00000000", "00010000"}},
    { 0x0021, {"00c80020", "00004044", "78186000", "00000000", "003f0010", "00000100", "00100000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000037", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03ff9c10", "00000040"}},
    { 0x0026, {"00000000", "9c000000", "000000ff", "00009e69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00fe3fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ff800000", "00000003", "00000000", "00000000", "00001f00", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"e0000006", "5f7fffff", "0000ffdb", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "10000000"}}
};

const FontCharSet charset_LiberationSansItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"3c0fffff", "00000000", "00000000", "00000000", "fffe0000", "ffffffff", "ffff00ff", "001f07ff"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"fffdffff", "561dfc47", "40000010", "81f0fc00", "001f03ff", "803fffff", "00000000", "00010000"}},
    { 0x0021, {"00c80020", "00004044", "78186000", "00000000", "003f0010", "00000100", "00100000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000037", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03ff9c10", "00000040"}},
    { 0x0026, {"00000000", "9c000000", "000000ff", "00009e69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00fe3fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ff800000", "00000003", "00000000", "00000000", "00001f00", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"e0000006", "5f7fffff", "0000ffdb", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "10000000"}}
};

const FontCharSet charset_LiberationSansRegular = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"3c0fffff", "00000000", "00000000", "00000000", "fffe0000", "ffffffff", "ffff00ff", "001f07ff"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"fffdffff", "561dfc47", "40000010", "81f0fc00", "001f03ff", "803fffff", "00000000", "00010000"}},
    { 0x0021, {"00c80020", "00004044", "78186000", "00000000", "003f0010", "00000100", "00100000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000037", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03ff9c10", "00000040"}},
    { 0x0026, {"00000000", "9c000000", "000000ff", "00009e69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00fe3fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ff800000", "00000003", "00000000", "00000000", "00001f00", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"e0000006", "5f7fffff", "0000ffdb", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "10000000"}}
};

const FontCharSet charset_LiberationSansNarrowBold = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "00040000", "00000000", "00000000", "fc000000"}},
    { 0x0002, {"0f000000", "00000000", "00000000", "00000000", "00000000", "00000000", "3f0002c0", "00000000"}},
    { 0x0003, {"00000000", "00000000", "00000000", "40000000", "ffffd7f0", "fffffffb", "00007fff", "00000000"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "000c0000", "00030000", "00000000", "00000000", "00000000"}},
    { 0x001e, {"00000000", "00000000", "00000000", "00000000", "0000003f", "00000000", "00000000", "000c0000"}},
    { 0x0020, {"7fbb0000", "560d0047", "00000010", "80000000", "00000000", "00001098", "00000000", "00000000"}},
    { 0x0021, {"00480020", "00004044", "78000000", "00000000", "003f0000", "00000100", "00000000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000033", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03008c10", "00000040"}},
    { 0x0026, {"00000000", "1c000000", "00000005", "00001c69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f0, {"00000026", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"00000006", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_LiberationSansNarrowBoldItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "00040000", "00000000", "00000000", "fc000000"}},
    { 0x0002, {"0f000000", "00000000", "00000000", "00000000", "00000000", "00000000", "3f0002c0", "00000000"}},
    { 0x0003, {"00000000", "00000000", "00000000", "40000000", "ffffd7f0", "fffffffb", "00007fff", "00000000"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "000c0000", "00030000", "00000000", "00000000", "00000000"}},
    { 0x001e, {"00000000", "00000000", "00000000", "00000000", "0000003f", "00000000", "00000000", "000c0000"}},
    { 0x0020, {"7fbb0000", "560d0047", "00000010", "80000000", "00000000", "00001098", "00000000", "00000000"}},
    { 0x0021, {"00480020", "00004044", "78000000", "00000000", "003f0000", "00000100", "00000000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000033", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03008c10", "00000040"}},
    { 0x0026, {"00000000", "1c000000", "00000005", "00001c69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f0, {"00000026", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"00000006", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_LiberationSansNarrowItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "00040000", "00000000", "00000000", "fc000000"}},
    { 0x0002, {"0f000000", "00000000", "00000000", "00000000", "00000000", "00000000", "3f0002c0", "00000000"}},
    { 0x0003, {"00000000", "00000000", "00000000", "40000000", "ffffd7f0", "fffffffb", "00007fff", "00000000"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "000c0000", "00030000", "00000000", "00000000", "00000000"}},
    { 0x001e, {"00000000", "00000000", "00000000", "00000000", "0000003f", "00000000", "00000000", "000c0000"}},
    { 0x0020, {"7fbb0000", "560d0047", "00000010", "80000000", "00000000", "00001098", "00000000", "00000000"}},
    { 0x0021, {"00480020", "00004044", "78000000", "00000000", "003f0000", "00000100", "00000000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000033", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03008c10", "00000040"}},
    { 0x0026, {"00000000", "1c000000", "00000005", "00001c69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f0, {"00000026", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"00000006", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_LiberationSansNarrowRegular = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "00040000", "00000000", "00000000", "fc000000"}},
    { 0x0002, {"0f000000", "00000000", "00000000", "00000000", "00000000", "00000000", "3f0002c0", "00000000"}},
    { 0x0003, {"00000000", "00000000", "00000000", "40000000", "ffffd7f0", "fffffffb", "00007fff", "00000000"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "000c0000", "00030000", "00000000", "00000000", "00000000"}},
    { 0x001e, {"00000000", "00000000", "00000000", "00000000", "0000003f", "00000000", "00000000", "000c0000"}},
    { 0x0020, {"7fbb0000", "560d0047", "00000010", "80000000", "00000000", "00001098", "00000000", "00000000"}},
    { 0x0021, {"00480020", "00004044", "78000000", "00000000", "003f0000", "00000100", "00000000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000033", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03008c10", "00000040"}},
    { 0x0026, {"00000000", "1c000000", "00000005", "00001c69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f0, {"00000026", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"00000006", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_LiberationSerifBold = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"3c0fffff", "00000000", "00000000", "00000000", "fffe0000", "ffffffff", "ffff00ff", "001f07ff"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"fffdffff", "561dfc47", "40000010", "81f0fc00", "001f03ff", "803fffff", "00000000", "00010000"}},
    { 0x0021, {"00c80020", "00004044", "78186000", "00000000", "003f0010", "00000100", "00100000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000037", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03ff9c10", "00000040"}},
    { 0x0026, {"00000000", "1c000000", "00000005", "00009e69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00fe3fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ff800000", "00000003", "00000000", "00000000", "00001f00", "00000000", "00000000", "00000000"}},
    { 0x00f0, {"00000010", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"e0000006", "5f7fffff", "0000ffdb", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "10000000"}}
};

const FontCharSet charset_LiberationSerifBoldItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"3c0fffff", "00000000", "00000000", "00000000", "fffe0000", "ffffffff", "ffff00ff", "001f07ff"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"fffdffff", "561dfc47", "40000010", "81f0fc00", "001f03ff", "803fffff", "00000000", "00010000"}},
    { 0x0021, {"00c80020", "00004044", "78186000", "00000000", "003f0010", "00000100", "00100000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000037", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03ff9c10", "00000040"}},
    { 0x0026, {"00000000", "1c000000", "00000005", "00009e69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00fe3fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ff800000", "00000003", "00000000", "00000000", "00001f00", "00000000", "00000000", "00000000"}},
    { 0x00f0, {"00000030", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"e0000006", "5f7fffff", "0000ffdb", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "10000000"}}
};

const FontCharSet charset_LiberationSerifItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"3c0fffff", "00000000", "00000000", "00000000", "fffe0000", "ffffffff", "ffff00ff", "001f07ff"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"fffdffff", "561dfc47", "40000010", "81f0fc00", "001f03ff", "803fffff", "00000000", "00010000"}},
    { 0x0021, {"00c80020", "00004044", "78186000", "00000000", "003f0010", "00000100", "00100000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000037", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03ff9c10", "00000040"}},
    { 0x0026, {"00000000", "1c000000", "00000005", "00009e69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00fe3fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ff800000", "00000003", "00000000", "00000000", "00001f00", "00000000", "00000000", "00000000"}},
    { 0x00f0, {"00007c36", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"e0000006", "5f7fffff", "0000ffdb", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "10000000"}}
};

const FontCharSet charset_LiberationSerifRegular = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "7c30ffff", "ffffd7f0", "fffffffb", "ffff7fff", "ffffffff"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"3c0fffff", "00000000", "00000000", "00000000", "fffe0000", "ffffffff", "ffff00ff", "001f07ff"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "000007ff", "c0000000"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "4fffffff", "ffffffff", "ffffffff", "03ffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"fffdffff", "561dfc47", "40000010", "81f0fc00", "001f03ff", "803fffff", "00000000", "00010000"}},
    { 0x0021, {"00c80020", "00004044", "78186000", "00000000", "003f0010", "00000100", "00100000", "00000000"}},
    { 0x0022, {"c6268044", "00000a00", "00000100", "00000037", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0023, {"00010004", "00000003", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"11111005", "10101010", "ffff0000", "00001fff", "000f1111", "14041c03", "03ff9c10", "00000040"}},
    { 0x0026, {"00000000", "1c000000", "00000005", "00009e69", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "00fe3fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002e, {"00800000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ff800000", "00000003", "00000000", "00000000", "00001f00", "00000000", "00000000", "00000000"}},
    { 0x00f0, {"00000010", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"e0000006", "5f7fffff", "0000ffdb", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000000", "0000000f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "10000000"}}
};

const FontCharSet charset_NotoSansBold = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "fcffffff", "ffffd7f0", "fffffffb", "ffffffff", "ffff0003"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"ffffffff", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0009, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0010, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "08000000"}},
    { 0x001a, {"00000000", "00000000", "00000000", "00000000", "00000000", "ffff0000", "00007fa1", "00000000"}},
    { 0x001c, {"00000000", "00000000", "00000000", "00000000", "000001ff", "00000000", "00000000", "00000000"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "fbffffff"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"ffffffff", "ffffffff", "ffffffff", "fff3ffdf", "1fff7fff", "ffffffff", "00000001", "00010000"}},
    { 0x0021, {"ffffffff", "ffffffff", "ffffffff", "00000000", "00000218", "00000000", "00000000", "00000000"}},
    { 0x0022, {"00040000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00001000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "ffffffff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002d, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "ffffffff"}},
    { 0x002e, {"ffffffff", "ffffffff", "3fffffff", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a6, {"00000000", "00000000", "ffffffff", "ffffffff", "ffffffff", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "03eb07ff", "fffc0000"}},
    { 0x00a8, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00a9, {"00000000", "00004000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ab, {"00000000", "ffff0000", "ffffffff", "00000fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"0000007f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000001", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "30000000"}},
    { 0x0107, {"00000000", "00000000", "00000000", "00000000", "ffffffbf", "07fdffff", "00000000", "00000000"}},
    { 0x01df, {"7fffffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_NotoSansBoldItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "fcffffff", "ffffd7f0", "fffffffb", "ffffffff", "ffff0003"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"ffffffff", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0009, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x001a, {"00000000", "00000000", "00000000", "00000000", "00000000", "ffff0000", "00007fa1", "00000000"}},
    { 0x001c, {"00000000", "00000000", "00000000", "00000000", "000001ff", "00000000", "00000000", "00000000"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "fbffffff"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"ffffffff", "ffffffff", "ffffffff", "fff3ffdf", "1fff7fff", "ffffffff", "00000000", "00010000"}},
    { 0x0021, {"ffffffff", "ffffffff", "ffffffff", "00000000", "00000210", "00000000", "00000000", "00000000"}},
    { 0x0022, {"00040000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00001000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "ffffffff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002d, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "ffffffff"}},
    { 0x002e, {"ffffffff", "ffffffff", "3fffffff", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a6, {"00000000", "00000000", "ffffffff", "ffffffff", "ffffffff", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "03eb07ff", "fffc0000"}},
    { 0x00a8, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00a9, {"00000000", "00004000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ab, {"00000000", "ffff0000", "ffffffff", "00000fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"0000007f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000001", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "30000000"}},
    { 0x0107, {"00000000", "00000000", "00000000", "00000000", "ffffffbf", "07fdffff", "00000000", "00000000"}},
    { 0x011a, {"00000000", "00000000", "00000000", "00000000", "00000000", "00030000", "00000000", "00000000"}},
    { 0x01df, {"7fffffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_NotoSansItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "fcffffff", "ffffd7f0", "fffffffb", "ffffffff", "ffff0003"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"ffffffff", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0009, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x001a, {"00000000", "00000000", "00000000", "00000000", "00000000", "ffff0000", "00007fa1", "00000000"}},
    { 0x001c, {"00000000", "00000000", "00000000", "00000000", "000001ff", "00000000", "00000000", "00000000"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "fbffffff"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"ffffffff", "ffffffff", "ffffffff", "fff3ffdf", "1fff7fff", "ffffffff", "00000000", "00010000"}},
    { 0x0021, {"ffffffff", "ffffffff", "ffffffff", "00000000", "00000210", "00000000", "00000000", "00000000"}},
    { 0x0022, {"00040000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00001000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "ffffffff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002d, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "ffffffff"}},
    { 0x002e, {"ffffffff", "ffffffff", "3fffffff", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a6, {"00000000", "00000000", "ffffffff", "ffffffff", "ffffffff", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "03eb07ff", "fffc0000"}},
    { 0x00a8, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00a9, {"00000000", "00004000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ab, {"00000000", "ffff0000", "ffffffff", "00000fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"0000007f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000001", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "30000000"}},
    { 0x0107, {"00000000", "00000000", "00000000", "00000000", "ffffffbf", "07fdffff", "00000000", "00000000"}},
    { 0x011a, {"00000000", "00000000", "00000000", "00000000", "00000000", "00030000", "00000000", "00000000"}},
    { 0x01df, {"7fffffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_NotoSansRegular = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "fcffffff", "ffffd7f0", "fffffffb", "ffffffff", "ffff0003"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"ffffffff", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0009, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0010, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "08000000"}},
    { 0x001a, {"00000000", "00000000", "00000000", "00000000", "00000000", "ffff0000", "00007fa1", "00000000"}},
    { 0x001c, {"00000000", "00000000", "00000000", "00000000", "000001ff", "00000000", "00000000", "00000000"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "fbffffff"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"ffffffff", "ffffffff", "ffffffff", "fff3ffdf", "1fff7fff", "ffffffff", "00000001", "00010000"}},
    { 0x0021, {"ffffffff", "ffffffff", "ffffffff", "00000000", "00000218", "00000000", "00000000", "00000000"}},
    { 0x0022, {"00040000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00001000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "ffffffff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002d, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "ffffffff"}},
    { 0x002e, {"ffffffff", "ffffffff", "3fffffff", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a6, {"00000000", "00000000", "ffffffff", "ffffffff", "ffffffff", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "03eb07ff", "fffc0000"}},
    { 0x00a8, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00a9, {"00000000", "00004000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ab, {"00000000", "ffff0000", "ffffffff", "00000fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"0000007f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000001", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "30000000"}},
    { 0x0107, {"00000000", "00000000", "00000000", "00000000", "ffffffbf", "07fdffff", "00000000", "00000000"}},
    { 0x01df, {"7fffffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_NotoSerifBold = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "fcffffff", "ffffd7f0", "fffffffb", "ffffffff", "ffff0003"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"ffffffff", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0010, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "08000000"}},
    { 0x001a, {"00000000", "00000000", "00000000", "00000000", "00000000", "ffff0000", "00007fa1", "00000000"}},
    { 0x001c, {"00000000", "00000000", "00000000", "00000000", "000001ff", "00000000", "00000000", "00000000"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "fbffffff"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"ffffffff", "ffffffff", "ffffffff", "fff3ffdf", "1fff7fff", "ffffffff", "00000001", "00010000"}},
    { 0x0021, {"ffffffff", "ffffffff", "ffffffff", "00000000", "00000218", "00000000", "00000000", "00000000"}},
    { 0x0022, {"00040000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00001000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "ffffffff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002d, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "ffffffff"}},
    { 0x002e, {"ffffffff", "ffffffff", "3fffffff", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a6, {"00000000", "00000000", "ffffffff", "ffffffff", "ffffffff", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "03eb07ff", "fffc0000"}},
    { 0x00a9, {"00000000", "00004000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ab, {"00000000", "ffff0000", "ffffffff", "00000fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"0000007f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000001", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "30000000"}},
    { 0x0107, {"00000000", "00000000", "00000000", "00000000", "ffffffbf", "07fdffff", "00000000", "00000000"}},
    { 0x01df, {"7fffffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_NotoSerifRegular = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "fcffffff", "ffffd7f0", "fffffffb", "ffffffff", "ffff0003"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"ffffffff", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0010, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "08000000"}},
    { 0x001a, {"00000000", "00000000", "00000000", "00000000", "00000000", "ffff0000", "00007fa1", "00000000"}},
    { 0x001c, {"00000000", "00000000", "00000000", "00000000", "000001ff", "00000000", "00000000", "00000000"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "fbffffff"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"ffffffff", "ffffffff", "ffffffff", "fff3ffdf", "1fff7fff", "ffffffff", "00000001", "00010000"}},
    { 0x0021, {"ffffffff", "ffffffff", "ffffffff", "00000000", "00000218", "00000000", "00000000", "00000000"}},
    { 0x0022, {"00040000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00001000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "ffffffff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002d, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "ffffffff"}},
    { 0x002e, {"ffffffff", "ffffffff", "3fffffff", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a6, {"00000000", "00000000", "ffffffff", "ffffffff", "ffffffff", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "03eb07ff", "fffc0000"}},
    { 0x00a9, {"00000000", "00004000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ab, {"00000000", "ffff0000", "ffffffff", "00000fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"0000007f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000001", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "30000000"}},
    { 0x0107, {"00000000", "00000000", "00000000", "00000000", "ffffffbf", "07fdffff", "00000000", "00000000"}},
    { 0x01df, {"7fffffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_NotoSerifBoldItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "fcffffff", "ffffd7f0", "fffffffb", "ffffffff", "ffff0003"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"ffffffff", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x001a, {"00000000", "00000000", "00000000", "00000000", "00000000", "ffff0000", "00007fa1", "00000000"}},
    { 0x001c, {"00000000", "00000000", "00000000", "00000000", "000001ff", "00000000", "00000000", "00000000"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "fbffffff"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"ffffffff", "ffffffff", "ffffffff", "fff3ffdf", "1fff7fff", "ffffffff", "00000000", "00010000"}},
    { 0x0021, {"ffffffff", "ffffffff", "ffffffff", "00000000", "00000210", "00000000", "00000000", "00000000"}},
    { 0x0022, {"00040000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00001000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "ffffffff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002d, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "ffffffff"}},
    { 0x002e, {"ffffffff", "ffffffff", "3fffffff", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a6, {"00000000", "00000000", "ffffffff", "ffffffff", "ffffffff", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "03eb07ff", "fffc0000"}},
    { 0x00a9, {"00000000", "00004000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ab, {"00000000", "ffff0000", "ffffffff", "00000fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"0000007f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000001", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "30000000"}},
    { 0x0107, {"00000000", "00000000", "00000000", "00000000", "ffffffbf", "07fdffff", "00000000", "00000000"}},
    { 0x011a, {"00000000", "00000000", "00000000", "00000000", "00000000", "00030000", "00000000", "00000000"}},
    { 0x01df, {"7fffffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_NotoSerifItalic = 
{
    { 0x0000, {"00000000", "ffffffff", "ffffffff", "7fffffff", "00000000", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0001, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0002, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0003, {"ffffffff", "ffffffff", "ffffffff", "fcffffff", "ffffd7f0", "fffffffb", "ffffffff", "ffff0003"}},
    { 0x0004, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x0005, {"ffffffff", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x001a, {"00000000", "00000000", "00000000", "00000000", "00000000", "ffff0000", "00007fa1", "00000000"}},
    { 0x001c, {"00000000", "00000000", "00000000", "00000000", "000001ff", "00000000", "00000000", "00000000"}},
    { 0x001d, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "fbffffff"}},
    { 0x001e, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff"}},
    { 0x001f, {"3f3fffff", "ffffffff", "aaff3f3f", "3fffffff", "ffffffff", "ffdfffff", "efcfffdf", "7fdcffff"}},
    { 0x0020, {"ffffffff", "ffffffff", "ffffffff", "fff3ffdf", "1fff7fff", "ffffffff", "00000000", "00010000"}},
    { 0x0021, {"ffffffff", "ffffffff", "ffffffff", "00000000", "00000210", "00000000", "00000000", "00000000"}},
    { 0x0022, {"00040000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0025, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00001000", "00000000"}},
    { 0x002c, {"00000000", "00000000", "00000000", "ffffffff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x002d, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "ffffffff"}},
    { 0x002e, {"ffffffff", "ffffffff", "3fffffff", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00a6, {"00000000", "00000000", "ffffffff", "ffffffff", "ffffffff", "00000000", "00000000", "00000000"}},
    { 0x00a7, {"ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "ffffffff", "03eb07ff", "fffc0000"}},
    { 0x00a9, {"00000000", "00004000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00ab, {"00000000", "ffff0000", "ffffffff", "00000fff", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fb, {"0000007f", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00fe, {"00000001", "0000ffff", "00000000", "00000000", "00000000", "00000000", "00000000", "80000000"}},
    { 0x00ff, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "30000000"}},
    { 0x0107, {"00000000", "00000000", "00000000", "00000000", "ffffffbf", "07fdffff", "00000000", "00000000"}},
    { 0x011a, {"00000000", "00000000", "00000000", "00000000", "00000000", "00030000", "00000000", "00000000"}},
    { 0x01df, {"7fffffff", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}}
};

const FontCharSet charset_Opens = 
{
    { 0x0000, {"00000000", "ffffff6f", "e8000000", "78000001", "00000000", "8853fbb4", "00800000", "00800000"}},
    { 0x0001, {"00000000", "00000000", "000c0000", "61000003", "08040000", "00000000", "00000000", "00000000"}},
    { 0x0002, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "17000cc0", "00000000"}},
    { 0x0003, {"000015df", "00000000", "00000000", "00000000", "fffe0000", "fffe03fb", "006603ff", "00220000"}},
    { 0x0020, {"77580000", "061d0047", "00000000", "00000000", "00000000", "00001b1e", "08820000", "00000000"}},
    { 0x0021, {"352e8484", "00230094", "00000000", "00000000", "01bf0000", "002c0000", "001f0000", "000003f0"}},
    { 0x0022, {"65efabbd", "1011ffbf", "22300128", "fc000c33", "03e003ff", "40c00030", "00000020", "0003c000"}},
    { 0x0023, {"00000f00", "00100603", "00000000", "00080000", "00000000", "00000000", "c0000000", "00000000"}},
    { 0x0024, {"00000000", "00000000", "00000000", "000003ff", "00000000", "00000000", "00000000", "00000400"}},
    { 0x0025, {"0000000f", "00000000", "00000000", "00000000", "00000000", "fffc0c03", "00808c3f", "00000040"}},
    { 0x0026, {"48064000", "16000000", "00000000", "00000069", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0027, {"1f986104", "83f9f83e", "0247a012", "00000004", "051fffff", "01040004", "00000004", "000003c0"}},
    { 0x002a, {"00000000", "00000000", "00000000", "60000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x0030, {"0c000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00e0, {"ff4ffffe", "05dfffff", "00000000", "00000000", "c9fffeff", "ffffffe3", "3a7bffff", "00000000"}},
    { 0x00e1, {"ffffffff", "ffffffff", "2003ffff", "00000400", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00e2, {"ffffffff", "7fffffff", "ffffffff", "00000003", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00e3, {"1fffe000", "00010000", "00002040", "00000000", "20002000", "00000e08", "00010000", "00000000"}},
    { 0x00e4, {"78000683", "ff9dd2e0", "7fffffff", "ffffcfc0", "ffe7f800", "ffffffff", "0000001f", "00000000"}},
    { 0x00e5, {"e00000b8", "000fdfff", "dfffff00", "1f3f0000", "00f9e0f0", "00000020", "00000000", "00000000"}},
    { 0x00e6, {"00000000", "00000000", "00000000", "00000000", "ffffffff", "0000000f", "00000000", "00000000"}},
    { 0x00f0, {"00000000", "03ff0000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"}},
    { 0x00f8, {"00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "7ffff800"}}
};

/*
* Font Attributes Data
*/
void FontData::SetCaladeaBoldFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Caladea"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_CaladeaBold);
}

void FontData::SetCaladeaBoldItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Caladea"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    // FontConfig has style name 'Italic', but 'Bold Italic' would fits the pattern???
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_CaladeaBoldItalic);
}

void FontData::SetCaladeaItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Caladea"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_CaladeaItalic);
}

void FontData::SetCaladeaRegularFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Caladea"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Regular"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_CaladeaRegular);
}

void FontData::SetCarlitoBoldFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Carlito"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_CarlitoBold);
}

void FontData::SetCarlitoBoldItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Carlito"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_CarlitoBoldItalic);
}

void FontData::SetCarlitoItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Carlito"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_CarlitoItalic);
}

void FontData::SetCarlitoRegularFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Carlito"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Regular"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_CarlitoRegular);
}

void FontData::SetLiberationMonoBoldFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Mono"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_FIXED);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationMonoBold);
}

void FontData::SetLiberationMonoBoldItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Mono"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_FIXED);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationMonoBoldItalic);
}

void FontData::SetLiberationMonoItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Mono"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_FIXED);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationMonoItalic);
}

void FontData::SetLiberationMonoRegularFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Mono"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Regular"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_FIXED);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationMonoRegular);
}

void FontData::SetLiberationSansBoldFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Sans"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationSansBold);
}

void FontData::SetLiberationSansBoldItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Sans"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationSansBoldItalic);
}

void FontData::SetLiberationSansItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Sans"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationSansItalic);
}

void FontData::SetLiberationSansRegularFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Sans"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Regular"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationSansRegular);
}

void FontData::SetLiberationSansNarrowBoldFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Sans Narrow"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_CONDENSED);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationSansNarrowBold);
}

void FontData::SetLiberationSansNarrowBoldItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Sans Narrow"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_CONDENSED);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationSansNarrowBoldItalic);
}

void FontData::SetLiberationSansNarrowItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Sans Narrow"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_CONDENSED);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationSansNarrowItalic);
}

void FontData::SetLiberationSansNarrowRegularFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Sans Narrow"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Regular"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_CONDENSED);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationSansNarrowRegular);
}

void FontData::SetLiberationSerifBoldFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Serif"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationSerifBold);
}

void FontData::SetLiberationSerifBoldItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Serif"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationSerifBoldItalic);
}

void FontData::SetLiberationSerifItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Serif"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationSerifItalic);
}

void FontData::SetLiberationSerifRegularFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Liberation Serif"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Regular"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_LiberationSerifRegular);
}

void FontData::SetNotoSansBoldFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Noto Sans"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_NotoSansBold);
}

void FontData::SetNotoSansBoldItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Noto Sans"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_NotoSansBoldItalic);
}

void FontData::SetNotoSansItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Noto Sans"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_NotoSansItalic);
}

void FontData::SetNotoSansRegularFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Noto Sans"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Regular"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_NotoSansRegular);
}

void FontData::SetNotoSerifBoldFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Noto Serif"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_NotoSerifBold);
}

void FontData::SetNotoSerifRegularFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Noto Serif"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Regular"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_NotoSerifRegular);
}

void FontData::SetNotoSerifBoldItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Noto Serif"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Bold Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_BOLD);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_NotoSerifBoldItalic);
}

void FontData::SetNotoSerifItalicFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("Noto Serif"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Italic"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NORMAL);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_NotoSerifItalic);
}

void FontData::SetOpenSymbolRegularFont(FontAttributes& fontAttributes)
{
    fontAttributes.SetFamilyName(OUString::createFromAscii("OpenSymbol"));
    fontAttributes.SetFamilyType(FontFamily::FAMILY_DONTKNOW);
    fontAttributes.SetStyleName(OUString::createFromAscii("Regular"));
    fontAttributes.SetWeight(FontWeight::WEIGHT_NORMAL);
    fontAttributes.SetItalic(FontItalic::ITALIC_NONE);
    fontAttributes.SetPitch(FontPitch::PITCH_VARIABLE);
    fontAttributes.SetWidthType(FontWidth::WIDTH_NORMAL);
    fontAttributes.SetMicrosoftSymbolEncoded(false);
    fontAttributes.SetQuality(512);
    fontAttributes.SetBitmapFromCharSet(charset_Opens);
}


/**
 * @brief Populating the PrintFontManager Data
 * 
 * @param PrintFontMap 
 * @param FontFileToFontIDMap 
 */
void FontData::populatePrintFontManagerData(
    std::unordered_map<fontID, PrintFont>& PrintFontMap,
    std::unordered_map<OString, o3tl::sorted_vector<fontID>>& FontFileToFontIDMap)
{
    /*
        There should be 33 Fonts
        8 Families * 4 Options (Bold, BoldItalic, Italic, Regular) + 1 Opens Font
    */
    std::vector<PrintFont> PrintFontArray
        = { GetPrintFontCaladeaBold(),
            GetPrintFontCaladeaBoldItalic(),
            GetPrintFontCaladeaItalic(),
            GetPrintFontCaladeaRegular(),
            GetPrintFontCarlitoBold(),
            GetPrintFontCarlitoBoldItalic(),
            GetPrintFontCarlitoItalic(),
            GetPrintFontCarlitoRegular(),
            GetPrintFontLiberationMonoBold(),
            GetPrintFontLiberationMonoBoldItalic(),
            GetPrintFontLiberationMonoItalic(),
            GetPrintFontLiberationMonoRegular(),
            GetPrintFontLiberationSansBold(),
            GetPrintFontLiberationSansBoldItalic(),
            GetPrintFontLiberationSansItalic(),
            GetPrintFontLiberationSansRegular(),
            GetPrintFontLiberationSansNarrowBold(),
            GetPrintFontLiberationSansNarrowBoldItalic(),
            GetPrintFontLiberationSansNarrowItalic(),
            GetPrintFontLiberationSansNarrowRegular(),
            GetPrintFontLiberationSerifBold(),
            GetPrintFontLiberationSerifBoldItalic(),
            GetPrintFontLiberationSerifItalic(),
            GetPrintFontLiberationSerifRegular(),
            GetPrintFontNotoSansBold(),
            GetPrintFontNotoSansBoldItalic(),
            GetPrintFontNotoSansItalic(),
            GetPrintFontNotoSansRegular(),
            GetPrintFontNotoSerifBold(),
            GetPrintFontNotoSerifRegular(),
            GetPrintFontNotoSerifBoldItalic(),
            GetPrintFontNotoSerifItalic(),
            GetPrintFontOpens() };

    fontID id = 0;
    for (PrintFont printFont : PrintFontArray)
    {
        PrintFontMap.emplace(id, printFont);
        FontFileToFontIDMap[printFont.m_aFontFile].insert(id);
        id++;
    }
}
}