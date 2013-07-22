/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef StringImpl_h
#define StringImpl_h

#include <limits.h>
#include "wtf/ASCIICType.h"
#include "wtf/Forward.h"
#include "wtf/StdLibExtras.h"
#include "wtf/StringHasher.h"
#include "wtf/Vector.h"
#include "wtf/WTFExport.h"
#include "wtf/unicode/Unicode.h"

#if USE(CF)
typedef const struct __CFString * CFStringRef;
#endif

#ifdef __OBJC__
@class NSString;
#endif

namespace WTF {

struct CStringTranslator;
template<typename CharacterType> struct HashAndCharactersTranslator;
struct HashAndUTF8CharactersTranslator;
struct LCharBufferTranslator;
struct CharBufferFromLiteralDataTranslator;
struct SubstringTranslator;
struct UCharBufferTranslator;
template<typename> class RetainPtr;

enum TextCaseSensitivity { TextCaseSensitive, TextCaseInsensitive };

typedef bool (*CharacterMatchFunctionPtr)(UChar);
typedef bool (*IsWhiteSpaceFunctionPtr)(UChar);

// Define STRING_STATS to turn on run time statistics of string sizes and memory usage
#undef STRING_STATS

#ifdef STRING_STATS
struct StringStats {
    inline void add8BitString(unsigned length)
    {
        ++m_totalNumberStrings;
        ++m_number8BitStrings;
        m_total8BitData += length;
    }

    inline void add16BitString(unsigned length)
    {
        ++m_totalNumberStrings;
        ++m_number16BitStrings;
        m_total16BitData += length;
    }

    void removeString(StringImpl*);
    void printStats();

    static const unsigned s_printStringStatsFrequency = 5000;
    static unsigned s_stringRemovesTillPrintStats;

    unsigned m_totalNumberStrings;
    unsigned m_number8BitStrings;
    unsigned m_number16BitStrings;
    unsigned long long m_total8BitData;
    unsigned long long m_total16BitData;
};

void addStringForStats(StringImpl*);
void removeStringForStats(StringImpl*);

#define STRING_STATS_ADD_8BIT_STRING(length) StringImpl::stringStats().add8BitString(length); addStringForStats(this)
#define STRING_STATS_ADD_16BIT_STRING(length) StringImpl::stringStats().add16BitString(length); addStringForStats(this)
#define STRING_STATS_REMOVE_STRING(string) StringImpl::stringStats().removeString(string); removeStringForStats(this)
#else
#define STRING_STATS_ADD_8BIT_STRING(length) ((void)0)
#define STRING_STATS_ADD_16BIT_STRING(length) ((void)0)
#define STRING_STATS_REMOVE_STRING(string) ((void)0)
#endif

class WTF_EXPORT StringImpl {
    WTF_MAKE_NONCOPYABLE(StringImpl);
    // This is needed because we malloc() space for a StringImpl plus an
    // immediately following buffer, as a performance tweak.
    NEW_DELETE_SAME_AS_MALLOC_FREE;
    friend struct WTF::CStringTranslator;
    template<typename CharacterType> friend struct WTF::HashAndCharactersTranslator;
    friend struct WTF::HashAndUTF8CharactersTranslator;
    friend struct WTF::CharBufferFromLiteralDataTranslator;
    friend struct WTF::LCharBufferTranslator;
    friend struct WTF::SubstringTranslator;
    friend struct WTF::UCharBufferTranslator;
    friend class AtomicStringImpl;

private:
    enum BufferOwnership {
        BufferInternal,
        BufferOwned,
        // NOTE: Adding more ownership types needs to extend m_hashAndFlags as we're at capacity
    };

    // Used to construct static strings, which have an special refCount that can never hit zero.
    // This means that the static string will never be destroyed, which is important because
    // static strings will be shared across threads & ref-counted in a non-threadsafe manner.
    enum ConstructStaticStringTag { ConstructStaticString };
    StringImpl(const UChar* characters, unsigned length, ConstructStaticStringTag)
        : m_data16(characters)
        , m_refCount(s_refCountFlagIsStaticString)
        , m_length(length)
        , m_hashAndFlags(BufferOwned)
    {
        // Ensure that the hash is computed so that AtomicStringHash can call existingHash()
        // with impunity. The empty string is special because it is never entered into
        // AtomicString's HashKey, but still needs to compare correctly.
        STRING_STATS_ADD_16BIT_STRING(m_length);

        hash();
    }

    // Used to construct static strings, which have an special refCount that can never hit zero.
    // This means that the static string will never be destroyed, which is important because
    // static strings will be shared across threads & ref-counted in a non-threadsafe manner.
    StringImpl(const LChar* characters, unsigned length, ConstructStaticStringTag)
        : m_data8(characters)
        , m_refCount(s_refCountFlagIsStaticString)
        , m_length(length)
        , m_hashAndFlags(s_hashFlag8BitBuffer | BufferOwned)
    {
        // Ensure that the hash is computed so that AtomicStringHash can call existingHash()
        // with impunity. The empty string is special because it is never entered into
        // AtomicString's HashKey, but still needs to compare correctly.
        STRING_STATS_ADD_8BIT_STRING(m_length);

        hash();
    }

    // FIXME: there has to be a less hacky way to do this.
    enum Force8Bit { Force8BitConstructor };
    // Create a normal 8-bit string with internal storage (BufferInternal)
    StringImpl(unsigned length, Force8Bit)
        : m_data8(reinterpret_cast<const LChar*>(this + 1))
        , m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_hashAndFlags(s_hashFlag8BitBuffer | BufferInternal)
    {
        ASSERT(m_data8);
        ASSERT(m_length);

        STRING_STATS_ADD_8BIT_STRING(m_length);
    }

    // Create a normal 16-bit string with internal storage (BufferInternal)
    StringImpl(unsigned length)
        : m_data16(reinterpret_cast<const UChar*>(this + 1))
        , m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_hashAndFlags(BufferInternal)
    {
        ASSERT(m_data16);
        ASSERT(m_length);

        STRING_STATS_ADD_16BIT_STRING(m_length);
    }

    // Create a StringImpl adopting ownership of the provided buffer (BufferOwned)
    StringImpl(const LChar* characters, unsigned length)
        : m_data8(characters)
        , m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_hashAndFlags(s_hashFlag8BitBuffer | BufferOwned)
    {
        ASSERT(m_data8);
        ASSERT(m_length);

        STRING_STATS_ADD_8BIT_STRING(m_length);
    }

    enum ConstructFromLiteralTag { ConstructFromLiteral };
    StringImpl(const char* characters, unsigned length, ConstructFromLiteralTag)
        : m_data8(reinterpret_cast<const LChar*>(characters))
        , m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_hashAndFlags(s_hashFlag8BitBuffer | BufferInternal)
    {
        ASSERT(m_data8);
        ASSERT(m_length);
        ASSERT(!characters[length]);

        STRING_STATS_ADD_8BIT_STRING(0);
    }

    // Create a StringImpl adopting ownership of the provided buffer (BufferOwned)
    StringImpl(const UChar* characters, unsigned length)
        : m_data16(characters)
        , m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_hashAndFlags(BufferOwned)
    {
        ASSERT(m_data16);
        ASSERT(m_length);

        STRING_STATS_ADD_16BIT_STRING(m_length);
    }

    enum CreateEmptyUnique_T { CreateEmptyUnique };
    StringImpl(CreateEmptyUnique_T)
        : m_data16(reinterpret_cast<const UChar*>(1))
        , m_refCount(s_refCountIncrement)
        , m_length(0)
    {
        ASSERT(m_data16);
        // Set the hash early, so that all empty unique StringImpls have a hash,
        // and don't use the normal hashing algorithm - the unique nature of these
        // keys means that we don't need them to match any other string (in fact,
        // that's exactly the oposite of what we want!), and teh normal hash would
        // lead to lots of conflicts.
        unsigned hash = reinterpret_cast<uintptr_t>(this);
        hash <<= s_flagCount;
        if (!hash)
            hash = 1 << s_flagCount;
        m_hashAndFlags = hash | BufferInternal;

        STRING_STATS_ADD_16BIT_STRING(m_length);
    }
public:
    ~StringImpl();

    static PassRefPtr<StringImpl> create(const UChar*, unsigned length);
    static PassRefPtr<StringImpl> create(const LChar*, unsigned length);
    static PassRefPtr<StringImpl> create8BitIfPossible(const UChar*, unsigned length);
    template<size_t inlineCapacity>
    static PassRefPtr<StringImpl> create8BitIfPossible(const Vector<UChar, inlineCapacity>& vector)
    {
        return create8BitIfPossible(vector.data(), vector.size());
    }

    ALWAYS_INLINE static PassRefPtr<StringImpl> create(const char* s, unsigned length) { return create(reinterpret_cast<const LChar*>(s), length); }
    static PassRefPtr<StringImpl> create(const LChar*);
    ALWAYS_INLINE static PassRefPtr<StringImpl> create(const char* s) { return create(reinterpret_cast<const LChar*>(s)); }

    static PassRefPtr<StringImpl> createFromLiteral(const char* characters, unsigned length);
    template<unsigned charactersCount>
    ALWAYS_INLINE static PassRefPtr<StringImpl> createFromLiteral(const char (&characters)[charactersCount])
    {
        COMPILE_ASSERT(charactersCount > 1, StringImplFromLiteralNotEmpty);
        COMPILE_ASSERT((charactersCount - 1 <= ((unsigned(~0) - sizeof(StringImpl)) / sizeof(LChar))), StringImplFromLiteralCannotOverflow);

        return createFromLiteral(characters, charactersCount - 1);
    }
    static PassRefPtr<StringImpl> createFromLiteral(const char* characters);

    static PassRefPtr<StringImpl> createUninitialized(unsigned length, LChar*& data);
    static PassRefPtr<StringImpl> createUninitialized(unsigned length, UChar*& data);
    template <typename T> static ALWAYS_INLINE PassRefPtr<StringImpl> tryCreateUninitialized(unsigned length, T*& output)
    {
        if (!length) {
            output = 0;
            return empty();
        }

        if (length > ((std::numeric_limits<unsigned>::max() - sizeof(StringImpl)) / sizeof(T))) {
            output = 0;
            return 0;
        }
        StringImpl* resultImpl;
        if (!tryFastMalloc(sizeof(T) * length + sizeof(StringImpl)).getValue(resultImpl)) {
            output = 0;
            return 0;
        }
        output = reinterpret_cast<T*>(resultImpl + 1);

        if (sizeof(T) == sizeof(char))
            return adoptRef(new (NotNull, resultImpl) StringImpl(length, Force8BitConstructor));

        return adoptRef(new (NotNull, resultImpl) StringImpl(length));
    }

    static PassRefPtr<StringImpl> createEmptyUnique()
    {
        return adoptRef(new StringImpl(CreateEmptyUnique));
    }

    // Reallocate the StringImpl. The originalString must be only owned by the PassRefPtr,
    // and the buffer ownership must be BufferInternal. Just like the input pointer of realloc(),
    // the originalString can't be used after this function.
    static PassRefPtr<StringImpl> reallocate(PassRefPtr<StringImpl> originalString, unsigned length, LChar*& data);
    static PassRefPtr<StringImpl> reallocate(PassRefPtr<StringImpl> originalString, unsigned length, UChar*& data);

    // If this StringImpl has only one reference, we can truncate the string by updating
    // its m_length property without actually re-allocating its buffer.
    void truncateAssumingIsolated(unsigned length)
    {
        ASSERT(hasOneRef());
        ASSERT(length <= m_length);
        ASSERT(bufferOwnership() == BufferInternal);
        m_length = length;
    }

    static unsigned flagsOffset() { return OBJECT_OFFSETOF(StringImpl, m_hashAndFlags); }
    static unsigned flagIs8Bit() { return s_hashFlag8BitBuffer; }
    static unsigned dataOffset() { return OBJECT_OFFSETOF(StringImpl, m_data8); }

    template<typename CharType, size_t inlineCapacity>
    static PassRefPtr<StringImpl> adopt(Vector<CharType, inlineCapacity>& vector)
    {
        if (size_t size = vector.size()) {
            ASSERT(vector.data());
            RELEASE_ASSERT(size <= std::numeric_limits<unsigned>::max());
            return adoptRef(new StringImpl(vector.releaseBuffer(), size));
        }
        return empty();
    }

    static PassRefPtr<StringImpl> adopt(StringBuffer<UChar>&);
    static PassRefPtr<StringImpl> adopt(StringBuffer<LChar>&);

    unsigned length() const { return m_length; }
    bool is8Bit() const { return m_hashAndFlags & s_hashFlag8BitBuffer; }
    bool hasInternalBuffer() const { return bufferOwnership() == BufferInternal; }
    bool hasOwnedBuffer() const { return bufferOwnership() == BufferOwned; }

    ALWAYS_INLINE const LChar* characters8() const { ASSERT(is8Bit()); return m_data8; }
    ALWAYS_INLINE const UChar* characters16() const { ASSERT(!is8Bit()); return m_data16; }

    template <typename CharType>
    ALWAYS_INLINE const CharType * getCharacters() const;

    size_t cost()
    {
        if (m_hashAndFlags & s_hashFlagDidReportCost)
            return 0;

        m_hashAndFlags |= s_hashFlagDidReportCost;
        return m_length;
    }

    size_t sizeInBytes() const;

    bool isEmptyUnique() const
    {
        return !length() && !isStatic();
    }

    bool isAtomic() const { return m_hashAndFlags & s_hashFlagIsAtomic; }
    void setIsAtomic(bool isAtomic)
    {
        if (isAtomic)
            m_hashAndFlags |= s_hashFlagIsAtomic;
        else
            m_hashAndFlags &= ~s_hashFlagIsAtomic;
    }

    bool isStatic() const { return m_refCount & s_refCountFlagIsStaticString; }

private:
    // The high bits of 'hash' are always empty, but we prefer to store our flags
    // in the low bits because it makes them slightly more efficient to access.
    // So, we shift left and right when setting and getting our hash code.
    void setHash(unsigned hash) const
    {
        ASSERT(!hasHash());
        // Multiple clients assume that StringHasher is the canonical string hash function.
        ASSERT(hash == (is8Bit() ? StringHasher::computeHashAndMaskTop8Bits(m_data8, m_length) : StringHasher::computeHashAndMaskTop8Bits(m_data16, m_length)));
        ASSERT(!(hash & (s_flagMask << (8 * sizeof(hash) - s_flagCount)))); // Verify that enough high bits are empty.

        hash <<= s_flagCount;
        ASSERT(!(hash & m_hashAndFlags)); // Verify that enough low bits are empty after shift.
        ASSERT(hash); // Verify that 0 is a valid sentinel hash value.

        m_hashAndFlags |= hash; // Store hash with flags in low bits.
    }

    unsigned rawHash() const
    {
        return m_hashAndFlags >> s_flagCount;
    }

public:
    bool hasHash() const
    {
        return rawHash() != 0;
    }

    unsigned existingHash() const
    {
        ASSERT(hasHash());
        return rawHash();
    }

    unsigned hash() const
    {
        if (hasHash())
            return existingHash();
        return hashSlowCase();
    }

    inline bool hasOneRef() const
    {
        return m_refCount == s_refCountIncrement;
    }

    inline void ref()
    {
        m_refCount += s_refCountIncrement;
    }

    inline void deref()
    {
        if (m_refCount == s_refCountIncrement) {
            delete this;
            return;
        }

        m_refCount -= s_refCountIncrement;
    }

    static StringImpl* empty();

    // FIXME: Does this really belong in StringImpl?
    template <typename T> static void copyChars(T* destination, const T* source, unsigned numCharacters)
    {
        if (numCharacters == 1) {
            *destination = *source;
            return;
        }

        // FIXME: Is this implementation really faster than memcpy?
        if (numCharacters <= s_copyCharsInlineCutOff) {
            unsigned i = 0;
#if (CPU(X86) || CPU(X86_64))
            const unsigned charsPerInt = sizeof(uint32_t) / sizeof(T);

            if (numCharacters > charsPerInt) {
                unsigned stopCount = numCharacters & ~(charsPerInt - 1);

                const uint32_t* srcCharacters = reinterpret_cast<const uint32_t*>(source);
                uint32_t* destCharacters = reinterpret_cast<uint32_t*>(destination);
                for (unsigned j = 0; i < stopCount; i += charsPerInt, ++j)
                    destCharacters[j] = srcCharacters[j];
            }
#endif
            for (; i < numCharacters; ++i)
                destination[i] = source[i];
        } else
            memcpy(destination, source, numCharacters * sizeof(T));
    }

    ALWAYS_INLINE static void copyChars(UChar* destination, const LChar* source, unsigned numCharacters)
    {
        for (unsigned i = 0; i < numCharacters; ++i)
            destination[i] = source[i];
    }

    // Some string features, like refcounting and the atomicity flag, are not
    // thread-safe. We achieve thread safety by isolation, giving each thread
    // its own copy of the string.
    PassRefPtr<StringImpl> isolatedCopy() const;

    PassRefPtr<StringImpl> substring(unsigned pos, unsigned len = UINT_MAX);

    UChar operator[](unsigned i) const
    {
        ASSERT_WITH_SECURITY_IMPLICATION(i < m_length);
        if (is8Bit())
            return m_data8[i];
        return m_data16[i];
    }
    UChar32 characterStartingAt(unsigned);

    bool containsOnlyWhitespace();

    int toIntStrict(bool* ok = 0, int base = 10);
    unsigned toUIntStrict(bool* ok = 0, int base = 10);
    int64_t toInt64Strict(bool* ok = 0, int base = 10);
    uint64_t toUInt64Strict(bool* ok = 0, int base = 10);
    intptr_t toIntPtrStrict(bool* ok = 0, int base = 10);

    int toInt(bool* ok = 0); // ignores trailing garbage
    unsigned toUInt(bool* ok = 0); // ignores trailing garbage
    int64_t toInt64(bool* ok = 0); // ignores trailing garbage
    uint64_t toUInt64(bool* ok = 0); // ignores trailing garbage
    intptr_t toIntPtr(bool* ok = 0); // ignores trailing garbage

    // FIXME: Like the strict functions above, these give false for "ok" when there is trailing garbage.
    // Like the non-strict functions above, these return the value when there is trailing garbage.
    // It would be better if these were more consistent with the above functions instead.
    double toDouble(bool* ok = 0);
    float toFloat(bool* ok = 0);

    PassRefPtr<StringImpl> lower();
    PassRefPtr<StringImpl> upper();

    PassRefPtr<StringImpl> fill(UChar);
    // FIXME: Do we need fill(char) or can we just do the right thing if UChar is ASCII?
    PassRefPtr<StringImpl> foldCase();

    PassRefPtr<StringImpl> stripWhiteSpace();
    PassRefPtr<StringImpl> stripWhiteSpace(IsWhiteSpaceFunctionPtr);
    PassRefPtr<StringImpl> simplifyWhiteSpace();
    PassRefPtr<StringImpl> simplifyWhiteSpace(IsWhiteSpaceFunctionPtr);

    PassRefPtr<StringImpl> removeCharacters(CharacterMatchFunctionPtr);
    template <typename CharType>
    ALWAYS_INLINE PassRefPtr<StringImpl> removeCharacters(const CharType* characters, CharacterMatchFunctionPtr);

    size_t find(LChar character, unsigned start = 0);
    size_t find(char character, unsigned start = 0);
    size_t find(UChar character, unsigned start = 0);
    size_t find(CharacterMatchFunctionPtr, unsigned index = 0);
    size_t find(const LChar*, unsigned index = 0);
    ALWAYS_INLINE size_t find(const char* s, unsigned index = 0) { return find(reinterpret_cast<const LChar*>(s), index); }
    size_t find(StringImpl*);
    size_t find(StringImpl*, unsigned index);
    size_t findIgnoringCase(const LChar*, unsigned index = 0);
    ALWAYS_INLINE size_t findIgnoringCase(const char* s, unsigned index = 0) { return findIgnoringCase(reinterpret_cast<const LChar*>(s), index); }
    size_t findIgnoringCase(StringImpl*, unsigned index = 0);

    size_t findNextLineStart(unsigned index = UINT_MAX);

    size_t reverseFind(UChar, unsigned index = UINT_MAX);
    size_t reverseFind(StringImpl*, unsigned index = UINT_MAX);
    size_t reverseFindIgnoringCase(StringImpl*, unsigned index = UINT_MAX);

    size_t count(LChar) const;

    bool startsWith(StringImpl* str, bool caseSensitive = true) { return (caseSensitive ? reverseFind(str, 0) : reverseFindIgnoringCase(str, 0)) == 0; }
    bool startsWith(UChar) const;
    bool startsWith(const char*, unsigned matchLength, bool caseSensitive) const;
    template<unsigned matchLength>
    bool startsWith(const char (&prefix)[matchLength], bool caseSensitive = true) const { return startsWith(prefix, matchLength - 1, caseSensitive); }

    bool endsWith(StringImpl*, bool caseSensitive = true);
    bool endsWith(UChar) const;
    bool endsWith(const char*, unsigned matchLength, bool caseSensitive) const;
    template<unsigned matchLength>
    bool endsWith(const char (&prefix)[matchLength], bool caseSensitive = true) const { return endsWith(prefix, matchLength - 1, caseSensitive); }

    PassRefPtr<StringImpl> replace(UChar, UChar);
    PassRefPtr<StringImpl> replace(UChar, StringImpl*);
    ALWAYS_INLINE PassRefPtr<StringImpl> replace(UChar pattern, const char* replacement, unsigned replacementLength) { return replace(pattern, reinterpret_cast<const LChar*>(replacement), replacementLength); }
    PassRefPtr<StringImpl> replace(UChar, const LChar*, unsigned replacementLength);
    PassRefPtr<StringImpl> replace(UChar, const UChar*, unsigned replacementLength);
    PassRefPtr<StringImpl> replace(StringImpl*, StringImpl*);
    PassRefPtr<StringImpl> replace(unsigned index, unsigned len, StringImpl*);

    WTF::Unicode::Direction defaultWritingDirection(bool* hasStrongDirectionality = 0);

#if USE(CF)
    RetainPtr<CFStringRef> createCFString();
#endif
#ifdef __OBJC__
    operator NSString*();
#endif

#ifdef STRING_STATS
    ALWAYS_INLINE static StringStats& stringStats() { return m_stringStats; }
#endif

private:

    bool isASCIILiteral() const
    {
        return is8Bit() && hasInternalBuffer() && reinterpret_cast<const void*>(m_data8) != reinterpret_cast<const void*>(this + 1);
    }

    // This number must be at least 2 to avoid sharing empty, null as well as 1 character strings from SmallStrings.
    static const unsigned s_copyCharsInlineCutOff = 20;

    BufferOwnership bufferOwnership() const { return static_cast<BufferOwnership>(m_hashAndFlags & s_hashMaskBufferOwnership); }
    template <class UCharPredicate> PassRefPtr<StringImpl> stripMatchedCharacters(UCharPredicate);
    template <typename CharType, class UCharPredicate> PassRefPtr<StringImpl> simplifyMatchedCharactersToSpace(UCharPredicate);
    NEVER_INLINE unsigned hashSlowCase() const;

    // The bottom bit in the ref count indicates a static (immortal) string.
    static const unsigned s_refCountFlagIsStaticString = 0x1;
    static const unsigned s_refCountIncrement = 0x2; // This allows us to ref / deref without disturbing the static string flag.

    // The bottom 8 bits in the hash are flags.
    static const unsigned s_flagCount = 8;
    static const unsigned s_flagMask = (1u << s_flagCount) - 1;
    COMPILE_ASSERT(s_flagCount == StringHasher::flagCount, StringHasher_reserves_enough_bits_for_StringImpl_flags);

    static const unsigned s_hashFlag8BitBuffer = 1u << 6;
    static const unsigned s_unusedHashFlag = 1u << 5;
    static const unsigned s_hashFlagIsAtomic = 1u << 4;
    static const unsigned s_hashFlagDidReportCost = 1u << 3;
    static const unsigned s_hashMaskBufferOwnership = 1u | (1u << 1);

#ifdef STRING_STATS
    static StringStats m_stringStats;
#endif

public:
    struct StaticASCIILiteral {
        // These member variables must match the layout of StringImpl.
        const LChar* m_data8;
        unsigned m_refCount;
        unsigned m_length;
        unsigned m_hashAndFlags;

        static const unsigned s_initialRefCount = s_refCountFlagIsStaticString;
        static const unsigned s_initialFlags = s_hashFlag8BitBuffer | BufferInternal;
        static const unsigned s_hashShift = s_flagCount;
    };

#ifndef NDEBUG
    void assertHashIsCorrect()
    {
        ASSERT(hasHash());
        ASSERT(existingHash() == StringHasher::computeHashAndMaskTop8Bits(characters8(), length()));
    }
#endif

private:
    // These member variables must match the layout of StaticASCIILiteral.
    union {  // Pointers first: crbug.com/232031
        const LChar* m_data8;
        const UChar* m_data16;
    };
    unsigned m_refCount;
    unsigned m_length;
    mutable unsigned m_hashAndFlags;
};

COMPILE_ASSERT(sizeof(StringImpl) == sizeof(StringImpl::StaticASCIILiteral), StringImpl_should_match_its_StaticASCIILiteral);

template <>
ALWAYS_INLINE const LChar* StringImpl::getCharacters<LChar>() const { return characters8(); }

template <>
ALWAYS_INLINE const UChar* StringImpl::getCharacters<UChar>() const { return characters16(); }

WTF_EXPORT bool equal(const StringImpl*, const StringImpl*);
WTF_EXPORT bool equal(const StringImpl*, const LChar*);
inline bool equal(const StringImpl* a, const char* b) { return equal(a, reinterpret_cast<const LChar*>(b)); }
WTF_EXPORT bool equal(const StringImpl*, const LChar*, unsigned);
WTF_EXPORT bool equal(const StringImpl*, const UChar*, unsigned);
inline bool equal(const StringImpl* a, const char* b, unsigned length) { return equal(a, reinterpret_cast<const LChar*>(b), length); }
inline bool equal(const LChar* a, StringImpl* b) { return equal(b, a); }
inline bool equal(const char* a, StringImpl* b) { return equal(b, reinterpret_cast<const LChar*>(a)); }
WTF_EXPORT bool equalNonNull(const StringImpl* a, const StringImpl* b);

template<typename CharType>
ALWAYS_INLINE bool equal(const CharType* a, const CharType* b, unsigned length) { return !memcmp(a, b, length * sizeof(CharType)); }

ALWAYS_INLINE bool equal(const LChar* a, const UChar* b, unsigned length)
{
    for (unsigned i = 0; i < length; ++i) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

ALWAYS_INLINE bool equal(const UChar* a, const LChar* b, unsigned length) { return equal(b, a, length); }

WTF_EXPORT bool equalIgnoringCase(const StringImpl*, const StringImpl*);
WTF_EXPORT bool equalIgnoringCase(const StringImpl*, const LChar*);
inline bool equalIgnoringCase(const LChar* a, const StringImpl* b) { return equalIgnoringCase(b, a); }
WTF_EXPORT bool equalIgnoringCase(const LChar*, const LChar*, unsigned);
WTF_EXPORT bool equalIgnoringCase(const UChar*, const LChar*, unsigned);
inline bool equalIgnoringCase(const UChar* a, const char* b, unsigned length) { return equalIgnoringCase(a, reinterpret_cast<const LChar*>(b), length); }
inline bool equalIgnoringCase(const LChar* a, const UChar* b, unsigned length) { return equalIgnoringCase(b, a, length); }
inline bool equalIgnoringCase(const char* a, const UChar* b, unsigned length) { return equalIgnoringCase(b, reinterpret_cast<const LChar*>(a), length); }
inline bool equalIgnoringCase(const char* a, const LChar* b, unsigned length) { return equalIgnoringCase(b, reinterpret_cast<const LChar*>(a), length); }
inline bool equalIgnoringCase(const UChar* a, const UChar* b, int length)
{
    ASSERT(length >= 0);
    return !Unicode::umemcasecmp(a, b, length);
}
WTF_EXPORT bool equalIgnoringCaseNonNull(const StringImpl*, const StringImpl*);

WTF_EXPORT bool equalIgnoringNullity(StringImpl*, StringImpl*);

template<typename CharacterType>
inline size_t find(const CharacterType* characters, unsigned length, CharacterType matchCharacter, unsigned index = 0)
{
    while (index < length) {
        if (characters[index] == matchCharacter)
            return index;
        ++index;
    }
    return notFound;
}

ALWAYS_INLINE size_t find(const UChar* characters, unsigned length, LChar matchCharacter, unsigned index = 0)
{
    return find(characters, length, static_cast<UChar>(matchCharacter), index);
}

inline size_t find(const LChar* characters, unsigned length, UChar matchCharacter, unsigned index = 0)
{
    if (matchCharacter & ~0xFF)
        return notFound;
    return find(characters, length, static_cast<LChar>(matchCharacter), index);
}

inline size_t find(const LChar* characters, unsigned length, CharacterMatchFunctionPtr matchFunction, unsigned index = 0)
{
    while (index < length) {
        if (matchFunction(characters[index]))
            return index;
        ++index;
    }
    return notFound;
}

inline size_t find(const UChar* characters, unsigned length, CharacterMatchFunctionPtr matchFunction, unsigned index = 0)
{
    while (index < length) {
        if (matchFunction(characters[index]))
            return index;
        ++index;
    }
    return notFound;
}

template<typename CharacterType>
inline size_t findNextLineStart(const CharacterType* characters, unsigned length, unsigned index = 0)
{
    while (index < length) {
        CharacterType c = characters[index++];
        if ((c != '\n') && (c != '\r'))
            continue;

        // There can only be a start of a new line if there are more characters
        // beyond the current character.
        if (index < length) {
            // The 3 common types of line terminators are 1. \r\n (Windows),
            // 2. \r (old MacOS) and 3. \n (Unix'es).

            if (c == '\n')
                return index; // Case 3: just \n.

            CharacterType c2 = characters[index];
            if (c2 != '\n')
                return index; // Case 2: just \r.

            // Case 1: \r\n.
            // But, there's only a start of a new line if there are more
            // characters beyond the \r\n.
            if (++index < length)
                return index;
        }
    }
    return notFound;
}

template<typename CharacterType>
inline size_t reverseFindLineTerminator(const CharacterType* characters, unsigned length, unsigned index = UINT_MAX)
{
    if (!length)
        return notFound;
    if (index >= length)
        index = length - 1;
    CharacterType c = characters[index];
    while ((c != '\n') && (c != '\r')) {
        if (!index--)
            return notFound;
        c = characters[index];
    }
    return index;
}

template<typename CharacterType>
inline size_t reverseFind(const CharacterType* characters, unsigned length, CharacterType matchCharacter, unsigned index = UINT_MAX)
{
    if (!length)
        return notFound;
    if (index >= length)
        index = length - 1;
    while (characters[index] != matchCharacter) {
        if (!index--)
            return notFound;
    }
    return index;
}

ALWAYS_INLINE size_t reverseFind(const UChar* characters, unsigned length, LChar matchCharacter, unsigned index = UINT_MAX)
{
    return reverseFind(characters, length, static_cast<UChar>(matchCharacter), index);
}

inline size_t reverseFind(const LChar* characters, unsigned length, UChar matchCharacter, unsigned index = UINT_MAX)
{
    if (matchCharacter & ~0xFF)
        return notFound;
    return reverseFind(characters, length, static_cast<LChar>(matchCharacter), index);
}

inline size_t StringImpl::find(LChar character, unsigned start)
{
    if (is8Bit())
        return WTF::find(characters8(), m_length, character, start);
    return WTF::find(characters16(), m_length, character, start);
}

ALWAYS_INLINE size_t StringImpl::find(char character, unsigned start)
{
    return find(static_cast<LChar>(character), start);
}

inline size_t StringImpl::find(UChar character, unsigned start)
{
    if (is8Bit())
        return WTF::find(characters8(), m_length, character, start);
    return WTF::find(characters16(), m_length, character, start);
}

inline unsigned lengthOfNullTerminatedString(const UChar* string)
{
    size_t length = 0;
    while (string[length] != UChar(0))
        ++length;
    RELEASE_ASSERT(length <= std::numeric_limits<unsigned>::max());
    return static_cast<unsigned>(length);
}

template<size_t inlineCapacity>
bool equalIgnoringNullity(const Vector<UChar, inlineCapacity>& a, StringImpl* b)
{
    if (!b)
        return !a.size();
    if (a.size() != b->length())
        return false;
    if (b->is8Bit())
        return equal(a.data(), b->characters8(), b->length());
    return equal(a.data(), b->characters16(), b->length());
}

template<typename CharacterType1, typename CharacterType2>
static inline int codePointCompare(unsigned l1, unsigned l2, const CharacterType1* c1, const CharacterType2* c2)
{
    const unsigned lmin = l1 < l2 ? l1 : l2;
    unsigned pos = 0;
    while (pos < lmin && *c1 == *c2) {
        ++c1;
        ++c2;
        ++pos;
    }

    if (pos < lmin)
        return (c1[0] > c2[0]) ? 1 : -1;

    if (l1 == l2)
        return 0;

    return (l1 > l2) ? 1 : -1;
}

static inline int codePointCompare8(const StringImpl* string1, const StringImpl* string2)
{
    return codePointCompare(string1->length(), string2->length(), string1->characters8(), string2->characters8());
}

static inline int codePointCompare16(const StringImpl* string1, const StringImpl* string2)
{
    return codePointCompare(string1->length(), string2->length(), string1->characters16(), string2->characters16());
}

static inline int codePointCompare8To16(const StringImpl* string1, const StringImpl* string2)
{
    return codePointCompare(string1->length(), string2->length(), string1->characters8(), string2->characters16());
}

static inline int codePointCompare(const StringImpl* string1, const StringImpl* string2)
{
    if (!string1)
        return (string2 && string2->length()) ? -1 : 0;

    if (!string2)
        return string1->length() ? 1 : 0;

    bool string1Is8Bit = string1->is8Bit();
    bool string2Is8Bit = string2->is8Bit();
    if (string1Is8Bit) {
        if (string2Is8Bit)
            return codePointCompare8(string1, string2);
        return codePointCompare8To16(string1, string2);
    }
    if (string2Is8Bit)
        return -codePointCompare8To16(string2, string1);
    return codePointCompare16(string1, string2);
}

static inline bool isSpaceOrNewline(UChar c)
{
    // Use isASCIISpace() for basic Latin-1.
    // This will include newlines, which aren't included in Unicode DirWS.
    return c <= 0x7F ? WTF::isASCIISpace(c) : WTF::Unicode::direction(c) == WTF::Unicode::WhiteSpaceNeutral;
}

inline PassRefPtr<StringImpl> StringImpl::isolatedCopy() const
{
    if (isASCIILiteral())
        return StringImpl::createFromLiteral(reinterpret_cast<const char*>(m_data8), m_length);
    if (is8Bit())
        return create(m_data8, m_length);
    return create(m_data16, m_length);
}

struct StringHash;

// StringHash is the default hash for StringImpl* and RefPtr<StringImpl>
template<typename T> struct DefaultHash;
template<> struct DefaultHash<StringImpl*> {
    typedef StringHash Hash;
};
template<> struct DefaultHash<RefPtr<StringImpl> > {
    typedef StringHash Hash;
};

}

using WTF::StringImpl;
using WTF::equal;
using WTF::equalNonNull;
using WTF::TextCaseSensitivity;
using WTF::TextCaseSensitive;
using WTF::TextCaseInsensitive;

#endif
