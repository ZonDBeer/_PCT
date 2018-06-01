unsigned const max_hazard_pointers = 100000;

struct hazard_pointer {
	std::atomic<std::thread::id> id;
	std::atomic<void*> pointer;
};

hazard_pointer hazard_pointers[max_hazard_pointers];

class hp_owner {
	hazard_pointer* hp;
public:
	hp_owner(hp_owner const&) = delete;
	hp_owner operator = (hp_owner const&) = delete;
	
	/* Если compare_exchange_strong возвращает false, значит, записью владеет
	   другой поток, поэтому мы идём дальше.
	*/
	/* Если compare_exchange_strong возвращает true, то мы успешно зарезервировали
	   запись для текущего, поэтому можем сохранить её адрес и прекратить поиск.
	*/
	// Если мы дошли до конца списка и не обнаружили свободной записи, то исключение
	hp_owner() : hp(nullptr) {
		for (unsigned i = 0; i < max_hazard_pointers; ++i) {
			std::thread::id old_id;
			// Пытаемся заявить права на владение указателем опасности 
			if(hazard_pointers[i].id.compare_exchange_strong(
				old_id, std::this_thread::get_id())) {
				hp = &hazard_pointers[i];
				break;	
			}
		}
		
		if(!hp) {
			throw std::runtime_error("No hazard pointers available");
		}
	}
	
	std::atomic<void*>& get_pointer() {
		return hp->pointer;
	}
	
	~hp_owner() {
		hp->pointer.store(nullptr);
		hp->id.store(std::thread::id());
	}
};

/* Работает это следующим образом: в первый раз, когда каждый поток вызывает
   эту функцию, создается новый экземпляр hp_owner. Его конструктор ищет в таблице пар
   незанятую запись. Последующие обращения происходят гораздо быстрее, потому что
   указатель запомнен и просматривать таблицу снова нет нужды
*/
std::atomic<void*>& get_hazard_pointer_for_current_thread() {
	// У каждого потока свой указатель опасности 
	thread_local static hp_owner hazard;
	// Возвращаем полученный от этого объекта указатель
	return hazard.get_pointer();
}

// ищет переданное значение в таблице указателей опасности 
bool outstanding_hazard_pointers_for(void* p) {
	for (unsigned i = 0; i < max_hazard_pointers; ++i) {
		if(hazard_pointers[i].pointer.load() == p) {
			return true;
		}
	}
	return false;
}

// Приводит тип void* к типу параметризированного 
// указателя, а затем удаляет объект, на который указывает.
template<typename T>
void do_delete(void* p) {
	delete static_cast<T*>(p);
}

struct data_to_reclaim {
	void* data;
	std::function<void(void*)> deleter;
	data_to_reclaim* next;
	
	template<typename T>
	data_to_reclaim(T* p) : 
		data(p),
		deleter(&do_delete<T>),
		next(0)
	{}
	
	~data_to_reclaim() {
		deleter(data);
	}
};

std::atomic<data_to_reclaim*> nodes_to_reclaim;
std::atomic<unsigned> hazard_counter;

void add_to_reclaim_list(data_to_reclaim* node) {
	node->next = nodes_to_reclaim.load();
	while(!nodes_to_reclaim.compare_exchange_weak(node->next, node));
	++hazard_counter; //std::cout << "hazard_counter =" << hazard_counter << "\n";
}

// Просто создаёт новый экземпляр data_to_reclaim для переданного
// указателя и добавляет его в список отложенного освобождения.
template<typename T>
void reclaim_later(T* data) {
	add_to_reclaim_list(new data_to_reclaim(data));
}


void delete_nodes_with_no_hazards() {
	if(hazard_counter < 2 * max_hazard_pointers) return;
	//std::cout << "Reached critical value, deleting pending nodes..." << std::endl;
	// Заявляем права на владение всем списком подлежащих освобоождению узлов
	data_to_reclaim* current = nodes_to_reclaim.exchange(nullptr);
	// Далее мы по очереди просматриваем все узлы списка, проверяя,
	// ссылаются ли на них не сброшенные указатели опасности. 
	while(current) {
		data_to_reclaim* const next = current->next;
		if(!outstanding_hazard_pointers_for(current->data)) {
			// Если нет, то запись удаляем
			//std::cout << "DELETE = " << current->data << "\n";
			delete current;
			--hazard_counter;
		} else {
			// В противном случае мы возвращаем элемент в список
			add_to_reclaim_list(current);
		}
		current = next;
	}
}
