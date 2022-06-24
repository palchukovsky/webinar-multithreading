#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

/*
  Предположим, что есть совместно используемый ресурс, создание которого
  настолько затратно, что заниматься этим хочется лишь в крайней необходимости,
  когда пользователь обратился к этому ресурсу: возможно, он открывает
  подключение к базе данных или выделяет слишком большой объем памяти.
*/

/*
  Каждая операция, требующая ресурса, сначала проверяет, не был ли он
  инициализирован, и, если не был, прежде чем воспользоваться этим ресурсом,
  инициализирует его.
*/

struct some_resource {
  some_resource() { std::cout << "constructor some_resource" << std::endl; }
  void do_something() { std::cout << "do_something" << std::endl; }
};

std::shared_ptr<some_resource> resource_ptr;
std::mutex resource_mutex;

void motivation() {
  {
    const std::unique_lock<std::mutex> lk(resource_mutex);
    if (!resource_ptr) {
      resource_ptr.reset(new some_resource);
    }
  }

  resource_ptr->do_something();  // считаем, что сама операция потокобезопасна,
                                 // например, это запрос к БД
}

/*
  Но это может привести к ненужным блокировкам потоков, использующих ресурс.
  Каждый поток будет вынужден ожидать разблокировки мьютекса, чтобы проверить,
  не был ли ресурс уже инициализирован.
*/

/*
  Шаблон блокировки с двойной проверкой: сначала указатель считывается без
  получения блокировки, которая устанавливается, только если он имеет значение
  nullptr.

  СОСТОЯНИЕ ГОНКИ НЕОПРЕДЕЛЁННОЕ ПОВЕДЕНИЕ
*/

void undefined_behaviour_with_double_checked_locking() {
  if (!resource_ptr) {
    const std::lock_guard<std::mutex> lk(resource_mutex);
    if (!resource_ptr) {
      resource_ptr.reset(new some_resource);
    }
  }
  resource_ptr->do_something();
}

/*
  Стандартная библиотека C++ предоставляет компоненты std::once_flag и
  std::call_once. Вместо блокировки мьютекса и явной проверки указателя каждый
  поток может безопасно воспользоваться функцией std::call_once, зная, что к
  моменту возвращения управления из этой функции указатель будет инициализирован
  каким-либо потоком.
*/

std::once_flag resource_flag;

void init_resource() { resource_ptr.reset(new some_resource); }

void test1_call_once() {
  std::call_once(resource_flag, init_resource);
  resource_ptr->do_something();
}

/*
  Альтернатива для задач инициализации - локальные статические переменные.

  Начиная с C++11 инициализация выполняется только в одном потоке, и никакие
  другие потоки не будут продолжать выполнение до тех пор, пока эта
  инициализация не будет завершена.
*/

some_resource& get_instance() {
  static some_resource instance;

  return instance;
}

void test2_static() {
  some_resource& resource = get_instance();
  resource.do_something();
}

int main() {
  {
    std::thread t1(test1_call_once);
    std::thread t2(test1_call_once);

    t1.join();
    t2.join();
  }

  std::cout << std::endl
            << "##############################################################"
            << std::endl
            << std::endl;

  {
    std::thread t1(test2_static);
    std::thread t2(test2_static);

    t1.join();
    t2.join();
  }

  return 0;
}