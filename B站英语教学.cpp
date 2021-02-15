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
  
  ① 产生：例如使用两把锁时
  class ResourceMu
  {
    std::mutex _mu1;
    std::mutex _mu2;
    ofstream _f;
  public:
    ResourceMu(){_f.open("file.tex");}
    void shared_print_1(string s, int v)
    {
      std::lock_guard<std::mutex> locker_1(_mu1);
      std::lock_guard<std::mutex> locker_2(_mu2);
      _f << "From " << s << ":" << v << endl; 
    }
    void shared_print_2(string s, int v)
    {
      std::lock_guard<std::mutex> locker_2(_mu2);
      std::lock_guard<std::mutex> locker_1(_mu1);
      _f << "From " << s << ":" << v << endl; 
    }
  };
  
  void function_thread(ResourceMu& rm) // 子线程调用
  {
    for(int i = 0; i < 100; ++i)
    {
      rm.shared_print_1("thread", i);
    }
  }

  void function_main(ResourceMu& rm) // 主线程调用
  {
    for(int i = 0; i > -100; --i)
    {
      rm.shared_print_2("mian", i);
    }
  }

  int main()
  {
    ResourceMu rm;
    std::thread t1(function_thread, std::ref(rm));
    function_main();
    
    t1.join();
    return 0;
  }

  ② 避免这种情况的死锁：保证每次上锁的顺序一致。C++提供了保证上锁顺序的机制：std::lock(_mu1, _mu2);
  std::lock(_mu1, _mu2);
  std::lock_guard<std::mutex> locker_1(_mu1, std::adopt_lock);
  std::lock_guard<std::mutex> locker_2(_mu2, std::adopt_lock);
  
6. unique_lock<std::mutex> 比起lock_guard更有灵活性： 
  ① 可以直接解锁
  std::unique_lock<std::mutex> locker(_mu);
  //...需要上锁的代码
  locker.unlock();
  //...不需要上锁的代码
  
  ② 可以先创建锁而不上锁：
  std::unique_lock<std::mutex> locker(_mu, std::defer_lock);
  //...不需要上锁的代码
  lockder.lock();
  //...需要上锁的代码

  ③ unique_lock和lock_guard都不可以被复制，但是unique_lock可以被move
  
  ④ unique_lock具有灵活性的代价是性能相较而言更低。
      
7. Lazy Initialization(Initialization Upon First Use Idiom)
  
  ① 什么是Lazy Initialization
  class LogFile
  {
    std::mutex _mu;
    std::mutex _mu_open;
    ofstream _f;
  public:
    LogFile(){_f.open("Log.txt");};//每次创建类则打开一个文件，但是我们想要的是在使用shared_print函数时候才打开文件
    void shared_print(){...}
  }
  将打开文件操作移动到shared_print()中，即为Lazy Initialization；
  
  ② 考虑线程安全：
  void shared_print(string id, int value)
  {
    //打开文件：只被打开一次
    if(!_f.isopen())
    {
      std::unique_lock<std::mutex> locker_open(_mu_open);//这不是线程安全的（*）
      _f.open("Log.txt");
    }
    
    //...操作_f的代码，会调用多次
    std::unique<std::mutex> locker(_mu);
    ...
  }
  分析为什么不是线程安全的：thread1发现_if未打开，所以给_mu_open上锁（还未打开文件），此时执行至thread2发现文件未打开，则等待给_mu_open上锁，一旦thread1打开文件并释放锁之后，thread2再次打开文件。

  ③ 解决上述问题:
  void shared_print(string id, int value)
  {
    //打开文件：只被打开一次
    {
      std::unique_lock<std::mutex> locker_open(_mu_open);//该代码则保证文件只打开一次，是线程安全的。
      if(!_f.isopen())
      {
        _f.open("Log.txt");
      }
    }
 
    //...操作_f的代码，会调用多次
    std::unique<std::mutex> locker(_mu);
    ...
  }
  
  引入多次上锁解锁的操作带来的资源浪费问题：
  
  ④ 解决上述问题，使用once_flag
  std::once_flag _flag;
  std::call_once(_flag, [&](){_f.open("Log.txt");}); //thread safe and efficient.
      
      
      
      
      
      
      
