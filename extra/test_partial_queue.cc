#include <cstdlib>
#include <iomanip>
#include <iostream>

#include "torrent/utils/partial_queue.h"
#include "torrent/utils/timer.h"

void
test_fill() {
  torrent::utils::partial_queue queue;
  queue.enable(8);
  queue.clear();

  std::cout << "test_fill()" << std::endl;
  std::cout << "  inserting: ";

  int i = 0;

  while (true) {
    uint8_t k = rand() % 256;

    if (queue.insert(k, k))
      std::cout << '[' << (uint32_t)k << ']' << ' ';
    //     else
    //       std::cout << '<' << (uint32_t)k << '>' << ' ';

    if (queue.is_full() && ++i == 100)
      break;
  }

  std::cout << std::endl << "  popping: ";

  while (queue.prepare_pop()) {
    std::cout << queue.pop() << ' ';
  }

  std::cout << std::endl;
}

void
test_random() {
  torrent::utils::partial_queue queue;
  queue.enable(8);
  queue.clear();

  std::cout << "test_random()" << std::endl;
  std::cout << "  inserting: ";

  for (int i = 0; i < 10 && !queue.is_full(); ++i) {
    uint8_t k = rand() % 256;

    if (queue.insert(k, k))
      std::cout << '[' << (uint32_t)k << ']' << ' ';
    else
      std::cout << '<' << (uint32_t)k << '>' << ' ';
  }

  std::cout << std::endl << "  popping: ";

  while (queue.prepare_pop()) {
    if (rand() % 2) {
      std::cout << queue.pop() << ' ';
    } else {
      uint8_t k = rand() % 128;

      if (queue.insert(k, k))
        std::cout << '[' << (uint32_t)k << ']' << ' ';
      else
        std::cout << '<' << (uint32_t)k << '>' << ' ';
    }
  }

  std::cout << std::endl;
}

int
main(int argc, char** argv) {
  srand(torrent::utils::timer::current().usec());

  std::cout << "sizeof(torrent::utils::partial_queue): "
            << sizeof(torrent::utils::partial_queue) << std::endl;

  test_fill();
  test_random();

  return 0;
}
