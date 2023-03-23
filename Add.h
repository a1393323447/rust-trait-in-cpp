//
// Created by 朕与将军解战袍 on 2023/3/23.
//

#ifndef __TRAITS_DEMO_ADD_H__
#define __TRAITS_DEMO_ADD_H__

#include <concepts>
#include <functional>

// 定义 `structural trait` Add
// Add 除 `Self` 外还拥有两个类型参数:
// - Rhs: 右操作数类型
// - Out: 和的类型
template<typename Self, typename Rhs = Self, typename Out = Self>
struct Add {
    // 简单的构造函数
    Add(Self s);

    // Add trait 要求实现的函数
    auto add(Rhs rhs) -> Out;

    // 在 Add 中储存 Self 类型的对象
    // 用于实现类似与调用成员函数的语法
    Self s;
};

// 用于静态检查的 `concept`
template<typename Self, typename Rhs = Self, typename Out = Self>
concept AddTrait = requires(Add<Self, Rhs, Out> self, Rhs other) {
    { self.add(other) } -> std::same_as<Out>;
};

enum struct Dyn {};

template<typename Rhs, typename Out>
struct Add<Dyn, Rhs, Out> {

    template<typename Self>
    requires AddTrait<Self, Rhs, Out>
    Add(Self s):
    add([s = std::move(s)](Rhs rhs) -> Out {
        return Add<Self, Rhs, Out>(s).add(rhs);
    })
    {}

    const std::function<Out(Rhs)> add;
};

template<typename Rhs, typename Out>
using DynAdd = Add<Dyn, Rhs, Out>;

template<typename Self>
requires std::integral<Self> || std::floating_point<Self>
struct Add<Self> {
    using Out = Self;

    Add(Self s): self(s) {}

    auto add(Self other) -> Out {
        return self + other;
    }

    Self self;
};

// 为所有实现了 AddTrait 的 `Self` 都实现 operator+
template<typename Self, typename Rhs = Self, typename Out = Self>
requires AddTrait<Self, Rhs, Out>
auto operator+(Self lhs, Rhs rhs) -> Out {
    return Add<Self, Rhs, Out>(lhs).add(rhs);
}

// 为动态发分发的 Add trait 对象也实现 operator+
template<typename Rhs, typename Out>
auto operator+(Add<Dyn, Rhs, Out> lhs, Rhs rhs) -> Out {
    return lhs.add(rhs);
}

#endif //__TRAITS_DEMO_ADD_H__
