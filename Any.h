#ifndef __ANY
#define __ANY

#include "constants.h"

#include <string.h>

class BaseHolder
{
public:
    BaseHolder() {}
    virtual ~BaseHolder() {}
    virtual BaseHolder *clone() = 0;
};

template <typename T>
class Holder : public BaseHolder
{
public:
    Holder(const T &t)
        : _value(t)
    {
    }
    ~Holder() {}
    BaseHolder *clone() override
    {
        return new Holder<T>(_value);
    }

public:
    T _value;
};

class Any
{
public:
    template <typename ValueType>
    Any(const ValueType &value)
    {
        _pValue = new Holder<ValueType>(value);
    }

    Any() :_pValue(nullptr) {}

    Any(const Any &any) {
        if (any._pValue)
            _pValue = any._pValue->clone();
        else 
            _pValue = 0;
    }

    ~Any() {
        if (_pValue)
            delete _pValue;
    }

    Any& operator=(const Any& any) {
        _pValue = nullptr;
        if (any._pValue) {
            _pValue = any._pValue->clone();
        }
        return *this;
    }

    template <typename ValueType>
    Any &operator=(const ValueType &value)
    {
        Any tmp(value);
        if (_pValue)
            delete _pValue;
        _pValue = tmp._pValue;
        tmp._pValue = nullptr;
        return *this;
    }

    template <typename ValueType>
    ValueType *anyCast() const
    {
        Holder<ValueType> *p = dynamic_cast<Holder<ValueType> *>(_pValue);
        return p ? &p->_value : nullptr;
    }

    template <typename ValueType>
    ValueType &anyRefCast()
    {
        return (dynamic_cast<Holder<ValueType> &>(*_pValue))._value;
    }

    bool operator<(const Any &b) const
    {
        if (isNull() || b.isNull()) {
            if (isNull() && b.isNull()) return false;
            if (isNull() && !b.isNull()) return true;
            if (!isNull() && b.isNull()) return false;
        } else if (b.anyCast<char*>() != nullptr) {
            return *this < (const char*)*b.anyCast<char*>();
        } else if (b.anyCast<int>() != nullptr) {
            return *this < *b.anyCast<int>();
        } else if (b.anyCast<long double>() != nullptr) {
            return *this < *b.anyCast<long double>();
        } else if (b.anyCast<uint32_t>() != nullptr) {
            return *this < *b.anyCast<uint32_t>();
        }
        return false;
    }

    bool operator>(const Any &b) const
    {
        if (isNull() || b.isNull()) {
            if (isNull() && b.isNull()) return false;
            if (isNull() && !b.isNull()) return false;
            if (!isNull() && b.isNull()) return true;
        } else if (b.anyCast<char*>() != nullptr) {
            return *this > (const char*)*b.anyCast<char*>();
        } else if (b.anyCast<int>() != nullptr) {
            return *this > *b.anyCast<int>();
        } else if (b.anyCast<long double>() != nullptr) {
            return *this > *b.anyCast<long double>();
        } else if (b.anyCast<uint32_t>() != nullptr) {
            return *this > *b.anyCast<uint32_t>();
        }
        return false;
    }

    bool operator==(const Any &b) const
    {
        if (isNull() || b.isNull()) {
            if (isNull() && b.isNull()) return true;
            if (isNull() && !b.isNull()) return false;
            if (!isNull() && b.isNull()) return false;
        } else if (b.anyCast<char*>() != nullptr) {
            return *this == (const char*)*b.anyCast<char*>();
        } else if (b.anyCast<int>() != nullptr) {
            return *this == *b.anyCast<int>();
        } else if (b.anyCast<long double>() != nullptr) {
            return *this == *b.anyCast<long double>();
        } else if (b.anyCast<uint32_t>() != nullptr) {
            return *this == *b.anyCast<uint32_t>();
        }
        return false;
    }

    bool operator<=(const Any &b) const { return !(*this > b); }

    bool operator>=(const Any &b) const { return !(*this < b); }

    bool operator!=(const Any &b) const { return !(*this == b); }

    bool operator<(const char* value) const
    {
        if (isNull()) {
            return true; 
        } else if (this->anyCast<char*>() != nullptr) {
            return strcmp(*this->anyCast<char*>(), value) < 0;
        }
        return false;
    }

    bool operator>(const char* value) const
    {
        if (isNull()) {
            return false; 
        } else if (this->anyCast<char*>() != nullptr) {
            return strcmp(*this->anyCast<char*>(), value) > 0;
        }
        return false;
    }

    bool operator==(const char* value) const
    {
        if (isNull()) {
            return false; 
        } else if (this->anyCast<char*>() != nullptr) {
            return strcmp(*this->anyCast<char*>(), value) == 0;
        }
        return false;
    }

    bool operator<=(const char* value) const { return !(*this > value); }

    bool operator>=(const char* value) const { return !(*this < value); }

    bool operator!=(const char* value) const { return !(*this == value); }

    template <typename ValueType>
    bool operator<(const ValueType &value) const
    {
        if (isNull()) {
            return true; 
        } else if (this->anyCast<ValueType>() != nullptr) {
            return *this->anyCast<ValueType>() < value;
        }
        return false;
    }

    template <typename ValueType>
    bool operator>(const ValueType &value) const
    {
        if (isNull()) {
            return false; 
        } else if (this->anyCast<ValueType>() != nullptr) {
            return *this->anyCast<ValueType>() > value;
        }
        return false;
    }

    template <typename ValueType>
    bool operator==(const ValueType &value) const
    {
        if (isNull()) {
            return false; 
        } else if (this->anyCast<ValueType>() != nullptr) {
            return *this->anyCast<ValueType>() == value;
        }
        return false;
    }

    template <typename ValueType>
    bool operator<=(const ValueType &value) const { return !(*this > value); }

    template <typename ValueType>
    bool operator>=(const ValueType &value) const { return !(*this < value); }

    template <typename ValueType>
    bool operator!=(const ValueType &value) const { return !(*this == value); }

    bool isNull() const { return _pValue == nullptr; }

private:
    BaseHolder *_pValue;
};

#endif