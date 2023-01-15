// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "LogPage.h"

#include "Application.h"

#include <QIcon>
#include <QScrollBar>
#include <QShortcut>
#include <QTabBar>

#include "launch/LaunchTask.h"

#include "ui/ColorCache.h"
#include "ui/GuiUtil.h"

#include <BuildConfig.h>

#include <QQmlProperty>
#include <QQuickItem>

class LogFormatProxyModel final : public QIdentityProxyModel {
   public:
    LogFormatProxyModel(LogColorCache&& color_cache, QFont&& font, QObject* parent = nullptr)
        : QIdentityProxyModel(parent), m_colors(color_cache), m_font(font)
    {}

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles = QIdentityProxyModel::roleNames();
        roles[Qt::FontRole] = "text_font";
        roles[Qt::ForegroundRole] = "foreground_color";
        roles[Qt::BackgroundRole] = "background_color";
        return roles;
    }

    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override
    {
        switch (role) {
            case Qt::FontRole:
                return m_font;
            case Qt::ForegroundRole: {
                auto level = (MessageLevel::Enum) QIdentityProxyModel::data(index, LogModel::LevelRole).toInt();
                return m_colors.getFront(level);
            }
            case Qt::BackgroundRole: {
                auto level = (MessageLevel::Enum) QIdentityProxyModel::data(index, LogModel::LevelRole).toInt();
                return m_colors.getBack(level);
            }
            default:
                return QIdentityProxyModel::data(index, role);
        }
    }

   private:
    LogColorCache m_colors;
    QFont m_font;
};

LogPage::LogPage(InstancePtr instance, QWidget* parent)
    : QQuickWidget(QStringLiteral("qrc:///pages/instance/LogPage.qml"), parent), m_instance(std::move(instance))
{
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    connect(this, &QQuickWidget::statusChanged, this, &LogPage::onStatusChanged);

    // set up text colors in the log proxy and adapt them to the current theme foreground and background
    auto origForeground = palette().color(foregroundRole());
    auto origBackground = palette().color(backgroundRole());

    // set up fonts in the log proxy
    QString fontFamily = APPLICATION->settings()->get("ConsoleFont").toString();
    bool conversionOk = false;
    int fontSize = APPLICATION->settings()->get("ConsoleFontSize").toInt(&conversionOk);
    if (!conversionOk)
        fontSize = 11;

    m_proxy = new LogFormatProxyModel(LogColorCache(origForeground, origBackground), QFont(fontFamily, fontSize), this);

    // set up instance and launch process recognition
    {
        if (auto launchTask = m_instance->getLaunchTask(); launchTask)
            setInstanceLaunchTaskChanged(launchTask, true);
        connect(m_instance.get(), &BaseInstance::launchTaskChanged, this, &LogPage::onInstanceLaunchTaskChanged);
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Signal never gets called in Qt5 for some god-forsaken reason o.O
    onQuickWidgetReady();
#endif
}

bool LogPage::shouldDisplay() const
{
    return m_instance->isRunning() || m_proxy->rowCount() > 0;
}

bool LogPage::apply()
{
    saveSettings();

    return true;
}

void LogPage::onStatusChanged(QQuickWidget::Status status)
{
    if (status == QQuickWidget::Status::Ready)
        onQuickWidgetReady();
}

void LogPage::onQuickWidgetReady()
{
    [[maybe_unused]] bool no_errors = QQmlProperty::write(rootObject(), "logModel", QVariant::fromValue(m_proxy));
    Q_ASSERT(no_errors);

    no_errors = connect(rootObject(), SIGNAL(onWrapModeChanged(int)), this, SLOT(wrapModeChanged(int)));
    Q_ASSERT(no_errors);
    no_errors = connect(rootObject(), SIGNAL(onSuspendedChanged(bool)), this, SLOT(suspendedChanged(bool)));
    Q_ASSERT(no_errors);
    no_errors = connect(rootObject(), SIGNAL(onUseRegexChanged(bool)), this, SLOT(useRegexChanged(bool)));
    Q_ASSERT(no_errors);
    no_errors = connect(rootObject(), SIGNAL(onCopyPressed(QString)), this, SLOT(copyPressed(QString)));
    Q_ASSERT(no_errors);
    no_errors = connect(rootObject(), SIGNAL(onUploadPressed()), this, SLOT(uploadPressed()));
    Q_ASSERT(no_errors);
    no_errors = connect(rootObject(), SIGNAL(onClearPressed()), this, SLOT(clearPressed()));
    Q_ASSERT(no_errors);
    no_errors = connect(rootObject(), SIGNAL(onSearchRequested(QString, bool)), this, SLOT(searchRequested(QString, bool)));
    Q_ASSERT(no_errors);

    loadSettings();

    if (m_model)
        QMetaObject::invokeMethod(rootObject(), "gotSourceModel");
}

void LogPage::setInstanceLaunchTaskChanged(LaunchTask::Ptr proc, bool initial)
{
    m_process = proc;
    if (m_process) {
        m_model = proc->getLogModel();
        m_proxy->setSourceModel(m_model.get());

        if (initial)
            loadSettings();

        QMetaObject::invokeMethod(rootObject(), "gotSourceModel");
    } else {
        m_proxy->setSourceModel(nullptr);
        m_model.reset();

        QMetaObject::invokeMethod(rootObject(), "lostSourceModel");
    }
}

void LogPage::onInstanceLaunchTaskChanged(LaunchTask::Ptr proc)
{
    setInstanceLaunchTaskChanged(proc, false);
}

void LogPage::wrapModeChanged(int new_state)
{
    [[maybe_unused]] bool no_errors = QQmlProperty::write(rootObject(), "wrapMode", new_state);
    Q_ASSERT(no_errors);
}

void LogPage::suspendedChanged(bool new_state)
{
    Q_ASSERT(m_model);
    m_model->suspend(new_state);

    [[maybe_unused]] bool no_errors = QQmlProperty::write(rootObject(), "suspended", m_model->suspended());
    Q_ASSERT(no_errors);
}

void LogPage::useRegexChanged(bool new_state)
{
    m_use_regex_in_search = new_state;

    [[maybe_unused]] bool no_errors = QQmlProperty::write(rootObject(), "useRegexInSearch", m_use_regex_in_search);
    Q_ASSERT(no_errors);
}

void LogPage::copyPressed(QString to_copy)
{
    if (!m_model)
        return;

    auto text_to_copy = to_copy;
    if (text_to_copy.isEmpty())
        text_to_copy = m_model->toPlainText();

    m_model->append(MessageLevel::Launcher, QString("Clipboard copy at: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)));
    GuiUtil::setClipboardText(text_to_copy);
}

void LogPage::uploadPressed()
{
    if (!m_model)
        return;

    // FIXME: turn this into a proper task and move the upload logic out of GuiUtil!
    m_model->append(MessageLevel::Launcher, tr("Log upload triggered at: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)));
    auto url = GuiUtil::uploadPaste(tr("Minecraft Log"), m_model->toPlainText(), this);
    if (!url.has_value()) {
        m_model->append(MessageLevel::Error, tr("Log upload canceled"));
    } else if (url->isNull()) {
        m_model->append(MessageLevel::Error, tr("Log upload failed!"));
    } else {
        m_model->append(MessageLevel::Launcher, tr("Log uploaded to: %1").arg(url.value()));
    }
}

void LogPage::clearPressed()
{
    if (!m_model)
        return;

    m_model->clear();
}

inline int positiveModulo(int a, int b) { return a >= 0 ? a % b : (a + b) % b; }

void LogPage::searchRequested(QString search_string, bool reverse)
{
    if (!m_model)
        return;

    if (search_string != m_search_request.first || m_use_regex_in_search != m_search_request.second) {
        m_search_request.first = search_string;
        m_search_request.second = m_use_regex_in_search;

        m_search_results = m_model->search(search_string, m_use_regex_in_search);
        m_current_search_offset = -1;
    }

    int next_line = -1;

    if (!m_search_results.isEmpty()) {
        if (reverse)
            m_current_search_offset = positiveModulo(m_current_search_offset - 1, m_search_results.size());
        else
            m_current_search_offset = positiveModulo(m_current_search_offset + 1, m_search_results.size());

        if (m_current_search_offset < m_search_results.size())
            next_line = m_search_results.at(m_current_search_offset);
    }

    [[maybe_unused]] bool no_errors = QMetaObject::invokeMethod(rootObject(), "goToLine", Q_ARG(QVariant, next_line));
    Q_ASSERT(no_errors);
}

void LogPage::loadSettings()
{
    [[maybe_unused]] bool no_errors = true;

    if (m_model) {
        no_errors = QQmlProperty::write(rootObject(), "suspended", m_model->suspended());
        Q_ASSERT(no_errors);
    }

    // TODO: Get saved state from last usage
}

void LogPage::saveSettings()
{
    // TODO: Save state for next usage
}

/*



void LogPage::on_findButton_clicked()
{
    auto modifiers = QApplication::keyboardModifiers();
    bool reverse = modifiers & Qt::ShiftModifier;
    ui->text->findNext(ui->searchBar->text(), reverse);
}
*/
