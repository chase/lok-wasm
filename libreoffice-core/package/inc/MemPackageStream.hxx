/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_PACKAGE_INC_MEMPACKAGESTREAM_HXX
#define INCLUDED_PACKAGE_INC_MEMPACKAGESTREAM_HXX

#include <com/sun/star/io/XStream.hpp>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/packages/XDataSinkEncrSupport.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertyChangeListener.hpp>
#include <com/sun/star/beans/XVetoableChangeListener.hpp>
#include <com/sun/star/beans/UnknownPropertyException.hpp>
#include <cppuhelper/implbase.hxx>
#include <tools/stream.hxx>
#include <vector>
#include <memory>
#include <unotools/streamwrap.hxx>
#include <rtl/ref.hxx>

#include "MemPackageEntry.hxx"

class MemPackage;

class MemPackageStream final : public cppu::ImplInheritanceHelper
<
    MemPackageEntry,
    css::io::XStream,
    css::io::XActiveDataSink,
    css::packages::XDataSinkEncrSupport
>
{
private:
    rtl::Reference<utl::OStreamWrapper> m_aStreamWrapper;
    OUString m_sMediaType;
    
public:
    // Constructor for reading existing data
    MemPackageStream(std::vector<uint8_t> data);

    // Constructor for writing new data
    MemPackageStream();

    rtl::Reference<utl::OStreamWrapper> streamWrapper() const { return m_aStreamWrapper; }

    virtual ~MemPackageStream() override;

    // XStream
    virtual css::uno::Reference<css::io::XInputStream> SAL_CALL getInputStream() override;
    virtual css::uno::Reference<css::io::XOutputStream> SAL_CALL getOutputStream() override;

    // XActiveDataSink
    virtual void SAL_CALL setInputStream( const css::uno::Reference< css::io::XInputStream >& aStream ) override;

    // XDataSinkEncrSupport
    virtual css::uno::Reference<css::io::XInputStream> SAL_CALL getDataStream() override;
    virtual css::uno::Reference<css::io::XInputStream> SAL_CALL getRawStream() override;
    virtual void SAL_CALL setDataStream(const css::uno::Reference<css::io::XInputStream>& aStream) override;
    virtual void SAL_CALL setRawStream(const css::uno::Reference<css::io::XInputStream>& aStream) override;
    virtual css::uno::Reference<css::io::XInputStream> SAL_CALL getPlainRawStream() override;

    // XPropertySet (from MemPackageEntry)
    virtual void SAL_CALL setPropertyValue(const OUString& PropertyName, const css::uno::Any& aValue) override;
    virtual css::uno::Any SAL_CALL getPropertyValue(const OUString& PropertyName) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 