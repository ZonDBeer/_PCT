#ifndef LAB_4_RES_H
#define LAB_4_RES_H

#include <random>
#include <deque>
#include <future>
#include <thread>

class Inhab										// описивает жителя
{
private:
    int k;
    bool res = false;							// в деревне ли?
    double active;
public:
    Inhab(int _k);
    bool isResult();
    void operation();
    void wait(std::future<bool> &, std::future<double>&);

    int comp = 0;
    int expired = 0;
};

class Report									// заявка
{
public:
    Report(std::promise<double>&, std::promise<bool>&);

    std::promise<double> &time;					// она возвращает будущий результат
    std::promise<bool> &result;
};

class Official									// чиновник
{
private:
    int k;
    double time;
    bool res = false;
public:
    Official(int _k);
    bool isResult();
    void work(Report&);

    int com = 0;								// выполненных
    int expired = 0;							// ожидание выполненных
};

#endif //LAB_4_RES_H
