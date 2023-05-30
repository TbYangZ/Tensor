---
marp: true
backgroundColor: #FFF
paginate: false
theme: gaia
_class: lead
math: katex
---

# 简单的Tensor实现

##### 高级程序设计II项目报告

##### 杨卓，王研博

---

## 一、实现 Tensor 的想法

Tensor 是机器学习具体实现过程中的重要数据结构。我们考虑到对它的实现过程涉及到这学期所学 C++ 各种知识，以及面向对象的思想，同时具有一定的现实意义，故选择实现它作为我们的项目。

Tensor 的功能绝大多数是参考自 pytorch 中的 tensor 具有的功能。

---

## 二、实现思路

将 Tensor 拆成三个部分：

+ 数据 `storage`
+ 形状 `shape`
+ 步长 `stride`

这三个分别写一个类实现，然后 Tensor 就是以这三个类的变量作为成员变量。

---

## 三、主要的类的声明

类 `TensorImpl` 是 Tensor 的具体实现部分。

类 `Exp<>` 是模板类，用于实现 Lazy Evaluation。

类 `Tensor` 继承自类 `Exp<TensorImpl>` 是 Tensor 的外层封装，用户能够直接调用的只能是这个类里的成员函数。

---

## 四、部分功能

#### 1. 内存共享

由于使用共享指针数组实现的 storage，故对一个通过拷贝构造得到的张量，二者的存储部分是共享内存的。这对应 pytorch 中的 tensor 的性质一样。

由于 C++ 本身并没有共享指针数组，于是手动实现一个。

---

#### 2. Lazy Evaluation

在给出表达式的时候，并不直接计算，而是通过某种手段将这串表达式存储下来，在需要的时候才进行计算。

具体实现，是用奇异递归模板实现的静态多态，在编译时处理完成。重载了赋值运算符，所以只在赋值的时候才会进行计算这串表达式。

支持的操作有按照位置的加减乘除、矩阵乘法、三角函数等。

---

#### 3. BroadCast 广播

在进行机器学习代码的编写中，往往是对多个 batch 同时操作，但是权重矩阵的大小显然不需要再多一个维度。于是广播操作就很有必要。

如数据集 data 是形状为 `(64,3,10,10)` 的 Tensor，第一个维度表示数据的 batch；权重矩阵 W 是形状为 `(3,10,10)` 的 Tensor，对二者进行运算时，就需要对 W 作用域 data 的第一个维度的所有数据，得到的结果仍然为 `(64,3,10,10)` 的 Tensor。

实现时对输入坐标进行判断即可。

---

#### 4. 异常检测

为了保证每一个操作都是合法的，不会出现错误的结果，我们实现了 `Error` 类，继承自 `std::exception`。

```cpp
struct Error: public std::exception {
        Error(const char* file, const char* func, unsigned int line);
        const char* what() const noexcept;

        static char msg_[300];
        const char* file_;
        const char* func_;
        const unsigned int line_;
    };
```

---

再通过宏操作，实现各种检测，以避免操作出现问题。

几个例子：

```cpp
#define ERROR_LOCATION __FILE__, __func__, __LINE__
#define THROW_ERROR(format, ...)	do {	\
    std::sprintf(::st::err::Error::msg_, (format), ##__VA_ARGS__);    \
    throw ::st::err::Error(ERROR_LOCATION);                           \
	} while(0)
// base assert macro
#define CHECK_TRUE(expr, format, ...) \
		if(!(expr)) THROW_ERROR((format), ##__VA_ARGS__)

#define CHECK_NOT_NULL(ptr, format, ...) \
		if(nullptr == (ptr)) THROW_ERROR((format), ##__VA_ARGS__)
```

---

#### 5. 迭代器

本来是想着运算过程的一个实现方法，但实际上使用奇异递归模板后就不需要迭代器来实现运算。

迭代器不支持随机访问，即使可以通过下标访问 Tensor 的元素。

```cpp
Tensor::iterator it = tensor.begin();
it != tensor.end();
```

---

## 五、不足之处和功能展望

1. 迭代器

随机访问：理论上迭代器是可以支持随机访问，在今后的版本可能会实现。

实现位置：在 Tensor 这个类中有具体的实现。应该在 TensorImpl 这个类中实现。

---

2. 自动求导功能

在这个版本没有实现自动求导这一很重要的功能，在今后的版本可能会实现。