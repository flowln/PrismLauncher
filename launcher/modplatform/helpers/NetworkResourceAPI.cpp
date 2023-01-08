#include "NetworkResourceAPI.h"

#include "Application.h"
#include "net/NetJob.h"

#include "modplatform/ModIndex.h"

NetJob::Ptr NetworkResourceAPI::searchProjects(SearchArgs&& args, SearchCallbacks&& callbacks) const
{
    auto search_url_optional = getSearchURL(args);
    if (!search_url_optional.has_value()) {
        callbacks.on_fail("Failed to create search URL", -1);
        return nullptr;
    }

    auto search_url = search_url_optional.value();

    auto response = new QByteArray();
    auto netJob = new NetJob(QString("%1::Search").arg(debugName()), APPLICATION->network());

    netJob->addNetAction(Net::Download::makeByteArray(QUrl(search_url), response));

    QObject::connect(netJob, &NetJob::succeeded, [=]{
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from " << debugName() << " at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;

            callbacks.on_fail(parse_error.errorString(), -1);

            return;
        }

        callbacks.on_succeed(doc);
    });

    QObject::connect(netJob, &NetJob::failed, [=](QString reason){
        int network_error_code = -1;
        if (auto* failed_action = netJob->getFailedActions().at(0); failed_action && failed_action->m_reply)
            network_error_code = failed_action->m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

       callbacks.on_fail(reason, network_error_code); 
    });
    QObject::connect(netJob, &NetJob::aborted, [=]{
       callbacks.on_abort(); 
    });

    return netJob;
}

NetJob::Ptr NetworkResourceAPI::getProjectInfo(ProjectInfoArgs&& args, ProjectInfoCallbacks&& callbacks) const
{
    auto response = new QByteArray();
    auto job = getProject(args.pack.addonId.toString(), response);

    QObject::connect(job.get(), &NetJob::succeeded, [response, callbacks, args] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response for mod info at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        callbacks.on_succeed(doc, args.pack);
    });

    return job;
}

NetJob::Ptr NetworkResourceAPI::getProjectVersions(VersionSearchArgs&& args, VersionSearchCallbacks&& callbacks) const
{
    auto versions_url_optional = getVersionsURL(args);
    if (!versions_url_optional.has_value())
        return nullptr;

    auto versions_url = versions_url_optional.value();

    auto netJob = new NetJob(QString("%1::Versions").arg(args.pack.name), APPLICATION->network());
    auto response = new QByteArray();

    netJob->addNetAction(Net::Download::makeByteArray(versions_url, response));

    QObject::connect(netJob, &NetJob::succeeded, [=] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response for getting versions at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        callbacks.on_succeed(doc, args.pack);
    });

    QObject::connect(netJob, &NetJob::finished, [response] {
        delete response;
    });

    return netJob;
}

NetJob::Ptr NetworkResourceAPI::getProject(QString addonId, QByteArray* response) const
{
    auto project_url_optional = getInfoURL(addonId);
    if (!project_url_optional.has_value())
        return nullptr;

    auto project_url = project_url_optional.value();

    auto netJob = new NetJob(QString("%1::GetProject").arg(addonId), APPLICATION->network());

    netJob->addNetAction(Net::Download::makeByteArray(QUrl(project_url), response));

    QObject::connect(netJob, &NetJob::finished, [response] {
        delete response;
    });

    return netJob;
}
