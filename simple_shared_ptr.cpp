/**
 * 要实现一个简化版本的 shared_ptr，需要考虑以下几点：

    在智能指针类中存储裸指针（raw pointer）和引用计数。
    在构造函数中为裸指针和引用计数分配内存。
    在拷贝构造函数和赋值操作符中正确地更新引用计数。
    在析构函数中递减引用计数，并在引用计数为零时删除对象和引用计数。
 */
#include <iostream>

template <typename T>
class SimpleSharedPtr {
   public:
    // 构造函数
    explicit SimpleSharedPtr(T* ptr = nullptr)
        : ptr_(ptr), cnt_(ptr ? new size_t(1) : nullptr) {}

    // 拷贝构造函数
    SimpleSharedPtr(const SimpleSharedPtr& other) {
        ptr_ = other.ptr_;
        cnt_ = other.cnt_;
        if (cnt_) {
            ++(*cnt_);
        }
    }

    // 赋值运算符重载
    SimpleSharedPtr& operator=(const SimpleSharedPtr& other) {
        if (this != &other) {
            release();  // 判断原先的sharedptr是否可以释放
            ptr_ = other.ptr_;
            cnt_ = other.cnt_;
            if (cnt_) {
                ++(*cnt_);
            }
        }
        return *this;
    }

    // 析构函数
    ~SimpleSharedPtr() { release(); }

    // 相关函数
    T* get() const { return ptr_; }
    size_t use_count() const { return cnt_ ? *cnt_ : 0; }
    // 运算符重载
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }

   private:
    void release() {
        if (cnt_ && --(*cnt_) == 0) {
            delete ptr_;
            delete cnt_;
        }
    }

    T* ptr_;
    size_t* cnt_;
};

class MyClass {
   public:
    MyClass() { std::cout << "MyClass 构造函数\n"; }
    ~MyClass() { std::cout << "MyClass 析构函数\n"; }
    void do_something() { std::cout << "MyClass::do_something() 被调用\n"; }
};

int main() {
    {
        SimpleSharedPtr<MyClass> ptr1(new MyClass());
        {
            SimpleSharedPtr<MyClass> ptr2 = ptr1;
            ptr1->do_something();
            ptr2->do_something();
            std::cout << "引用计数: " << ptr1.use_count() << std::endl;
        }
        std::cout << "引用计数: " << ptr1.use_count() << std::endl;
    }

    return 0;
}