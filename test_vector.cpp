#include <deque>
#include <stdio.h>
int main(int argc, char const *argv[])
{
    std::deque<int> dq;
    dq.insert(dq.end(), 1);
    dq.insert(dq.end(), 2);
    dq.insert(dq.end(), 3);
    dq.insert(dq.end(), 4);

    std::deque<int> dq2;
    dq2.insert(dq2.end(), dq.begin(), dq.end());
    dq2.insert(dq2.end(), dq.begin(), dq.end());
    while (!dq2.empty())
    {
        fprintf(stdout, "dq2 value=%d\n", dq2.front());
        dq2.pop_front();
    }

    fprintf(stdout, "deque size=%lu\n", dq.size());
    return 0;
}