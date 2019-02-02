/*  LOOT

A load order optimisation tool for Oblivion, Skyrim, Fallout 3 and
Fallout: New Vegas.

Copyright (C) 2014 WrinklyNinja

This file is part of LOOT.

LOOT is free software: you can redistribute
it and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

LOOT is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LOOT.  If not, see
<https://www.gnu.org/licenses/>.
*/

#ifndef LOOT_GUI_QUERY_EDITOR_CLOSED_QUERY
#define LOOT_GUI_QUERY_EDITOR_CLOSED_QUERY

#include "gui/cef/query/json.h"
#include "gui/cef/query/types/metadata_query.h"
#include "gui/state/loot_state.h"

namespace loot {
class EditorClosedQuery : public MetadataQuery {
public:
  EditorClosedQuery(LootState& state, nlohmann::json editorState) :
      MetadataQuery(state),
      state_(state),
      applyEdits_(editorState.at("applyEdits")) {
    try {
      metadata_ = editorState.at("metadata");
    } catch (...) {
      state_.DecrementUnappliedChangeCounter();
      throw;
    }
  }

  std::string executeLogic() {
    if (applyEdits_) {
      applyUserEdits();
    }
    state_.DecrementUnappliedChangeCounter();

    return generateJsonResponse(metadata_.GetName());
  }

private:
  std::optional<PluginMetadata> getNonUserMetadata() {
    auto logger = getLogger();
    if (logger) {
      logger->trace("Getting non-user metadata for: {}", metadata_.GetName());
    }

    auto plugin = state_.GetCurrentGame().GetPlugin(metadata_.GetName());
    if (plugin) {
      return MetadataQuery::getNonUserMetadata(plugin);
    }

    return state_.GetCurrentGame().GetMasterlistMetadata(metadata_.GetName());
  }

  PluginMetadata getUserMetadata() {
    // metadata_ may have no group or may have a group of "default" or another
    // value. "default" should become no group if there is no non-user metadata,
    // or if the non-user metadata also has no group or the "default" group.
    auto nonUserMetadata = getNonUserMetadata();
    if (nonUserMetadata.has_value()) {
      auto userMetadata = metadata_.NewMetadata(nonUserMetadata.value());
      if (userMetadata.GetGroup() == std::optional(Group().GetName())
        && nonUserMetadata.value().GetGroup().value_or(Group().GetName()) == Group().GetName()) {
        userMetadata.UnsetGroup();
      }
      return userMetadata;
    }

    auto userMetadata = metadata_;
    if (userMetadata.GetGroup() == std::optional(Group().GetName())) {
      userMetadata.UnsetGroup();
    }

    return userMetadata;
  }

  void applyUserEdits() {
    auto logger = getLogger();
    if (logger) {
      logger->trace("Applying user edits for: {}", metadata_.GetName());
    }

    // Determine what metadata in the response is user-added.
    auto userMetadata = getUserMetadata();

    // Now erase any existing userlist entry.
    if (logger) {
      logger->trace("Erasing the existing userlist entry.");
    }
    state_.GetCurrentGame().ClearUserMetadata(metadata_.GetName());

    // Add a new userlist entry if necessary.
    if (!userMetadata.HasNameOnly()) {
      if (logger) {
        logger->trace("Adding new metadata to new userlist entry.");
      }
      state_.GetCurrentGame().AddUserMetadata(userMetadata);
    }

    // Save edited userlist.
    state_.GetCurrentGame().SaveUserMetadata();
  }

  LootState& state_;
  const bool applyEdits_;
  PluginMetadata metadata_;
};
}

#endif
