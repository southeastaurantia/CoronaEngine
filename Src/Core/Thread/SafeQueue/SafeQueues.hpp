// SPDX-License-Identifier: MIT
#pragma once

/**
 * @file SafeQueues.hpp
 * @brief 无锁安全队列总入口头文件，包含 8 种常用并发队列：
 * - SPSC: 定长/不定长
 * - MPSC: 定长/不定长
 * - SPMC: 定长/不定长
 * - MPMC: 定长/不定长
 *
 * 使用说明：
 * - 定长（Bounded*）为编译期固定容量的环形缓冲队列；
 * - 不定长（Unbounded*）为链表式 Michael-Scott 队列；
 * - SPMC/MPMC 的不定长变体使用简易 Hazard Pointer 做节点回收；
 * - 所有实现均为 header-only，无外部依赖。
 */

#include "BoundedSPSCQueue.hpp"
#include "BoundedMPSCQueue.hpp"
#include "BoundedSPMCQueue.hpp"
#include "BoundedMPMCQueue.hpp"

#include "UnboundedSPSCQueue.hpp"
#include "UnboundedMPSCQueue.hpp"
#include "UnboundedSPMCQueue.hpp"
#include "UnboundedMPMCQueue.hpp"

namespace Corona
{
	/**
	 * @name 便捷别名（聚合头导出）
	 * @{ */
	template <typename T, std::size_t CapacityPow2>
	using BoundedSPSC = BoundedSPSCQueue<T, CapacityPow2>;
	template <typename T, std::size_t CapacityPow2>
	using BoundedMPSC = BoundedMPSCQueue<T, CapacityPow2>;
	template <typename T, std::size_t CapacityPow2>
	using BoundedSPMC = BoundedSPMCQueue<T, CapacityPow2>;
	template <typename T, std::size_t CapacityPow2>
	using BoundedMPMC = BoundedMPMCQueue<T, CapacityPow2>;

	template <typename T>
	using UnboundedSPSC = UnboundedSPSCQueue<T>;
	template <typename T>
	using UnboundedMPSC = UnboundedMPSCQueue<T>;
	template <typename T>
	using UnboundedSPMC = UnboundedSPMCQueue<T>;
	template <typename T>
	using UnboundedMPMC = UnboundedMPMCQueue<T>;
	/** @} */
} // namespace Corona
