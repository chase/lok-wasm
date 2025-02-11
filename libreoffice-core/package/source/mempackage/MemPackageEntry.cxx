/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <MemPackageEntry.hxx>
#include <MemPackageFolder.hxx>
#include <com/sun/star/lang/NoSupportException.hpp>
#include <comphelper/storagehelper.hxx>

using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::container;
using namespace com::sun::star::beans;

MemPackageEntry::MemPackageEntry()
    : m_bIsFolder(false)
    , m_pParent(nullptr)
{
}

MemPackageEntry::~MemPackageEntry()
{
}

// XNamed
OUString SAL_CALL MemPackageEntry::getName()
{
    return m_sName;
}

void SAL_CALL MemPackageEntry::setName(const OUString& aName)
{
    if (m_pParent && !m_sName.isEmpty() && m_pParent->hasByName(m_sName))
        m_pParent->removeByName(m_sName);

    // Check for valid zip entry name
    if (!::comphelper::OStorageHelper::IsValidZipEntryFileName(aName, true))
        throw RuntimeException("Unexpected character is used in file name.");

    m_sName = aName;

    if (m_pParent)
        m_pParent->doInsertByName(this, false);
}

// XChild
Reference<XInterface> SAL_CALL MemPackageEntry::getParent()
{
    return cppu::getXWeak(m_pParent);
}

void MemPackageEntry::doSetParent(MemPackageFolder* pNewParent)
{
    m_pParent = pNewParent;
    if (!m_sName.isEmpty() && !pNewParent->hasByName(m_sName))
        pNewParent->doInsertByName(this, false);
}

void SAL_CALL MemPackageEntry::setParent(const Reference<XInterface>& xNewParent)
{
    if (!xNewParent.is())
        throw NoSupportException();

    MemPackageFolder* pNewParent = dynamic_cast<MemPackageFolder*>(xNewParent.get());
    if (!pNewParent)
        throw NoSupportException();

    if (pNewParent != m_pParent)
    {
        if (m_pParent && !m_sName.isEmpty() && m_pParent->hasByName(m_sName))
            m_pParent->removeByName(m_sName);
        doSetParent(pNewParent);
    }
}

// XPropertySet
Reference<XPropertySetInfo> SAL_CALL MemPackageEntry::getPropertySetInfo()
{
    return Reference<XPropertySetInfo>();
}

void SAL_CALL MemPackageEntry::addPropertyChangeListener(const OUString& /*aPropertyName*/,
    const Reference<XPropertyChangeListener>& /*xListener*/)
{
}

void SAL_CALL MemPackageEntry::removePropertyChangeListener(const OUString& /*aPropertyName*/,
    const Reference<XPropertyChangeListener>& /*aListener*/)
{
}

void SAL_CALL MemPackageEntry::addVetoableChangeListener(const OUString& /*PropertyName*/,
    const Reference<XVetoableChangeListener>& /*aListener*/)
{
}

void SAL_CALL MemPackageEntry::removeVetoableChangeListener(const OUString& /*PropertyName*/,
    const Reference<XVetoableChangeListener>& /*aListener*/)
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 