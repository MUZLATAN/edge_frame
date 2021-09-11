#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <iostream>
#include <glog/logging.h>

namespace whale {
namespace vision {
template <typename M, typename T, typename K = std::string>
class ObjectManager : public std::unordered_map<K, T> {
 public:
	static M* GetInstance(void) {
		static M instance;
		return &instance;
	}

	static bool Find(const K& name) {
		auto manager = GetInstance();

		if (manager->count(name) == 0) return false;

		return true;
	}

	static T& Get(const K& name) {
		auto manager = GetInstance();

		return (*manager)[name];
	}

	static bool Get(const K& name, T& val) {
		auto manager = GetInstance();

		if (manager->count(name)) {
			val = manager->at(name);
			return true;
		}

		return false;
	}

	static bool Add(const std::string& name, const T& data) {
		auto manager = GetInstance();

		if (manager->count(name)) return false;

		(*manager)[name] = data;

		return true;
	}

	static bool Replace(const K& name, const T& data) {
		auto manager = GetInstance();

		if (manager->count(name) == 0) return false;

		T& old_data = (*manager)[name];
		FreeObject(old_data);

		(*manager)[name] = data;

		return true;
	}

	static bool Remove(const K& name) {
		auto manager = GetInstance();
		auto ir = (*manager).begin();
		auto end = (*manager).end();

		while (ir != end) {
			if (ir->first == name) {
				FreeObject(ir->second);
				manager->erase(ir);
				return true;
			}

			ir++;
		}

		return false;
	}

	static void StartSeqAccess(void) {
		auto manager = GetInstance();

		manager->ResetInternalIR();
	}

	static int GetNum(void) {
		auto manager = GetInstance();

		return manager->size();
	}

	static T GetSeqObj(void) {
		auto manager = GetInstance();

		return manager->GetNextObj();
	}

	template <typename U>
	static typename std::enable_if<std::is_pointer<U>::value, void>::type
	FreeObject(U& obj) {
		delete obj;
	}

	template <typename U>
	static typename std::enable_if<!std::is_pointer<U>::value, void>::type
	FreeObject(U& obj) {}

	virtual ~ObjectManager() {
		auto manager = GetInstance();
		auto ir = (*manager).begin();
		auto end = (*manager).end();

		while (ir != end) {
			FreeObject(ir->second);
			ir++;
		}
	}

	void ResetInternalIR(void) { seq_ir_ = this->begin(); }

	T GetNextObj(void) {
		T& ret = seq_ir_->second;
		LOG(INFO) << "key: " << seq_ir_->first ;

		seq_ir_++;

		return ret;
	}

 protected:
	typename std::unordered_map<K, T>::iterator seq_ir_;
};

template <typename M, typename T, typename K = std::string>
class SafeObjectManager : public ObjectManager<M, T, K> {
 public:
	static bool SafeFind(const K& name) {
		auto manager = ObjectManager<M, T>::GetInstance();

		manager->Lock();

		bool ret = true;

		if (manager->count(name) == 0) ret = false;

		manager->Unlock();

		return ret;
	}

	static bool SafeGet(const K& name, T& val) {
		auto manager = ObjectManager<M, T>::GetInstance();
		bool ret = true;

		manager->Lock();

		if (manager->count(name))
			val = (*manager)[name];
		else
			ret = false;

		manager->Unlock();

		return ret;
	}

	static bool SafeAdd(const K& name, const T& data) {
		auto manager = ObjectManager<M, T>::GetInstance();

		manager->Lock();

		if (manager->count(name)) {
			manager->Unlock();
			return false;
		}

		(*manager)[name] = data;

		manager->Unlock();

		return true;
	}

	static bool SafeReplace(const K& name, const T& data) {
		auto manager = ObjectManager<M, T>::GetInstance();

		manager->Lock();

		if (manager->count(name) == 0) {
			manager->Unlock();

			return false;
		}

		T& old_data = (*manager)[name];
		FreeObject(old_data);

		(*manager)[name] = data;

		manager->Unlock();

		return true;
	}

	static bool SafeRemove(const K& name) {
		auto manager = ObjectManager<M, T>::GetInstance();

		manager->Lock();

		auto ir = (*manager).begin();
		auto end = (*manager).end();

		while (ir != end) {
			if (ir->first == name) {
				ObjectManager<M, T>::FreeObject(ir->second);
				manager->erase(ir);

				manager->Unlock();
				return true;
			}

			ir++;
		}

		manager->Unlock();
		return false;
	}

	static bool SafeRemoveOnly(const K& name) {
		auto manager = ObjectManager<M, T>::GetInstance();

		manager->Lock();

		auto ir = (*manager).begin();
		auto end = (*manager).end();

		while (ir != end) {
			if (ir->first == name) {
				manager->erase(ir);
				manager->Unlock();
				return true;
			}

			ir++;
		}

		manager->Unlock();
		return false;
	}

	static void Get(void) {
		auto manager = ObjectManager<M, T>::GetInstance();

		manager->Lock();
	}

	static void Put(void) {
		auto manager = ObjectManager<M, T>::GetInstance();

		manager->Unlock();
	}

	~SafeObjectManager() {}

	void Lock(void) { my_mutex.lock(); }

	void Unlock(void) { my_mutex.unlock(); }

	static void Debug() {
		auto manager = ObjectManager<M, T>::GetInstance();
		auto ir = (*manager).begin();
		auto end = (*manager).end();

		while (ir != end) {
			LOG(INFO) << "key: " << ir->first << " ptr: " << ir->second ;
			ir++;
		}
	}

 private:
	std::mutex my_mutex;
};
}	 // namespace vision
}	 // namespace whale
