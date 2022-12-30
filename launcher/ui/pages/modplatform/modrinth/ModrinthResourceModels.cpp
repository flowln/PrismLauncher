// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ModrinthResourceModels.h"

#include "modplatform/modrinth/ModrinthAPI.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"

namespace ResourceDownload {

ModrinthModModel::ModrinthModModel(BaseInstance const& base) : ModModel(base, new ModrinthAPI) {}

void ModrinthModModel::loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    ::Modrinth::loadIndexedPack(m, obj);
}

void ModrinthModModel::loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    ::Modrinth::loadExtraPackData(m, obj);
}

void ModrinthModModel::loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr)
{
    ::Modrinth::loadIndexedPackVersions(m, arr, APPLICATION->network(), &m_base_instance);
}

auto ModrinthModModel::documentToArray(QJsonDocument& obj) const -> QJsonArray
{
    return obj.object().value("hits").toArray();
}

ModrinthResourcePackModel::ModrinthResourcePackModel(const BaseInstance& base)  : ResourcePackResourceModel(base, new ModrinthAPI){}

void ModrinthResourcePackModel::loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    ::Modrinth::loadIndexedPack(m, obj);
}

void ModrinthResourcePackModel::loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    ::Modrinth::loadExtraPackData(m, obj);
}

void ModrinthResourcePackModel::loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr)
{
    ::Modrinth::loadIndexedPackVersions(m, arr, APPLICATION->network(), &m_base_instance);
}

auto ModrinthResourcePackModel::documentToArray(QJsonDocument& obj) const -> QJsonArray
{
    return obj.object().value("hits").toArray();
}

ModrinthShaderPackModel::ModrinthShaderPackModel(const BaseInstance& base)  : ShaderPackResourceModel(base, new ModrinthAPI){}

void ModrinthShaderPackModel::loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    ::Modrinth::loadIndexedPack(m, obj);
}

void ModrinthShaderPackModel::loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    ::Modrinth::loadExtraPackData(m, obj);
}

void ModrinthShaderPackModel::loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr)
{
    ::Modrinth::loadIndexedPackVersions(m, arr, APPLICATION->network(), &m_base_instance);
}

auto ModrinthShaderPackModel::documentToArray(QJsonDocument& obj) const -> QJsonArray
{
    return obj.object().value("hits").toArray();
}

}  // namespace ResourceDownload
