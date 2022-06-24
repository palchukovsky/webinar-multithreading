#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

// std::mutex - защита общих данных от одновременного доступа нескольких потоков
void test1_mutex() {
  std::cout << "Testing mutex..." << std::endl;

  std::mutex counter_mutex;

  std::size_t shared_counter = 1;
  std::atomic_int32_t shared_counter_atomic{1};

  const auto worker = [&counter_mutex, &shared_counter,
                       &shared_counter_atomic]() {
    for (int i = 0; i < 10000; ++i) {
      // "Захват" мьютекса.
      counter_mutex.lock();
      /*
        В эту область кода, теперь, может зайти только поток, который первый
        выполнил "lock". Остальные - будуж ждать пока владеющий мьютексом
        поток не вызовет unlock().

        Сейчас можно безопасно работать с теми данными, которые защищены этим
        мьютексом.
      */
      shared_counter = shared_counter + 1;

      /*
        После окончания работы с защищёнными данными нужно снять лок с
        мьютекса. Если не соблюсти порядок вызовов lock-unlock - поведение не
        определено.

        Если просто никогда не вызвать unlock() - никакой другой поток никогда
        не сможет выполнить lock и получится deadlock.

        Старайтесь как можно сильнее минимизировать время удерживание
        мьютекса!
      */
      counter_mutex.unlock();

      // Atomic не требует защиты!
      ++shared_counter_atomic;

      /*
        Как использование RAII (resource acquisition is initialization),
        например, std::scoped_lock, поможет сделать этот код лучше?

        Попробуйте сравнить std::scoped_lock c std::lock_guard. Его сейчас лучше
        не использовать, но в более старших проектах вы его будете встречать.

        См. test2_mutex_many
      */
    }
  };

  auto thread_count = std::thread::hardware_concurrency();
  std::cout << "std::thread::hardware_concurrency = " << thread_count
            << std::endl;
  if (!thread_count) {
    thread_count = 2;
  }

  std::vector<std::thread> threads;
  for (std::size_t i = 0; i < thread_count; ++i) {
    threads.emplace_back(worker);
  }

  for (auto& th : threads) {
    if (th.joinable()) {
      th.join();
    }
  }

  std::cout << "Testing thread finished with values:" << std::endl
            << "\tshared_counter = " << shared_counter << std::endl
            << "\tshared_counter_atomic = " << shared_counter_atomic
            << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

// Несколько мьютексов.
void test2_mutex_many() {
  std::cout << "Testing many mutexes..." << std::endl;

  struct Employee {
    const std::string id;
    std::mutex m;

    explicit Employee(const std::string id) : id(id) {}

    void send_mail() {
      // simulate a time-consuming messaging operation
      std::cout << "Sending invite for " + id + "\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    void invite_to_lunch(Employee& partner) {
      /*
        std::lock — функция, одновременно блокирующая два и более мьютекса в
        фиксированном порядке без риска вызвать взаимную блокировку.

        Общий совет по обходу взаимной блокировки заключается в постоянной
        блокировке двух мьютексов в одном и том же порядке: если всегда
        блокировать мьютекс А перед блокировкой мьютекса Б, то взаимной
        блокировки никогда не произойдет.
      */
      std::lock(this->m, partner.m);
      // Какие тут могут быть проблемы? Ведь уже вызван std::lock!
      std::lock_guard<std::mutex> lk1(this->m, std::adopt_lock);
      std::lock_guard<std::mutex> lk2(partner.m, std::adopt_lock);
      /*
          Equivalent code (if unique_locks are needed, e.g. for condition
         variables) std::unique_lock<std::mutex> lk1(this->m, std::defer_lock);
              std::unique_lock<std::mutex> lk2(partner.m, std::defer_lock);
              std::lock(lk1, lk2);

          Superior solution available in C++17
              std::scoped_lock lk(this->m, partner.m);
      */

      this->send_mail();
      partner.send_mail();
    }
  };

  Employee e1("e1");
  Employee e2("e2");

  std::thread thread1([&e1, &e2]() { e1.invite_to_lunch(e2); });
  std::thread thread2([&e1, &e2]() { e2.invite_to_lunch(e1); });

  thread1.join();
  thread2.join();

  std::cout << "Testing many mutexes finished." << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

void test3_deadlock() {
  std::cout << "Testing deadlock..." << std::endl;

  struct Employee {
    Employee(std::string id) : id(id) {}
    std::string id;
    std::mutex m;

    void send_mail() {
      // simulate a time-consuming messaging operation
      std::cout << "Sending invite for " + id + "\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    void invite_to_lunch(Employee& partner) {
      const std::scoped_lock<std::mutex> lk1(this->m);
      this->send_mail();

      const std::scoped_lock<std::mutex> lk2(partner.m);
      partner.send_mail();
    }
  };

  Employee e1("e1");
  Employee e2("e2");

  std::thread thread1([&e1, &e2]() { e1.invite_to_lunch(e2); });
  std::thread thread2([&e1, &e2]() { e2.invite_to_lunch(e1); });

  thread1.join();
  thread2.join();

  std::cout << "Testing deadlock finished." << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

// std::recursive_mutex - мьютекс позволяющий безопасно вызывать lock,
// даже если данный поток его уже вызвал
void test4_recursive_mutex() {
  std::cout << "Testing recursive mutex..." << std::endl;

  class Tester {
    std::recursive_mutex m_mutex;
    std::string m_shared;

   public:
    void func1() {
      const std::scoped_lock<std::recursive_mutex> lock(m_mutex);
      m_shared = "func1";
      std::cout << "in fun1, shared variable is now " << m_shared << std::endl;
    }
    void func2() {
      const std::scoped_lock<std::recursive_mutex> lock(m_mutex);
      m_shared = "func2";
      std::cout << "in fun2, shared variable is now " << m_shared << std::endl;
      /*
        Иногда логика программы такова, что функция, который уже владеет
        мьютексом в текущем потоке, вызывает некую другую функцию (или
        рекурсиво, себя же). Если использовать обычный std::mutext - произойдёт
        deadlock. std::recursive_mutex позволяет такое.

        Попрбуйте изменить тип m_mutex на std::mutex и посмотрите что
        получиться.
      */
      func1();
      std::cout << "back in fun2, shared variable is " << m_shared << std::endl;
    };
  };

  Tester tester;
  std::thread thread1(&Tester::func1, &tester);
  std::thread thread2(&Tester::func2, &tester);
  thread1.join();
  thread2.join();

  std::cout << "Testing recursive mutex finished." << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

// std::shared_mutex - мьютекс позволяющий оптимизировать работу, в моделях
// "мало писателей, но много читателей", ведь чтение не требует защиты от друго
// чтения, и может просиходить паралельно
void test5_shared_mutex() {
  std::cout << "Testing shared mutex..." << std::endl;

  class ThreadSafeCounter {
    mutable std::shared_mutex m_mutex;
    unsigned int m_value = 0;

   public:
    ThreadSafeCounter() = default;

    // Multiple threads/readers can read the counter's value at the same time.
    unsigned int get() const {
      const std::shared_lock<std::shared_mutex> lock(m_mutex);
      return m_value;
    }

    // Only one thread/writer can increment/write the counter's value.
    void increment() {
      const std::unique_lock<std::shared_mutex> lock(m_mutex);
      ++m_value;
    }

    // Only one thread/writer can reset/write the counter's value.
    void reset() {
      const std::unique_lock<std::shared_mutex> lock(m_mutex);
      m_value = 0;
    }
  };

  ThreadSafeCounter counter;

  auto increment_and_print = [&counter]() {
    for (int i = 0; i < 3; i++) {
      counter.increment();

      std::ostringstream ss;
      ss << std::this_thread::get_id() << ' ' << counter.get() << '\n';
      std::cout << ss.str();
    }
  };

  std::thread thread1(increment_and_print);
  std::thread thread2(increment_and_print);
  thread1.join();
  thread2.join();

  std::cout << "Testing shared mutex finished." << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

int main() {
  test1_mutex();

  std::cout << std::endl
            << "##############################################################"
            << std::endl
            << std::endl;

  test2_mutex_many();

  std::cout << std::endl
            << "##############################################################"
            << std::endl
            << std::endl;

  test3_deadlock();

  std::cout << std::endl
            << "##############################################################"
            << std::endl
            << std::endl;

  test4_recursive_mutex();

  std::cout << std::endl
            << "##############################################################"
            << std::endl
            << std::endl;

  test5_shared_mutex();

  return 0;
}

////////////////////////////////////////////////////////////////////////////////