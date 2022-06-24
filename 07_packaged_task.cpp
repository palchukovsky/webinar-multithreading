#include <cmath>
#include <functional>
#include <future>
#include <iostream>
#include <thread>

/*
  std::packaged_task - обёртка для любого callable объекта, которая
  перехватывает исключения и сохраняет результат, при этом не требуется вручную
  создавать объект promise

  std::packaged_task работает так же, как если бы мы создали объект std::promise
  и сохранили в него результат работы функции
*/

void task_lambda() {
  std::packaged_task<double(double, double)> task(
      [](const double a, const double b) { return std::pow(a, b); });
  std::future<double> result = task.get_future();

  task(2, 9);

  std::cout << "task_lambda:\t" << result.get() << '\n';
}

namespace {

// unique function to avoid disambiguating the std::pow overload set
double f(const double x, const double y) { return std::pow(x, y); }

}  // namespace

void task_bind() {
  std::packaged_task<double()> task(std::bind(f, 2, 11));
  std::future<double> result = task.get_future();

  task();

  std::cout << "task_bind:\t" << result.get() << '\n';
}

void task_thread() {
  std::packaged_task<double(double, double)> task(f);
  std::future<double> result = task.get_future();

  std::thread task_td(std::move(task), 2, 10);
  task_td.join();

  std::cout << "task_thread:\t" << result.get() << '\n';
}

void task_exception() {
  std::packaged_task<void()> task(
      []() { throw std::runtime_error("Oooops..."); });
  auto result = task.get_future();

  task();

  try {
    result.get();
  } catch (const std::exception &ex) {
    std::cout << "task_exception: \texception: " << ex.what() << std::endl;
    return;
  }

  std::cout << "task_exception: \tsuccess" << std::endl;
}

int main() {
  task_lambda();
  task_bind();
  task_thread();
  task_exception();
}