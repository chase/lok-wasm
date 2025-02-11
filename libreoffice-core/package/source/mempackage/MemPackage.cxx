/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <MemPackage.hxx>
#include <MemPackageFolder.hxx>
#include <MemPackageStream.hxx>

#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/container/NoSuchElementException.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <cppuhelper/supportsservice.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/ofopxmlhelper.hxx>

using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::container;
using namespace com::sun::star::beans;
using namespace com::sun::star::io;

MemPackage::MemPackage(const Reference<XComponentContext>& xContext,
                       std::unordered_map<std::string, std::vector<int8_t>> contents)
    : m_xContext(xContext)
    , m_contents(std::move(contents))
{
    m_xRootFolder = new MemPackageFolder();
}

MemPackage::~MemPackage()
{
}

void SAL_CALL MemPackage::initialize(const Sequence<Any>& /*aArguments*/)
{
}

Any SAL_CALL MemPackage::getByHierarchicalName(const OUString& aName)
{
    if (aName == "/")
        return Any(Reference<XInterface>(static_cast<XHierarchicalNameAccess*>(this)));

    if (!m_xRootFolder->hasByName(aName))
        throw NoSuchElementException();

    return m_xRootFolder->getByName(aName);
}

sal_Bool SAL_CALL MemPackage::hasByHierarchicalName(const OUString& aName)
{
    if (aName == "/")
        return true;

    return m_xRootFolder->hasByName(aName);
}

Reference<XInterface> SAL_CALL MemPackage::createInstance()
{
    return *new MemPackageStream();
}

Reference<XInterface> SAL_CALL MemPackage::createInstanceWithArguments(const Sequence<Any>& aArguments)
{
    bool bIsFolder = false;
    uno::Reference < XInterface > xRef;
    if (aArguments.getLength() > 0)
        aArguments[0] >>= bIsFolder;

    if (bIsFolder)
        xRef = *new MemPackageFolder();
    else
        xRef = *new MemPackageStream();

    return xRef;
}

void SAL_CALL MemPackage::commitChanges()
{
    // lock the component for the time of committing
    ::osl::MutexGuard aGuard(m_aMutexHolder->GetMutex());

    // For in-memory package, we don't need temporary files, just update the contents directly
    // Remove old [Content_Types].xml
    static constexpr OUString aContentTypes(u"[Content_Types].xml"_ustr);
    if (m_xRootFolder->hasByName(aContentTypes))
        m_xRootFolder->removeByName(aContentTypes);

    // Create content types file
    std::vector<beans::StringPair> aOverrides;
    m_xRootFolder->collectStreamContents(m_contents, aOverrides);

    // Add default entries
    Sequence<beans::StringPair> aDefaultsSequence
    {
        { "xml", "application/xml" },
        { "rels", "application/vnd.openxmlformats-package.relationships+xml" },
        { "png", "image/png" },
        { "jpeg", "image/jpeg" }
    };

    // Convert overrides vector to sequence
    Sequence<beans::StringPair> aOverridesSequence(comphelper::containerToSequence(aOverrides));

    // Create a new stream and write content types
    MemPackageStream* pStream = new MemPackageStream();
    Reference<XOutputStream> xOutStream = pStream->getOutputStream();

    ::comphelper::OFOPXMLHelper::WriteContentSequence(xOutStream, aDefaultsSequence, aOverridesSequence, m_xContext);
    xOutStream->closeOutput();

    m_xRootFolder->insertByName(aContentTypes, pStream);
}

sal_Bool SAL_CALL MemPackage::hasPendingChanges()
{
    return false;
}

Sequence<util::ElementChange> SAL_CALL MemPackage::getPendingChanges()
{
    return Sequence<util::ElementChange>();
}

Reference<XPropertySetInfo> SAL_CALL MemPackage::getPropertySetInfo()
{
    return Reference<XPropertySetInfo>();
}

void SAL_CALL MemPackage::setPropertyValue(const OUString& /*PropertyName*/, const Any& /*aValue*/)
{
    // No-op for in-memory package
}

Any SAL_CALL MemPackage::getPropertyValue(const OUString& PropertyName)
{
    if (PropertyName == "HasElements")
        return Any(!m_contents.empty());
    
    throw UnknownPropertyException(PropertyName);
}

void SAL_CALL MemPackage::addPropertyChangeListener(const OUString& /*PropertyName*/,
    const Reference<XPropertyChangeListener>& /*xListener*/)
{
}

void SAL_CALL MemPackage::removePropertyChangeListener(const OUString& /*PropertyName*/,
    const Reference<XPropertyChangeListener>& /*aListener*/)
{
}

void SAL_CALL MemPackage::addVetoableChangeListener(const OUString& /*PropertyName*/,
    const Reference<XVetoableChangeListener>& /*aListener*/)
{
}

void SAL_CALL MemPackage::removeVetoableChangeListener(const OUString& /*PropertyName*/,
    const Reference<XVetoableChangeListener>& /*aListener*/)
{
}

OUString MemPackage::getImplementationName()
{
    return "com.sun.star.comp.MemPackage";
}

Sequence<OUString> MemPackage::getSupportedServiceNames()
{
    return { "com.sun.star.packages.Package" };
}

sal_Bool SAL_CALL MemPackage::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 