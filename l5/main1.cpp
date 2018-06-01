#include <iostream>
#include <atomic>
#include <thread>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <vector>

#define NUMBER 100000
#define THREAD 4

unsigned const max_hazard_pointers = 100000;

struct hazard_pointer
{
    std::atomic<std::thread::id> id;
    std::atomic<void*> pointer;
};

hazard_pointer hazard_pointers[max_hazard_pointers];

class hp_owner  // определяем владельца указателя
{
private:
    hazard_pointer *hp;
public:
    hp_owner(hp_owner const &) = delete;
    hp_owner operator = (hp_owner const&) = delete;

    hp_owner() : hp(nullptr)
    {
        for (unsigned i = 0; i < max_hazard_pointers; ++i)
        {
            std::thread::id old_id;

            if (hazard_pointers[i].id.compare_exchange_strong(
                    old_id, std::this_thread::get_id()))
            {
                hp = &hazard_pointers[i];
                break;
            }
        }

        if (!hp)
        {
            throw std::runtime_error("No hazard pointers available");
        }
    }

    std::atomic<void*> &get_pointer()			// вернуть адрес
    {
        return hp->pointer;
    }

    ~hp_owner()
    {
        hp->pointer.store(nullptr);
        hp->id.store(std::thread::id());
    }
};

std::atomic<void*> &get_hazard_pointer_for_current_thread()					// создаем обьект
{
    thread_local static hp_owner hazard;

    return hazard.get_pointer();
}

bool outstanding_hazard_pointers_for(void *p)		// есть ли адрес в массиве
{
    for (unsigned i = 0; i < max_hazard_pointers; ++i)
    {
        if (hazard_pointers[i].pointer.load() == p)
            return true;
    }

    return false;
}

template <typename T> void do_delete(void *p)				// удаляет узел из стека
{
    delete static_cast<T*>(p);
}

struct data_to_reclaim												// список для удаления
{
    void *data;
    std::function<void(void*)> deleter;
    data_to_reclaim *next;

    template <typename T> data_to_reclaim(T *p) : data(p), deleter(&do_delete<T>),
                                                  next(0)
    { }

    ~data_to_reclaim()
    {
        deleter(data);
    }
};

std::atomic<data_to_reclaim*> nodes_to_reclaim;  // список
std::atomic<unsigned> hazard_counter;

void add_to_reclaim_list(data_to_reclaim *node)
{
    node->next = nodes_to_reclaim.load();

    while (!nodes_to_reclaim.compare_exchange_weak(node->next, node));

    ++hazard_counter;
}

template <typename T> void reclaim_later(T *data)  				// добавляет всписок с указателем
{
    add_to_reclaim_list(new data_to_reclaim(data));
}

void delete_nodes_with_no_hazards()// удалить узлы указателей опасности
{
    if (hazard_counter < 2 * max_hazard_pointers) return;

    data_to_reclaim *current = nodes_to_reclaim.exchange(nullptr);

    while (current)
    {
        data_to_reclaim *const next = current->next;

        if (outstanding_hazard_pointers_for(current->data))
        {
            delete current;
            --hazard_counter;
        }
        else
        {
            add_to_reclaim_list(current);
        }
        current = next;
    }
}

template <typename T> class lock_free_stack				// 
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        node *next;

        node(T const &data_) : data(std::make_shared<T>(data_)) { }
    };

    std::atomic<node*> head;
public:
    void push(T const &data)
    {
        node *const new_node = new node(data);
        new_node->next = head.load(std::memory_order_relaxed);		// создает новую голову

        while (!head.compare_exchange_weak(new_node->next, new_node, // пытаемся запихать голову в стэк
        std::memory_order_release, std::memory_order_relaxed));   //compare_exchange_weak  сравнивает
    }

    std::shared_ptr<T> pop()					// 
    {
        std::atomic<void*> &hp = get_hazard_pointer_for_current_thread(); // получ указатель на список удаления
        node *old_head = head.load(std::memory_order_relaxed);

        do
        {
            node *temp;

            do
            {
                temp = old_head;					// проблема аба
                hp.store(old_head);
                old_head = head.load();
            }
            while (old_head != temp);
        }
        while (old_head && !head.compare_exchange_strong(old_head, old_head->next,
        std::memory_order_acquire, std::memory_order_relaxed));  

        hp.store(nullptr);
        std::shared_ptr<T> res;

        if (old_head)
        {
            res.swap(old_head->data);

            if (outstanding_hazard_pointers_for(old_head))
            {
                reclaim_later(old_head);
            }
            else
            {
                delete old_head;
            }
            delete_nodes_with_no_hazards();
        }

        return res;
    }
};


lock_free_stack<int> stack;
unsigned seed = time(nullptr);
std::vector<std::thread> PUSH(THREAD);
std::vector<std::thread> POP(THREAD);

void pusher()
{
    for (int i = 0; i < NUMBER; ++i)
    {
        auto span = rand_r(&seed) % 120 + 1;
        stack.push(span);
    }
}

void poper()
{
    for (auto i = 0; i < NUMBER; ++i)
    {
        std::shared_ptr<int> temp = stack.pop();
    }
}


int main()
{
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    start = std::chrono::high_resolution_clock::now();

    for (auto &thr : PUSH) thr = std::thread(pusher);
    for (auto &thr : PUSH) thr.join();

    end = std::chrono::high_resolution_clock::now();

    std::cout << "PUSH const time : "
    << std::chrono::duration_cast<std::chrono::microseconds> (end - start).count()
            << " (ms)" << std::endl;

    start = std::chrono::high_resolution_clock::now();

    for (auto &thr : POP) thr = std::thread(poper);
    for (auto &thr : POP) thr.join();

    end = std::chrono::high_resolution_clock::now();

    std::cout << "POP const time : "
    << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
            << " (ms)" << std::endl;

    return 0;
}

























