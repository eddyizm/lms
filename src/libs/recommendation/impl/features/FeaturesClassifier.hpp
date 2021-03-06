/*
 * Copyright (C) 2018 Emeric Poupon
 *
 * This file is part of LMS.
 *
 * LMS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LMS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LMS.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <optional>
#include <string>

#include "som/DataNormalizer.hpp"
#include "som/Network.hpp"
#include "FeaturesClassifierCache.hpp"
#include "FeaturesDefs.hpp"
#include "IClassifier.hpp"

namespace Database
{
	class Session;
}

namespace Recommendation {

using FeatureWeight = double;

class FeaturesClassifier : public IClassifier
{
	public:
		FeaturesClassifier() = default;
		FeaturesClassifier(const FeaturesClassifier&) = delete;
		FeaturesClassifier(FeaturesClassifier&&) = delete;
		FeaturesClassifier& operator=(const FeaturesClassifier&) = delete;
		FeaturesClassifier& operator=(FeaturesClassifier&&) = delete;

		using FeaturesFetchFunc = std::function<std::optional<std::unordered_map<std::string, std::vector<double>>>(Database::IdType /*trackId*/, const std::unordered_set<std::string>& /*features*/)>;
		// Default is to retrieve the features from the database (may be slow).
		// Use this only if you want to train different searchers with some cached data
		static void setFeaturesFetchFunc(FeaturesFetchFunc func) { _featuresFetchFunc = func; }

		static const FeatureSettingsMap& getDefaultTrainFeatureSettings();

	private:

		std::string_view getName() const override { return "Features"; }

		bool load(Database::Session& session, bool forceReload, const ProgressCallback& progressCallback) override;
		void requestCancelLoad() override;

		std::unordered_set<Database::IdType> getSimilarTracksFromTrackList(Database::Session& session, Database::IdType tracklistId, std::size_t maxCount) const override;
		std::unordered_set<Database::IdType> getSimilarTracks(Database::Session& session, const std::unordered_set<Database::IdType>& tracksId, std::size_t maxCount) const override;
		std::unordered_set<Database::IdType> getSimilarReleases(Database::Session& session, Database::IdType releaseId, std::size_t maxCount) const override;
		std::unordered_set<Database::IdType> getSimilarArtists(Database::Session& session,
				Database::IdType artistId,
				EnumSet<Database::TrackArtistLinkType> linkTypes,
				std::size_t maxCount) const override;

		bool loadFromCache(Database::Session& session, const FeaturesClassifierCache& cache);

		// Use training (may be very slow)
		struct TrainSettings
		{
			std::size_t iterationCount {10};
			float sampleCountPerNeuron {4};
			FeatureSettingsMap featureSettingsMap;
		};
		bool loadFromTraining(Database::Session& session, const TrainSettings& trainSettings, const ProgressCallback& progressCallback);

		using ObjectPositions = std::unordered_map<Database::IdType, std::unordered_set<SOM::Position>>;
		using MatrixOfObjects = SOM::Matrix<std::unordered_set<Database::IdType>>;

		bool load(Database::Session& session,
				SOM::Network network,
				const ObjectPositions& tracksPosition);

		FeaturesClassifierCache toCache() const;

		static std::unordered_set<SOM::Position> getMatchingRefVectorsPosition(const std::unordered_set<Database::IdType>& ids, const ObjectPositions& objectPositions);
		static std::unordered_set<Database::IdType> getObjectsIds(const std::unordered_set<SOM::Position>& positionSet, const MatrixOfObjects& objectsMap);

		std::unordered_set<Database::IdType> getSimilarObjects(const std::unordered_set<Database::IdType>& ids,
				const SOM::Matrix<std::unordered_set<Database::IdType>>& objectsMap,
				const ObjectPositions& objectPosition,
				std::size_t maxCount) const;

		bool				_loadCancelled {};
		std::unique_ptr<SOM::Network>	_network;
		double				_networkRefVectorsDistanceMedian {};

		ObjectPositions     _artistPositions;
		std::unordered_map<Database::TrackArtistLinkType, MatrixOfObjects> _artistsMap;

		MatrixOfObjects		_releasesMap;
		ObjectPositions		_releasePositions;

		MatrixOfObjects		_tracksMap;
		ObjectPositions		_trackPositions;

		static inline FeaturesFetchFunc _featuresFetchFunc;
};

} // ns Recommendation
