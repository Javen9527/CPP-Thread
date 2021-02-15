链接：https://www.bilibili.com/video/BV1ut411y7u5?p=3

1. 创建线程的数量：通过std::thread::hardware_concurrency()来查看

2. 子线程和主线程共享内存：
  ① 通过添加ref的引用：std::ref(var);
  ② 通过指针
  
3. 通过std::move(var)来给子线程传递参数可以避免资源拷贝

4. 主线程和子线程同时使用一个资源时（注意是同时），避免资源竞争，需要使用mutex

  ① 最初始的用法：
  //下面对共享资源cout进行上锁：
    std::mutex mu;
    void shared_print(string msg)
    {
      mu.lock();
      cout << msg << endl;
      mu.unlock();
    }
    
    ② 如果上面的cout << msg << endl抛出异常，则锁永远被锁住，无法解锁。-> 使用lock_guard，在其生命周期结束时候自动解锁
    std::lock_guard<std::mutex> guard(mu); // RAII
    
    ③ 但是上述做法不是线程安全的：即cout是一个全局的，仍然可以在其他地方不通过锁就可以访问。
    针对这样一般使用：通过一个类，维护想要使用的资源以及一把锁。这个时候那么该资源就完全被锁保护了。注意不要向外界返回要保护的资源。
    
    ⑤ 需要注意，使用mutex只是避免同时使用一个资源（data race），而不一定是线程安全的。例如：
    class stack
    {
        int* _data;
        std::mutex _mu;
    public：       
        int& top();//返回值
        void pop();//弹出值
    }
    
    void function(stack& st)//该函数会被很多线程使用
    {
        int value = st.top();
        st.pop();
        // ... value的处理
    }
    分析这段代码不是线程安全的原因：thread1在使用top()后立即thread2使用top()，导致同一个值处理两次，然后thread1再pop()，接着thread2再pop()，导致弹出了一个还没有处理的值。
    所以解决方案是：将取值top和弹出pop操作结合在一个函数里面。
    
5. 死锁
      
      
      
      
      
      
      
      
      
      
      
      
      
