// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_STRONG_TYPEDEF_HPP_INCLUDED
#define TYPE_SAFE_STRONG_TYPEDEF_HPP_INCLUDED

#include <iosfwd>
#include <iterator>
#include <type_traits>
#include <utility>

#include <type_safe/detail/all_of.hpp>
#include <type_safe/config.hpp>

namespace type_safe
{
    /// A strong typedef emulation.
    ///
    /// Unlike regular typedefs, this does create a new type and only allows explicit conversion from the underlying one.
    /// The `Tag` is used to differentiate between different strong typedefs to the same type.
    /// It is designed to be used as a base class and does not provide any operations by itself.
    /// Use the types in the `strong_typedef_op` namespace to generate operations
    /// and/or your own member functions.
    ///
    /// Example:
    /// ```cpp
    /// struct my_handle
    /// : strong_typedef<my_handle, void*>,
    ///   strong_typedef_op::equality_comparision<my_handle>
    /// {
    ///     using strong_typedef::strong_typedef;
    /// };
    ///
    /// struct my_int
    /// : strong_typedef<my_int, int>,
    ///   strong_typedef_op::integer_arithmetic<my_int>,
    ///   strong_typedef_op::equality_comparision<my_int>,
    ///   strong_typedef_op::relational_comparision<my_int>
    /// {
    ///     using strong_typedef::strong_typedef;
    /// };
    /// ```
    template <class Tag, typename T>
    class strong_typedef
    {
    public:
        /// \effects Value initializes the underlying value.
        constexpr strong_typedef() : value_()
        {
        }

        /// \effects Copy (1)/moves (2) the underlying value.
        /// \group value_ctor
        explicit constexpr strong_typedef(const T& value) : value_(value)
        {
        }

        /// \group value_ctor
        explicit constexpr strong_typedef(T&& value) noexcept(
            std::is_nothrow_move_constructible<T>::value)
        : value_(static_cast<T&&>(value)) // std::move() might not be constexpr
        {
        }

        /// \returns A reference to the stored underlying value.
        /// \group value_conv
        explicit TYPE_SAFE_CONSTEXPR14 operator T&() TYPE_SAFE_LVALUE_REF noexcept
        {
            return value_;
        }

        /// \group value_conv
        explicit constexpr operator const T&() const TYPE_SAFE_LVALUE_REF noexcept
        {
            return value_;
        }

#if TYPE_SAFE_USE_REF_QUALIFIERS
        /// \group value_conv
        explicit TYPE_SAFE_CONSTEXPR14 operator T &&() && noexcept
        {
            return std::move(value_);
        }

        /// \group value_conv
        explicit constexpr operator const T &&() const && noexcept
        {
            return std::move(value_);
        }
#endif

        friend void swap(strong_typedef& a, strong_typedef& b) noexcept
        {
            using std::swap;
            swap(static_cast<T&>(a), static_cast<T&>(b));
        }

    private:
        T value_;
    };

    /// \exclude
    namespace detail
    {
        template <class Tag, typename T>
        T underlying_type(strong_typedef<Tag, T>);
    } // namespace detail

    /// The underlying type of the [ts::strong_typedef]().
    /// \exclude target
    template <class StrongTypedef>
    using underlying_type =
        decltype(detail::underlying_type(std::declval<typename std::decay<StrongTypedef>::type>()));

    /// Accesses the underlying value.
    /// \returns A reference to the underlying value.
    /// \group strong_typedef_get
    template <class Tag, typename T>
    TYPE_SAFE_CONSTEXPR14 T& get(strong_typedef<Tag, T>& type) noexcept
    {
        return static_cast<T&>(type);
    }

    /// \group strong_typedef_get
    template <class Tag, typename T>
    constexpr const T& get(const strong_typedef<Tag, T>& type) noexcept
    {
        return static_cast<const T&>(type);
    }

    /// \group strong_typedef_get
    template <class Tag, typename T>
    TYPE_SAFE_CONSTEXPR14 T&& get(strong_typedef<Tag, T>&& type) noexcept
    {
        return static_cast<T&&>(static_cast<T&>(type));
    }

    /// \group strong_typedef_get
    template <class Tag, typename T>
    constexpr const T&& get(const strong_typedef<Tag, T>&& type) noexcept
    {
        return static_cast<const T&&>(static_cast<const T&>(type));
    }

    /// Some operations for [ts::strong_typedef]().
    ///
    /// They all generate operators forwarding to the underlying type,
    /// inherit from then in the typedef definition.
    namespace strong_typedef_op
    {
        /// \exclude
        namespace detail
        {
            template <typename From, typename To>
            using enable_if_convertible = typename std::
                enable_if<!std::is_same<typename std::decay<From>::type, To>::value
                          && !std::is_base_of<To, typename std::decay<From>::type>::value
                          && std::is_convertible<typename std::decay<From>::type, To>::value>::type;

            template <typename From, typename To>
            using enable_if_convertible_same = typename std::
                enable_if<std::is_convertible<typename std::decay<From>::type, To>::value>::type;

            template <class StrongTypedef>
            constexpr const underlying_type<StrongTypedef>& get_underlying(
                const StrongTypedef& type)
            {
                return get(type);
            }

            template <class StrongTypedef>
            TYPE_SAFE_CONSTEXPR14 underlying_type<StrongTypedef>&& get_underlying(
                StrongTypedef&& type)
            {
                return get(static_cast<StrongTypedef&&>(type));
            }

            // ensure constexpr
            template <class T>
            constexpr T&& forward(typename std::remove_reference<T>::type& t) noexcept
            {
                return static_cast<T&&>(t);
            }

            template <class T>
            constexpr T&& forward(typename std::remove_reference<T>::type&& t) noexcept
            {
                static_assert(!std::is_lvalue_reference<T>::value,
                              "Can not forward an rvalue as an lvalue.");
                return static_cast<T&&>(t);
            }
        } // namespace detail

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op, Name, Result)                                                 \
    /** \exclude */                                                                                \
    template <class StrongTypedef>                                                                 \
    constexpr Result operator Op(const Name<StrongTypedef>& lhs, const Name<StrongTypedef>& rhs)   \
    {                                                                                              \
        return Result(                                                                             \
            detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(lhs))          \
                Op detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(rhs))); \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <class StrongTypedef>                                                                 \
    constexpr Result operator Op(Name<StrongTypedef>&& lhs, const Name<StrongTypedef>& rhs)        \
    {                                                                                              \
        return Result(                                                                             \
            detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&>(lhs))                \
                Op detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(rhs))); \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <class StrongTypedef>                                                                 \
    constexpr Result operator Op(const Name<StrongTypedef>& lhs, Name<StrongTypedef>&& rhs)        \
    {                                                                                              \
        return Result(detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(     \
            lhs)) Op detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(rhs)));    \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <class StrongTypedef>                                                                 \
    constexpr Result operator Op(Name<StrongTypedef>&& lhs, Name<StrongTypedef>&& rhs)             \
    {                                                                                              \
        return Result(detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&>(           \
            lhs)) Op detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(rhs)));    \
    }                                                                                              \
    /* mixed */                                                                                    \
    /** \exclude */                                                                                \
    template <class StrongTypedef, typename Other,                                                 \
              typename = detail::enable_if_convertible<Other&&, StrongTypedef>>                    \
    constexpr Result operator Op(const Name<StrongTypedef>& lhs, Other&& rhs)                      \
    {                                                                                              \
        return Result(detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(     \
            lhs)) Op detail::get_underlying<StrongTypedef>(detail::forward<Other>(rhs)));          \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <class StrongTypedef, typename Other,                                                 \
              typename = detail::enable_if_convertible<Other&&, StrongTypedef>>                    \
    constexpr Result operator Op(Name<StrongTypedef>&& lhs, Other&& rhs)                           \
    {                                                                                              \
        return Result(detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(lhs))     \
                          Op detail::get_underlying<StrongTypedef>(detail::forward<Other>(rhs)));  \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <class StrongTypedef, typename Other,                                                 \
              typename = detail::enable_if_convertible<Other&&, StrongTypedef>>                    \
    constexpr Result operator Op(Other&& lhs, const Name<StrongTypedef>& rhs)                      \
    {                                                                                              \
        return Result(                                                                             \
            detail::get_underlying<StrongTypedef>(detail::forward<Other>(lhs))                     \
                Op detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(rhs))); \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <class StrongTypedef, typename Other,                                                 \
              typename = detail::enable_if_convertible<Other&&, StrongTypedef>>                    \
    constexpr Result operator Op(Other&& lhs, Name<StrongTypedef>&& rhs)                           \
    {                                                                                              \
        return Result(detail::get_underlying<StrongTypedef>(detail::forward<Other>(                \
            lhs)) Op detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(rhs)));    \
    }

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP_MIXED(Op, Name, Result)                                           \
    /** \exclude */                                                                                \
    template <class StrongTypedef, typename OtherArg, typename Other,                              \
              typename = detail::enable_if_convertible_same<Other&&, OtherArg>>                    \
    constexpr Result operator Op(const Name<StrongTypedef, OtherArg>& lhs, Other&& rhs)            \
    {                                                                                              \
        return Result(get(static_cast<const StrongTypedef&>(lhs)) Op detail::forward<Other>(rhs)); \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <class StrongTypedef, typename OtherArg, typename Other,                              \
              typename = detail::enable_if_convertible_same<Other&&, OtherArg>>                    \
    constexpr Result operator Op(Name<StrongTypedef, OtherArg>&& lhs, Other&& rhs)                 \
    {                                                                                              \
        return Result(get(static_cast<StrongTypedef&&>(lhs)) Op detail::forward<Other>(rhs));      \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <class StrongTypedef, typename OtherArg, typename Other,                              \
              typename = detail::enable_if_convertible_same<Other&&, OtherArg>>                    \
    constexpr Result operator Op(Other&& lhs, const Name<StrongTypedef, OtherArg>& rhs)            \
    {                                                                                              \
        return Result(detail::forward<Other>(lhs) Op get(static_cast<const StrongTypedef&>(rhs))); \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <class StrongTypedef, typename OtherArg, typename Other,                              \
              typename = detail::enable_if_convertible_same<Other&&, OtherArg>>                    \
    constexpr Result operator Op(Other&& lhs, Name<StrongTypedef, OtherArg>&& rhs)                 \
    {                                                                                              \
        return Result(detail::forward<Other>(lhs) Op get(static_cast<StrongTypedef&&>(rhs)));      \
    }

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND(Op, Other)                                               \
    /** \exclude */                                                                                \
    friend StrongTypedef& operator Op(StrongTypedef& lhs, const Other& rhs)                        \
    {                                                                                              \
        using type = underlying_type<StrongTypedef>;                                               \
        static_cast<type&>(lhs) Op static_cast<const type&>(rhs);                                  \
        return lhs;                                                                                \
    }                                                                                              \
    /** \exclude */                                                                                \
    friend StrongTypedef& operator Op(StrongTypedef& lhs, Other&& rhs)                             \
    {                                                                                              \
        using type = underlying_type<StrongTypedef>;                                               \
        static_cast<type&>(lhs) Op std::move(static_cast<type&>(rhs));                             \
        return lhs;                                                                                \
    }                                                                                              \
    /** \exclude */                                                                                \
    friend StrongTypedef&& operator Op(StrongTypedef&& lhs, const Other& rhs)                      \
    {                                                                                              \
        using type = underlying_type<StrongTypedef>;                                               \
        std::move(static_cast<type&>(lhs)) Op static_cast<const type&>(rhs);                       \
        return std::move(lhs);                                                                     \
    }                                                                                              \
    /** \exclude */                                                                                \
    friend StrongTypedef&& operator Op(StrongTypedef&& lhs, Other&& rhs)                           \
    {                                                                                              \
        using type = underlying_type<StrongTypedef>;                                               \
        std::move(static_cast<type&>(lhs)) Op std::move(static_cast<type&>(rhs));                  \
        return std::move(lhs);                                                                     \
    }

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(Name, Op)                                          \
    template <class StrongTypedef>                                                                 \
    struct Name                                                                                    \
    {                                                                                              \
        TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND(Op## =, StrongTypedef)                                   \
    };                                                                                             \
    TYPE_SAFE_DETAIL_MAKE_OP(Op, Name, StrongTypedef)                                              \
    template <class StrongTypedef, typename Other>                                                 \
    struct mixed_##Name                                                                            \
    {                                                                                              \
        TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND(Op## =, Other)                                           \
    };                                                                                             \
    TYPE_SAFE_DETAIL_MAKE_OP_MIXED(Op, mixed_##Name, StrongTypedef)

        template <class StrongTypedef>
        struct equality_comparison
        {
        };
        TYPE_SAFE_DETAIL_MAKE_OP(==, equality_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP(!=, equality_comparison, bool)

        template <class StrongTypedef, typename Other>
        struct mixed_equality_comparison
        {
        };
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(==, mixed_equality_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(!=, mixed_equality_comparison, bool)

        template <class StrongTypedef>
        struct relational_comparison
        {
        };
        TYPE_SAFE_DETAIL_MAKE_OP(<, relational_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP(<=, relational_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP(>, relational_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP(>=, relational_comparison, bool)

        template <class StrongTypedef, typename Other>
        struct mixed_relational_comparison
        {
        };
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(<, mixed_relational_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(<=, mixed_relational_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(>, mixed_relational_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(>=, mixed_relational_comparison, bool)

        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(addition, +)
        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(subtraction, -)
        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(multiplication, *)
        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(division, /)
        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(modulo, %)

        template <class StrongTypedef>
        struct increment
        {
            /// \exclude
            StrongTypedef& operator++()
            {
                using type = underlying_type<StrongTypedef>;
                ++static_cast<type&>(static_cast<StrongTypedef&>(*this));
                return static_cast<StrongTypedef&>(*this);
            }

            /// \exclude
            StrongTypedef operator++(int)
            {
                auto result = static_cast<StrongTypedef&>(*this);
                ++*this;
                return result;
            }
        };

        template <class StrongTypedef>
        struct decrement
        {
            /// \exclude
            StrongTypedef& operator--()
            {
                using type = underlying_type<StrongTypedef>;
                --static_cast<type&>(static_cast<StrongTypedef&>(*this));
                return static_cast<StrongTypedef&>(*this);
            }

            /// \exclude
            StrongTypedef operator--(int)
            {
                auto result = static_cast<StrongTypedef&>(*this);
                --*this;
                return result;
            }
        };

        template <class StrongTypedef>
        struct unary_plus
        {
        };

        /// \exclude
        template <class StrongTypedef>
        constexpr StrongTypedef operator+(const unary_plus<StrongTypedef>& lhs)
        {
            return StrongTypedef(+get(static_cast<const StrongTypedef&>(lhs)));
        }
        /// \exclude
        template <class StrongTypedef>
        constexpr StrongTypedef operator+(unary_plus<StrongTypedef>&& lhs)
        {
            return StrongTypedef(+get(static_cast<StrongTypedef&&>(lhs)));
        }

        template <class StrongTypedef>
        struct unary_minus
        {
        };

        /// \exclude
        template <class StrongTypedef>
        constexpr StrongTypedef operator-(const unary_minus<StrongTypedef>& lhs)
        {
            return StrongTypedef(-get(static_cast<const StrongTypedef&>(lhs)));
        }
        /// \exclude
        template <class StrongTypedef>
        constexpr StrongTypedef operator-(unary_minus<StrongTypedef>&& lhs)
        {
            return StrongTypedef(-get(static_cast<StrongTypedef&&>(lhs)));
        }

        template <class StrongTypedef>
        struct integer_arithmetic : unary_plus<StrongTypedef>,
                                    unary_minus<StrongTypedef>,
                                    addition<StrongTypedef>,
                                    subtraction<StrongTypedef>,
                                    multiplication<StrongTypedef>,
                                    division<StrongTypedef>,
                                    modulo<StrongTypedef>,
                                    increment<StrongTypedef>,
                                    decrement<StrongTypedef>
        {
        };

        template <class StrongTypedef>
        struct floating_point_arithmetic : unary_plus<StrongTypedef>,
                                           unary_minus<StrongTypedef>,
                                           addition<StrongTypedef>,
                                           subtraction<StrongTypedef>,
                                           multiplication<StrongTypedef>,
                                           division<StrongTypedef>
        {
        };

        template <class StrongTypedef>
        struct complement
        {
        };

        /// \exclude
        template <class StrongTypedef>
        constexpr StrongTypedef operator~(const complement<StrongTypedef>& lhs)
        {
            return StrongTypedef(~get(static_cast<const StrongTypedef&>(lhs)));
        }
        /// \exclude
        template <class StrongTypedef>
        constexpr StrongTypedef operator~(complement<StrongTypedef>&& lhs)
        {
            return StrongTypedef(~get(static_cast<StrongTypedef&&>(lhs)));
        }

        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(bitwise_or, |)
        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(bitwise_xor, ^)
        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(bitwise_and, &)

        template <class StrongTypedef>
        struct bitmask : complement<StrongTypedef>,
                         bitwise_or<StrongTypedef>,
                         bitwise_xor<StrongTypedef>,
                         bitwise_and<StrongTypedef>
        {
        };

        template <class StrongTypedef, typename IntT>
        struct bitshift
        {
            TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND(<<=, IntT)
            TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND(>>=, IntT)
        };
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(<<, bitshift, StrongTypedef)
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(>>, bitshift, StrongTypedef)

        template <class StrongTypedef, typename Result, typename ResultPtr = Result*,
                  typename ResultConstPtr = const Result*>
        struct dereference
        {
            /// \exclude
            Result& operator*()
            {
                using type = underlying_type<StrongTypedef>;
                return *static_cast<type&>(static_cast<StrongTypedef&>(*this));
            }

            /// \exclude
            const Result& operator*() const
            {
                using type = underlying_type<StrongTypedef>;
                return *static_cast<const type&>(static_cast<const StrongTypedef&>(*this));
            }

            /// \exclude
            ResultPtr operator->()
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<type&>(static_cast<StrongTypedef&>(*this));
            }

            /// \exclude
            ResultConstPtr operator->() const
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(static_cast<const StrongTypedef&>(*this));
            }
        };

        template <class StrongTypedef, typename Result, typename Index = std::size_t>
        struct array_subscript
        {
            /// \exclude
            Result& operator[](const Index& i)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<type&>(static_cast<StrongTypedef&>(*this))[i];
            }

            /// \exclude
            const Result& operator[](const Index& i) const
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(static_cast<const StrongTypedef&>(*this))[i];
            }
        };

        template <class StrongTypedef, class Category, typename T,
                  typename Distance = std::ptrdiff_t>
        struct iterator : dereference<StrongTypedef, T, T*, const T*>, increment<StrongTypedef>
        {
            using iterator_category = Category;
            using value_type        = T;
            using distance_type     = Distance;
            using pointer           = T*;
            using reference         = T&;
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct input_iterator : iterator<StrongTypedef, std::input_iterator_tag, T, Distance>,
                                equality_comparison<StrongTypedef>
        {
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct output_iterator : iterator<StrongTypedef, std::output_iterator_tag, T, Distance>
        {
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct forward_iterator : input_iterator<StrongTypedef, T, Distance>
        {
            using iterator_category = std::forward_iterator_tag;
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct bidirectional_iterator : forward_iterator<StrongTypedef, T, Distance>,
                                        decrement<StrongTypedef>
        {
            using iterator_category = std::bidirectional_iterator_tag;
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct random_access_iterator : bidirectional_iterator<StrongTypedef, T, Distance>,
                                        array_subscript<StrongTypedef, T, Distance>,
                                        relational_comparison<StrongTypedef>
        {
            using iterator_category = std::random_access_iterator_tag;

            /// \exclude
            StrongTypedef& operator+=(const Distance& d)
            {
                using type = underlying_type<StrongTypedef>;
                static_cast<type&>(static_cast<StrongTypedef&>(*this)) += d;
                return static_cast<StrongTypedef&>(*this);
            }

            /// \exclude
            StrongTypedef& operator-=(const Distance& d)
            {
                using type = underlying_type<StrongTypedef>;
                static_cast<type&>(static_cast<StrongTypedef&>(*this)) -= d;
                return static_cast<StrongTypedef&>(*this);
            }

            /// \exclude
            friend StrongTypedef operator+(const StrongTypedef& iter, const Distance& n)
            {
                using type = underlying_type<StrongTypedef>;
                return StrongTypedef(static_cast<const type&>(iter) + n);
            }

            /// \exclude
            friend StrongTypedef operator+(const Distance& n, const StrongTypedef& iter)
            {
                return iter + n;
            }

            /// \exclude
            friend StrongTypedef operator-(const StrongTypedef& iter, const Distance& n)
            {
                using type = underlying_type<StrongTypedef>;
                return StrongTypedef(static_cast<const type&>(iter) - n);
            }

            /// \exclude
            friend Distance operator-(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(lhs) - static_cast<const type&>(rhs);
            }
        };

        template <class StrongTypedef>
        struct input_operator
        {
            /// \exclude
            template <typename Char, class CharTraits>
            friend std::basic_istream<Char, CharTraits>& operator>>(
                std::basic_istream<Char, CharTraits>& in, StrongTypedef& val)
            {
                using type = underlying_type<StrongTypedef>;
                return in >> static_cast<type&>(val);
            }
        };

        template <class StrongTypedef>
        struct output_operator
        {
            /// \exclude
            template <typename Char, class CharTraits>
            friend std::basic_ostream<Char, CharTraits>& operator<<(
                std::basic_ostream<Char, CharTraits>& out, const StrongTypedef& val)
            {
                using type = underlying_type<StrongTypedef>;
                return out << static_cast<const type&>(val);
            }
        };

#undef TYPE_SAFE_DETAIL_MAKE_OP
#undef TYPE_SAFE_DETAIL_MAKE_OP_MIXED
#undef TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND
#undef TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP
    } // namespace strong_typedef_op
} // namespace type_safe

#endif // TYPE_SAFE_STRONG_TYPEDEF_HPP_INCLUDED
