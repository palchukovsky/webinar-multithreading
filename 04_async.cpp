#include <future>
#include <iostream>
#include <mutex>

////////////////////////////////////////////////////////////////////////////////

namespace {

int do_something_very_interesting_and_long(const int i) { return 42 + i; }

}  // namespace

/*
  std::async запускает функцию f асинхронно (потенциально в отдельном потоке,
  который может быть частью пула потоков) и возвращает std::future, который в
  конечном итоге будет содержать результат вызова этой функции.

  std::async позволяет установить политику запуска задачи:
    * std::launch::async выполняет вызываемый объект f в новом потоке
      выполнения.
    * std::launch::deferred не порождает новый поток выполнения. Функция
      выполняется лениво.
    * std::launch::async | std::launch::deferred в зависимости от реализации,
      производится или асинхронное выполнение, или ленивое
*/
void test1_async() {
  std::future<int> r1 =
      std::async(std::launch::async, do_something_very_interesting_and_long, 1);

  auto r2 = std::async(std::launch::deferred,
                       do_something_very_interesting_and_long, 2);

  auto r3 = std::async(std::launch::deferred | std::launch::async,
                       do_something_very_interesting_and_long, 3);

  std::cout << "Result1 = " << r1.get() << std::endl;
  std::cout << "Result2 = " << r2.get() << std::endl;
  std::cout << "Result3 = " << r3.get() << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

void test2_exception() {
  const auto bad_worker = []() -> int {
    throw std::logic_error("Ooooops");

    return 42;
  };

  auto result =
      std::async(std::launch::deferred | std::launch::async, bad_worker);

  try {
    const auto value = result.get();
  } catch (const std::exception& ex) {
    std::cout << "Exception! " << ex.what() << std::endl;
  }
}

////////////////////////////////////////////////////////////////////////////////

int main() {
  test1_async();

  std::cout << std::endl
            << "##############################################################"
            << std::endl
            << std::endl;

  test2_exception();

  return 0;
}

////////////////////////////////////////////////////////////////////////////////