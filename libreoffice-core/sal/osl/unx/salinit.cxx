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

#include <config_features.h>

#include <sal/config.h>
// MACRO: use console.log/warn
#include <config_global.h>
#include <osl/process.h>
#include <sal/main.h>

#include "saltime.hxx"
#include "soffice.hxx"
// MACRO: use console.log/warn

extern "C" {

void sal_detail_initialize(int argc, char ** argv) {
    if (argc == sal::detail::InitializeSoffice)
    {
        sal::detail::setSoffice();
        return;
    }
#if defined MACOSX && !HAVE_FEATURE_MACOSX_SANDBOX
    // On macOS when not sandboxed, soffice can restart itself via exec (see
    // restartOnMac in desktop/source/app/app.cxx), which leaves all file
    // descriptors open, which in turn can have unwanted effects (see
    // <https://bugs.libreoffice.org/show_bug.cgi?id=50603> "Unable to update
    // LibreOffice without resetting user profile"). But closing fds in
    // restartOnMac before calling exec does not work, as additional threads
    // might still be running then, which can still use those fds and cause
    // crashes. Therefore, the simplest solution is to close fds at process
    // start (as early as possible, so that no other threads have been created
    // yet that might already have opened some fds); this is done for all kinds
    // of processes here, not just soffice, but hopefully none of our processes
    // rely on being spawned with certain fds already open. Unfortunately, Mac
    // macOS appears to have no better interface to close all fds (like
    // closefrom):
    long openMax = sysconf(_SC_OPEN_MAX);
    // When LibreOffice restarts itself on macOS 11 beta on arm64, for
    // some reason sysconf(_SC_OPEN_MAX) returns 0x7FFFFFFFFFFFFFFF,
    // so use a sanity limit here.
    if (openMax == -1 || openMax == std::numeric_limits<long>::max()) {
        openMax = 100000;
    }
    assert(openMax >= 0 && openMax <= std::numeric_limits< int >::max());
    for (int fd = 3; fd < int(openMax); ++fd) {
        struct stat s;
        if (fstat(fd, &s) != -1 && S_ISREG(s.st_mode))
            close(fd);
    }
#endif
    sal_initGlobalTimer();
// MACRO: use console.log/warn

    osl_setCommandArgs(argc, argv);
}

void sal_detail_deinitialize() {}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
