template<typename T>
class lock_free_stack {

private:
	struct node {
		std::shared_ptr<T> data;
		node* next;
		
		node(T const& data_) : 
			data(std::make_shared<T>(data_))
		{}
	};
	
	std::atomic<node*> head;
	
public:
	void push(T const& data) {
		node* const new_node = new node(data);
		new_node->next = head.load(std::memory_order_relaxed);
		
		while(!head.compare_exchange_weak(new_node->next, new_node,
			std::memory_order_release, std::memory_order_relaxed));
	}
	
	std::shared_ptr<T> pop() {
		// Возвращаем ссылку на указатель опасности
		std::atomic<void*>& hp = get_hazard_pointer_for_current_thread();
		// получаем голову узла
		node* old_head = head.load(std::memory_order_relaxed);
		do {
			node* temp;
			// Цикл, пока указатель опасности не установлен на head
			do {
				temp = old_head;
				hp.store(old_head);
				old_head = head.load();
			} while(old_head != temp);
		} while(old_head && !head.compare_exchange_strong(old_head, old_head->next,
				std::memory_order_acquire, std::memory_order_relaxed));
		
		hp.store(nullptr); //закончив, очищаем указатель опасности  
		std::shared_ptr<T> res;
		
		if(old_head) {
			res.swap(old_head->data);
			// Прежде чем удалять узел, проверяем, нет ли ссылающихся 
			// на него указателей пасности
			if(outstanding_hazard_pointers_for(old_head)) {
				// Если удалять узел нельзя, то помещаем его в список ожидающих
				reclaim_later(old_head);
			} else {
				// В противном случае узел можно удалять немедленно
				delete old_head;
			}
			// После достижения hazard_counter < 2 * max_hazard_pointers
			delete_nodes_with_no_hazards();
		}
		
		return res;
	}
};
