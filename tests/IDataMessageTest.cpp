#include <iostream>

#include "TestDataMessage.h"
#include "queue/readerwriterqueue.h"

template <typename T2>
using brwQueue = moodycamel::BlockingReaderWriterQueue<T2>;

int main()
{
    brwQueue<TestDataMessage*>* queue = new brwQueue<TestDataMessage*>();

    TestDataMessage* msg1 = new TestDataMessage(5);
    std::memcpy(msg1->data(), "Hello", 5);
    msg1->size(5);
    queue->enqueue(msg1);

    TestDataMessage* msg2;
    queue->wait_dequeue_timed(msg2, std::chrono::milliseconds(50));
    
    std::cout << msg2->size() << "\n";
    std::cout << (char*)msg2->data() << "\n";

    delete msg1;
    delete queue;
    return 0;
}
