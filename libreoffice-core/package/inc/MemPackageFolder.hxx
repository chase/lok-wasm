/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_PACKAGE_INC_MEMPACKAGEFOLDER_HXX
#define INCLUDED_PACKAGE_INC_MEMPACKAGEFOLDER_HXX

#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/beans/XPropertyChangeListener.hpp>
#include <com/sun/star/beans/XVetoableChangeListener.hpp>
#include <com/sun/star/beans/UnknownPropertyException.hpp>
#include <com/sun/star/beans/StringPair.hpp>
#include <rtl/ref.hxx>
#include <cppuhelper/implbase.hxx>
#include <unordered_map>

#include "MemPackageEntry.hxx"
#include "MemPackageStream.hxx"

struct MemContentInfo
{
    rtl::Reference<MemPackageEntry> xPackageEntry;
    bool bFolder;
    union
    {
        MemPackageFolder* pFolder;
        MemPackageStream* pStream;
    };

    MemContentInfo(MemPackageStream* pNewStream);
    MemContentInfo(MemPackageFolder* pNewFolder);
    MemContentInfo(const MemContentInfo&);
    MemContentInfo(MemContentInfo&&);
    MemContentInfo& operator=(const MemContentInfo&);
    MemContentInfo& operator=(MemContentInfo&&);
    ~MemContentInfo();
};

typedef std::unordered_map<OUString, MemContentInfo> ContentHash;

class MemPackageFolder final : public cppu::ImplInheritanceHelper
<
    MemPackageEntry,
    css::container::XNameContainer,
    css::container::XEnumerationAccess
>
{
private:
    ContentHash m_aContents;

public:
    MemPackageFolder();
    virtual ~MemPackageFolder() override;

    void doInsertByName(MemPackageEntry* pEntry, bool bSetParent);
    MemContentInfo& doGetByName(const OUString& aName);

    // XNameContainer
    virtual void SAL_CALL insertByName(const OUString& aName, const css::uno::Any& aElement) override;
    virtual void SAL_CALL removeByName(const OUString& Name) override;

    // XNameReplace
    virtual void SAL_CALL replaceByName(const OUString& aName, const css::uno::Any& aElement) override;

    // XNameAccess
    virtual css::uno::Any SAL_CALL getByName(const OUString& aName) override;
    virtual css::uno::Sequence<OUString> SAL_CALL getElementNames() override;
    virtual sal_Bool SAL_CALL hasByName(const OUString& aName) override;

    // XElementAccess
    virtual css::uno::Type SAL_CALL getElementType() override;
    virtual sal_Bool SAL_CALL hasElements() override;

    // XEnumerationAccess
    virtual css::uno::Reference<css::container::XEnumeration> SAL_CALL createEnumeration() override;

    // XPropertySet (from MemPackageEntry)
    virtual void SAL_CALL setPropertyValue(const OUString& PropertyName, const css::uno::Any& aValue) override;
    virtual css::uno::Any SAL_CALL getPropertyValue(const OUString& PropertyName) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;

    // Helper methods
    void collectStreamContents(std::unordered_map<std::string, std::vector<int8_t>>& rContents,
                             std::vector<css::beans::StringPair>& rOverrides);

    // New direct insertion method
    void insertByName(const OUString& aName, MemPackageStream* pStream);
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 