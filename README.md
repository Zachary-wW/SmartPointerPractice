# C++ Smart Pointer

## 1 为什么使用智能指针

### 1.1 Memory Leak (内存泄漏)

在大型项目中容易忽视释放堆上面的内存

```C++
void test_memory_leak(bool open)
{
    A *a = new A();

    if(open)
    {
        // 代码变复杂过程中，很可能漏了 delete(a);
        return;
    }

    delete(a);
    return;
}
```

### 1.2 多线程下的析构问题

在一个class中创建一个新的thread，在该类对象析构的时候该thread可能还在运行，此时就会造成该thread引用的对象指针为野指针，一个简单的修改就是在class的析构函数中加上`join`以此等待线程的退出。但是如果多个thread同时使用该对象，一个释放了，另一个线程还在使用。这也会造成野指针的情况。

## 2 智能指针的基本用法

智能指针可以帮助程序员管理堆上的内存，我们只需要申请，释放交给sp，C++有三种sp: **`unique_ptr, shared_ptr, weak_ptr`**

### 2.1 unique_ptr

`unique_ptr`拥有对象的唯一使用权，两个`unique_ptr`不能指向同一个对象。

一个`unique_ptr`不能复制到另一个`unique_ptr`中，只能使用移动语义的方式转移资源的所有权，关于std::move的简单介绍[link](https://zhuanlan.zhihu.com/p/256239799)：

```C++
std::unique_ptr<A> a1(new A());
std::unique_ptr<A> a2 = a1;//编译报错，不允许复制
std::unique_ptr<A> a3 = std::move(a1);//可以转移所有权，所有权转义后a1不再拥有任何指针
```

智能指针本质上就是利用RAII的思想封装成类，故智能指针本身具有一些方法：

```C++
/**
* 在调用sp的方法时使用 . 
*/

std::unique_ptr<A> a1(new A());
A *origin_a = a1.get();//尽量不要暴露原生指针
if(a1)
{
    // a1 拥有指针
}

// 常见用法，转义拥有权 
// 释放所有权 返回原生指针
std::unique_ptr<A> a2(a1.release());

a2.reset(new A());// 释放并销毁原有对象，持有一个新对象
a2.reset();// 释放并销毁原有对象，等同于下面的写法
a2 = nullptr;// 释放并销毁原有对象
```

### 2.2 shared_ptr

与`unique_ptr`不同，`shared_ptr`是共享引用的对象，采用引用计数的方式实现共享，当引用计数为0时释放对象。类似的，它也有以下方法：

```C++
std::shared_ptr<A> a1(new A()); // ERROR: a1 = new A();
std::shared_ptr<A> a2 = a1;//编译正常，允许所有权的共享

A *origin_a = a1.get();//尽量不要暴露原生指针

if(a1)
{
    // a1 拥有指针
}

if(a1.unique())
{
    // 如果返回true，引用计数为1
}

long a1_use_count = a1.use_count();//引用计数数量

a1.reset(); // 释放并销毁原生指针。如果参数为一个新指针，将管理这个新指针
a1.swap(a2); // 交换指向的对象
```

#### shared_ptr的初始化

一般不使用`new`，因为使用`new`的方式创建`shared_ptr`会导致出现两次内存申请，而`std::make_shared`在内部实现时只会申请一个内存。[reference](https://stackoverflow.com/questions/27082860/new-and-make-shared-for-shared-pointers)

#### shared_ptr的线程安全问题：

- 同一个shared_ptr被多个线程“读”是安全的。
- 同一个shared_ptr被多个线程“写”是不安全的。 
- 共享引用计数的不同的sharedd_ptr被多个线程”写“ 是安全的。

首先，为什么我们会讨论shared_ptr的线程安全问题？其一：shared_ptr中的引用计数是共享的，既然是共享的就涉及到线程安全的问题，共享计数的加减能否保证线程安全？其二：shared_ptr在改变指向对象的时候是否线程安全？其三：shared_ptr本质是一个类模板，类模板的并发操作的线程安全性。[参考](https://www.zhihu.com/question/56836057)

针对第一点，我们需要弄清楚**shared_ptr中的引用计数是如何实现的**，为什么不同`shared_ptr`能够共享引用计数呢？`shared_ptr`的声明如下：

```C++
template<class _Tp>
class shared_ptr
{
public:
    typedef _Tp element_type;

private:
    element_type*      __ptr_; // 原生指针
    __shared_weak_count* __cntrl_; // 控制块指针，指向sharedptr和weakptr的数量
    //...
}
```

__shared_weak_count类：

```c++
class __shared_weak_count
    : private __shared_count
{
    // weak ptr计数
    long __shared_weak_owners_;

public:
    // 内部共享计数和weak计数都为0
    explicit __shared_weak_count(long __refs = 0) _NOEXCEPT
        : __shared_count(__refs),
          __shared_weak_owners_(__refs) {}
protected:
    virtual ~__shared_weak_count();

public:
    // 调用通过父类的__add_shared，增加共享引用计数
    void __add_shared() _NOEXCEPT {
      __shared_count::__add_shared();
    }
    // 增加weak引用计数
    void __add_weak() _NOEXCEPT {
      __libcpp_atomic_refcount_increment(__shared_weak_owners_);
    }
    // 调用父类的__release_shared，如果释放了原生指针的内存，还需要调用__release_weak，因为内部weak计数默认为0
    void __release_shared() _NOEXCEPT {
      if (__shared_count::__release_shared())
        __release_weak();
    }
    // weak引用计数减1
    void __release_weak() _NOEXCEPT;
    // 获取共享计数
    long use_count() const _NOEXCEPT {return __shared_count::use_count();}
    __shared_weak_count* lock() _NOEXCEPT;

private:
    // weak计数为0的处理
    virtual void __on_zero_shared_weak() _NOEXCEPT = 0;
};
```

`shared_count`类：

```c++
// 共享计数类
class __shared_count
{
    __shared_count(const __shared_count&);
    __shared_count& operator=(const __shared_count&);

protected:
  
  
    //------------------- 共享计数-------------------------
    long __shared_owners_;
    //------------------- 共享计数-------------------------
  
    virtual ~__shared_count();
private:
    // 引用计数变为0的回调，一般是进行内存释放
    virtual void __on_zero_shared() _NOEXCEPT = 0;

public:
    // 构造函数，需要注意内部存储的引用计数是从0开始，外部看到的引用计数其实为1
    explicit __shared_count(long __refs = 0) _NOEXCEPT
        : __shared_owners_(__refs) {}

    // 增加共享计数
    void __add_shared() _NOEXCEPT {
      __libcpp_atomic_refcount_increment(__shared_owners_);
    }
    // 释放共享计数，如果共享计数为0（内部为-1），则调用__on_zero_shared进行内存释放
    bool __release_shared() _NOEXCEPT {
      if (__libcpp_atomic_refcount_decrement(__shared_owners_) == -1) {
        __on_zero_shared();
        return true;
      }
      return false;
    }

    // 返回引用计数，需要对内部存储的引用计数+1处理
    long use_count() const _NOEXCEPT {
        return __libcpp_relaxed_load(&amp;__shared_owners_) + 1;
    }
};

```

可以看到，`share_ptr`类中声明了两个指针，一个指向原生指针，一个指向控制块，这个控制块中包含了引用计数、weak ptr的数量、删除器、分配器等，**故引用计数变量是存储在堆上面的，且该加减操作是原子操作（从名字也不难看出：`libcpp_atomic_refcount_increment`），所以是线程安全的。**

现在讨论刚刚提到的问题2: 多线程下`shared_ptr`在改变指针指向的过程中是否线程安全，其实也就是总结的第二点：同一个`shared_ptr`在多线程“写”下不安全。例如：

```c++
void fn(shared_ptr<A>& sp) {
    ...
    if (..) {
        sp = other_sp;
    } else if (...) {
        sp = other_sp2;
	} 
}
```

当程序调用函数fn时，假设需要修改shared_ptr的指向，此时需要将原来的引用计数-1，新的+1，这显然不是一个原子操作。但如果是这样，则是安全的：

```C++
void fn(shared_ptr<A> sp) { // 使用值传递
    ...
    if (..) {
        sp = other_sp;
    } else if (...) {
        sp = other_sp2;
} }
```

### 2.3 weak_ptr

为什么会有`weak_ptr`？一句话概括：**解决shared_ptr的循环引用计数的问题，避免内存泄漏**。例如：

```c++
struct A{
    shared_ptr<B> b;
};
struct B{
    shared_ptr<A> a;
};
shared_ptr<A> pa = make_shared<A>();
shared_ptr<B> pb = make_shared<B>();

// 下面语句像不像一个环
pa->b = pb;
pb->a = pa;
```

上面的例子就会导致循环引用，造成无法正常释放（两者都在等待对方释放），应该改成：

```C++
struct A{
    shared_ptr<B> b;
};
struct B{
    weak_ptr<A> a; // 修改一方为弱引用
};
shared_ptr<A> pa = make_shared<A>();
shared_ptr<B> pb = make_shared<B>();
pa->b = pb;
pb->a = pa;
```

**weak_ptr只作为观测作用，它不增加shared_ptr的引用计数，没有重载相关的运算符**

```C++
class _LIBCPP_TEMPLATE_VIS weak_ptr
{
public:
    typedef _Tp element_type;
private:
    element_type*        __ptr_; // 有原生指针，但是没有重载->，无法访问原始资源
    __shared_weak_count* __cntrl_; 

}

// 通过shared_ptr构造weak_ptr。会将shared_ptr的成员变量地址进行复制。增加weak引用计数
weak_ptr<_Tp>::weak_ptr(shared_ptr<_Yp> const&amp; __r,
                        typename enable_if<is_convertible<_Yp*, _Tp*>::value, __nat*>::type)
                         _NOEXCEPT
    : __ptr_(__r.__ptr_),
      __cntrl_(__r.__cntrl_)
{
    if (__cntrl_)
        __cntrl_->__add_weak();
}

// weak_ptr析构器
template<class _Tp>
weak_ptr<_Tp>::~weak_ptr()
{
    if (__cntrl_)
        __cntrl_->__release_weak();
}
```

weak_ptr的方法：

```C++
std::shared_ptr<A> a1(new A());
std::weak_ptr<A> weak_a1 = a1;//不增加shared_ptr引用计数

if(weak_a1.expired()) // expired用来判断原生指针是否被释放
{
    //如果为true，weak_a1对应的原生指针已经被释放了
}

long a1_use_count = weak_a1.use_count();// 返回原生指针的引用计数数量

if(std::shared_ptr<A> shared_a = weak_a1.lock()) // lock返回一个非空的sharedptr（没有被释放的情况下），被释放返回空
{
    //此时可以通过shared_a进行原生指针的方法调用
}

weak_a1.reset();//将weak_a1置空
```

weak_ptr还有一个作用：**一切应该不具有对象所有权，又想安全访问对象的情况**[Link](https://csguide.cn/cpp/memory/how_to_understand_weak_ptr.html#%E6%B7%B1%E5%85%A5%E7%90%86%E8%A7%A3weak-ptr-%E8%B5%84%E6%BA%90%E6%89%80%E6%9C%89%E6%9D%83%E9%97%AE%E9%A2%98)

```c++
std::weak_ptr<SomeClass> wp{ sp };

if (!wp.expired()) {// 在多线程中，这个过程中sp可能被释放
    wp.lock()->DoSomething();
}

// 正确做法
auto sp = wp.lock(); // lock函数是一个atomic操作
if (sp) {
    sp->DoSomething();
}
```



## Credit

[c++ 11 的shared_ptr多线程安全](https://www.zhihu.com/question/56836057)

[C++ 智能指针最佳实践&源码分析](https://zhuanlan.zhihu.com/p/436290273)

仅供学习，如有侵权，请联系。