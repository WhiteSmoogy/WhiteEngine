// Copyright (c) .NET Foundation and Contributors. All Rights Reserved. Licensed under the MIT License (MIT). See License.md in the repository root for more information.

namespace ClangSharp.Abstractions;

internal enum ValueKind
{
    Unknown,
    Primitive,
    Enumerator,
    Unmanaged,
    String,
    GuidMember,
}
