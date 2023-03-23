# Cpp: 在 `Cpp 20` 中模拟 `Rust` 中的 `trait`
## 前言
`Rust` 中一个比较吸引我的特性是 `trait` 。在 `cpp` 推出 `concept` 特性后，有
不少人会拿它和 `trait` 进行比较。但在我看来，两者相差甚远。其中最明显的不同点有:
- `trait` 既可以静态分发，也可以动态分发。而 `concept` 只是一种静态检查的手段。
- `trait` 可以“动态地”拓展一个结构体的方法。而 `concept` 只是一种静态检查的手段。
    在 `cpp` 中，一个类所拥有的方法都必须在类定义时定义。

而 `cpp` 的类型系统理论上是要比 `rust` 的类型系统要强大，所以我尝试着在 `cpp` 中
模仿 `rust` 的 `trait` 特性。最终得到了:

```c++
// file: Point.h

// 引入 trait Add
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

// main.cpp
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
    std::vector<DynAdd<float, float>> adds = { 1.2f, a };
    for (const auto& add: adds) {
        auto s = add + 1.2f;
        std::cout << "sum = " << s << '\n';
    }

    return 0;
}
```
从上面的代码可以看出，我们现在可以看到，我们做到了 `rust` 中 `trait` 的特性:
- 可为类拓展方法
- 既可以静态分发，也可以动态分发

这是怎么实现的呢？请听我逐步分解。

## `trait` 与 `concept`
在前言中，我多次提及 `trait` 与 `concept` 是不同的东西，`concept` 只是一种
静态检查的手段。这言外之意，起始是 `trait` 所承担的功能或者说职责是要比 `concept`
多的。我们再回头看一下 `trait` 和 `concept` 的差异点，并尝试分析 `trait` 比
`concept` 多了什么:
- `trait` 可为类拓展方法
- `trait` 既可以静态分发，也可以动态分发

### 为类拓展方法
首先第一点就是：在 `rust` 中，我们可以为一个已经定义的类型实现一种 `trait` ，从而
拓展其拥有的方法，如:
```rust
trait SayHi {
    fn say(&self);
}

impl SayHi for i32 {
    fn say(&self) {
        println!("Hi {self}");
    }
}

fn main() {
    1.say();
}
```
这从 `cpp` 的角度看来，似乎是一件不可思议的事情。但这在 `rust` 中类似与一个语法糖:
```rust
fn main() {
    <i32 as SayHi>::say(&1);
}
```
在使用完全限定语法后，我们初步解开了 `trait` 的神秘面纱。可以看到实际上 `say` 方法
的调用过程是 `<i32 as SayHi>` 调用的（这个说法可能不太准确）。换到 `cpp` 中，我们
可以认为，`say` 的调用实际上是先构建了一个 `SayHi<i32>` 对象, 再由它去调用 `say` 
函数。而为 `i32` 实现 `SayHi` 的过程，可以看作 `SayHi<T> [T = i32]` 的特化。

在进行了上述分析后，我们也就有了在 `cpp` 中模仿 `trait` 的初步思路:
- 通过 `concept` 实现相关的静态检查
- 通过 `struct` 定义 `trait` ，如: `Add trait` 可以定义为 `struct Add<Self>`
（在下文中将这种类型称为 `structural trait`）
- 通过对 `structural trait` 的类型参数 `Self` 进行特化，来为 `Self` 类型实现
相应的 `trait`
- 通过构建 `structural trait` 的对象来调用实现

通过文字很难详细地说明思路，那么我们直接上代码:
```c++
// file: Add.h

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
```
一个可静态分发、可为类拓展方法的 `structural trait Add` 就定义好了。那么该如何使用呢？
```c++
// file: Add.h

// 为所有 整数 和 浮点数 实现 Add
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

// file: Point.h

// 引入 trait Add
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

// file: main.cpp
#include <iostream>

#include "Add.h"
#include "Point.h"

int main() {
    // 静态分发
    Point<float> a { .x = 1.f, .y = 1.f };
    Point<float> b { .x = 2.f, .y = 2.f };
    // 构建 Add 对象 获取 Point<float> 的 `Add` 实现
    auto sum = Add(a).add(b);
    std::cout << sum.x << ' ' << sum.y << '\n';
}
```
首先，我们在 `Add.h` 中通过对 `Add` 进行偏特化，为所有的整数和浮点数都实现了 
`Add trait`。然后我们在 `Point.h` 定义 `Point` 结构体时，
只是简单地定义了它的数据成员。 然后通过对 `Add` 进行偏特化，为 `Point` 实现了
`Add trait`。最后在 `main.cpp` 中我们就可以通过构建构建 Add 对象 获取 
Point<float> 的 `Add` 实现，从而实现了 `Point<float>` 的相加。

但上面代码中的调用方式并不优雅，我们能不能直接使用 `+` 运算符呢？答案是可以的，
我们只要为所有实现了 `Add trait` 的类型都实现 `operator+` 即可:

```c++
// file: Add.h

// 为所有实现了 AddTrait 的 `Self` 都实现 operator+
template<typename Self, typename Rhs = Self, typename Out = Self>
requires AddTrait<Self, Rhs, Out>
auto operator+(Self lhs, Rhs rhs) -> Out {
    return Add<Self, Rhs, Out>(lhs).add(rhs);
}
```
这样，我们就可以使用 `+` 运算符了:
```c++
// file: main.cpp
#include <iostream>

#include "Add.h"
#include "Point.h"

int main() {
    // 静态分发
    Point<float> a { .x = 1.f, .y = 1.f };
    Point<float> b { .x = 2.f, .y = 2.f };
    // 通过 operator+ 调用 add 实现
    auto sum = a + b;
    std::cout << sum.x << ' ' << sum.y << '\n';
}
```
### 动态分发
在 `rust` 中 `trait` 可以通过 `dyn` 关键字进行动态分发:
```rust
trait SayHi {
    fn say(&self);
}

impl SayHi for i32 {
    fn say(&self) {
        println!("Hi from i32 {self}");
    }
}

impl SayHi for i64 {
    fn say(&self) {
        println!("Hi from i64 {self}");
    }
}

fn main() {
    let say_hi: Vec<Box<dyn SayHi>> = vec![Box::new(1i32), Box::new(2i64)];
    for s in say_hi {
        s.say();
    }
}

// 终端输出:
// Hi from i32 1
// Hi from i64 2
```
而做到动态分发，我们就要擦除 `Add` 中的类型参数 `Self` 。而在 `cpp` 中进行类型
擦除，我本人只知道两种方法:
- 子类继承父类，再通过将子类对象向上转型为父类对象，实现类型参数
- 通过将接口改为 `std::function`，通过 `lambda` 捕获 `Self` 对象，
在 `lambda` 中调用捕获对象的接口实现，从而在接口中抹除 `Self` 参数

而我希望”动态地“为类型拓展方法，所以选择第二中方法实现类型擦除:
```c++
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

// 为动态发分发的 Add trait 对象也实现 operator+
template<typename Rhs, typename Out>
auto operator+(Add<Dyn, Rhs, Out> lhs, Rhs rhs) -> Out {
    return lhs.add(rhs);
}
```
可以看到，在上述实现中，`add` 不再是 `Add` 中的一个成员函数，而是变成了一个类型为
 `std::function<Out(Rhs)>` 的数据成员。`Self` 对象不再储存在 `Add` 中，而
是在 `Add` 对象构造时被捕获到 `add` 闭包中。我们从而抹除了 `Add` 中 `Self` 参数，
将其变成了一个标记类型 `Dyn` 。最终我们也实现了 `Add` 的动态分发。

我们再为 `Point<float>` 实现和 `float` 相加得到 `float` 的 `Add trait`，
用来测试动态分发功能:
```c++
// file: Point.h

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


// file: main.cpp
#include <vector>
#include <iostream>

#include "Add.h"
#include "Point.h"

int main() {
    // 动态分发
    // 类似于 rust 中的 dyn Add<Rhs = float, Out = float>
    float floating_point = 1.2f;
    Point<float> point { .x = 1.f, .y = 1.f };
    std::vector<DynAdd<float, float>> v_lhs = { floating_point, a };
    for (const auto& lhs: v_lhs) {
        auto s = lhs + 1.2f;
        std::cout << "sum = " << s << '\n';
    }
    
    return 0;
}

// 终端输出
// sum = 2.4
// sum = 2.2
```

## 不足
虽然我们成功模仿了 `rust` 中的 `trait` 的特性。但其调用方式却不够优雅
(必须先构造 `trait` 对象才能调用实现、无法链式调用):
```c++
// 注意! 这是伪代码
template<typename Self>
struct Foo {
    auto bar() -> Self;
};

struct Bar{};
template<>
struct Foo<Bar> {
    auto bar() -> Self {
        ...
    }
};

// 无法直接调用
auto bar = Bar{};
bar.bar(); // error: No member named 'bar' in 'Bar'

// 必须先构造 trait 对象
Foo(bar).bar();

// 无法链式调用, 必须不断嵌套调用
Foo(Foo(bar).bar()).bar();
```

## 结语
在这篇文章中，我在 `cpp` 中通过偏特化和类型擦除的方法，模拟了 `rust` 中的 `trait`
对象。但这种方法仍存在很多不足。有任何疑问或者改进意见的，请留言，我会认真
阅读并答复。
