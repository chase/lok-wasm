/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_PACKAGE_SOURCE_MEMPACKAGE_MEMPACKAGEFOLDERENUMERATION_HXX
#define INCLUDED_PACKAGE_SOURCE_MEMPACKAGE_MEMPACKAGEFOLDERENUMERATION_HXX

#include <com/sun/star/container/XEnumeration.hpp>
#include <cppuhelper/implbase.hxx>
#include <unordered_map>

struct MemContentInfo;

class MemPackageFolderEnumeration final : public cppu::WeakImplHelper<css::container::XEnumeration>
{
    std::unordered_map<OUString, MemContentInfo> m_aContents;
    std::unordered_map<OUString, MemContentInfo>::const_iterator m_aIterator;

public:
    explicit MemPackageFolderEnumeration(const std::unordered_map<OUString, MemContentInfo>& rContents);
    virtual ~MemPackageFolderEnumeration() override;

    // XEnumeration
    virtual sal_Bool SAL_CALL hasMoreElements() override;
    virtual css::uno::Any SAL_CALL nextElement() override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 