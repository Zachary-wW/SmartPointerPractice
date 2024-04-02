#include <iostream>
#include <memory>

class Sp;

class Sp1 {
   private:
    std::weak_ptr<Sp> sptr_;

   public:
    Sp1(std::shared_ptr<Sp>& sptr) : sptr_(sptr) {}
};

class Sp2 {
   private:
    std::weak_ptr<Sp> sptr_;

   public:
    Sp2(std::shared_ptr<Sp>& sptr) : sptr_(sptr) {}
};

class Sp3 {
   private:
    std::weak_ptr<Sp> sptr_;

   public:
    Sp3(std::shared_ptr<Sp>& sptr) : sptr_(sptr) {}
};

class Sp : public std::enable_shared_from_this<Sp> {
   private:
    std::shared_ptr<Sp1> sptr1_;
    std::shared_ptr<Sp2> sptr2_;
    int num_;

   public:
    Sp() {}
    Sp(int n) { num_ = n; }
    int getNum() { return num_; }
    void test_unique_ptr();
    void test_shared_ptr();
    void test_weak_ptr();
    void set12(const std::shared_ptr<Sp1>& sptr1,
               const std::shared_ptr<Sp2>& sptr2) {
        sptr1_ = sptr1;
        sptr2_ = sptr2;
    }
    // 如果想在该类的内部把对象的指针传给其他对象
    void new_Sp3() {
        // ERROR
        /*
        std::shared_ptr<Sp> sptr_this(this); //
        如果这样操作产生的sharedptr与当前的类没有关系 std::shared_ptr<Sp3>
        sptr3(new Sp3(sptr_this));
        */

        // CORRECT
        std::shared_ptr<Sp> sptr_this(shared_from_this());
        std::shared_ptr<Sp3> sptr3(new Sp3(sptr_this));
    }
};

void Sp::test_unique_ptr() {
    // 拥有对象的唯一所有权 不能存在两个uniqueptr指向同一个对象
    std::unique_ptr<Sp> uptr(new Sp());

    // get方法 获取原有的裸指针
    Sp* originPtr = uptr.get();

    // bool方法 查看是否有指针
    if (uptr) {
        std::cout << "Has a pointer! " << std::endl;
    }

    // release方法 将所有权转交给其他的sp
    std::unique_ptr<Sp> uptr1(uptr.release());

    if (!uptr) {
        std::cout << "Has Not a pointer! " << std::endl;
    }

    // reset方法
    // 如果传入一个新的指针 那么接管它 原来的被销毁
    uptr.reset(new Sp());
    // 不传入参数 等价于uptr1 = nullptr
    uptr1.reset();
}

void Sp::test_shared_ptr() {
    // 共享指针的所有权
    std::shared_ptr<Sp> sptr(new Sp());
    std::shared_ptr<Sp> sptr1(sptr);
    // get
    Sp* origin = sptr.get();

    // bool
    if (sptr) {
        std::cout << "Has a pointer! " << std::endl;
    }

    std::cout << "Before Reset, Number of Count: " << sptr.use_count()
              << std::endl;

    // use_count返回引用计数大小
    // std::cout << "Number: " << sptr.use_count() << std::endl;

    sptr1.reset();

    if (sptr.unique()) {
        std::cout << "After Reset, Number of Count: " << sptr.use_count()
                  << std::endl;
    }

    std::shared_ptr<Sp> sptr3 = std::make_shared<Sp>(10);
    std::shared_ptr<Sp> sptr4 = std::make_shared<Sp>(5);

    std::cout << "Before Swap "
              << "sptr3: " << sptr3->getNum() << " sptr4: " << sptr4->getNum()
              << std::endl;
    sptr3.swap(sptr4);
    std::cout << "After Swap "
              << "sptr3: " << sptr3->getNum() << " sptr4: " << sptr4->getNum()
              << std::endl;
}

void Sp::test_weak_ptr() {
    // weakptr是为了避免shareptr出现循环引用的情况 造成内存泄露
    std::shared_ptr<Sp> sptr(new Sp());
    std::weak_ptr<Sp> wptr(sptr);  // 不增加引用计数

    if (sptr.unique()) {
        std::cout << "Weak_ptr Do NOT Increase Count!" << std::endl;
    }

    sptr.reset();

    if (wptr.expired()) {
        std::cout << "Released! " << std::endl;
    }

    std::shared_ptr<Sp> tmp_sptr =
        wptr.lock();  // 获取shared_ptr 如果被释放则为空
    if (tmp_sptr == nullptr) {
        std::cout << "Released! Get Lock is nullptr!" << std::endl;
    }

    wptr.reset();
}

int main(int argc, char* argv[]) {
    Sp sp;

    std::string arg = "";
    std::cin >> arg;

    if (arg == "up")
        sp.test_unique_ptr();
    else if (arg == "sp")
        sp.test_shared_ptr();
    else if (arg == "wp")
        sp.test_weak_ptr();
    else
        printf("No Argument OR Wrong Argument!");

    return 0;
}