/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "MemPackageFolder.hxx"
#include "MemPackageStream.hxx"
#include "MemPackageFolderEnumeration.hxx"

#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/container/ElementExistException.hpp>
#include <com/sun/star/container/NoSuchElementException.hpp>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>

using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::container;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;

MemContentInfo::MemContentInfo(MemPackageStream* pNewStream)
    : xPackageEntry(pNewStream)
    , bFolder(false)
    , pStream(pNewStream)
{
}

MemContentInfo::MemContentInfo(MemPackageFolder* pNewFolder)
    : xPackageEntry(pNewFolder)
    , bFolder(true)
    , pFolder(pNewFolder)
{
}

MemContentInfo::MemContentInfo(const MemContentInfo&) = default;
MemContentInfo::MemContentInfo(MemContentInfo&&) = default;
MemContentInfo& MemContentInfo::operator=(const MemContentInfo&) = default;
MemContentInfo& MemContentInfo::operator=(MemContentInfo&&) = default;

MemContentInfo::~MemContentInfo()
{
    if (bFolder)
        pFolder->clearParent();
    else
        pStream->clearParent();
}

MemPackageFolder::MemPackageFolder()
{
    m_bIsFolder = true;
}

MemPackageFolder::~MemPackageFolder()
{
}

void MemPackageFolder::doInsertByName(MemPackageEntry* pEntry, bool bSetParent)
{
    if (pEntry->IsFolder())
        m_aContents.emplace(pEntry->getName(), MemContentInfo(static_cast<MemPackageFolder*>(pEntry)));
    else
        m_aContents.emplace(pEntry->getName(), MemContentInfo(static_cast<MemPackageStream*>(pEntry)));

    if (bSetParent)
        pEntry->doSetParent(this);
}

MemContentInfo& MemPackageFolder::doGetByName(const OUString& aName)
{
    auto it = m_aContents.find(aName);
    if (it == m_aContents.end())
        throw NoSuchElementException();
    return it->second;
}

void SAL_CALL MemPackageFolder::insertByName(const OUString& aName, const Any& aElement)
{
    if (hasByName(aName))
        throw ElementExistException();

    Reference<XInterface> xInterface;
    if (!(aElement >>= xInterface))
        throw IllegalArgumentException();

    MemPackageFolder* pFolder = dynamic_cast<MemPackageFolder*>(xInterface.get());
    if (pFolder)
        m_aContents.emplace(aName, MemContentInfo(pFolder));
    else
    {
        MemPackageStream* pStream = dynamic_cast<MemPackageStream*>(xInterface.get());
        if (!pStream)
            throw IllegalArgumentException();
        m_aContents.emplace(aName, MemContentInfo(pStream));
    }
}

void MemPackageFolder::insertByName(const OUString& aName, MemPackageStream* pStream)
{
    if (hasByName(aName))
        throw ElementExistException();

    if (!pStream)
        throw IllegalArgumentException();

    m_aContents.emplace(aName, MemContentInfo(pStream));
}

void SAL_CALL MemPackageFolder::removeByName(const OUString& Name)
{
    if (!m_aContents.erase(Name))
        throw NoSuchElementException();
}

Reference<XEnumeration> SAL_CALL MemPackageFolder::createEnumeration()
{
    return new MemPackageFolderEnumeration(m_aContents);
}

Type SAL_CALL MemPackageFolder::getElementType()
{
    return cppu::UnoType<XInterface>::get();
}

sal_Bool SAL_CALL MemPackageFolder::hasElements()
{
    return !m_aContents.empty();
}

Any SAL_CALL MemPackageFolder::getByName(const OUString& aName)
{
    auto it = m_aContents.find(aName);
    if (it == m_aContents.end())
        throw NoSuchElementException();

    return Any(uno::Reference(cppu::getXWeak(it->second.xPackageEntry.get())));
}

Sequence<OUString> SAL_CALL MemPackageFolder::getElementNames()
{
    return comphelper::mapKeysToSequence(m_aContents);
}

sal_Bool SAL_CALL MemPackageFolder::hasByName(const OUString& aName)
{
    return m_aContents.find(aName) != m_aContents.end();
}

void SAL_CALL MemPackageFolder::replaceByName(const OUString& aName, const Any& aElement)
{
    if (!hasByName(aName))
        throw NoSuchElementException();

    removeByName(aName);
    insertByName(aName, aElement);
}

// XServiceInfo
OUString SAL_CALL MemPackageFolder::getImplementationName()
{
    return "com.sun.star.comp.MemPackageFolder";
}

sal_Bool SAL_CALL MemPackageFolder::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

Sequence<OUString> SAL_CALL MemPackageFolder::getSupportedServiceNames()
{
    return { "com.sun.star.packages.PackageFolder" };
}

void SAL_CALL MemPackageFolder::setPropertyValue(const OUString& PropertyName, const Any& aValue)
{
    if (PropertyName == "MediaType")
        aValue >>= m_sMediaType;
    else
        throw UnknownPropertyException(PropertyName);
}

Any SAL_CALL MemPackageFolder::getPropertyValue(const OUString& PropertyName)
{
    if (PropertyName == "MediaType")
        return Any(m_sMediaType);
    
    throw UnknownPropertyException(PropertyName);
}

void MemPackageFolder::collectStreamContents(std::unordered_map<std::string, std::vector<int8_t>>& rContents,
                                                std::vector<beans::StringPair>& rOverrides)
{
    for (const auto& [name, content] : m_aContents)
    {
        if (content.bFolder)
        {
            // Recursively collect contents from subfolders
            content.pFolder->collectStreamContents(rContents, rOverrides);
        }
        else
        {
            // Get stream data and add to contents map
            rtl::Reference<utl::OStreamWrapper> pStream = content.pStream->streamWrapper();
            std::vector<int8_t> data(pStream->getLength());
            pStream->readSomeBytes(data.data(), data.size());

            // Store in contents map
            rContents[OUStringToOString(name, RTL_TEXTENCODING_UTF8).getStr()] = std::move(data);

            // Add content type override
            Any aMediaType = content.pStream->getPropertyValue("MediaType");
            OUString sMediaType;
            aMediaType >>= sMediaType;
            if (!sMediaType.isEmpty())
            {
                rOverrides.push_back(beans::StringPair("/" + name, sMediaType));
            }
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 