/*
 *  utility.h
 *
 *  Copyright (C) 2003, 2004, 2006 - Niko Ritari
 *  Copyright (C) 2003, 2004, 2006 - Jani Rivinoja
 *
 *  This file is part of Outgun.
 *
 *  Outgun is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Outgun is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Outgun; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef UTILITY_H_INC
#define UTILITY_H_INC

#include <limits>
#include <string>
#include <vector>

#include "nassert.h" // for __attribute__ for non-GCC as well as nAssert

// try to keep the includes down: if new includes are needed, consider a separate header

#ifdef __GNUC__
#define PRINTF_FORMAT(a, b) __attribute__ ((format (printf, a, b)))
#else
#define PRINTF_FORMAT(a, b)
#endif

// to be used as a base class when a class is needed to not have a copy constructor or copy assignment operator available
class NoCopying {
    NoCopying(const NoCopying&) throw () { }
    NoCopying& operator=(const NoCopying&) throw () { return *this; }

protected:
    NoCopying() throw () { }
};

class NoCopyConstruct {
    NoCopyConstruct(const NoCopyConstruct&) throw () { }

protected:
    NoCopyConstruct() throw () { }
};

class NoCopyAssign {
    NoCopyAssign& operator=(const NoCopyAssign&) throw () { return *this; }

protected:
    NoCopyAssign() throw () { }
};

class Lockable {
public:
    virtual ~Lockable() throw () { }

    virtual void lock() throw () = 0;
    virtual void unlock() throw () = 0;
};

/** Base class for lockable classes whose lock/unlock methods are const.
 * The apparent case is internal use of a mutable Lockable to provide locking.
 */
class ConstLockable : public Lockable {
    void lock  () throw () { static_cast<const ConstLockable&>(*this).lock  (); }
    void unlock() throw () { static_cast<const ConstLockable&>(*this).unlock(); }

public:
    virtual ~ConstLockable() throw () { }

    virtual void lock() const throw () = 0;
    virtual void unlock() const throw () = 0;
};

/** Lock a Lockable for the lifetime of the object. */
class Lock : private NoCopying {
    Lockable& t;

public:
    Lock(Lockable& target) throw () : t(target) { t.lock(); }
    ~Lock() throw () { t.unlock(); }
};

/** Unlock a Lockable for the lifetime of the object. */
class Unlock : private NoCopying {
    Lockable& t;

public:
    Unlock(Lockable& target) throw () : t(target) { t.unlock(); }
    ~Unlock() throw () { t.lock(); }
};

/** Lock a ConstLockable for the lifetime of the object.
 * Needed in place of regular Lock when the target is actually const.
 */
class ConstLock : private NoCopying {
    const ConstLockable& t;

public:
    ConstLock(const ConstLockable& target) throw () : t(target) { t.lock(); }
    ~ConstLock() throw () { t.unlock(); }
};

/** Unlock a ConstLockable for the lifetime of the object.
 * Needed in place of regular Unlock when the target is actually const.
 */
class ConstUnlock : private NoCopying {
    const ConstLockable& t;

public:
    ConstUnlock(const ConstLockable& target) throw () : t(target) { t.unlock(); }
    ~ConstUnlock() throw () { t.lock(); }
};

/** Reference to an object that may be trashed by the recipient.
 * Provides a means to indicate in code that a function will manipulate its reference argument
 * without guaranteeing anything to the caller.
 * Intended to be constructed using trashable_ref().
 * To use members of the wrapped reference, use either trt->member; or { T& t = trt; t.member; }
 */
template<class T> class TrashableRef {
    T& t;

    TrashableRef(T& t_) throw () : t(t_) { }

public:
    static TrashableRef construct(T& t) throw () { return t; }

    operator       T&()       throw () { return t; }
    operator const T&() const throw () { return t; }

          T* operator->()       throw () { return &t; }
    const T* operator->() const throw () { return &t; }
};

template<class T> TrashableRef<T> trashable_ref(T& t) throw () { return TrashableRef<T>::construct(t); }

/** Pointer to an object whose destruction is to be taken care of by the recipient.
 * Provides a means to indicate in code that a function will assume control over the destruction of its pointer argument.
 * Intended to be constructed using give_control().
 */
template<class T> class ControlledPtr {
    T* p;

    ControlledPtr(T* p_) throw () : p(p_) { }

public:
    static ControlledPtr construct(T* p) throw () { return p; }

    operator       T*()       throw () { return p; }
    operator const T*() const throw () { return p; }

          T* operator->()       throw () { return p; }
    const T* operator->() const throw () { return p; }

          T& operator*()       throw () { return *p; }
    const T& operator*() const throw () { return *p; }
};

template<class T> ControlledPtr<T> give_control(T* p) throw () { return ControlledPtr<T>::construct(p); }

template<class T> T bound(T val, T lb, T hb) throw () { return val <= lb ? lb : val >= hb ? hb : val; }

int atoi(const std::string& str) throw ();
std::string itoa(int val) throw ();
std::string itoa_w(int val, int width, bool left = false) throw ();
std::string fcvt(double val) throw ();
std::string fcvt(double val, int precision) throw ();
int iround(double value) throw ();
int iround_bound(double value) throw (); // if value is out of int range, nearest value is used
int numberWidth(int num) throw ();   // how many characters num takes when printed

inline double sqr(double value) throw () { // the square of the given value (just to keep the code readable)
    return value * value;
}

inline int sqr(int value) throw () {
    return value * value;
}

template<class Int1T, class Int2T> Int2T positiveModulo(Int1T val, Int2T modulus) throw () {
    nAssert(modulus > 0);
    return val >= 0 ? val % modulus : modulus - (-val % modulus);
}

double positiveFmod(double val, double modulus) throw ();

template<class UnsignedIntT> UnsignedIntT rotateRight(UnsignedIntT val, int bits) throw () {
    nAssert(std::numeric_limits<UnsignedIntT>::is_integer && !std::numeric_limits<UnsignedIntT>::is_signed);
    const unsigned typew = sizeof(UnsignedIntT) * CHAR_BIT;
    bits = positiveModulo(bits, typew);
    return (val >> bits) | (val << (typew - bits));
}

template<class UnsignedIntT> UnsignedIntT rotateLeft(UnsignedIntT val, int bits) throw () { rotateRight(val, -bits); }

/// Returns the current time in the standard format.
std::string date_and_time() throw ();

/// Get a verbal approximation of the given time interval
std::string approxTime(int seconds) throw ();

/// UTF-8 mode for Linux
extern bool utf8_mode;

/// Check if the command line is UTF-8
void check_utf8_mode() throw ();

/// Convert string to uppercase.
std::string toupper(std::string str) throw ();

/// Convert string to lowercase.
std::string tolower(std::string str) throw ();

/// Convert Latin 1 character to upper case.
unsigned char latin1_toupper(unsigned char c) throw ();

/// Convert Latin 1 character to lower case.
unsigned char latin1_tolower(unsigned char c) throw ();

/// Convert character from Latin 1 encoding to UTF-8 encoding
std::string latin1_to_utf8(unsigned char c) throw ();

/// Convert string from Latin 1 encoding to UTF-8 encoding
std::string latin1_to_utf8(const std::string& str) throw ();

/// Convert string from UTF-8 encoding to Latin 1 encoding
std::string utf8_to_latin1(const std::string& str) throw ();

/// Case insensitive Latin 1 encoded string comparison.
bool cmp_case_ins(const std::string& a, const std::string& b) throw ();

/// Strip beginning and trailing whitespaces.
std::string trim(std::string str) throw ();

/// Replace all occurences of /s1/ with /s2/ in /text/.
std::string replace_all(std::string text, const std::string& s1, const std::string& s2) throw ();

/// Replace all occurences of /s1/ with /s2/ in /text/, modifying /text/.
std::string& replace_all_in_place(std::string& text, const std::string& s1, const std::string& s2) throw ();

/// Replace all occurences of /c1/ with /c2/ in /text/, modifying /text/.
std::string& replace_all_in_place(std::string& text, char c1, char c2) throw ();

/// Replace characters &<>"' with HTML entities or character references.
std::string escape_for_html(std::string text) throw ();

/// Pad /text/ with /pad/ from the given side until its length is /size/ characters. Do nothing if length >= /size/.
std::string pad_to_size_left (std::string text, int size, char pad = ' ') throw ();

/// Pad /text/ with /pad/ from the given side until its length is /size/ characters. Do nothing if length >= /size/.
std::string pad_to_size_right(std::string text, int size, char pad = ' ') throw ();

bool find_nonprintable_char(const std::string& str) throw ();
bool is_nonprintable_char(unsigned char c) throw ();

/// Replace control characters with their C escape sequences (note: for readability, it doesn't convert \ to \\ so the result might be ambiguous)
std::string formatForLogging(const std::string& str) throw ();

/// Split string to lines, but only at whitespaces.
std::vector<std::string> split_to_lines(const std::string& source, int lineLength, int indent = 0, bool keep_spaces = false) throw ();

/// strspnp: (Watcom definition) find from str the first char not in charset
char* strspnp(char* str, const char* charset) throw ();

/// strspnp: (Watcom definition) find from str the first char not in charset
const char* strspnp(const char* str, const char* charset) throw ();

class LineReceiver {
protected:
    LineReceiver() throw () { }
    virtual ~LineReceiver() throw () { }

public:
    virtual LineReceiver& operator()(const std::string& str) throw () = 0;
};

class Log;
class LogSet : public LineReceiver {
    // the objects aren't owned by this class
    Log* normalLog;
    Log* errorLog;
    Log* securityLog;

public:
    LogSet(Log* normal, Log* error, Log* security) throw () : normalLog(normal), errorLog(error), securityLog(security) { }  // null pointers are allowed
    ~LogSet() throw () { }

    LogSet& operator()(const std::string&) throw ();
    LogSet& operator()(const char* fmt, ...) throw () PRINTF_FORMAT(2, 3);
    LogSet& error(const std::string&) throw ();
    LogSet& security(const char* fmt, ...) throw () PRINTF_FORMAT(2, 3);

    Log* accessNormal() throw () { return normalLog; }
    Log* accessError() throw () { return errorLog; }
    Log* accessSecurity() throw () { return securityLog; }
};

extern bool g_allowBlockingMessages;    // controls all messageBox calls; disable to suppress all external message boxes (assertions included)

void messageBox(const std::string& heading, const std::string& msg, bool blocking = true) throw (); // blocking may not be controllable

void criticalError(const std::string& msg) throw () __attribute__ ((noreturn));

class MemoryLog;
void errorMessage(const std::string& heading, MemoryLog& errorLog, const std::string& footer) throw ();

void rotate_angle(double& angle, double shift) throw ();

class FileName {
public:
    FileName(const std::string& fullName) throw ();

    const std::string& getPath() const throw () { return path; } // without trailing separator
    const std::string& getBaseName() const throw () { return base; }
    const std::string& getExtension() const throw () { return ext; } // with '.'
    std::string getFull() const throw ();

private:
    std::string path, base, ext;
};

class CartesianProductIterator {
    const int N1, N2;
    int val1, val2;

public:
    CartesianProductIterator(int n1, int n2) throw () : N1(n1), N2(n1 ? n2 : 0), val1(n1 - 1), val2(-1) { }
    bool next() throw () {
        if (++val1 == N1) {
            if (++val2 == N2)
                return false;
            val1 = 0;
        }
        return true;
    }
    std::pair<int, int> operator()() const throw () { return std::pair<int, int>(val1, val2); } // only valid after next() returning true
    int i1() const throw () { return val1; }
    int i2() const throw () { return val2; }
};

#endif
