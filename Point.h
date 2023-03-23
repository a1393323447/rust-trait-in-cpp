//
// Created by 朕与将军解战袍 on 2023/3/23.
//

#ifndef __TRAITS_DEMO_POINT_H__
#define __TRAITS_DEMO_POINT_H__

#include "Add.h"

template<typename T>
struct Point {
    T x;
    T y;
};

// 对于所有实现了 AddTrait 的类型 T
// 我们为 Point<T> 实现 AddTrait
// 等价的 `rust` 代码:
//
// impl<T> Add for Point<T>
// where T: Add<Output = T>,
// {
//     type Output = Point<T>;
//
//     fn add(self, rhs: Self) -> Self::Output {
//         Point {
//             x: self.x + rhs.x,
//             y: self.y + rhs.y,
//         }
//     }
// }
//
template<typename T>
requires AddTrait<T>
struct Add<Point<T>> {
    using Self = Point<T>;
    using Out = Self;

    Add(Self s): self(s) {}

    auto add(Self other) -> Out {
        auto x_sum = self.x + other.x;
        auto y_sum = self.y + other.y;

        return Self {
                .x = x_sum,
                .y = y_sum,
        };
    }

    Self self;
};

// 为 Point<float> 实现
// AddTrait<Self = Point<float>, Rhs = float, Out = float>
// 换言之, 我们可以将 Point<float> 和 float 相加得到 float
template<>
struct Add<Point<float>, float, float> {
    using Self = Point<float>;

    Add(Self s): self(s) {}

    auto add(float rhs) -> float {
        return self.x + rhs;
    }

    Self self;
};

#endif //__TRAITS_DEMO_POINT_H__
