/*
 * MutexLocker.h
 *
 *  Created on: Sep 4, 2021
 *      Author: sbela
 */

#ifndef MUTEXLOCKER_H_
#define MUTEXLOCKER_H_

#include "semphr.h"
#include "console.h"

#define LIKELY(expr)    __builtin_expect(!!(expr), true)

namespace AS
{

class MutexLocker final
{
public:
	inline explicit MutexLocker(const SemaphoreHandle_t m)
	{
		assert((reinterpret_cast<uint32_t>(m) & uint32_t(1u)) == uint32_t(0));
		m_ptr = uint32_t(m);
		if (LIKELY(m))
		{
			 if (xSemaphoreTake(m, (TickType_t)10) == pdTRUE)
			 {
				 m_ptr |= uint32_t(1u);
				 HPrintf("{LOCK[%d]}", m_ptr);
			 }
		}
	}

	inline ~MutexLocker() { unlock(); }

	inline void unlock()
	{
		if ((m_ptr & uint32_t(1u)) == uint32_t(1u))
		{
			m_ptr &= ~uint32_t(1u);
			xSemaphoreGive(mutex());
			HPrintf("{UNLOCK[%d]}", m_ptr);
		}
	}

	inline void relock()
	{
		if (m_ptr)
		{
			if ((m_ptr & uint32_t(1u)) == uint32_t(0u))
			{
				 if (xSemaphoreTake(mutex(), (TickType_t)10) == pdTRUE)
					 m_ptr |= uint32_t(1u);
			}
		}
	}

	inline SemaphoreHandle_t mutex() const
	{
		return reinterpret_cast<SemaphoreHandle_t>(m_ptr & ~uint32_t(1u));
	}
private:
	MutexLocker(const MutexLocker &) = delete;
	MutexLocker& operator=(const MutexLocker &) = delete;
	uint32_t m_ptr { 0 };
};

} /* namespace AS */

#endif /* MUTEXLOCKER_H_ */
