#ifndef KVFIFO_H
#define KVFIFO_H

#include <map>
#include <list>
#include <iterator> 	// For std::forward_iterator_tag
#include <cstddef>  	// For std::ptrdiff_t
#include <memory>   	// For std::shared_ptr
#include <stdexcept>	// For std::invalid_argument
#include <iostream>		// For debug

// Przy wykonywaniu zmiany kolejki, patrzymy ile obiektów patrzy na nasz shared pointer;
// Jeśli patrzy więcej niż 1 obiekt, to musimy wykonać głęboką kopię przed zmianą.

//todo pomocnicza struktura informująca czy sami jestesmy właścicielami struktury,
//czy przed zmianą musimy zrobić głęboką copię ....
// Klasa kvfifo powinna udostępniać niżej opisane operacje. Przy każdej operacji
// podana jest jej oczekiwana złożoność czasowa przy założeniu, że nie trzeba
// wykonywać kopii. Oczekiwana złożoność czasowa operacji kopiowania przy zapisie
// wynosi O(n log n), gdzie n oznacza liczbę elementów przechowywanych w kolejce.
// Wszystkie operacje muszą zapewniać co najmniej silną odporność na wyjątki,
// todo konstruktor przenoszący i destruktor muszą być no-throw.

template <typename K, typename V> class kvfifo {
private:
	using data_list_t = std::list<std::pair<const K, V>>;
	using data_map_value_t = std::list<typename data_list_t::iterator>;


	// Specjalna klasa służąca do obsługi wyjątków, schemat użycia jest
	// następujący:
	// Na początku wywołania każdej funkcji która może się nie powieść tworzymy obiekt
	// exception_guard g z domyślnie zdefiniowanym parametrem failure jako true.
	// Jeśli podczas wykonywania funkcji zostanie zgłoszony wyjątek, to wykonanie
	// zostanie wstrzymane i zostanie zawołany destruktor obiektu g, który
	// zrobi undo ewentualnie zrobionej / zaczętej i nieudanej głębokiej kopii obiektu.
	// Jeśli wszystko się udało, to na koniec funkcji wołamy metodę
	// g.no_failure(), która ustawia parametr failure na false.
	class exception_guard {
		public:
			exception_guard(kvfifo *guarded_kvfifo) :
			guarded_kvfifo(guarded_kvfifo),
			guarded_queue_data(guarded_kvfifo->queue_data),
			guarded_unshareable(guarded_kvfifo->unshareable) {}

			~exception_guard() noexcept {
				if (failure) {
					// Undo deep copy:
					guarded_kvfifo->queue_data = guarded_queue_data;
					guarded_kvfifo->unshareable = guarded_unshareable;
				}
			}

			void no_failure() {
				failure = false;
			}


		private:
			kvfifo* guarded_kvfifo;
			std::shared_ptr<struct data> guarded_queue_data;
			bool guarded_unshareable;
			bool failure = true;
	};

	struct data {
		//mapa klucz, lista iteratorów na pozycje z elements w których, te klucze, value występują.
		std::map<const K, data_map_value_t> map;

		//lista dwukierunkowa z elementami
		data_list_t elements;

		// Use default empty object constructor:
		data() = default;

		// Deep Copy Constructor:
		data(struct data const& other) {
			data_list_t new_elements;
			std::map<const K, data_map_value_t> new_map;

			for (auto it = other.elements.begin(); it != other.elements.end(); ++it) {
				new_elements.push_back({it->first, it->second});
			}

			for (auto it = new_elements.begin(); it != new_elements.end(); ++it) {
				auto map_pos = new_map.find(it->first);
				if (map_pos == new_map.end()) {
					data_map_value_t new_list;
					new_list.push_back(it);
					new_map.emplace(it->first, new_list);
				} else
					map_pos->second.push_back(it);
			}

			std::swap(map, new_map);
			std::swap(elements, new_elements);
		}

		// Use default destructor:
		~data() noexcept = default;
	};

	//ZDECYDOWANIE POPRAW!!!
	void make_free_to_edit() {
		if (queue_data.use_count() > 1) {
			//sprawdz czy mamy wystarczającą pamieć?
			void* memory_check = malloc(2 * sizeof(queue_data));
			if (memory_check == NULL)
				throw std::bad_alloc("Not enough memory"); //to wydadałoby naprawic..
			else {
				free(memory_check);
				std::shared_ptr<data> new_ptr = std::make_shared<data>(*queue_data);
				queue_data = new_ptr;
			}
		}
	}

	// void free_empty_map_node(K deleted_key) {
	// 	if (queue_data->map.find(deleted_key)->empty()) {
	// 		delete queue_data->map.find(deleted_key);
	// 		queue_data->map.erase(deleted_key);
	// 	}
	// }

	// Wołamy tą funkcję gdy tylko będziemy zwracać niestałą referencję
	// do naszej kolejki: about_to_modify(true)
	// lub przy modyfikacji kolejki: about_to_modify(false)
	// Funkcja sprawdza, czy głęboka kopia jest konieczna
	// i aktualizuje atrybut unshareable.
	// Źródło: http://www.gotw.ca/gotw/044.htm
	// Umawiamy się że domyślnie kolejka po modyfikacji będzie shareable
	// (np. operacje pop, push)
	// Ale czasami będzie unshareable, np gdy damy referncję metodą front().
	void about_to_modify(bool copy_is_unshareable = false) {
		// Głębokiej kopii **nie robimy** wtw gdy:
		// Jesteśmy unshareable, bo to znaczy że mamy wyłączność na queue_data
		// Jesteśmy shareable, ale mamy wyłączność na wskaźnik.

		if (!(queue_data.unique()) && !unshareable) {
			// Zrób deep copy:
			// std::cout<<"deep copy";
			queue_data = std::make_shared<data>(*queue_data);
		}
		// std::cout<<std::endl;
		// W przeciwnym razie możemy zmodyfikować nasze queue_data.
		// Aktualizuj atrybut unshareable:
		unshareable = copy_is_unshareable;
	}



public:
	// DEBUG:
	void print_kvfifo() {
		for (auto x : queue_data->elements)
			std::cout << "(" << x.first << " " << x.second << ") ";
		std::cout << "MAP: ";
		for (auto x : queue_data->map) {
			for (auto y : x.second)
				std::cout << "(" << y->first << " " << y->second << ") ";
		}
		std::cout << "\n";
	}

	bool unshareable = false;
	std::shared_ptr<struct data> queue_data;

	// Constructors:
	kvfifo() {
		//std::allocate_shared, std::allocate_shared_for_overwrite moze trzeba??
		std::cout<<"empty constructor called: ";
		queue_data = std::make_shared<data>();
		// std::cout<<queue_data.use_count()<<"\n";
	}

	kvfifo(kvfifo const& other) {
		// std::cout << "copy\n";
		std::cout<<"copy constructor called: ";
		// Jeśli możliwe, nie rób deep copy:
		if (!other.unshareable) {
			// std::cout << "no deep copy: ";
			queue_data = other.queue_data;
		} else {
			// std::cout << "deep copy: ";
			queue_data = std::make_shared<data>(*(other.queue_data));
		}
		// std::cout<<"this: "<<queue_data.use_count()<<", other: "<< other.queue_data.use_count()<<"\n";
	}

	explicit kvfifo(kvfifo&& other) noexcept {
		std::cout << "move\n";

		queue_data = other.queue_data;
		other.queue_data = nullptr; //albo NULL
	}

	// Operators:
	kvfifo& operator=(kvfifo other) noexcept {
		std::cout<<"operator= called: ";
		// Jeśli możliwe, nie rób deep copy:
		if (!other.unshareable) {
			// std::cout<<"no deep copy ";
			queue_data = other.queue_data;
		} else {
			// std::cout<<"deep copy";
			queue_data = std::make_shared<data>(*(other.queue_data));
		}
		// Na pewno będziemy shareable:
		unshareable = false;
		// std::cout<<"this: "<<queue_data.use_count()<<", other: "<< other.queue_data.use_count()<<"\n";
		return *this;
	}

	// Methods:

	// Wstawia wartość v na koniec kolejki, nadając jej klucz k. Złożoność O(log n).
	void push(K const& k, V const& v) {
		// Zmodyfikujemy, ale potem będziemy shareable (bo referencje się
		// unieważniają). Chyba że nie będzie pamięci :DDDDD
		// std::cout<<"push call: "<<queue_data.use_count()<<" ";
		about_to_modify();

		queue_data->elements.push_back({k, v});
		if (!queue_data->map.contains(k))
			queue_data->map.emplace(k, data_map_value_t());
		// else std::cout << "push: kluczbyl\n";
		auto iter = queue_data->map.find(k);
		iter->second.push_back(std::prev(queue_data->elements.end()));
	}

	// Usuwa pierwszy element z kolejki. Jeśli kolejka jest pusta, to podnosi wyjątek std::invalid_argument. Złożoność O(log n).
	void pop() {
		// std::cout<<"pop() call. Before: ";
		if (empty()) throw std::invalid_argument("Empty queue");
		about_to_modify();

		K deleted_key = queue_data->elements.front().first;
		queue_data->map.find(deleted_key)->second.pop_front();
		if (queue_data->map.find(deleted_key)->second.empty()) {
			auto iter = queue_data->map.find(deleted_key);
			queue_data->map.erase(iter);
		}
		queue_data->elements.pop_front();

	}

	// Usuwa pierwszy element o podanym kluczu z kolejki. Jeśli podanego klucza
	// nie ma w kolejce, to podnosi wyjątek std::invalid_argument. Złożoność O(log n).
	void pop(K const& x) {
		// std::cout<<"pop(K) call ";
		if (count(x) == 0) throw std::invalid_argument("No matching key");

		about_to_modify();

		auto iter = queue_data->map.find(x);
		queue_data->elements.erase(iter->second.front());
		iter->second.pop_front();

		if (iter->second.size() == 0)
			queue_data->map.erase(x);

	}

	// Przesuwa elementy o kluczu k na koniec kolejki, zachowując
	// ich kolejność względem siebie. Zgłasza wyjątek std::invalid_argument, gdy
	// elementu o podanym kluczu nie ma w kolejce. Złożoność O(m + log n), gdzie m to
	// liczba przesuwanych elementów.
	void move_to_back(K const& k) {
		// std::cout<<"move_to_back call ";
		size_t how_many = count(k);
		if (how_many == 0) throw std::invalid_argument("No matching key");

		about_to_modify();

		// iterator w mapie.
		auto iter = queue_data->map.find(k);
		for (size_t i = 0; i < how_many; i++) {
			queue_data->elements.erase(iter->second.front());
			queue_data->elements.push_back({iter->first, iter->second.front()->second});
			iter->second.push_back(std::prev(queue_data->elements.end()));
			iter->second.pop_front();
		}
	}

	// Zwraca parę referencji do klucza i wartości znajdującej na początku / końcu kolejki.
	// W wersji nie-const zwrócona para powinna umożliwiać modyfikowanie wartości, ale nie klucza. Dowolna operacja
	// modyfikująca kolejkę może unieważnić zwrócone referencje. Jeśli kolejka jest
	// pusta, to podnosi wyjątek std::invalid_argument. Złożoność O(1).
	std::pair<K const&, V&> front() {
		if (empty()) throw std::invalid_argument("Empty queue");

		about_to_modify(true);
		// return queue_data->elements.front();
		return {queue_data->elements.front().first, queue_data->elements.front().second};
	}

	std::pair<K const&, V const&> front() const {
		if (empty()) throw std::invalid_argument("Empty queue");
		return queue_data->elements.front();
	}

	std::pair<K const&, V&> back() {
		if (empty()) throw std::invalid_argument("Empty queue");

		about_to_modify(true);

		return {queue_data->elements.back().first, queue_data->elements.back().second};
	}

	std::pair<K const&, V const&> back() const {
		if (empty()) throw std::invalid_argument("Empty queue");
		return queue_data->elements.back();
	}

	// Metody first i last zwracają odpowiednio pierwszą i ostatnią parę
	// klucz-wartość o danym kluczu, podobnie jak front i back. Jeśli podanego klucza
	// nie ma w kolejce, to podnosi wyjątek std::invalid_argument. Złożoność O(log n).
	std::pair<K const&, V&> first(K const& key) {
		if (count(key) == 0) throw std::invalid_argument("No matching key");

		about_to_modify(true);

		auto iter = queue_data->map.find(key);
		auto list_iter = iter->second.front();
		std::pair<K const&, V&> res =
		{list_iter->first, list_iter->second};

		return res;
	}

	std::pair<K const&, V const&> first(K const& key) const {
		if (count(key) == 0) throw std::invalid_argument("No matching key");
		auto iter = queue_data->map.find(key);
		auto list_iter = iter->second.front();
		std::pair<K const&, V const&> res =
		{list_iter->first, list_iter->second};

		return res;
	}

	std::pair<K const&, V&> last(K const& key) {
		if (count(key) == 0) throw std::invalid_argument("No matching key");

		about_to_modify(true);

		auto iter = queue_data->map.find(key);
		auto list_iter = iter->second.back();
		std::pair<K const&, V&> res =
		{list_iter->first, list_iter->second};

		return res;
	}

	std::pair<K const&, V const&> last(K const& key) const {
		if (count(key) == 0) throw std::invalid_argument("No matching key");
		auto iter = queue_data->map.find(key);
		auto list_iter = iter->second.back();

		std::pair<K const&, V const&> res =
		{list_iter->first, list_iter->second};

		return res;
	}


	// Zwraca liczbę elementów w kolejce.
	size_t size() const {
		return queue_data->elements.size();
	}

	// Zwraca true, gdy kolejka jest pusta, a false w przeciwnym przypadku.
	bool empty() const {
		return  queue_data->elements.empty();
	}

	// Zwraca liczbę elementów w kolejce o podanym kluczu.
	size_t count(K const& k) const {
		if (!queue_data->map.contains(k)) return 0;
		return queue_data->map.find(k)->second.size();
	}

	// Usuwa wszystkie elementy z kolejki. Złożoność O(n).
	void clear() {
		about_to_modify();
		queue_data->map.clear();
		queue_data->elements.clear();
	}

	// TODO: Iterators:
	// Iterator k_iterator oraz metody k_begin i k_end, pozwalające przeglądać zbiór
	// kluczy w rosnącej kolejności wartości. Iteratory mogą być unieważnione przez
	// dowolną zakończoną powodzeniem operację modyfikującą kolejkę oraz operacje
	// front, back, first i last w wersjach bez const. Iterator musi spełniać koncept
	// std::bidirectional_iterator. Wszelkie operacje w czasie O(log n). Przeglądanie
	// całej kolejki w czasie O(n). Iterator służy jedynie do przeglądania kolejki
	// i za jego pomocą nie można modyfikować kolejki, więc zachowuje się jak
	// const_iterator z biblioteki standardowej.

	// Trzeba się zastanowić nad tym co dokładnie przechowuje k_iterator
	// i jak wygląda inkrementacja. Na razie niech będzie to wrapper na zwykly
	// std::map<K, std::list <std::list::Iterator>>::Iterator.
	struct k_iterator {
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type   = std::ptrdiff_t;
		using map_iter_t 		= typename
		                          std::map<const K, std::list<typename std::list<std::pair<const K, V>>::iterator>>::iterator;

		// Constructor
		k_iterator(map_iter_t iter) : iter(iter) {}

		// Bidirectional const iterator's methods:

		// Dereference 1:
		const K& operator*() const {
			return iter->first;
		}

		// Dereference 2:
		const K* operator->() const {
			return &(iter->first);
		}

		// Prefix increment:
		k_iterator& operator++() {
			iter = std::next(iter);
			return *this;
		}

		// Postfix increment (int is a dummy argument, just to
		// differentiate ++iter from iter++)
		k_iterator operator++(int) {
			k_iterator tmp = *this;
			++(*this);
			return tmp;
		}

		// Prefix decrement:
		k_iterator& operator--() {
			iter = std::prev(iter);
			return *this;
		}

		// Postfix decrement:
		k_iterator operator--(int) {
			k_iterator tmp = *this;
			--(*this);
			return tmp;
		}

		// Comparison:
		bool operator==(const k_iterator& b) {
			return iter == b.iter;
		}

		bool operator!=(const k_iterator& b) {
			return iter != b.iter;
		}
	private:
		map_iter_t iter;
	};

	k_iterator k_begin() const {
		return k_iterator(queue_data->map.begin());
	}

	k_iterator k_end() const {
		return k_iterator(queue_data->map.end());
	}

};


// Tam gdzie jest to możliwe i uzasadnione należy opatrzyć metody kwalifikatorami const i noexcept.
// Klasa kvfifo powinna być przezroczysta na wyjątki, czyli powinna przepuszczać
// wszelkie wyjątki zgłaszane przez wywoływane przez nią funkcje i przez operacje
// na jej składowych, a obserwowalny stan obiektów nie powinien się zmienić.
// W szczególności operacje modyfikujące zakończone niepowodzeniem nie powinny
// unieważniać iteratorów.

#endif // KVFIFO_H