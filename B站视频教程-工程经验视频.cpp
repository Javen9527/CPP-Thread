链接：https://www.bilibili.com/video/BV1Yb411L7ak?p=4&spm_id_from=pageDriver

一、C++11新标准线程库
1. window以前做法：CreateThread(), _beginthread(), _beginthreadexe()创建线程
2. linux以前做法：pthread_create()创建线程
3. 以往的多线程代码不能跨平台(操作系统提供的接口) -> POSIX thread(pthread)可以跨平台，但是在各个平台需要做一番配置
4. 从C++11新标准开始，从C++语言本身增加对多线程的支持，可移植性（跨平台）

二、多线程基础
1. 进程结束的标志：主线程执行结束（main函数）。主线程结束后，一般操作系统会强行终止还没有结束的子线程。
2. 创建线程：用可调用对象实例化thread对象。
3. join() -> 主线程阻塞等待子线程执行结束
4. detach() -> 主线程和子线程分离，最终由C++运行时库回收（守护线程） 
5. joinable() -> 对一个线程只能调用一次join和detach
6. 可调用对象：函数、重载了()的类、lamda表达式













 N、编写多线程程序的坑
 
1. 类对象作为可调用对象实例化thread对象时，如果类中的()操作函数操作的是类维护的引用，且该引用是在主线程中的局部变量，则在使用detach的时候，该局部变量生命周期结束导致出现问题。
class ExeClass
{
public:
    int& m_i;
public:
    ExeClass(int& i) :m_i(i) {}
    void operator()()
    {
        cout << " Value = " << m_i << endl;
    }
};

int main()
{
    int value = 5;
    ExeClass eClass(value);

    thread t1(eClass);
    t1.detach();

    return 0;
}
    
2. 使用可调用对象实例化thread对象时候的传参问题：
  ① 传入引用时，实际上还是复制的（假引用），需要使用ref();
  ② 传入指针时，实际上是真的指针，使用detach的时候需要注意，如果指针指向局部变量，注意它的生命周期



























