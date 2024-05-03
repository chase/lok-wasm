/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#ifndef INCLUDED_OOX_HELPER_EXPANDEDSTORAGE_HXX
#define INCLUDED_OOX_HELPER_EXPANDEDSTORAGE_HXX

#include "com/sun/star/uno/Sequence.h"
#include "sal/types.h"
#include <vector>

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <oox/helper/storagebase.hxx>
#include <rtl/ustring.hxx>

namespace com::sun::star {
    namespace embed { class XStorage; }
    namespace io { class XInputStream; }
    namespace io { class XOutputStream; }
    namespace io { class XStream; }
    namespace uno { class XComponentContext; }
}

namespace oox {




using namespace com::sun::star;

struct ExpandedPart {
    const OUString&  path;
    const OUString&  sha;
    css::uno::Sequence<sal_Int8> content;

    ExpandedPart(const OUString& path_, const OUString& sha_, css::uno::Sequence<sal_Int8> content_)
        : path(path_)
        , sha(sha_)
        , content(content_){};
};

class ExpandedStorage final : public StorageBase
{
public:
    // Reading in from an existing docx file
    explicit            ExpandedStorage(
                            const css::uno::Reference< css::uno::XComponentContext >& rxContext,
                            const css::uno::Reference< css::io::XInputStream >& rxInStream,
                            bool bRepairStorage );

    // reading in from a BOM
    explicit            ExpandedStorage(
                            std::vector<ExpandedPart>& parts,
                            bool bRepairStorage);

    virtual             ~ExpandedStorage() override;

private:

    virtual bool        implIsStorage() const override;

    virtual css::uno::Reference< css::embed::XStorage >
                        implGetXStorage() const override;

    virtual void        implGetElementNames( ::std::vector< OUString >& orElementNames ) const override;

    virtual StorageRef  implOpenSubStorage( const OUString& rElementName, bool bCreateMissing ) override;

    virtual css::uno::Reference< css::io::XInputStream >
                        implOpenInputStream( const OUString& rElementName ) override;

    virtual css::uno::Reference< css::io::XOutputStream >
                        implOpenOutputStream( const OUString& rElementName ) override;

    /** Commits the current storage. */
    virtual void        implCommit() const override;

    void addPart(ExpandedPart& part);

private:
    std::unordered_map<OUString, ExpandedPart> m_parts;
};


} // namespace oox

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
