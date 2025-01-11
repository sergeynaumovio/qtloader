// Copyright (C) 2025 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QSCOPEDSTORAGE_H
#define QSCOPEDSTORAGE_H

#include <QtCore/qglobal.h>

template<typename T, int Size, int Alignment = 8>
class QScopedStorage
{
    Q_DISABLE_COPY_MOVE(QScopedStorage)

    alignas(Alignment) std::byte d_storage[Size];

    template<int TSize, int TAlignment>
    static void checkSize() noexcept
    {
        static_assert(Size == TSize, "sizeof(QScopedStorage) and sizeof(T) mismatch");
        static_assert(Alignment == TAlignment, "alignof(QScopedStorage) and alignof(T) mismatch");
    }

    T *d_func() const noexcept { return reinterpret_cast<T *>((void *)(&d_storage)); }

public:
    template<typename... Args>
    explicit QScopedStorage(Args&&... args) { new (d_func()) T(std::forward<Args>(args)...); }

    ~QScopedStorage() noexcept
    {
        d_func()->~T();
        checkSize<sizeof(T), alignof(T)>();
    }

    T *operator->() const noexcept { return d_func(); }
    T &operator*() const noexcept { return *d_func(); }
};

#endif // QSCOPEDSTORAGE_H
