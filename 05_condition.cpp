#include <atomic>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

/*
  Условная переменная связана с каким-либо условием, и один или несколько
  потоков могут ожидать выполнения этого условия. Когда другой выполнит условие,
  он может известить об этом один или несколько потоков, ожидающих условную
  переменную, чтобы разбудить их и позволить продолжить работу.

  Любой поток, который ожидает наступления события от std::condition_variable,
  должен:
    * Заблокировать мьютекс с помощью std::unique_lock<std::mutex>.
    * Проверить, что необходимое условие ещё не выполнено.
    * Вызвать метод wait, wait_for или wait_until. Операции ожидания освобождают
      мьютекс и приостанавливают выполнение потока.
    * Когда получено уведомление, истёк тайм-аут или произошло ложное
      пробуждение, поток пробуждается, и мьютекс повторно блокируется.
    * Поток должен проверить, что условие, действительно, выполнено, и
     возобновить ожидание, если пробуждение было ложным.

  Можно воспользоваться перегрузкой методов wait, wait_for и wait_until, которая
  принимает предикат для проверки условия и выполняет перепроверку сама.

  Поток, который приводит к событию и уведомляет другие потоки должен:
    * захватить std::mutex (обычно через std::lock_guard)
    * выполнить необходимые изменения, пока удерживается блокировка мьютекса
    * выполнить notify_one или notify_all на std::condition_variable (блокировка
      не обязана удерживаться для уведомления)
*/

namespace simple {

std::mutex m;
std::condition_variable cv;
std::string data;
bool ready = false;
bool processed = false;

void worker_thread() {
  // Wait until test_simple() sends data
  std::unique_lock<std::mutex> lk(m);
  cv.wait(lk, [] { return ready; });

  // after the wait, we own the lock.
  std::cout << "Worker thread is processing data\n";
  data += " after processing";

  // Send data back to test_simple()
  processed = true;
  std::cout << "Worker thread signals data processing completed\n";

  // Manual unlocking is done before notifying, to avoid waking up
  // the waiting thread only to block again (see notify_one for details)
  lk.unlock();
  cv.notify_one();
}

void test_simple() {
  std::thread worker(worker_thread);

  data = "Example data";
  // send data to the worker thread
  {
    std::lock_guard<std::mutex> lk(m);
    ready = true;
    std::cout << "test_simple() signals data ready for processing\n";
  }
  cv.notify_one();  // notify_all();

  // wait for the worker
  {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [] { return processed; });
  }
  std::cout << "Back in test_simple(), data = " << data << '\n';

  worker.join();
}

}  // namespace simple

////////////////////////////////////////////////////////////////////////////////

/*
  Пример применения
  Производитель-потребитель
*/
namespace real {

std::atomic<bool> finished{false};
std::mutex conditionMutex;
std::condition_variable condition;

std::vector<int> data;

void consumer() {
  std::unique_lock<std::mutex> lck{conditionMutex};

  while (!finished) {
    while (data.empty() && !finished) {
      condition.wait(lck);
    }

    std::cout << "Consumer - next data to proceed!\n";
    for (auto& v : data) {
      std::cout << v << ' ';
    }
    std::cout << std::endl;

    data.clear();
  }

  std::cout << "Consumer - finished!" << std::endl;
}

void producer() {
  const int step = 5;
  int counter = 0;

  while (!finished) {
    {
      std::lock_guard<std::mutex> guard{conditionMutex};
      std::cout << "Producer - generate data" << std::endl;
      for (int i = 0; i < step; ++i) {
        data.emplace_back(++counter);
      }
    }
    condition.notify_one();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  condition.notify_one();

  std::cout << "Producer - finished!" << std::endl;
}

void test_condition() {
  std::cout << "Producer-consumer example" << std::endl;

  std::thread consumerThread{consumer};
  std::thread producerThread{producer};

  std::this_thread::sleep_for(std::chrono::seconds(3));

  finished = true;
  producerThread.join();
  consumerThread.join();
}

}  // namespace real

////////////////////////////////////////////////////////////////////////////////

/*
  std::condition_variable_any
  Этот тип условной переменной имеет такой же интерфейс, как
  std::condition_variable, но может использоваться не только с
  std::unique_lock<std::mutex>, а с любым блокируемым типом.
  Работает медленее, чем std::condition_variable.
*/
namespace logging {

struct LoggingMutex {
 public:
  void lock() {
    m_mutex.lock();
    std::cout << "mutex locked by thread_id: " << std::this_thread::get_id()
              << std::endl;
  }

  void unlock() {
    std::cout << "mutex unlocked by thread_id: " << std::this_thread::get_id()
              << std::endl;
    m_mutex.unlock();
  }

 private:
  std::mutex m_mutex;
};

LoggingMutex loggingMutex;
std::condition_variable_any condition;
bool startFlag = false;

void folower() {
  std::unique_lock<LoggingMutex> lck{loggingMutex};
  while (!startFlag) {
    condition.wait(lck);
  }
  std::cout << "Folower wake up! threadId = " << std::this_thread::get_id()
            << std::endl;
}

void leader() {
  std::cout << "Leader - prepare command..." << std::endl;
  std::lock_guard<LoggingMutex> guard{loggingMutex};
  startFlag = true;
  condition.notify_all();
}

void test_leader_folowers() {
  std::cout << "Leader-folowers example" << std::endl;

  const std::size_t folowersCount = 5;
  std::vector<std::thread> folowersThr;

  for (std::size_t i = 0; i < folowersCount; ++i) {
    folowersThr.emplace_back(folower);
  };

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::thread leaderThr{leader};

  for (auto& thr : folowersThr) {
    thr.join();
  }
  leaderThr.join();
}

}  // namespace logging

////////////////////////////////////////////////////////////////////////////////

int main() {
  simple::test_simple();

  std::cout << std::endl
            << "##############################################################"
            << std::endl
            << std::endl;

  real::test_condition();

  std::cout << std::endl
            << "##############################################################"
            << std::endl
            << std::endl;

  logging::test_leader_folowers();

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
