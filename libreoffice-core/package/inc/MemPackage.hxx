/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_PACKAGE_INC_MEMPACKAGE_HXX
#define INCLUDED_PACKAGE_INC_MEMPACKAGE_HXX

#include <cppuhelper/implbase.hxx>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <rtl/ref.hxx>
#include <comphelper/refcountedmutex.hxx>

#include <unordered_map>
#include <string>

#include "MemPackageFolder.hxx"

class MemPackage final : public cppu::WeakImplHelper
                    <
                       css::lang::XInitialization,
                       css::lang::XSingleServiceFactory, 
                       css::lang::XServiceInfo,
                       css::container::XHierarchicalNameAccess,
                       css::util::XChangesBatch,
                       css::beans::XPropertySet
                    >
{
    rtl::Reference<comphelper::RefCountedMutex> m_aMutexHolder;

    rtl::Reference<MemPackageFolder> m_xRootFolder;
    const css::uno::Reference<css::uno::XComponentContext> m_xContext;
    std::unordered_map<std::string, std::vector<int8_t>> m_contents;

public:
    MemPackage(const css::uno::Reference<css::uno::XComponentContext>& xContext,
               std::unordered_map<std::string, std::vector<int8_t>> contents);
    virtual ~MemPackage() override;

    // XInitialization
    virtual void SAL_CALL initialize(const css::uno::Sequence<css::uno::Any>& aArguments) override;

    // XHierarchicalNameAccess
    virtual css::uno::Any SAL_CALL getByHierarchicalName(const OUString& aName) override;
    virtual sal_Bool SAL_CALL hasByHierarchicalName(const OUString& aName) override;

    // XSingleServiceFactory
    virtual css::uno::Reference<css::uno::XInterface> SAL_CALL createInstance() override;
    virtual css::uno::Reference<css::uno::XInterface> SAL_CALL createInstanceWithArguments(
        const css::uno::Sequence<css::uno::Any>& aArguments) override;

    // XChangesBatch
    virtual void SAL_CALL commitChanges() override;
    virtual sal_Bool SAL_CALL hasPendingChanges() override;
    virtual css::uno::Sequence<css::util::ElementChange> SAL_CALL getPendingChanges() override;

    // XPropertySet
    virtual css::uno::Reference<css::beans::XPropertySetInfo> SAL_CALL getPropertySetInfo() override;
    virtual void SAL_CALL setPropertyValue(const OUString& PropertyName, const css::uno::Any& aValue) override;
    virtual css::uno::Any SAL_CALL getPropertyValue(const OUString& PropertyName) override;
    virtual void SAL_CALL addPropertyChangeListener(const OUString& PropertyName,
        const css::uno::Reference<css::beans::XPropertyChangeListener>& xListener) override;
    virtual void SAL_CALL removePropertyChangeListener(const OUString& PropertyName,
        const css::uno::Reference<css::beans::XPropertyChangeListener>& aListener) override;
    virtual void SAL_CALL addVetoableChangeListener(const OUString& PropertyName,
        const css::uno::Reference<css::beans::XVetoableChangeListener>& aListener) override;
    virtual void SAL_CALL removeVetoableChangeListener(const OUString& PropertyName,
        const css::uno::Reference<css::beans::XVetoableChangeListener>& aListener) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;

    const std::unordered_map<std::string, std::vector<int8_t>>& getContents() const { return m_contents; }
    rtl::Reference<MemPackageFolder> getRootFolder() const { return m_xRootFolder; }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 