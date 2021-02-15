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
      
8. 条件变量    
  ① 生产者消费者模型引入的问题
  void function_1()//生产者
  {
    int count = 10;
    while(count > 0)
    {
      std::unique_lock<std::mutex> locker(mu);
      q.push_front(count);
      locker.unlock();
      std::this_thread::sleep_for(chrono::seconde(1));
      --count;
    }
  }
  void function_2()//消费者
  {
    int data = 0;
    while(data != 1)//该模型中消费者将一直循环判断q是否为空，导致低效
    {
      std::unique_lock<std::mutex> locker(mu);
      if(!q.empty())
      {
        data = q.back();
        q.pop_back();
        locker.unlock();
        cout << "thread2 got a number from thread1 :" << data << endl;
      }
      else
      {
        locker.unlock();
        std::this_thread::sleep_for(chrono::milliseconds(10));//通过等待减少循环次数
      }
    }
  }
  其中的问题是消费者一直循环判断导致的低效问题，可以通过在q为空时候等待的方式来减少循环。但是并不好判断需要等待多久。

  ② 引入条件变量
  void function_1()
  {
    int count = 10;
    while(count > 0)
    {
      std::unique_lock<std::mutex> locker(mu);
      q.push_front(count);
      locker.unlock();
      cond.notify_one();//当生产一个数据后，给出通知
      std::this_thread::sleep_for(chrono::seconds(1));
      --count;
    }
  }
  void function_2()
  {
    int data = 0;
    while(data != 1)
    {
      std::unique_lock<std::mutex> locker(mu);
      cond.wait(locker);//等待唤醒
      data = q.back();
      q.pop_back();
      locker.unlock();
      cout << "Thread 2 got a value from Thread1: " << data << endl;
    }
  }
  ·添加条件变量：std::condition_variable cond;
  ·在function_1中，添加cond.notify_one();
  ·在function_2中，删除对q.empty()的判断，而使用cond.wait(locker); 为什么wait需要locker参数？因为cond.wait将进入sleep状态，无需锁住任何资源，需要对locker解锁。当被唤醒时候，再次上锁。
  ·针对条件变量，注意需要使用unique_lock而不是lock_guard，因为会对locker多次解锁和上锁。
  ·防止function_2可能自己唤醒（spurious wake）：cond.wait(locker, [](){return !q.empty();});
  
9. Future and Promise      
  ① 父线程从子线程获得数据
  常规做法是：
  void function(int N, int& x)
  {
    int res = 1;
    for(int i = N; i > 1; --i)
      res *= i;
    x = res;
  }
  int main()
  {
    int x;
    std::thread t 1(function, 4, std::ref(x));
    
    t1.join();
    return 0;
  }
  此时，由于父线程和子线程共同使用x，则需要使用mutex，同时，需要保证由子线程产出x后父线程才获取x，所以需要条件变量。
  
  ② 更好的做法是：
  使用函数std::async(); 其第一个参数是可调用对象，后面的参数是可调用对象的参数（和线程参数一致）。其返回一个std::future<T>对象，T是可调用对象的返回值类型。
  int function(int N)
  {
    int res = 1;
    for(int i = N; i > 1; --i)
      res *= i;
    return res;
  }
  int main()
  {
    int x;
    std::future<int> fu = std::async(function, 4);//async是否创建一个新线程取决于参数：std::launch::deferred参数将不会使async函数创建新线程，使用std::launch::async则会创建新线程，或者使用两者的和|
    x = fu.get();//注意，get只能被调用一次
    
    return 0;
  }

  ③ 通过使用promise给出一个future，将父线程的值传给子线程
  int Myfunction(std::future<int>& f)//需要使用引用，因为future不能被复制，或者使用shared_future
  {
    int res = 1;
    int N = f.get();//只能调用一次
    for(int i = 1; i <= N; ++i)
    {
      res *= i;
    }
    return res;
  }
  int main()
  {
    int x; 
    
    //并不知道要给Myfunction传入什么值
    std::promise<int> p;
    std::future<int> f = p.get_future();
    
    //std::future<int> fu = std::async(Myfunction, 4);//此时并不知道是否需要传入4，则通过promise在未来决定
    std::future<int> fu = std::async(Myfunction, std::ref(f));
    
    //...执行一些代码后才知道需要给Myfunction传入什么值
    p.set_value(4);
    
    x = fu.get();
    
    return 0;
  }

  ④ future和promise 与 unique_lock一样，只能move不能复制 
  上述引入的问题：如果多个线程调用Myfunction，则会多次调用f.get()导致编译失败。使用shared_future可以避免该问题。
  std::shared_future<int> sf = f.share();

10. Callable object总结
  ① thread对象：std::thread t1(callableOjb, param)
  ② async函数：std::async(std::launch::async, callableObj, param)
  ③ bing函数：std::bind(callableObj, param)
  ④ call_once函数：std::call_once(once_flag, callableObj, param)

11. 针对可调用对象的用法总结：
  class A
  {
   public:
    void f(int x, char c){}
    long g(double x){return 0;}
    int operator()(int N){return 0;}
  };
  void foo(int x){}
  int main()
  {
    A a;
    std::thread t1(a, 6); //复制a对象，执行operator()(int N);
    std::thread t2(std::ref(a), 6); //引用a对象，执行operator()(int N);
    std::thread t3(std::move(a), 6); //移动a对象，执行operator()(int N)，注意a不能再在主线程中被使用；
    
    std::thread t4(A(), 6); //创建临时对象；
    std::thread t5(&A::f, a, 8, 'w'); // 复制a对象，执行a.f()；
    std::thread t6(&A::f, &a, 8, 'w'); // 引用a对象，执行a.f();
    
    std::thread t7([](int x){return x*x;}, 6); //使用lamda函数；
    std::thread t8(foo, 7);
  }

12. packaged task  

  ① 引入packaged task
  int factorial(int N)
  {
    int res = 1;
    for(int i = N; i > 1; --i)
      res *= i;
    return res;
  }
  int main()
  {
    std::packaged_task<int(int)> t(factorial);//注意不能像thread一样传入factorial的参数，需要使用bind：std::bind(factorial, 6);
    ///...do something els
    t(6);
    int x = t.get_future().get();
    
    return 0;
  }
  注意，使用bind后，需要变成：packaged_task<int()>，同时调用时候只能t();
  使用bind时候也可以这样：
  auto t = bind(factorial, 6);
  //...
  t();
  
  ② package_task的应用场景
  std::deque<std::packaged_task<int()>> task_q;//任务队列
  std::mutex mu;//task_q被主线程和子线程共同使用，需要锁防止同时使用
  std::condition_variable cond;
  int factorial(int N)
  {
    int res = 1;
    for(int i = N; i > 1; --i)
      res *= i;
    return res;
  }
 
  void thread_1()//在某个线程中执行任务
  {
    std::packaged_task<int()> t;
    {
      std::unique_lock<std::mutex> locker(mu);
      cond.wait(locker, [](){return !task_q.empty();});//当任务列表里面有任务才唤醒
      t = std::move(task_q.front()); 
      task_q.pop_front();
    }
    t();
  }

  int main()
  {
    std::thread t1(thread_1);
    std::packaged_task<int()> t(bind(factorial, 6));
    std::future<int> fu = t.get_future();//获取任务执行结果
    {
      std::unique_lock<std::mutex> locker(mu);
      task_q.push_back(std::move(t));
    }
    cond.notify_one();
    
    cout << fu.get() << endl;//查看结果
    t1.join();
    return 0;
  }

12. 获得future总结
  ① promise::get_future();
  ② package_task::get_future();
  ③ async() return a future;
  
  

