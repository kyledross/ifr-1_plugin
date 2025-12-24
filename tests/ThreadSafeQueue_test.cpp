#include <gtest/gtest.h>
#include "ThreadSafeQueue.h"
#include <thread>
#include <chrono>

TEST(ThreadSafeQueueTest, PushAndPop) {
    ThreadSafeQueue<int> queue;
    queue.Push(1);
    queue.Push(2);

    EXPECT_EQ(queue.Size(), 2);
    EXPECT_FALSE(queue.IsEmpty());

    auto val1 = queue.Pop();
    ASSERT_TRUE(val1.has_value());
    EXPECT_EQ(*val1, 1);

    auto val2 = queue.Pop();
    ASSERT_TRUE(val2.has_value());
    EXPECT_EQ(*val2, 2);

    auto val3 = queue.Pop();
    EXPECT_FALSE(val3.has_value());
    EXPECT_TRUE(queue.IsEmpty());
}

TEST(ThreadSafeQueueTest, Clear) {
    ThreadSafeQueue<int> queue;
    queue.Push(1);
    queue.Push(2);
    queue.Clear();

    EXPECT_TRUE(queue.IsEmpty());
    EXPECT_EQ(queue.Size(), 0);
    EXPECT_FALSE(queue.Pop().has_value());
}

TEST(ThreadSafeQueueTest, ThreadSafety) {
    ThreadSafeQueue<int> queue;
    const int count = 1000;

    std::thread producer([&queue, count]() {
        for (int i = 0; i < count; ++i) {
            queue.Push(i);
        }
    });

    std::thread consumer([&queue, count]() {
        int received = 0;
        while (received < count) {
            if (queue.Pop().has_value()) {
                received++;
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_TRUE(queue.IsEmpty());
}
