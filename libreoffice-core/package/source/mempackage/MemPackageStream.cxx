/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <MemPackageStream.hxx>
#include <MemPackage.hxx>

#include <com/sun/star/io/XSeekable.hpp>
#include <com/sun/star/packages/NoEncryptionException.hpp>
#include <com/sun/star/packages/NoRawFormatException.hpp>
#include <unotools/streamwrap.hxx>
#include <comphelper/seekableinput.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <sal/log.hxx>

using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::io;
using namespace com::sun::star::packages;
using namespace com::sun::star::beans;

MemPackageStream::MemPackageStream(std::vector<uint8_t> data)
{
    std::unique_ptr<SvMemoryStream> pMemStream(new SvMemoryStream(data.data(), data.size(), StreamMode::READWRITE));
    m_aStreamWrapper = new utl::OStreamWrapper(std::move(pMemStream));
}

MemPackageStream::MemPackageStream()
{
    std::unique_ptr<SvMemoryStream> pMemStream(new SvMemoryStream());
    m_aStreamWrapper = new utl::OStreamWrapper(std::move(pMemStream));
}

MemPackageStream::~MemPackageStream()
{
}

Reference<XInputStream> SAL_CALL MemPackageStream::getInputStream()
{
    return m_aStreamWrapper->getInputStream();
}

Reference<XOutputStream> SAL_CALL MemPackageStream::getOutputStream()
{
    return m_aStreamWrapper->getOutputStream();
}

Reference<XInputStream> SAL_CALL MemPackageStream::getDataStream()
{
    return getInputStream();
}

Reference<XInputStream> SAL_CALL MemPackageStream::getRawStream()
{
    SAL_WARN("package", "getRawStream is not supported for memory streams");
    return getInputStream();
}

void SAL_CALL MemPackageStream::setDataStream(const Reference<XInputStream>& /*aStream*/)
{
    SAL_WARN("package", "setDataStream is not supported for memory streams");
}

void SAL_CALL MemPackageStream::setRawStream(const Reference<XInputStream>& /*aStream*/)
{
    SAL_WARN("package", "setRawStream is not supported for memory streams");
}

Reference<XInputStream> SAL_CALL MemPackageStream::getPlainRawStream()
{
    return getInputStream();
}

void SAL_CALL MemPackageStream::setPropertyValue(const OUString& PropertyName, const Any& aValue)
{
    if (PropertyName == "MediaType")
        aValue >>= m_sMediaType;
    else
        throw UnknownPropertyException(PropertyName);
}

Any SAL_CALL MemPackageStream::getPropertyValue(const OUString& PropertyName)
{
    if (PropertyName == "MediaType")
        return Any(m_sMediaType);
    else if (PropertyName == "Size")
        return Any(m_aStreamWrapper->getLength());
    else if (PropertyName == "Encrypted")
        return Any(false);
    else if (PropertyName == "Compressed")
        return Any(false);
    
    throw UnknownPropertyException(PropertyName);
}

void SAL_CALL MemPackageStream::setInputStream(const Reference<XInputStream>& aStream)
{
    SAL_WARN("package", "setInputStream is not supported for memory streams");

    // Create a new memory stream from the input stream
    std::unique_ptr<SvMemoryStream> pMemStream(new SvMemoryStream());
    
    // Copy the input stream to our memory stream
    const sal_Int32 BUFFER_SIZE = 8192;
    Sequence<sal_Int8> buffer(BUFFER_SIZE);
    sal_Int32 bytesRead;
    while ((bytesRead = aStream->readSomeBytes(buffer, BUFFER_SIZE)) > 0)
    {
        pMemStream->WriteBytes(buffer.getConstArray(), bytesRead);
    }
    
    // Create a new stream wrapper
    m_aStreamWrapper = new utl::OStreamWrapper(std::move(pMemStream));
}
    

// XServiceInfo
OUString SAL_CALL MemPackageStream::getImplementationName()
{
    return "com.sun.star.comp.MemPackageStream";
}

sal_Bool SAL_CALL MemPackageStream::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

Sequence<OUString> SAL_CALL MemPackageStream::getSupportedServiceNames()
{
    return { "com.sun.star.packages.PackageStream" };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */