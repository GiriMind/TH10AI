#pragma once

#include <atomic>

#include "Base/Type.h"

namespace base
{
	class RefCountedBase
	{
	public:
		RefCountedBase();
		virtual ~RefCountedBase() = default;

		void incStrongCount();
		void decStrongCount();
		uint_t getStrongCount() const;
		void incWeakCount();
		void decWeakCount();
		uint_t getWeakCount() const;

	private:
		virtual void destruct() noexcept = 0;
		virtual void deallocate() noexcept = 0;

		std::atomic<uint_t> m_strongCount;
		std::atomic<uint_t> m_weakCount;
	};

	template <typename T>
	class RefCounted :
		public RefCountedBase
	{
	public:
		RefCounted() :
			m_object(nullptr)
		{
		}

		void setObject(T* object)
		{
			m_object = object;
		}

	private:
		virtual void destruct() noexcept override
		{
			m_object->~T();
		}

		virtual void deallocate() noexcept override
		{
			this->~RefCounted();
			delete[] this;
		}

		T* m_object;
	};
}