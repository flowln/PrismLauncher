// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "FlameResourcePages.h"
#include "ui_ResourcePage.h"

#include "FlameResourceModels.h"
#include "ui/dialogs/ResourceDownloadDialog.h"

namespace ResourceDownload {

FlameModPage::FlameModPage(ModDownloadDialog* dialog, BaseInstance& instance)
    : ModPage(dialog, instance)
{
    m_model = new FlameModModel(this);
    m_ui->packView->setModel(m_model);

    // index is used to set the sorting with the flame api
    m_ui->sortByBox->addItem(tr("Sort by Featured"));
    m_ui->sortByBox->addItem(tr("Sort by Popularity"));
    m_ui->sortByBox->addItem(tr("Sort by Last Updated"));
    m_ui->sortByBox->addItem(tr("Sort by Name"));
    m_ui->sortByBox->addItem(tr("Sort by Author"));
    m_ui->sortByBox->addItem(tr("Sort by Downloads"));

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems,
    // so it's best not to connect them in the parent's contructor...
    connect(m_ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlameModPage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FlameModPage::onVersionSelectionChanged);
    connect(m_ui->resourceSelectionButton, &QPushButton::clicked, this, &FlameModPage::onResourceSelected);

    m_ui->packDescription->setMetaEntry(metaEntryBase());
}

auto FlameModPage::validateVersion(ModPlatform::IndexedVersion& ver, QString mineVer, std::optional<ResourceAPI::ModLoaderTypes> loaders) const -> bool
{
    Q_UNUSED(loaders);
    return ver.mcVersion.contains(mineVer) && !ver.downloadUrl.isEmpty();
}

bool FlameModPage::optedOut(ModPlatform::IndexedVersion& ver) const
{
    return ver.downloadUrl.isEmpty();
}

// I don't know why, but doing this on the parent class makes it so that
// other mod providers start loading before being selected, at least with
// my Qt, so we need to implement this in every derived class...
auto FlameModPage::shouldDisplay() const -> bool { return true; }

void FlameModPage::openUrl(const QUrl& url)
{
    if (url.scheme().isEmpty()) {
        QString query = url.query(QUrl::FullyDecoded);

        if (query.startsWith("remoteUrl=")) {
            // attempt to resolve url from warning page
            query.remove(0, 10);
            ModPage::openUrl({QUrl::fromPercentEncoding(query.toUtf8())}); // double decoding is necessary
            return;
        }
    }

    ModPage::openUrl(url);
}

}  // namespace ResourceDownload
