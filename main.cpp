#include <vector>
#include <iostream>

#include "Add.h"
#include "Point.h"

int main() {
    // 静态分发
    Point<float> a { .x = 1.f, .y = 1.f };
    Point<float> b { .x = 2.f, .y = 2.f };
    auto sum = a + b;
    std::cout << sum.x << ' ' << sum.y << '\n';

    // 动态分发
    // 类似于 rust 中的 dyn Add<Rhs = float, Out = float>
    float floating_point = 1.2f;
    std::vector<DynAdd<float, float>> v_lhs = { floating_point, a };
    for (const auto& lhs: v_lhs) {
        auto s = lhs + 1.2f;
        std::cout << "sum = " << s << '\n';
    }

    return 0;
}
