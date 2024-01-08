#pragma once

#include <multiple_hasher.h>
#include <utils.h>

#include <functional>
#include <iomanip>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <utility>

struct Block {
	size_t nonce;
	Sha256 previous_block_hash;
};

struct MineBlockSharedData {
	bool block_mined;
	Sha256 block_hash;
	size_t nonce;
	std::mutex lock;
};

struct MinedBlock {
	Block block;
	Sha256 hash;
};

inline bool is_valid_block_hash(Sha256 hash, size_t difficulty) {
	bool is_valid = true;
	for (int i = 0; i <= difficulty / 2; ++i) {
		Byte low_part = hash[i] & 0x0f;
		Byte high_part = hash[i] >> 4;
		if (i * 2 < difficulty) is_valid = is_valid && (high_part == Byte(0));
		if (i * 2 + 1 < difficulty) is_valid = is_valid && (low_part == Byte(0));
	}
	return is_valid;
}

inline void mine_block_at_offset(MineBlockSharedData* shared_data, Block block, size_t difficulty, size_t total_threads = 1, size_t offset = 0) {
	Sha256 hash;
	size_t iter = 0;
	size_t nonce;
	do {
		{
			std::lock_guard<std::mutex> l(shared_data->lock);
			if (shared_data->block_mined) return;
		}
		nonce = iter * total_threads + offset;
		hash = sha256(nonce, block.previous_block_hash);
		iter += 1;
	} while (!is_valid_block_hash(hash, difficulty));

	{
		std::lock_guard<std::mutex> l(shared_data->lock);
		if (shared_data->block_mined) return; // in case two threads mined the block at the same time
		shared_data->block_mined = true;
		shared_data->nonce = nonce;
		shared_data->block_hash = hash;
	}
}

inline MinedBlock mine_block_parallel(Block block, size_t difficulty, size_t thread_count) {
	size_t concurrency = std::thread::hardware_concurrency();
	size_t assigned_threads = std::max(std::min(concurrency, thread_count), 1ull);
	std::vector<std::thread> threads(thread_count);

	MineBlockSharedData shared_data = {};
	for (size_t offset = 0; offset < assigned_threads; ++offset) {
		threads[offset] = std::thread(mine_block_at_offset, &shared_data, block, difficulty, assigned_threads, offset);
	}

	for (auto& thread : threads) {
		thread.join();
	}

	std::cout << "nonce: " << shared_data.nonce << std::endl;
	std::cout << "block hash: " << to_hex(shared_data.block_hash) << std::endl;

	MinedBlock mined_block;
	mined_block.block = {
		shared_data.nonce,
		block.previous_block_hash
	};
	mined_block.hash = shared_data.block_hash;

	return mined_block;
}
