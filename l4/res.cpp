#include "res.h"
/*
																					future обьек принимает то что возвращает поток чтобы отлавливать асинхронный поток метом гет
*/

Inhab::Inhab(int _k) : k(_k)							// конструктор иниц ранд знач от 0 до 1
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);
    
    active = dis(gen);									// активность жителя
}

void Inhab::operation()									
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::exponential_distribution<> d(active);
    std::this_thread::sleep_for(std::chrono::seconds((unsigned)std::round(d(gen))));
}

bool Inhab::isResult()									// присутствие
{
    return !res;
}

void Inhab::wait(std::future<bool> &result, std::future<double> &time)		// ожидание выполнения заявки
{
    std::this_thread::sleep_for(std::chrono::seconds((unsigned)std::round(time.get())));	

    bool status = result.get();							// если вып заявка то ув число выполненных

    if (status)
        ++comp;
    else
    {
        ++expired;

        if ((expired - comp) > k)
            res = true;
    }
}

Official::Official(int _k) : k(_k)						// время вып заявки
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(1, 10);

    time = dis(gen);
}

void Official::work(Report &task)						// работает по врем от 0 до 2т
{
    task.time.set_value(time);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 2 * time);

    double U = dis(gen);								// генерем время у

    if (U > time)
    {
        task.result.set_value(false);					// уст знач результата
        std::this_thread::sleep_for(std::chrono::seconds((unsigned)std::round(U)));		// спим у секунд

        ++expired;

        if ((expired - com) > k)
            res = true;
    }
    else
    {
        task.result.set_value(true);
        std::this_thread::sleep_for(std::chrono::seconds((unsigned)std::round(U)));
        ++com;
    }
}

bool Official::isResult()
{
    return !res;
}

Report::Report(std::promise<double> &ptime, std::promise<bool> &presult) : time(ptime), result(presult) { }

