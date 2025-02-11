/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "MemPackageFolderEnumeration.hxx"
#include "MemPackageFolder.hxx"

#include <com/sun/star/container/NoSuchElementException.hpp>
#include <com/sun/star/beans/StringPair.hpp>

using namespace com::sun::star;
using namespace com::sun::star::container;

#if OSL_DEBUG_LEVEL > 0
#define THROW_WHERE SAL_WHERE
#else
#define THROW_WHERE ""
#endif

MemPackageFolderEnumeration::MemPackageFolderEnumeration(
    const std::unordered_map<OUString, MemContentInfo>& rContents)
    : m_aContents(rContents)
    , m_aIterator(m_aContents.begin())
{
}

MemPackageFolderEnumeration::~MemPackageFolderEnumeration()
{
}

sal_Bool SAL_CALL MemPackageFolderEnumeration::hasMoreElements()
{
    return m_aIterator != m_aContents.end();
}

uno::Any SAL_CALL MemPackageFolderEnumeration::nextElement()
{
    uno::Any aAny;
    if (m_aIterator == m_aContents.end())
        throw container::NoSuchElementException(THROW_WHERE);
    aAny <<= uno::Reference(cppu::getXWeak((*m_aIterator).second.xPackageEntry.get()));
    ++m_aIterator;
    return aAny;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 