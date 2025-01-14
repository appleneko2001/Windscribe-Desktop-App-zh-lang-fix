#include "notificationsrequest.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "utils/logger.h"
#include "engine/utils/urlquery_utils.h"

namespace server_api {

NotificationsRequest::NotificationsRequest(QObject *parent, const QString &authHash) : BaseRequest(parent, RequestType::kGet),
    authHash_(authHash)
{
}

QUrl NotificationsRequest::url(const QString &domain) const
{
    QUrl url("https://" + hostname(domain, SudomainType::kApi) + "/Notifications");
    QUrlQuery query;
    urlquery_utils::addAuthQueryItems(query, authHash_);
    urlquery_utils::addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString NotificationsRequest::name() const
{
    return "Notifications";
}

void NotificationsRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonData =  jsonObject["data"].toObject();
    const QJsonArray jsonNotifications = jsonData["notifications"].toArray();

    for (const QJsonValue &value : jsonNotifications) {
        QJsonObject obj = value.toObject();
        types::Notification n;
        if (!n.initFromJson(obj)) {
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications (not all required fields)";
            setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
            return;
        }
        notifications_.push_back(n);
    }
    qCDebug(LOG_SERVER_API) << "Notifications request successfully executed";
}

} // namespace server_api {
