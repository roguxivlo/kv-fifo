#ifndef KVFIFO_H
#define KVFIFO_H

#include <map>
#include <list>
#include <iterator> // For std::forward_iterator_tag
#include <cstddef>  // For std::ptrdiff_t


struct data {
	list;
	map;
};

class kvfifo {
	std::shared_ptr<data> queue_data;
	//constructor:
	void foo(kvfifo fifo) {
		auto newqueue_data = std::makeshared<data>(*fifo->queue_data);
	}
};

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
	requires...
private:

	struct data {
		//mapa klucz, lista iterratorów na pozycje z elements w których, te klucze, value występują.
		std::map<K, std::list <std::list::Iterator>> map;
		//lista dwukierunkowa z elementami
		std::list<pair<K, V>> elements;
		data() = default;
		data(struct data const& other) {
			map(other.map);
			list(other.list);
		}
	};

	//ZDECYDOWANIE POPRAW!!!
	void make_free_to_edit() {
		if (queue_data.use_count > 1) {
			//sprawdz czy mamy wystarczającą pamieć?
			std::shared_ptr<data> new_ptr = std::make_shared<data>(*queue_data);
			queue_data = new_ptr;
		}
	}

	// void free_empty_map_node(K deleted_key) {
	// 	if (queue_data->map.find(deleted_key)->empty()) {
	// 		delete queue_data->map.find(deleted_key);
	// 		queue_data->map.erase(deleted_key);
	// 	}
	// }

public:
	std::shared_ptr<struct data> queue_data;

	// Constructors:
	kvfifo() {
		//std::allocate_shared, std::allocate_shared_for_overwrite moze trzeba??
		std::shared_ptr<data> queue_data = std::make_shared<data>();
	}

	kvfifo(kvfifo const& other) {
		queue_data = other.queue_data;
	}

	kvfifo(kvfifo&& other) {
		queue_data = other.queue_data;
		other.queue_data = nullptr; //albo NULL
	}

	kvfifo(kvfifo const& other, bool deep_copy) {
		if (deep_copy) {

		}
	}

	// Operators:
	kvfifo& operator=(kvfifo other) {
		queue_data = other.queue_data;
		return *this;
	}

	// Methods:

	// Wstawia wartość v na koniec kolejki, nadając jej klucz k. Złożoność O(log n).
	void push(K const& k, V const& v) {
		make_free_to_edit();
		queue_data->elements.push(std::make_pair(k, v));
		if (!queue_data->map.contains(k))
			queue_data->map.emplace(k, std::list<std::list::Iterator>());
		queue_data->map.find(k)->push_back(std::prev(queue_data->elements.end()));
	}

	// Usuwa pierwszy element z kolejki. Jeśli kolejka jest pusta, to podnosi wyjątek std::invalid_argument. Złożoność O(log n).
	void pop() {
		if (empty()) throw(std::invalid_argument());
		make_free_to_edit();

		K deleted_key = queue_data->elements.begin()->first;
		queue_data->map.find(deleted_key)->pop_front();
		if (queue_data->map.find(deleted_key)->empty()) {
			delete queue_data->map.find(deleted_key);
			queue_data->map.erase(deleted_key);
		}
		queue_data->elements.pop_front();
	}

	// Usuwa pierwszy element o podanym kluczu z kolejki. Jeśli podanego klucza
	// nie ma w kolejce, to podnosi wyjątek std::invalid_argument. Złożoność O(log n).
	void pop(K const& x) {
		if (count(x) == 0) throw(std::invalid_argument());
		make_free_to_edit();
		auto iter = queue_data->map.find(x);
		queue_data->list.erase(iter->second.front());
		iter->second.pop_front();

		if (iter->second.size() == 0)
			map.erase(x);

	}

	// Przesuwa elementy o kluczu k na koniec kolejki, zachowując
	// ich kolejność względem siebie. Zgłasza wyjątek std::invalid_argument, gdy
	// elementu o podanym kluczu nie ma w kolejce. Złożoność O(m + log n), gdzie m to
	// liczba przesuwanych elementów.
	void move_to_back(K const& k) {
		size_t how_many = count(k);
		if (how_many == 0) throw(std::invalid_argument());
		make_free_to_edit();

		auto iter = queue_data->map.find(x);
		for (size_t i = 0; i < how_many; i++) {
			queue_data->list.erase(iter->second.front());
			queue_data->list.push_back(std::make_pair(iter->first, iter->second.front()));
			iter->second.push_back(std::prev(queue_data->list.end()));
			iter->second.pop_front();
		}
	}

	// Zwraca parę referencji do klucza i wartości znajdującej na początku / końcu kolejki.
	// W wersji nie-const zwrócona para powinna umożliwiać modyfikowanie wartości, ale nie klucza. Dowolna operacja
	// modyfikująca kolejkę może unieważnić zwrócone referencje. Jeśli kolejka jest
	// pusta, to podnosi wyjątek std::invalid_argument. Złożoność O(1).
	std::pair<K const&, V&> front() {

	}

	std::pair<K const&, V const&> front() const {
		if (this.empty()) throw(std::invalid_argument());
		return queue_data->list.front();
	}

	std::pair<K const&, V&> back();

	std::pair<K const&, V const&> back() const {
		if (this.empty()) throw(std::invalid_argument());
		return make_pair((--elements.end())->first, (--elements.end())->second);
	}

	// Metody first i last zwracają odpowiednio pierwszą i ostatnią parę
	// klucz-wartość o danym kluczu, podobnie jak front i back. Jeśli podanego klucza
	// nie ma w kolejce, to podnosi wyjątek std::invalid_argument. Złożoność O(log n).
	std::pair<K const&, V&> first(K const& key);

	std::pair<K const&, V const&> first(K const& key) const {
		if (this.empty()) throw(std::invalid_argument());
		auto iter = map.find(key);

		std::pair<K const&, V const&> res =
		    make_pair(iter->begin()->first, iter->begin()->second);

		return res;
	}

	std::pair<K const&, V&> last(K const& key);

	std::pair<K const&, V const&> last(K const& key) const {
		if (this.empty()) throw(std::invalid_argument());
		auto iter = map.find(key);

		std::pair<K const&, V const&> res =
		    make_pair((--iter->end())->first, (--iter->end())->second);

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
	size_t count(K const&) const {
		if (!queue_data->map.contains(K)) return 0;
		queue_data->map.find(K)->size();
	}

	// Usuwa wszystkie elementy z kolejki. Złożoność O(n).
	void clear();

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
		using map_iter_t 		= std::map<K, std::list <std::list::Iterator>>;

		// Constructor
		Iterator(map_iter_t ptr) : m_ptr(ptr) {}

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

	k_iterator begin() const {
		return k_iterator(map.begin());
	}

	k_iterator end() const {
		return k_iterator(map.end());
	}

};


// Tam gdzie jest to możliwe i uzasadnione należy opatrzyć metody kwalifikatorami const i noexcept.
// Klasa kvfifo powinna być przezroczysta na wyjątki, czyli powinna przepuszczać
// wszelkie wyjątki zgłaszane przez wywoływane przez nią funkcje i przez operacje
// na jej składowych, a obserwowalny stan obiektów nie powinien się zmienić.
// W szczególności operacje modyfikujące zakończone niepowodzeniem nie powinny
// unieważniać iteratorów.

#endif // KVFIFO_H
