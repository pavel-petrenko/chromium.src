/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CSSGridTemplateValue_h
#define CSSGridTemplateValue_h

#include "core/css/CSSValue.h"
#include "core/rendering/style/GridCoordinate.h"
#include "wtf/text/StringHash.h"

namespace WebCore {

class CSSGridTemplateValue : public CSSValue {
public:
    static PassRefPtr<CSSGridTemplateValue> create(const NamedGridAreaMap& gridAreaMap, size_t rowCount, size_t columnCount) { return adoptRef(new CSSGridTemplateValue(gridAreaMap, rowCount, columnCount)); }
    ~CSSGridTemplateValue() { }

    String customCssText() const;

    const NamedGridAreaMap& gridAreaMap() const { return m_gridAreaMap; }
    size_t rowCount() const { return m_rowCount; }
    size_t columnCount() const { return m_columnCount; }

private:
    CSSGridTemplateValue(const NamedGridAreaMap&, size_t rowCount, size_t columnCount);

    NamedGridAreaMap m_gridAreaMap;
    size_t m_rowCount;
    size_t m_columnCount;
};

inline CSSGridTemplateValue* toCSSGridTemplateValue(CSSValue* value)
{
    ASSERT_WITH_SECURITY_IMPLICATION(value->isGridTemplateValue());
    return static_cast<CSSGridTemplateValue*>(value);
}

inline const CSSGridTemplateValue* toCSSGridTemplateValue(const CSSValue* value)
{
    ASSERT_WITH_SECURITY_IMPLICATION(value->isGridTemplateValue());
    return static_cast<const CSSGridTemplateValue*>(value);
}

// Catch unneeded cast.
void toCSSGridTemplateValue(const CSSGridTemplateValue*);

} // namespace WebCore

#endif // CSSGridTemplateValue_h
