#pragma once

#include <corona/threading/EventBus.h>
#include <corona/threading/SafeDataCache.h>

#include <memory>
#include <shared_mutex>
#include <typeindex>
#include <unordered_map>

namespace Corona {

// 线程安全的数据缓存中心，按类型惰性生成 SafeDataCache<T>
class DataCacheHub {
  public:
	struct IHolder {
		virtual ~IHolder() = default;
	};
	template <typename T>
	struct Holder : IHolder {
		SafeDataCache<T> cache;
	};

	template <typename T>
	SafeDataCache<T> &get()
	{
		const std::type_index key{typeid(T)};
		{
			std::shared_lock lock(mutex_);
			if (auto iter = caches_.find(key); iter != caches_.end()) {
				return static_cast<Holder<T> &>(*iter->second).cache;
			}
		}
		std::unique_lock lock(mutex_);
		if (auto iter = caches_.find(key); iter != caches_.end()) {
			return static_cast<Holder<T> &>(*iter->second).cache;
		}
		auto holder = std::make_unique<Holder<T>>();
		auto *raw = holder.get();
		caches_.emplace(key, std::move(holder));
		return raw->cache;
	}

  private:
	std::unordered_map<std::type_index, std::unique_ptr<IHolder>> caches_{};
	std::shared_mutex mutex_{};
};

// 线程安全的事件总线中心，按负载类型创建 EventBusT<T>
class EventBusHub {
  public:
	struct IHolder {
		virtual ~IHolder() = default;
	};
	template <typename T>
	struct Holder : IHolder {
		EventBusT<T> bus;
	};

	template <typename T>
	EventBusT<T> &get()
	{
		const std::type_index key{typeid(T)};
		{
			std::shared_lock lock(mutex_);
			if (auto iter = buses_.find(key); iter != buses_.end()) {
				return static_cast<Holder<T> &>(*iter->second).bus;
			}
		}
		std::unique_lock lock(mutex_);
		if (auto iter = buses_.find(key); iter != buses_.end()) {
			return static_cast<Holder<T> &>(*iter->second).bus;
		}
		auto holder = std::make_unique<Holder<T>>();
		auto *raw = holder.get();
		buses_.emplace(key, std::move(holder));
		return raw->bus;
	}

  private:
	std::unordered_map<std::type_index, std::unique_ptr<IHolder>> buses_{};
	std::shared_mutex mutex_{};
};

} // namespace Corona
