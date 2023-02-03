// Copyright (C) 2021 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QTLOADERGLOBAL_H
#define QTLOADERGLOBAL_H

#include <QtCore/qcompilerdetection.h>

#ifndef QT_STATIC
#  if defined(QT_BUILD_LOADER_LIB)
#    define Q_LOADER_EXPORT Q_DECL_EXPORT
#  else
#    define Q_LOADER_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_LOADER_EXPORT
#endif

#endif // QTLOADERGLOBAL_H
