#pragma once

#include <stdint.h>
#include <vector>
#include <utility>
#include <algorithm>

#include "ContrastiveDivergence.h"
#include "BackPropagation.h"
#include "AutoEncoderBackPropagation.h"

namespace OMLT
{
	template<typename T>
	class TrainingSchedule
	{
	public:
		TrainingSchedule(const struct T::ModelConfig& in_model_config, uint32_t in_minibatch_size, int32_t in_seed)
			: model_config(in_model_config)
			, minibatch_size(in_minibatch_size)
			, seed(in_seed)
			, epochs_remaining(0)
			, index(0)
		{
			
		}

		void AddTrainingConfig(struct T::TrainingConfig& in_train_config, uint32_t in_epochs)
		{
			train_config.push_back(std::pair<struct T::TrainingConfig, uint32_t>(in_train_config, in_epochs));
		}

		void StartTraining()
		{
			assert(train_config.size() > 0);
			epochs_remaining = train_config.front().second;
			index = 0;
		}

		bool TrainingComplete()
		{
			return index == train_config.size();
		}

		// returns true if we're done with the current parameters and need to get the next ones
		bool NextEpoch()
		{
			assert(index <= train_config.size());
			epochs_remaining = epochs_remaining == 0 ? 0 : epochs_remaining - 1;

			if(epochs_remaining == 0)
			{
				index = std::min(index + 1, train_config.size());
				// verify we have epochs left
				if((index) < train_config.size())
				{
					epochs_remaining = train_config[index].second;
				}
				return true;
			}

			return false;
		}

		bool GetTrainingConfig(struct T::TrainingConfig& out_config)
		{
			out_config = train_config[index].first;
			return true;
		}

		struct T::ModelConfig GetModelConfig() const
		{
			return model_config;
		}
		
		uint32_t GetMinibatchSize() const
		{
			return minibatch_size;
		}

		int32_t GetSeed() const
		{
			return seed;
		}

		uint32_t GetEpochs() const
		{
			return epochs_remaining;
		}

		static TrainingSchedule* FromJSON(const std::string& json);
	private:
		struct T::ModelConfig model_config;
		std::vector<std::pair<struct T::TrainingConfig, uint32_t>> train_config;
		uint32_t minibatch_size;
		int32_t seed;

		uint32_t epochs_remaining;
		uint32_t index;
	};
	// parsing specialization
	template<>
	TrainingSchedule<ContrastiveDivergence>* TrainingSchedule<ContrastiveDivergence>::FromJSON(const std::string& json);

	template<>
	TrainingSchedule<BackPropagation>* TrainingSchedule<BackPropagation>::FromJSON(const std::string& json);
}