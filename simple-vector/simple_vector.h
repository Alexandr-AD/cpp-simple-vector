#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <initializer_list>

#include "array_ptr.h"

class ReserveProxyObj
{
public:
    ReserveProxyObj(size_t capacity_to_reserve) : capacity_(capacity_to_reserve)
    {
    }

    size_t capacity_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve)
{
    return ReserveProxyObj(capacity_to_reserve);
};

template <typename Type>
class SimpleVector
{
public:
    using Iterator = Type *;
    using ConstIterator = const Type *;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : SimpleVector(size, Type{})
    {
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type &value) : size_(size), capacity_(size)
    {
        if (size)
        {
            ArrayPtr<Type> tmp(size);
            items_.swap(tmp);
            std::fill(begin(), end(), value);
        }
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) : SimpleVector(init.size())
    {
        std::copy(init.begin(), init.end(), begin());
    }

    SimpleVector(const SimpleVector &other) : SimpleVector(other.capacity_) //, size_(other.size_)
    {
        std::copy(other.begin(), other.end(), items_.Get());
        size_ = other.size_;
    }

    SimpleVector(SimpleVector &&other) : items_(std::move(other.items_))
    {
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
    }

    SimpleVector(ReserveProxyObj reserve_obj)
    {
        this->Reserve(reserve_obj.capacity_);
    }

    SimpleVector &operator=(const SimpleVector &rhs)
    {
        if (rhs == *this)
        {
            return *this;
        }

        SimpleVector tmp(rhs);
        this->swap(tmp);

        return *this;
    }

    SimpleVector &operator=(SimpleVector &&rhs)
    {
        if (rhs == *this)
        {
            return *this;
        }

        items_ = std::move(rhs.items_);
        this->size_ = std::exchange(rhs.size_, 0);
        this->capacity_ = std::exchange(rhs.capacity_, 0);

        return *this;
    }

    void Reserve(size_t new_capacity)
    {
        if (new_capacity > capacity_)
        {
            ArrayPtr<Type> arr_rmp(new_capacity);
            std::move(begin(), end(), arr_rmp.Get());

            items_.swap(arr_rmp);
            capacity_ = new_capacity;
        }
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type &item)
    {
        if (size_ < capacity_)
        {
            items_[size_] = item;
            ++size_;
        }
        else
        {
            Resize(size_ + 1);
            items_[size_ - 1] = item;
            // не очень понял, как это исправить, что делать в такой ситуации? 
            // возвращать копирование в resize? специально от него избавлялись
            // и меняли на move, чтобы работать с классами, в которых запрещено копирование
        }
    }

    void PushBack(Type &&item)
    {
        if (size_ < capacity_)
        {
            items_[size_] = std::move(item);
            ++size_;
        }
        else
        {
            Resize(size_ + 1);
            items_[size_ - 1] = std::move(item);
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type &value)
    {
        Iterator pos_tmp = const_cast<Iterator>(pos);
        auto dist = std::distance(begin(), pos_tmp);

        Resize(size_ + 1);
        pos_tmp = begin() + dist;
        std::copy(pos_tmp, end() - 1, pos_tmp + 1);
        *pos_tmp = value;
        return pos_tmp;
    }

    Iterator Insert(ConstIterator pos, Type &&value)
    {
        Iterator pos_tmp = const_cast<Iterator>(pos);
        auto dist = std::distance(begin(), pos_tmp);

        Resize(size_ + 1);
        pos_tmp = begin() + dist;
        std::move(pos_tmp, end() - 1, pos_tmp + 1);
        *pos_tmp = std::move(value);
        return pos_tmp;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept
    {
        if (size_)
        {
            --size_;
        }
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos)
    {
        Iterator tmp_pos = const_cast<Iterator>(pos);

        std::move(tmp_pos + 1, end(), tmp_pos);

        --size_;
        return tmp_pos;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector &other) noexcept
    {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept
    {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept
    {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept
    {
        return !size_;
    }

    // Возвращает ссылку на элемент с индексом index
    Type &operator[](size_t index) noexcept
    {
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type &operator[](size_t index) const noexcept
    {
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type &At(size_t index)
    {
        if (index >= size_)
        {
            throw std::out_of_range("out of range");
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type &At(size_t index) const
    {
        if (index >= size_)
        {
            throw std::out_of_range("out of range");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept
    {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size)
    {
        if (new_size <= size_)
        {
            size_ = new_size;
        }
        else if (new_size <= capacity_)
        {
            for (auto iter = this->end(); iter != this->begin()+new_size; ++iter)
            {
                *iter = Type{};
            }
            size_ = new_size;
        }
        else if (new_size > capacity_)
        {
            capacity_ = std::max(new_size, capacity_ * 2);

            ArrayPtr<Type> tmp(capacity_);
            std::move(begin(), end(), tmp.Get());
            items_.swap(tmp);

            for (auto iter = items_.Get() + size_; iter != items_.Get() + capacity_; ++iter)
            {
                *iter = Type{};
            }

            size_ = new_size;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept
    {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept
    {
        return (items_.Get() + size_);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept
    {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept
    {
        return (items_.Get() + size_);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept
    {
        return begin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept
    {
        return end();
    }

private:
    ArrayPtr<Type> items_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs)
{
    return !(lhs != rhs);
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs)
{
    return (lhs > rhs || lhs < rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs)
{
    return !(lhs > rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs)
{
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs)
{
    return !(lhs < rhs);
}