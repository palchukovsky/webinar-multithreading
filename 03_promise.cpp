#include <chrono>
#include <cmath>
#include <future>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

namespace {

int do_something_very_interesting_and_long(const int i) { return 42 + i; }

void motivation() {
  std::thread worker1{do_something_very_interesting_and_long, 1};
  std::thread worker2{do_something_very_interesting_and_long, 2};

  // Where are my results??
  worker1.join();
  worker2.join();
}
}  // namespace

////////////////////////////////////////////////////////////////////////////////

namespace {

void accumulate(const std::vector<int>::const_iterator &first,
                const std::vector<int>::const_iterator &last,
                std::promise<int> accumulate_promise) {
  const auto result = std::accumulate(first, last, 0);
  accumulate_promise.set_value(result);  // Notify future
}

}  // namespace

// std::promise - предоставляет средство для получения результа работы
// потока
void test1_future_promise() {
  std::cout << "Testing promise..." << std::endl;

  const std::vector<int> numbers = {1, 2, 3, 4, 5, 6};

  /*
    std::promise предоставляет средство для сохранения значения или исключения,
    которое позже асинхронно забирается через объект std::future, созданный
    объектом std::promise.

    Пара объектов std::promise и связанный с ним std::future образуют канал
    связи между потоками.
  */
  std::promise<int> accumulate_promise;
  std::future<int> accumulate_future = accumulate_promise.get_future();

  std::thread work_thread(accumulate, numbers.begin(), numbers.end(),
                          std::move(accumulate_promise));

  /*
    Значение, записанное с помощью promise, может быть прочитано с помощью
    объекта future.

    Тут же встроенное автоматическое ожидание момента когда результат будет
    готов.
  */
  std::cout << "result = " << accumulate_future.get() << '\n';

  work_thread.join();  // wait for thread completion

  std::cout << "Testing promise finished." << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

namespace {

void square_root(double x, std::promise<double> sqrt_promise) {
  if (x < 0) {
    sqrt_promise.set_exception(
        std::make_exception_ptr(std::out_of_range("x<0")));
    return;
  }
  sqrt_promise.set_value(sqrt(x));
}

}  // namespace

void test2_exception() {
  std::cout << "Testing exception..." << std::endl;

  std::promise<double> sqrt_promise;
  std::future<double> sqrt_future = sqrt_promise.get_future();
  std::thread work_thread(square_root, -1.0, std::move(sqrt_promise));

  /*
    std::promise предоставляет возможности сохранить исключение. Если вместо
    значения требуется сохранить исключение, то вместо set_value() вызывается
    метод set_exception().
  */
  try {
    sqrt_future.get();  // вызов get() бросает сохраненное исключение
  } catch (const std::out_of_range &ex) {
    std::cout << "error: " << ex.what() << std::endl;
  }

  work_thread.join();

  std::cout << "Testing exception finished." << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

namespace {

void do_work(std::promise<void> barrier) {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  barrier.set_value();
}

}  // namespace

/*
   promise-future можно использовать не только для передачи значения, но и
   просто для уведомления, если сохранить тип void
*/
void test3_event() {
  std::cout << "\ntest3_event\n";

  std::promise<void> barrier;
  std::future<void> barrier_future = barrier.get_future();
  std::thread new_work_thread(do_work, std::move(barrier));

  barrier_future.wait();
  new_work_thread.join();
}

////////////////////////////////////////////////////////////////////////////////

/*
  Чтобы получить значение из std::future в разных потоках, нужно использовать
  std::shared_future
*/

////////////////////////////////////////////////////////////////////////////////

int main() {
  test1_future_promise();

  std::cout << std::endl
            << "##############################################################"
            << std::endl
            << std::endl;

  test2_exception();

  std::cout << std::endl
            << "##############################################################"
            << std::endl
            << std::endl;

  test3_event();

  return 0;
}

////////////////////////////////////////////////////////////////////////////////